// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <sys/param.h>

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fnmatch.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shared/macro.h>
#include <shared/strbuf.h>
#include <shared/util.h>

#include "libkmod-internal.h"
#include "libkmod-index.h"

/* libkmod-index.c: module index file implementation
 *
 * Integers are stored as 32 bit unsigned in "network" order, i.e. MSB first.
 * All files start with a magic number.
 *
 * Magic spells "BOOTFAST", where the exact encoding varies across versions.
 *
 * The original implementation in modutils/module-init-tools used 0xB007FA57, but also
 * lacked the version fields. Thus later on, with module-init-tools commit 44d7ac4
 * ("add versioning to the binary module files"), the encoding was changed to 0xB007F457
 * (A -> 4) and the version fields were introduced. Shortly afterwards the format was
 * changed in backwards incompatible way and the major version was bumped to 2.
 *
 * We use a version string to keep track of changes to the binary format.
 * This is stored in the form: INDEX_MAJOR (hi) INDEX_MINOR (lo) just in
 * case we ever decide to have minor changes that are not incompatible.
 */
#define INDEX_MAGIC 0xB007F457
#define INDEX_VERSION_MAJOR 0x0002
#define INDEX_VERSION_MINOR 0x0001
#define INDEX_VERSION ((INDEX_VERSION_MAJOR << 16) | INDEX_VERSION_MINOR)

/* The index file maps keys to values. Both keys and values are ASCII strings.
 * Each key can have multiple values. Values are sorted by an integer priority.
 *
 * The reader also implements a wildcard search (including range expressions)
 * where the keys in the index are treated as patterns.
 * This feature is required for module aliases.
 */
#define INDEX_CHILDMAX 128u

/* Disk format:
 *
 *  uint32_t magic = INDEX_MAGIC;
 *  uint32_t version = INDEX_VERSION;
 *  uint32_t root_offset;
 *
 *  (node_offset & INDEX_NODE_MASK) specifies the file offset of nodes:
 *
 *       char[] prefix; // nul terminated
 *
 *       uint8_t first;
 *       uint8_t last;
 *       uint32_t children[last - first + 1];
 *
 *       uint32_t value_count;
 *       struct {
 *           uint32_t priority;
 *           char[] value; // nul terminated
 *       } values[value_count];
 *
 *  (node_offset & INDEX_NODE_FLAGS) indicates which fields are present.
 *  Empty prefixes are omitted, leaf nodes omit the three child-related fields.
 *
 *  This could be optimised further by adding a sparse child format
 *  (indicated using a new flag).
 *
 *
 * Implementation is based on a radix tree, or "trie".
 * Each arc from parent to child is labelled with a character.
 * Each path from the root represents a string.
 *
 * == Example strings ==
 *
 * ask
 * ate
 * on
 * once
 * one
 *
 * == Key ==
 *  + Normal node
 *  * Marked node, representing a key and its values.
 *
 * +
 * |-a-+-s-+-k-*
 * |   |
 * |   `-t-+-e-*
 * |
 * `-o-+-n-*-c-+-e-*
 *         |
 *         `-e-*
 *
 * Naive implementations tend to be very space inefficient; child pointers
 * are stored in arrays indexed by character, but most child pointers are null.
 *
 * Our implementation uses a scheme described by Wikipedia as a Patricia trie,
 *
 *     "easiest to understand as a space-optimized trie where
 *      each node with only one child is merged with its child"
 *
 * +
 * |-a-+-sk-*
 * |   |
 * |   `-te-*
 * |
 * `-on-*-ce-*
 *      |
 *      `-e-*
 *
 * We still use arrays of child pointers indexed by a single character;
 * the remaining characters of the label are stored as a "prefix" in the child.
 *
 * The paper describing the original Patricia trie works on individual bits -
 * each node has a maximum of two children, which increases space efficiency.
 * However for this application it is simpler to use the ASCII character set.
 * Since the index file is read-only, it can be compressed by omitting null
 * child pointers at the start and end of arrays.
 */

/* Format of node offsets within index file */
enum node_offset {
	INDEX_NODE_FLAGS = 0xF0000000, /* Flags in high nibble */
	INDEX_NODE_PREFIX = 0x80000000,
	INDEX_NODE_VALUES = 0x40000000,
	INDEX_NODE_CHILDS = 0x20000000,

	INDEX_NODE_MASK = 0x0FFFFFFF, /* Offset value */
};

struct wrtbuf {
	char bytes[4096];
	size_t len;
	int fd;
};

static void wrtbuf_init(struct wrtbuf *buf, int fd)
{
	buf->len = 0;
	buf->fd = fd;
}

static void wrtbuf_flush(struct wrtbuf *buf)
{
	if (buf->len > 0) {
		write_str_safe(buf->fd, buf->bytes, buf->len);
		buf->len = 0;
	}
}

static void wrtbuf_write(struct wrtbuf *buf, const char *p, size_t len)
{
	size_t todo = len;
	size_t done = 0;

	do {
		size_t n = MIN(sizeof(buf->bytes) - buf->len, todo);

		memcpy(buf->bytes + buf->len, p + done, n);
		buf->len += n;
		todo -= n;
		done += n;

		if (buf->len == sizeof(buf->bytes))
			wrtbuf_flush(buf);
	} while (todo > 0);
}

void index_values_free(struct index_value *values)
{
	while (values) {
		struct index_value *value = values;

		values = value->next;
		free(value);
	}
}

static int add_value(struct index_value **values, const char *value, size_t len,
		     uint32_t priority)
{
	struct index_value *v;

	/* find position to insert value */
	while (*values && (*values)->priority < priority)
		values = &(*values)->next;

	v = malloc(sizeof(struct index_value) + len + 1);
	if (!v)
		return -1;
	v->next = *values;
	v->priority = priority;
	v->len = len;
	memcpy(v->value, value, len);
	v->value[len] = '\0';
	*values = v;

	return 0;
}

static inline int read_char(FILE *in)
{
	return getc_unlocked(in);
}

static bool read_u32s(FILE *in, uint32_t *l, size_t n)
{
	size_t i;

	if (fread_unlocked(l, sizeof(uint32_t), n, in) != n) {
		return false;
	}
	for (i = 0; i < n; i++)
		l[i] = ntohl(l[i]);
	return true;
}

static inline bool read_u32(FILE *in, uint32_t *l)
{
	return read_u32s(in, l, 1);
}

/*
 * Index file searching
 */
struct index_file {
	FILE *file;
	uint32_t root_offset;
	char *tmp;
	size_t tmp_size;
};

struct index_node_f {
	struct index_file *idx;
	char *prefix; /* path compression */
	struct index_value *values;
	uint8_t first; /* range of child nodes */
	uint8_t last;
	uint32_t children[0];
};

static struct index_node_f *index_read(struct index_file *idx, uint32_t offset)
{
	struct index_node_f *node = NULL;
	char *prefix = NULL;
	size_t child_count = 0;
	FILE *fp = idx->file;

	if ((offset & INDEX_NODE_MASK) == 0)
		return NULL;

	if (fseek(fp, offset & INDEX_NODE_MASK, SEEK_SET) < 0)
		return NULL;

	if (offset & INDEX_NODE_PREFIX) {
		if (getdelim(&idx->tmp, &idx->tmp_size, '\0', fp) < 0)
			return NULL;
		prefix = strdup(idx->tmp);
	} else
		prefix = strdup("");

	if (prefix == NULL)
		goto err;

	if (offset & INDEX_NODE_CHILDS) {
		int first = read_char(fp);
		int last = read_char(fp);

		if (first == EOF || last == EOF || first > last)
			goto err;

		child_count = last - first + 1;

		node = malloc(sizeof(struct index_node_f) +
			      sizeof(uint32_t) * child_count);
		if (node == NULL)
			goto err;

		node->first = (uint8_t)first;
		node->last = (uint8_t)last;

		if (!read_u32s(fp, node->children, child_count))
			goto err;
	} else {
		node = malloc(sizeof(struct index_node_f));
		if (node == NULL)
			goto err;

		node->first = INDEX_CHILDMAX;
		node->last = 0;
	}

	node->values = NULL;
	if (offset & INDEX_NODE_VALUES) {
		uint32_t value_count;
		uint32_t priority;

		if (!read_u32(fp, &value_count))
			goto err;

		while (value_count--) {
			ssize_t n;

			if (!read_u32(fp, &priority))
				goto err;
			n = getdelim(&idx->tmp, &idx->tmp_size, '\0', fp);
			if (n < 0)
				goto err;
			add_value(&node->values, idx->tmp, n, priority);
		}
	}

	node->prefix = prefix;
	node->idx = idx;
	return node;
err:
	free(prefix);
	free(node);
	return NULL;
}

static void index_close(struct index_node_f *node)
{
	free(node->prefix);
	index_values_free(node->values);
	free(node);
}

struct index_file *index_file_open(const char *filename)
{
	FILE *file;
	uint32_t magic, version;
	struct index_file *new;

	file = fopen(filename, "re");
	if (!file)
		return NULL;

	if (!read_u32(file, &magic) || magic != INDEX_MAGIC)
		goto err;

	if (!read_u32(file, &version) || version >> 16 != INDEX_VERSION_MAJOR)
		goto err;

	new = malloc(sizeof(struct index_file));
	if (new == NULL)
		goto err;

	new->file = file;
	if (!read_u32(new->file, &new->root_offset)) {
		free(new);
		goto err;
	}
	new->tmp = NULL;
	new->tmp_size = 0;

	return new;
err:
	fclose(file);
	return NULL;
}

void index_file_close(struct index_file *idx)
{
	fclose(idx->file);
	free(idx->tmp);
	free(idx);
}

static struct index_node_f *index_readroot(struct index_file *in)
{
	return index_read(in, in->root_offset);
}

static struct index_node_f *index_readchild(const struct index_node_f *parent, uint8_t ch)
{
	if (parent->first <= ch && ch <= parent->last) {
		return index_read(parent->idx, parent->children[ch - parent->first]);
	}

	return NULL;
}

static void index_dump_node(struct index_node_f *node, struct strbuf *buf,
			    struct wrtbuf *wbuf)
{
	struct index_value *v;
	size_t pushed;

	pushed = strbuf_pushchars(buf, node->prefix);

	for (v = node->values; v != NULL; v = v->next) {
		wrtbuf_write(wbuf, buf->bytes, strbuf_used(buf));
		wrtbuf_write(wbuf, " ", 1);
		wrtbuf_write(wbuf, v->value, strlen(v->value));
		wrtbuf_write(wbuf, "\n", 1);
	}

	for (uint8_t ch = node->first; ch <= node->last; ch++) {
		struct index_node_f *child = index_readchild(node, ch);

		if (!child)
			continue;

		if (strbuf_pushchar(buf, ch)) {
			index_dump_node(child, buf, wbuf);
			strbuf_popchar(buf);
		}
	}

	strbuf_popchars(buf, pushed);
	index_close(node);
}

void index_dump(struct index_file *in, int fd, bool alias_prefix)
{
	DECLARE_STRBUF_WITH_STACK(buf, 128);
	struct index_node_f *root;
	struct wrtbuf wbuf;

	root = index_readroot(in);
	if (root == NULL)
		return;

	wrtbuf_init(&wbuf, fd);
	if (!alias_prefix || strbuf_pushchars(&buf, "alias "))
		index_dump_node(root, &buf, &wbuf);
	wrtbuf_flush(&wbuf);
}

static char *index_search__node(struct index_node_f *node, const char *key, int i)
{
	char *value;
	struct index_node_f *child;
	int j;

	while (node) {
		for (j = 0; node->prefix[j]; j++) {
			uint8_t ch = node->prefix[j];

			if (ch != key[i + j]) {
				index_close(node);
				return NULL;
			}
		}

		i += j;

		if (key[i] == '\0') {
			value = node->values != NULL ? strdup(node->values[0].value) :
						       NULL;

			index_close(node);
			return value;
		}

		child = index_readchild(node, key[i]);
		index_close(node);
		node = child;
		i++;
	}

	return NULL;
}

/*
 * Search the index for a key
 *
 * Returns the value of the first match
 *
 * The recursive functions free their node argument (using index_close).
 */
char *index_search(struct index_file *in, const char *key)
{
	// FIXME: return value by reference instead of strdup
	struct index_node_f *root;
	char *value;

	root = index_readroot(in);
	value = index_search__node(root, key, 0);

	return value;
}

/* Level 4: add all the values from a matching node */
static void index_searchwild__allvalues(struct index_node_f *node,
					struct index_value **out)
{
	struct index_value *v;

	for (v = node->values; v != NULL; v = v->next)
		add_value(out, v->value, v->len, v->priority);

	index_close(node);
}

/*
 * Level 3: traverse a sub-keyspace which starts with a wildcard,
 * looking for matches.
 */
static void index_searchwild__all(struct index_node_f *node, int j, struct strbuf *buf,
				  const char *subkey, struct index_value **out)
{
	size_t pushed;

	pushed = strbuf_pushchars(buf, &node->prefix[j]);

	for (uint8_t ch = node->first; ch <= node->last; ch++) {
		struct index_node_f *child = index_readchild(node, ch);

		if (!child)
			continue;

		if (strbuf_pushchar(buf, ch)) {
			index_searchwild__all(child, 0, buf, subkey, out);
			strbuf_popchar(buf);
		}
	}

	if (node->values) {
		const char *s = strbuf_str(buf);

		if (fnmatch(s, subkey, 0) == 0)
			index_searchwild__allvalues(node, out);
		else
			index_close(node);
	} else {
		index_close(node);
	}

	strbuf_popchars(buf, pushed);
}

/* Level 2: descend the tree (until we hit a wildcard) */
static void index_searchwild__node(struct index_node_f *node, struct strbuf *buf,
				   const char *key, struct index_value **out)
{
	struct index_node_f *child;
	int j;

	while (node) {
		for (j = 0; node->prefix[j]; j++) {
			uint8_t ch = node->prefix[j];

			if (ch == '*' || ch == '?' || ch == '[') {
				index_searchwild__all(node, j, buf, &key[j], out);
				return;
			}

			if (ch != key[j]) {
				index_close(node);
				return;
			}
		}

		key += j;

		child = index_readchild(node, '*');
		if (child) {
			if (strbuf_pushchar(buf, '*')) {
				index_searchwild__all(child, 0, buf, key, out);
				strbuf_popchar(buf);
			}
		}

		child = index_readchild(node, '?');
		if (child) {
			if (strbuf_pushchar(buf, '?')) {
				index_searchwild__all(child, 0, buf, key, out);
				strbuf_popchar(buf);
			}
		}

		child = index_readchild(node, '[');
		if (child) {
			if (strbuf_pushchar(buf, '[')) {
				index_searchwild__all(child, 0, buf, key, out);
				strbuf_popchar(buf);
			}
		}

		if (*key == '\0') {
			index_searchwild__allvalues(node, out);

			return;
		}

		child = index_readchild(node, *key);
		index_close(node);
		node = child;
		key++;
	}
}

/*
 * Search the index for a key.  The index may contain wildcards.
 *
 * Returns a list of all the values of matching keys.
 */
struct index_value *index_searchwild(struct index_file *in, const char *key)
{
	DECLARE_STRBUF_WITH_STACK(buf, 128);
	struct index_node_f *root = index_readroot(in);
	struct index_value *out = NULL;

	index_searchwild__node(root, &buf, key, &out);
	return out;
}

/**************************************************************************/
/*
 * Alternative implementation, using mmap to map all the file to memory when
 * starting
 */
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct index_mm {
	const struct kmod_ctx *ctx;
	void *mm;
	uint32_t root_offset;
	size_t size;
};

struct index_mm_value {
	uint32_t priority;
	size_t len;
	const char *value;
};

struct index_mm_node {
	const struct index_mm *idx;
	const char *prefix; /* mmap'ed value */
	unsigned char first;
	unsigned char last;
	const void *children; /* mmap'ed value */
	size_t value_count;
	const void *values; /* mmap'ed value */
};

static inline uint32_t read_u32_mm(const void **p)
{
	const uint8_t *addr = *(const uint8_t **)p;
	uint32_t v;

	/* addr may be unaligned to uint32_t */
	v = get_unaligned((const uint32_t *)addr);

	*p = addr + sizeof(uint32_t);
	return ntohl(v);
}

static inline uint8_t read_char_mm(const void **p)
{
	const uint8_t *addr = *(const uint8_t **)p;
	uint8_t v = *addr;
	*p = addr + sizeof(uint8_t);
	return v;
}

static inline const char *read_chars_mm(const void **p, size_t *rlen)
{
	const char *addr = *(const char **)p;
	size_t len = *rlen = strlen(addr);
	*p = addr + len + 1;
	return addr;
}

static inline void read_value_mm(const void **p, struct index_mm_value *v)
{
	v->priority = read_u32_mm(p);
	v->value = read_chars_mm(p, &v->len);
}

/* reads node into given node struct and returns its address on success or NULL on error. */
static struct index_mm_node *index_mm_read_node(const struct index_mm *idx,
						uint32_t offset,
						struct index_mm_node *node)
{
	const void *p;

	if ((offset & INDEX_NODE_MASK) == 0 || (offset & INDEX_NODE_MASK) >= idx->size)
		return NULL;

	p = (const char *)idx->mm + (offset & INDEX_NODE_MASK);

	if (offset & INDEX_NODE_PREFIX) {
		size_t len;
		node->prefix = read_chars_mm(&p, &len);
	} else {
		node->prefix = "";
	}

	if (offset & INDEX_NODE_CHILDS) {
		size_t child_count;

		node->first = read_char_mm(&p);
		node->last = read_char_mm(&p);

		if (node->first > node->last || node->first >= INDEX_CHILDMAX ||
		    node->last >= INDEX_CHILDMAX)
			return NULL;

		node->children = p;

		child_count = node->last - node->first + 1;
		p = (const char *)p + sizeof(uint32_t) * child_count;
	} else {
		node->first = INDEX_CHILDMAX;
		node->last = 0;
		node->children = NULL;
	}

	if (offset & INDEX_NODE_VALUES) {
		node->value_count = read_u32_mm(&p);
		node->values = p;
	} else {
		node->value_count = 0;
		node->values = NULL;
	}

	node->idx = idx;

	return node;
}

int index_mm_open(const struct kmod_ctx *ctx, const char *filename,
		  unsigned long long *stamp, struct index_mm **pidx)
{
	int fd, err;
	struct stat st;
	struct index_mm *idx;
	struct {
		uint32_t magic;
		uint32_t version;
		uint32_t root_offset;
	} hdr;
	const void *p;

	assert(pidx != NULL);

	DBG(ctx, "file=%s\n", filename);

	idx = malloc(sizeof(*idx));
	if (idx == NULL) {
		ERR(ctx, "malloc: %m\n");
		return -ENOMEM;
	}

	if ((fd = open(filename, O_RDONLY | O_CLOEXEC)) < 0) {
		err = -errno;
		DBG(ctx, "open(%s, O_RDONLY|O_CLOEXEC): %m\n", filename);
		goto fail_open;
	}

	if (fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(hdr)) {
		err = -EINVAL;
		goto fail_nommap;
	}

	if ((uintmax_t)st.st_size > SIZE_MAX) {
		err = -ENOMEM;
		goto fail_nommap;
	}

	idx->mm = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (idx->mm == MAP_FAILED) {
		err = -errno;
		ERR(ctx, "mmap(NULL, %" PRIu64 ", PROT_READ, MAP_PRIVATE, %d, 0): %m\n",
		    (uint64_t)st.st_size, fd);
		goto fail_nommap;
	}

	p = idx->mm;
	hdr.magic = read_u32_mm(&p);
	hdr.version = read_u32_mm(&p);
	hdr.root_offset = read_u32_mm(&p);

	if (hdr.magic != INDEX_MAGIC) {
		ERR(ctx, "magic check fail: %x instead of %x\n", hdr.magic, INDEX_MAGIC);
		err = -EINVAL;
		goto fail;
	}

	if (hdr.version >> 16 != INDEX_VERSION_MAJOR) {
		ERR(ctx, "major version check fail: %u instead of %u\n",
		    hdr.version >> 16, INDEX_VERSION_MAJOR);
		err = -EINVAL;
		goto fail;
	}

	idx->root_offset = hdr.root_offset;
	idx->size = st.st_size;
	idx->ctx = ctx;
	close(fd);

	*stamp = stat_mstamp(&st);
	*pidx = idx;

	return 0;

fail:
	munmap(idx->mm, st.st_size);
fail_nommap:
	close(fd);
fail_open:
	free(idx);
	return err;
}

void index_mm_close(struct index_mm *idx)
{
	munmap(idx->mm, idx->size);
	free(idx);
}

static struct index_mm_node *index_mm_readroot(const struct index_mm *idx,
					       struct index_mm_node *root)
{
	return index_mm_read_node(idx, idx->root_offset, root);
}

static struct index_mm_node *index_mm_readchild(const struct index_mm_node *parent,
						uint8_t ch, struct index_mm_node *child)
{
	if (parent->first <= ch && ch <= parent->last) {
		const void *p;
		uint32_t off;

		p = (const char *)parent->children +
		    sizeof(uint32_t) * (ch - parent->first);
		off = read_u32_mm(&p);

		return index_mm_read_node(parent->idx, off, child);
	}

	return NULL;
}

static void index_mm_dump_node(struct index_mm_node *node, struct strbuf *buf,
			       struct wrtbuf *wbuf)
{
	const void *p;
	size_t i, pushed;

	pushed = strbuf_pushchars(buf, node->prefix);

	for (i = 0, p = node->values; i < node->value_count; i++) {
		struct index_mm_value v;

		read_value_mm(&p, &v);
		wrtbuf_write(wbuf, buf->bytes, strbuf_used(buf));
		wrtbuf_write(wbuf, " ", 1);
		wrtbuf_write(wbuf, v.value, v.len);
		wrtbuf_write(wbuf, "\n", 1);
	}

	for (uint8_t ch = node->first; ch <= node->last; ch++) {
		struct index_mm_node *child, nbuf;

		child = index_mm_readchild(node, ch, &nbuf);
		if (child == NULL)
			continue;

		if (strbuf_pushchar(buf, ch)) {
			index_mm_dump_node(child, buf, wbuf);
			strbuf_popchar(buf);
		}
	}

	strbuf_popchars(buf, pushed);
}

void index_mm_dump(const struct index_mm *idx, int fd, bool alias_prefix)
{
	DECLARE_STRBUF_WITH_STACK(buf, 128);
	struct index_mm_node nbuf, *root;
	struct wrtbuf wbuf;

	root = index_mm_readroot(idx, &nbuf);
	if (root == NULL)
		return;

	wrtbuf_init(&wbuf, fd);
	if (!alias_prefix || strbuf_pushchars(&buf, "alias "))
		index_mm_dump_node(root, &buf, &wbuf);
	wrtbuf_flush(&wbuf);
}

static char *index_mm_search_node(struct index_mm_node *node, const char *key)
{
	char *value;
	int j;

	while (node) {
		for (j = 0; node->prefix[j]; j++) {
			uint8_t ch = node->prefix[j];

			if (ch != key[j])
				return NULL;
		}

		key += j;

		if (*key == '\0') {
			if (node->value_count > 0) {
				const char *p = node->values;

				/* return first value without priority */
				p += sizeof(uint32_t);
				value = strdup(p);
			} else {
				value = NULL;
			}

			return value;
		}

		node = index_mm_readchild(node, *key, node);
		key++;
	}

	return NULL;
}

/*
 * Search the index for a key
 *
 * Returns the value of the first match
 */
char *index_mm_search(const struct index_mm *idx, const char *key)
{
	// FIXME: return value by reference instead of strdup
	struct index_mm_node nbuf, *root;
	char *value;

	root = index_mm_readroot(idx, &nbuf);
	value = index_mm_search_node(root, key);

	return value;
}

/* Level 4: add all the values from a matching node */
static void index_mm_searchwild_allvalues(struct index_mm_node *node,
					  struct index_value **out)
{
	const void *p;
	size_t i;

	for (i = 0, p = node->values; i < node->value_count; i++) {
		struct index_mm_value v;

		read_value_mm(&p, &v);
		add_value(out, v.value, v.len, v.priority);
	}
}

/*
 * Level 3: traverse a sub-keyspace which starts with a wildcard,
 * looking for matches.
 */
static void index_mm_searchwild_all(struct index_mm_node *node, int j, struct strbuf *buf,
				    const char *subkey, struct index_value **out)
{
	size_t pushed;

	pushed = strbuf_pushchars(buf, &node->prefix[j]);

	for (uint8_t ch = node->first; ch <= node->last; ch++) {
		struct index_mm_node *child, nbuf;

		child = index_mm_readchild(node, ch, &nbuf);
		if (!child)
			continue;

		if (strbuf_pushchar(buf, ch)) {
			index_mm_searchwild_all(child, 0, buf, subkey, out);
			strbuf_popchar(buf);
		}
	}

	if (node->value_count > 0) {
		const char *s = strbuf_str(buf);

		if (fnmatch(s, subkey, 0) == 0)
			index_mm_searchwild_allvalues(node, out);
	}

	strbuf_popchars(buf, pushed);
}

/* Level 2: descend the tree (until we hit a wildcard) */
static void index_mm_searchwild_node(struct index_mm_node *node, struct strbuf *buf,
				     const char *key, struct index_value **out)
{
	while (node) {
		struct index_mm_node *child, nbuf;
		int j;

		for (j = 0; node->prefix[j]; j++) {
			uint8_t ch = node->prefix[j];

			if (ch == '*' || ch == '?' || ch == '[') {
				index_mm_searchwild_all(node, j, buf, key + j, out);
				return;
			}

			if (ch != key[j])
				return;
		}

		key += j;

		child = index_mm_readchild(node, '*', &nbuf);
		if (child) {
			if (strbuf_pushchar(buf, '*')) {
				index_mm_searchwild_all(child, 0, buf, key, out);
				strbuf_popchar(buf);
			}
		}

		child = index_mm_readchild(node, '?', &nbuf);
		if (child) {
			if (strbuf_pushchar(buf, '?')) {
				index_mm_searchwild_all(child, 0, buf, key, out);
				strbuf_popchar(buf);
			}
		}

		child = index_mm_readchild(node, '[', &nbuf);
		if (child) {
			if (strbuf_pushchar(buf, '[')) {
				index_mm_searchwild_all(child, 0, buf, key, out);
				strbuf_popchar(buf);
			}
		}

		if (*key == '\0') {
			index_mm_searchwild_allvalues(node, out);

			return;
		}

		node = index_mm_readchild(node, *key, node);
		key++;
	}
}

/*
 * Search the index for a key.  The index may contain wildcards.
 *
 * Returns a list of all the values of matching keys.
 */
struct index_value *index_mm_searchwild(const struct index_mm *idx, const char *key)
{
	DECLARE_STRBUF_WITH_STACK(buf, 128);
	struct index_mm_node nbuf, *root;
	struct index_value *out = NULL;

	root = index_mm_readroot(idx, &nbuf);
	index_mm_searchwild_node(root, &buf, key, &out);
	return out;
}
