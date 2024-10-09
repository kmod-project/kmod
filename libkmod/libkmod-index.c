// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

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
 * Magic spells "BOOTFAST". Second one used on newer versioned binary files.
 * #define INDEX_MAGIC_OLD 0xB007FA57
 *
 * We use a version string to keep track of changes to the binary format
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
 *       unsigned char first;
 *       unsigned char last;
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

void index_values_free(struct index_value *values)
{
	while (values) {
		struct index_value *value = values;

		values = value->next;
		free(value);
	}
}

static int add_value(struct index_value **values, const char *value, size_t len,
		     unsigned int priority)
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

static int read_char(FILE *in)
{
	int ch;

	errno = 0;
	ch = getc_unlocked(in);
	if (ch == EOF)
		errno = EINVAL;
	return ch;
}

static int read_u32(FILE *in, uint32_t *l)
{
	uint32_t val;

	errno = 0;
	if (fread(&val, sizeof(uint32_t), 1, in) != 1) {
		errno = EINVAL;
		return -1;
	}
	*l = ntohl(val);
	return 1;
}

static ssize_t buf_freadchars(struct strbuf *buf, FILE *in)
{
	ssize_t i = 0;
	int ch;

	while ((ch = read_char(in))) {
		if (ch == EOF || !strbuf_pushchar(buf, ch))
			return -1;
		i++;
	}

	return i;
}

/*
 * Index file searching
 */
struct index_node_f {
	FILE *file;
	char *prefix; /* path compression */
	struct index_value *values;
	unsigned char first; /* range of child nodes */
	unsigned char last;
	uint32_t children[0];
};

static struct index_node_f *index_read(FILE *in, uint32_t offset)
{
	struct index_node_f *node = NULL;
	char *prefix = NULL;
	int i, child_count = 0;

	if ((offset & INDEX_NODE_MASK) == 0)
		return NULL;

	if (fseek(in, offset & INDEX_NODE_MASK, SEEK_SET) < 0)
		return NULL;

	if (offset & INDEX_NODE_PREFIX) {
		struct strbuf buf;
		strbuf_init(&buf);
		if (buf_freadchars(&buf, in) < 0) {
			strbuf_release(&buf);
			return NULL;
		}
		prefix = strbuf_steal(&buf);
	} else
		prefix = strdup("");

	if (prefix == NULL)
		goto err;

	if (offset & INDEX_NODE_CHILDS) {
		int first = read_char(in);
		int last = read_char(in);

		if (first == EOF || last == EOF || first > last)
			goto err;

		child_count = last - first + 1;

		node = malloc(sizeof(struct index_node_f) +
			      sizeof(uint32_t) * child_count);
		if (node == NULL)
			goto err;

		node->first = (unsigned char)first;
		node->last = (unsigned char)last;

		for (i = 0; i < child_count; i++)
			if (read_u32(in, &node->children[i]) < 0)
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
		struct strbuf buf;
		const char *value;
		unsigned int priority;

		if (read_u32(in, &value_count) < 0)
			goto err;

		strbuf_init(&buf);
		while (value_count--) {
			if (read_u32(in, &priority) < 0 || buf_freadchars(&buf, in) < 0) {
				strbuf_release(&buf);
				goto err;
			}
			value = strbuf_str(&buf);
			if (value == NULL) {
				strbuf_release(&buf);
				goto err;
			}
			add_value(&node->values, value, buf.used, priority);
			strbuf_clear(&buf);
		}
		strbuf_release(&buf);
	}

	node->prefix = prefix;
	node->file = in;
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

struct index_file {
	FILE *file;
	uint32_t root_offset;
};

struct index_file *index_file_open(const char *filename)
{
	FILE *file;
	uint32_t magic, version;
	struct index_file *new;

	file = fopen(filename, "re");
	if (!file)
		return NULL;
	errno = EINVAL;

	if (read_u32(file, &magic) < 0 || magic != INDEX_MAGIC)
		goto err;

	if (read_u32(file, &version) < 0 || version >> 16 != INDEX_VERSION_MAJOR)
		goto err;

	new = malloc(sizeof(struct index_file));
	if (new == NULL)
		goto err;

	new->file = file;
	if (read_u32(new->file, &new->root_offset) < 0) {
		free(new);
		goto err;
	}

	errno = 0;
	return new;
err:
	fclose(file);
	return NULL;
}

void index_file_close(struct index_file *idx)
{
	fclose(idx->file);
	free(idx);
}

static struct index_node_f *index_readroot(struct index_file *in)
{
	return index_read(in->file, in->root_offset);
}

static struct index_node_f *index_readchild(const struct index_node_f *parent, int ch)
{
	if (parent->first <= ch && ch <= parent->last) {
		return index_read(parent->file, parent->children[ch - parent->first]);
	}

	return NULL;
}

static void index_dump_node(struct index_node_f *node, struct strbuf *buf, int fd)
{
	struct index_value *v;
	size_t pushed;
	int ch;

	pushed = strbuf_pushchars(buf, node->prefix);

	for (v = node->values; v != NULL; v = v->next) {
		write_str_safe(fd, buf->bytes, buf->used);
		write_str_safe(fd, " ", 1);
		write_str_safe(fd, v->value, strlen(v->value));
		write_str_safe(fd, "\n", 1);
	}

	for (ch = node->first; ch <= node->last; ch++) {
		struct index_node_f *child = index_readchild(node, ch);

		if (!child)
			continue;

		if (strbuf_pushchar(buf, ch)) {
			index_dump_node(child, buf, fd);
			strbuf_popchar(buf);
		}
	}

	strbuf_popchars(buf, pushed);
	index_close(node);
}

void index_dump(struct index_file *in, int fd, const char *prefix)
{
	struct index_node_f *root;
	struct strbuf buf;

	root = index_readroot(in);
	if (root == NULL)
		return;

	strbuf_init(&buf);
	if (strbuf_pushchars(&buf, prefix))
		index_dump_node(root, &buf, fd);
	strbuf_release(&buf);
}

static char *index_search__node(struct index_node_f *node, const char *key, int i)
{
	char *value;
	struct index_node_f *child;
	int ch;
	int j;

	while (node) {
		for (j = 0; node->prefix[j]; j++) {
			ch = node->prefix[j];

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
	int ch;

	pushed = strbuf_pushchars(buf, &node->prefix[j]);

	for (ch = node->first; ch <= node->last; ch++) {
		struct index_node_f *child = index_readchild(node, ch);

		if (!child)
			continue;

		if (strbuf_pushchar(buf, ch)) {
			index_searchwild__all(child, 0, buf, subkey, out);
			strbuf_popchar(buf);
		}
	}

	if (pushed && node->values) {
		const char *s = strbuf_str(buf);

		if (s != NULL && fnmatch(s, subkey, 0) == 0)
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
				   const char *key, int i, struct index_value **out)
{
	struct index_node_f *child;
	int j;
	int ch;

	while (node) {
		for (j = 0; node->prefix[j]; j++) {
			ch = node->prefix[j];

			if (ch == '*' || ch == '?' || ch == '[') {
				index_searchwild__all(node, j, buf, &key[i + j], out);
				return;
			}

			if (ch != key[i + j]) {
				index_close(node);
				return;
			}
		}

		i += j;

		child = index_readchild(node, '*');
		if (child) {
			if (strbuf_pushchar(buf, '*')) {
				index_searchwild__all(child, 0, buf, &key[i], out);
				strbuf_popchar(buf);
			}
		}

		child = index_readchild(node, '?');
		if (child) {
			if (strbuf_pushchar(buf, '?')) {
				index_searchwild__all(child, 0, buf, &key[i], out);
				strbuf_popchar(buf);
			}
		}

		child = index_readchild(node, '[');
		if (child) {
			if (strbuf_pushchar(buf, '[')) {
				index_searchwild__all(child, 0, buf, &key[i], out);
				strbuf_popchar(buf);
			}
		}

		if (key[i] == '\0') {
			index_searchwild__allvalues(node, out);

			return;
		}

		child = index_readchild(node, key[i]);
		index_close(node);
		node = child;
		i++;
	}
}

/*
 * Search the index for a key.  The index may contain wildcards.
 *
 * Returns a list of all the values of matching keys.
 */
struct index_value *index_searchwild(struct index_file *in, const char *key)
{
	struct index_node_f *root = index_readroot(in);
	struct strbuf buf;
	struct index_value *out = NULL;

	strbuf_init(&buf);
	index_searchwild__node(root, &buf, key, 0, &out);
	strbuf_release(&buf);
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

static const char _idx_empty_str[] = "";

struct index_mm {
	const struct kmod_ctx *ctx;
	void *mm;
	uint32_t root_offset;
	size_t size;
};

struct index_mm_value {
	unsigned int priority;
	size_t len;
	const char *value;
};

struct index_mm_value_array {
	struct index_mm_value *values;
	size_t len;
};

struct index_mm_node {
	struct index_mm *idx;
	const char *prefix; /* mmape'd value */
	struct index_mm_value_array values;
	unsigned char first;
	unsigned char last;
	uint32_t children[];
};

static inline uint32_t read_u32_mm(void **p)
{
	uint8_t *addr = *(uint8_t **)p;
	uint32_t v;

	/* addr may be unalined to uint32_t */
	v = get_unaligned((uint32_t *)addr);

	*p = addr + sizeof(uint32_t);
	return ntohl(v);
}

static inline uint8_t read_char_mm(void **p)
{
	uint8_t *addr = *(uint8_t **)p;
	uint8_t v = *addr;
	*p = addr + sizeof(uint8_t);
	return v;
}

static inline char *read_chars_mm(void **p, size_t *rlen)
{
	char *addr = *(char **)p;
	size_t len = *rlen = strlen(addr);
	*p = addr + len + 1;
	return addr;
}

static struct index_mm_node *index_mm_read_node(struct index_mm *idx, uint32_t offset)
{
	void *p = idx->mm;
	struct index_mm_node *node;
	const char *prefix;
	int i, child_count, value_count, children_padding;
	uint32_t children[INDEX_CHILDMAX];
	unsigned char first, last;

	if ((offset & INDEX_NODE_MASK) == 0)
		return NULL;

	p = (char *)p + (offset & INDEX_NODE_MASK);

	if (offset & INDEX_NODE_PREFIX) {
		size_t len;
		prefix = read_chars_mm(&p, &len);
	} else
		prefix = _idx_empty_str;

	if (offset & INDEX_NODE_CHILDS) {
		first = read_char_mm(&p);
		last = read_char_mm(&p);

		if (first > last || first >= INDEX_CHILDMAX || last >= INDEX_CHILDMAX)
			return NULL;

		child_count = last - first + 1;
		for (i = 0; i < child_count; i++)
			children[i] = read_u32_mm(&p);
	} else {
		first = INDEX_CHILDMAX;
		last = 0;
		child_count = 0;
	}

	children_padding =
		(sizeof(struct index_mm_node) + (sizeof(uint32_t) * child_count)) %
		sizeof(void *);

	if (offset & INDEX_NODE_VALUES)
		value_count = read_u32_mm(&p);
	else
		value_count = 0;

	node = malloc(sizeof(struct index_mm_node) + sizeof(uint32_t) * child_count +
		      children_padding + sizeof(struct index_mm_value) * value_count);
	if (node == NULL)
		return NULL;

	node->idx = idx;
	node->prefix = prefix;
	if (value_count == 0)
		node->values.values = NULL;
	else {
		node->values.values =
			(struct index_mm_value *)((char *)node +
						  sizeof(struct index_mm_node) +
						  sizeof(uint32_t) * child_count +
						  children_padding);
	}
	node->values.len = value_count;
	node->first = first;
	node->last = last;
	memcpy(node->children, children, sizeof(uint32_t) * child_count);

	for (i = 0; i < value_count; i++) {
		struct index_mm_value *v = node->values.values + i;
		v->priority = read_u32_mm(&p);
		v->value = read_chars_mm(&p, &v->len);
	}

	return node;
}

static void index_mm_free_node(struct index_mm_node *node)
{
	free(node);
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
	void *p;

	assert(pidx != NULL);

	DBG(ctx, "file=%s\n", filename);

	idx = malloc(sizeof(*idx));
	if (idx == NULL) {
		ERR(ctx, "malloc: %m\n");
		return -ENOMEM;
	}

	if ((fd = open(filename, O_RDONLY | O_CLOEXEC)) < 0) {
		DBG(ctx, "open(%s, O_RDONLY|O_CLOEXEC): %m\n", filename);
		err = -errno;
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
		ERR(ctx, "mmap(NULL, %" PRIu64 ", PROT_READ, MAP_PRIVATE, %d, 0): %m\n",
		    (uint64_t)st.st_size, fd);
		err = -errno;
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

static struct index_mm_node *index_mm_readroot(struct index_mm *idx)
{
	return index_mm_read_node(idx, idx->root_offset);
}

static struct index_mm_node *index_mm_readchild(const struct index_mm_node *parent, int ch)
{
	if (parent->first <= ch && ch <= parent->last) {
		return index_mm_read_node(parent->idx,
					  parent->children[ch - parent->first]);
	}

	return NULL;
}

static void index_mm_dump_node(struct index_mm_node *node, struct strbuf *buf, int fd)
{
	struct index_mm_value *itr, *itr_end;
	size_t pushed;
	int ch;

	pushed = strbuf_pushchars(buf, node->prefix);

	itr = node->values.values;
	itr_end = itr + node->values.len;
	for (; itr < itr_end; itr++) {
		write_str_safe(fd, buf->bytes, buf->used);
		write_str_safe(fd, " ", 1);
		write_str_safe(fd, itr->value, itr->len);
		write_str_safe(fd, "\n", 1);
	}

	for (ch = node->first; ch <= node->last; ch++) {
		struct index_mm_node *child = index_mm_readchild(node, ch);

		if (child == NULL)
			continue;

		if (strbuf_pushchar(buf, ch)) {
			index_mm_dump_node(child, buf, fd);
			strbuf_popchar(buf);
		}
	}

	strbuf_popchars(buf, pushed);
	index_mm_free_node(node);
}

void index_mm_dump(struct index_mm *idx, int fd, const char *prefix)
{
	struct index_mm_node *root;
	struct strbuf buf;

	root = index_mm_readroot(idx);
	if (root == NULL)
		return;

	strbuf_init(&buf);
	if (strbuf_pushchars(&buf, prefix))
		index_mm_dump_node(root, &buf, fd);
	strbuf_release(&buf);
}

static char *index_mm_search_node(struct index_mm_node *node, const char *key, int i)
{
	char *value;
	struct index_mm_node *child;
	int ch;
	int j;

	while (node) {
		for (j = 0; node->prefix[j]; j++) {
			ch = node->prefix[j];

			if (ch != key[i + j]) {
				index_mm_free_node(node);
				return NULL;
			}
		}

		i += j;

		if (key[i] == '\0') {
			value = node->values.len > 0 ?
					strdup(node->values.values[0].value) :
					NULL;

			index_mm_free_node(node);
			return value;
		}

		child = index_mm_readchild(node, key[i]);
		index_mm_free_node(node);
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
char *index_mm_search(struct index_mm *idx, const char *key)
{
	// FIXME: return value by reference instead of strdup
	struct index_mm_node *root;
	char *value;

	root = index_mm_readroot(idx);
	value = index_mm_search_node(root, key, 0);

	return value;
}

/* Level 4: add all the values from a matching node */
static void index_mm_searchwild_allvalues(struct index_mm_node *node,
					  struct index_value **out)
{
	struct index_mm_value *itr, *itr_end;

	itr = node->values.values;
	itr_end = itr + node->values.len;
	for (; itr < itr_end; itr++)
		add_value(out, itr->value, itr->len, itr->priority);

	index_mm_free_node(node);
}

/*
 * Level 3: traverse a sub-keyspace which starts with a wildcard,
 * looking for matches.
 */
static void index_mm_searchwild_all(struct index_mm_node *node, int j, struct strbuf *buf,
				    const char *subkey, struct index_value **out)
{
	size_t pushed;
	int ch;

	pushed = strbuf_pushchars(buf, &node->prefix[j]);

	for (ch = node->first; ch <= node->last; ch++) {
		struct index_mm_node *child = index_mm_readchild(node, ch);

		if (!child)
			continue;

		if (strbuf_pushchar(buf, ch)) {
			index_mm_searchwild_all(child, 0, buf, subkey, out);
			strbuf_popchar(buf);
		}
	}

	if (pushed && node->values.len > 0) {
		const char *s = strbuf_str(buf);

		if (s != NULL && fnmatch(s, subkey, 0) == 0)
			index_mm_searchwild_allvalues(node, out);
		else
			index_mm_free_node(node);
	} else {
		index_mm_free_node(node);
	}

	strbuf_popchars(buf, pushed);
}

/* Level 2: descend the tree (until we hit a wildcard) */
static void index_mm_searchwild_node(struct index_mm_node *node, struct strbuf *buf,
				     const char *key, int i, struct index_value **out)
{
	struct index_mm_node *child;
	int j;
	int ch;

	while (node) {
		for (j = 0; node->prefix[j]; j++) {
			ch = node->prefix[j];

			if (ch == '*' || ch == '?' || ch == '[') {
				index_mm_searchwild_all(node, j, buf, &key[i + j], out);
				return;
			}

			if (ch != key[i + j]) {
				index_mm_free_node(node);
				return;
			}
		}

		i += j;

		child = index_mm_readchild(node, '*');
		if (child) {
			if (strbuf_pushchar(buf, '*')) {
				index_mm_searchwild_all(child, 0, buf, &key[i], out);
				strbuf_popchar(buf);
			}
		}

		child = index_mm_readchild(node, '?');
		if (child) {
			if (strbuf_pushchar(buf, '?')) {
				index_mm_searchwild_all(child, 0, buf, &key[i], out);
				strbuf_popchar(buf);
			}
		}

		child = index_mm_readchild(node, '[');
		if (child) {
			if (strbuf_pushchar(buf, '[')) {
				index_mm_searchwild_all(child, 0, buf, &key[i], out);
				strbuf_popchar(buf);
			}
		}

		if (key[i] == '\0') {
			index_mm_searchwild_allvalues(node, out);

			return;
		}

		child = index_mm_readchild(node, key[i]);
		index_mm_free_node(node);
		node = child;
		i++;
	}
}

/*
 * Search the index for a key.  The index may contain wildcards.
 *
 * Returns a list of all the values of matching keys.
 */
struct index_value *index_mm_searchwild(struct index_mm *idx, const char *key)
{
	struct index_mm_node *root = index_mm_readroot(idx);
	struct strbuf buf;
	struct index_value *out = NULL;

	strbuf_init(&buf);
	index_mm_searchwild_node(root, &buf, key, 0, &out);
	strbuf_release(&buf);
	return out;
}
