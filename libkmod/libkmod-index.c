/* index.c: module index file shared functions for modprobe and depmod
    Copyright (C) 2008  Alan Jenkins <alan-jenkins@tuffmail.co.uk>.

    These programs are free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with these programs.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <arpa/inet.h> /* htonl */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fnmatch.h>

#include "libkmod-private.h"
#include "libkmod-index.h"
#include "macro.h"

void index_values_free(struct index_value *values)
{
	while (values) {
		struct index_value *value = values;

		values = value->next;
		free(value);
	}
}

static int add_value(struct index_value **values,
		     const char *value, unsigned int priority)
{
	struct index_value *v;
	int duplicate = 0;
	int len;

	/* report the presence of duplicate values */
	for (v = *values; v; v = v->next) {
		if (streq(v->value, value))
			duplicate = 1;
	}

	/* find position to insert value */
	while (*values && (*values)->priority < priority)
		values = &(*values)->next;

	len = strlen(value);
	v = NOFAIL(calloc(sizeof(struct index_value) + len + 1, 1));
	v->next = *values;
	v->priority = priority;
	memcpy(v->value, value, len + 1);
	*values = v;

	return duplicate;
}

static void read_error(void)
{
	fatal("Module index: unexpected error: %s\n"
			"Try re-running depmod\n", errno ? strerror(errno) : "EOF");
}

static int read_char(FILE *in)
{
	int ch;

	errno = 0;
	ch = getc_unlocked(in);
	if (ch == EOF)
		read_error();
	return ch;
}

static uint32_t read_long(FILE *in)
{
	uint32_t l;

	errno = 0;
	if (fread(&l, sizeof(uint32_t), 1, in) <= 0)
		read_error();
	return ntohl(l);
}

/*
 * Buffer abstract data type
 *
 * Used internally to store the current path during tree traversal.
 * They help build wildcard key strings to pass to fnmatch(),
 * as well as building values of matching keys.
 */

struct buffer {
	char *bytes;
	unsigned size;
	unsigned used;
};

static void buf__realloc(struct buffer *buf, unsigned size)
{
	if (size > buf->size) {
		buf->bytes = NOFAIL(realloc(buf->bytes, size));
		buf->size = size;
	}
}

static struct buffer *buf_create(void)
{
	struct buffer *buf;

	buf = NOFAIL(calloc(sizeof(struct buffer), 1));
	buf__realloc(buf, 256);
	return buf;
}

static void buf_destroy(struct buffer *buf)
{
	free(buf->bytes);
	free(buf);
}

/* Destroy buffer and return a copy as a C string */
static char *buf_detach(struct buffer *buf)
{
	char *bytes;

	bytes = NOFAIL(realloc(buf->bytes, buf->used + 1));
	bytes[buf->used] = '\0';

	free(buf);
	return bytes;
}

/* Return a C string owned by the buffer
   (invalidated if the buffer is changed).
 */
static const char *buf_str(struct buffer *buf)
{
	buf__realloc(buf, buf->used + 1);
	buf->bytes[buf->used] = '\0';
	return buf->bytes;
}

static void buf_pushchar(struct buffer *buf, char ch)
{
	buf__realloc(buf, buf->used + 1);
	buf->bytes[buf->used] = ch;
	buf->used++;
}

/* like buf_pushchars(), but the string comes from a file */
static unsigned buf_freadchars(struct buffer *buf, FILE *in)
{
	unsigned i = 0;
	int ch;

	while ((ch = read_char(in))) {
		buf_pushchar(buf, ch);
		i++;
	}

	return i;
}

static void buf_popchar(struct buffer *buf)
{
	buf->used--;
}

static void buf_popchars(struct buffer *buf, unsigned n)
{
	buf->used -= n;
}

static void buf_clear(struct buffer *buf)
{
	buf->used = 0;
}

/*
 * Index file searching (used only by modprobe)
 */

struct index_node_f {
	FILE *file;
	char *prefix;		/* path compression */
	struct index_value *values;
	unsigned char first;	/* range of child nodes */
	unsigned char last;
	uint32_t children[0];
};

static struct index_node_f *index_read(FILE *in, uint32_t offset)
{
	struct index_node_f *node;
	char *prefix;
	int i, child_count = 0;

	if ((offset & INDEX_NODE_MASK) == 0)
		return NULL;

	fseek(in, offset & INDEX_NODE_MASK, SEEK_SET);

	if (offset & INDEX_NODE_PREFIX) {
		struct buffer *buf = buf_create();
		buf_freadchars(buf, in);
		prefix = buf_detach(buf);
	} else
		prefix = NOFAIL(strdup(""));

	if (offset & INDEX_NODE_CHILDS) {
		char first = read_char(in);
		char last = read_char(in);
		child_count = last - first + 1;

		node = NOFAIL(malloc(sizeof(struct index_node_f) +
				     sizeof(uint32_t) * child_count));

		node->first = first;
		node->last = last;

		for (i = 0; i < child_count; i++)
			node->children[i] = read_long(in);
	} else {
		node = NOFAIL(malloc(sizeof(struct index_node_f)));
		node->first = INDEX_CHILDMAX;
		node->last = 0;
	}

	node->values = NULL;
	if (offset & INDEX_NODE_VALUES) {
		int value_count;
		struct buffer *buf = buf_create();
		const char *value;
		unsigned int priority;

		value_count = read_long(in);

		while (value_count--) {
			priority = read_long(in);
			buf_freadchars(buf, in);
			value = buf_str(buf);
			add_value(&node->values, value, priority);
			buf_clear(buf);
		}
		buf_destroy(buf);
	}

	node->prefix = prefix;
	node->file = in;
	return node;
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

/* Failures are silent; modprobe will fall back to text files */
struct index_file *index_file_open(const char *filename)
{
	FILE *file;
	uint32_t magic, version;
	struct index_file *new;

	file = fopen(filename, "r");
	if (!file)
		return NULL;
	errno = EINVAL;

	magic = read_long(file);
	if (magic != INDEX_MAGIC)
		return NULL;

	version = read_long(file);
	if (version >> 16 != INDEX_VERSION_MAJOR)
		return NULL;

	new = NOFAIL(malloc(sizeof(struct index_file)));
	new->file = file;
	new->root_offset = read_long(new->file);

	errno = 0;
	return new;
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

static struct index_node_f *index_readchild(const struct index_node_f *parent,
					    int ch)
{
	if (parent->first <= ch && ch <= parent->last)
		return index_read(parent->file,
		                       parent->children[ch - parent->first]);
	else
		return NULL;
}

static char *index_search__node(struct index_node_f *node, const char *key, int i)
{
	char *value;
	struct index_node_f *child;
	int ch;
	int j;

	while(node) {
		for (j = 0; node->prefix[j]; j++) {
			ch = node->prefix[j];

			if (ch != key[i+j]) {
				index_close(node);
				return NULL;
			}
		}
		i += j;

		if (key[i] == '\0') {
			if (node->values) {
				value = strdup(node->values[0].value);
				index_close(node);
				return value;
			} else {
				return NULL;
			}
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
		add_value(out, v->value, v->priority);

	index_close(node);
}

/*
 * Level 3: traverse a sub-keyspace which starts with a wildcard,
 * looking for matches.
 */
static void index_searchwild__all(struct index_node_f *node, int j,
				  struct buffer *buf,
				  const char *subkey,
				  struct index_value **out)
{
	int pushed = 0;
	int ch;

	while (node->prefix[j]) {
		ch = node->prefix[j];

		buf_pushchar(buf, ch);
		pushed++;
		j++;
	}

	for (ch = node->first; ch <= node->last; ch++) {
		struct index_node_f *child = index_readchild(node, ch);

		if (!child)
			continue;

		buf_pushchar(buf, ch);
		index_searchwild__all(child, 0, buf, subkey, out);
		buf_popchar(buf);
	}

	if (node->values) {
		if (fnmatch(buf_str(buf), subkey, 0) == 0)
			index_searchwild__allvalues(node, out);
	} else {
		index_close(node);
	}

	buf_popchars(buf, pushed);
}

/* Level 2: descend the tree (until we hit a wildcard) */
static void index_searchwild__node(struct index_node_f *node,
				   struct buffer *buf,
				   const char *key, int i,
				   struct index_value **out)
{
	struct index_node_f *child;
	int j;
	int ch;

	while(node) {
		for (j = 0; node->prefix[j]; j++) {
			ch = node->prefix[j];

			if (ch == '*' || ch == '?' || ch == '[') {
				index_searchwild__all(node, j, buf,
						      &key[i+j], out);
				return;
			}

			if (ch != key[i+j]) {
				index_close(node);
				return;
			}
		}
		i += j;

		child = index_readchild(node, '*');
		if (child) {
			buf_pushchar(buf, '*');
			index_searchwild__all(child, 0, buf, &key[i], out);
			buf_popchar(buf);
		}

		child = index_readchild(node, '?');
		if (child) {
			buf_pushchar(buf, '?');
			index_searchwild__all(child, 0, buf, &key[i], out);
			buf_popchar(buf);
		}

		child = index_readchild(node, '[');
		if (child) {
			buf_pushchar(buf, '[');
			index_searchwild__all(child, 0, buf, &key[i], out);
			buf_popchar(buf);
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
	struct buffer *buf = buf_create();
	struct index_value *out = NULL;

	index_searchwild__node(root, buf, key, 0, &out);
	buf_destroy(buf);
	return out;
}
