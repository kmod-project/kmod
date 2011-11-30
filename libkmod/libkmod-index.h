
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

#ifndef MODINITTOOLS_INDEX_H
#define MODINITTOOLS_INDEX_H

#include <stdint.h>

/* Integers are stored as 32 bit unsigned in "network" order, i.e. MSB first.
   All files start with a magic number.

   Magic spells "BOOTFAST". Second one used on newer versioned binary files.
 */
/* #define INDEX_MAGIC_OLD 0xB007FA57 */
#define INDEX_MAGIC 0xB007F457

/* We use a version string to keep track of changes to the binary format
 * This is stored in the form: INDEX_MAJOR (hi) INDEX_MINOR (lo) just in
 * case we ever decide to have minor changes that are not incompatible.
 */

#define INDEX_VERSION_MAJOR 0x0002
#define INDEX_VERSION_MINOR 0x0001
#define INDEX_VERSION ((INDEX_VERSION_MAJOR<<16)|INDEX_VERSION_MINOR)

/* The index file maps keys to values. Both keys and values are ASCII strings.
   Each key can have multiple values. Values are sorted by an integer priority.

   The reader also implements a wildcard search (including range expressions)
   where the keys in the index are treated as patterns.
   This feature is required for module aliases.
*/

/* Implementation is based on a radix tree, or "trie".
   Each arc from parent to child is labelled with a character.
   Each path from the root represents a string.

   == Example strings ==

   ask
   ate
   on
   once
   one

   == Key ==
    + Normal node
    * Marked node, representing a key and it's values.

   +
   |-a-+-s-+-k-*
   |   |
   |   `-t-+-e-*
   |
   `-o-+-n-*-c-+-e-*
           |
           `-e-*

   Naive implementations tend to be very space inefficient; child pointers
   are stored in arrays indexed by character, but most child pointers are null.

   Our implementation uses a scheme described by Wikipedia as a Patrica trie,

       "easiest to understand as a space-optimized trie where
        each node with only one child is merged with its child"

   +
   |-a-+-sk-*
   |   |
   |   `-te-*
   |
   `-on-*-ce-*
        |
        `-e-*

   We still use arrays of child pointers indexed by a single character;
   the remaining characters of the label are stored as a "prefix" in the child.

   The paper describing the original Patrica trie works on individiual bits -
   each node has a maximum of two children, which increases space efficiency.
   However for this application it is simpler to use the ASCII character set.
   Since the index file is read-only, it can be compressed by omitting null
   child pointers at the start and end of arrays.
*/

#define INDEX_PRIORITY_MIN UINT32_MAX

struct index_value {
	struct index_value *next;
	unsigned int priority;
	char value[0];
};

/* In-memory index (depmod only) */

#define INDEX_CHILDMAX 128
struct index_node {
	char *prefix;		/* path compression */
	struct index_value *values;
	unsigned char first;	/* range of child nodes */
	unsigned char last;
	struct index_node *children[INDEX_CHILDMAX]; /* indexed by character */
};

/* Disk format:

   uint32_t magic = INDEX_MAGIC;
   uint32_t version = INDEX_VERSION;
   uint32_t root_offset;

   (node_offset & INDEX_NODE_MASK) specifies the file offset of nodes:

        char[] prefix; // nul terminated

        char first;
        char last;
        uint32_t children[last - first + 1];

        uint32_t value_count;
        struct {
            uint32_t priority;
            char[] value; // nul terminated
        } values[value_count];

   (node_offset & INDEX_NODE_FLAGS) indicates which fields are present.
   Empty prefixes are ommitted, leaf nodes omit the three child-related fields.

   This could be optimised further by adding a sparse child format
   (indicated using a new flag).
 */

/* Format of node offsets within index file */
enum node_offset {
	INDEX_NODE_FLAGS    = 0xF0000000, /* Flags in high nibble */
	INDEX_NODE_PREFIX   = 0x80000000,
	INDEX_NODE_VALUES = 0x40000000,
	INDEX_NODE_CHILDS   = 0x20000000,

	INDEX_NODE_MASK     = 0x0FFFFFFF, /* Offset value */
};

struct index_file;

struct index_node *index_create(void);
void index_destroy(struct index_node *node);
int index_insert(struct index_node *node, const char *key,
		 const char *value, unsigned int priority);
void index_write(const struct index_node *node, FILE *out);

struct index_file *index_file_open(const char *filename);
void index_file_close(struct index_file *index);

/* Dump all strings in index as lines in a plain text file
   (prefix is prepended to each line)
*/
void index_dump(struct index_file *in, FILE *out, const char *prefix);

/* Return value for first matching key.
   Keys must be exactly equal to match - i.e. there are no wildcard patterns
*/
char *index_search(struct index_file *index, const char *key);

/* Return values for all matching keys.
   The keys in the index are treated as wildcard patterns using fnmatch()
*/
struct index_value *index_searchwild(struct index_file *index, const char *key);

void index_values_free(struct index_value *values);

#endif /* MODINITTOOLS_INDEX_H */
