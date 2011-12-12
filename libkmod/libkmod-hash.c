/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011  ProFUSION embedded systems
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "libkmod.h"
#include "libkmod-private.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct kmod_hash_entry {
	const char *key;
	const void *value;
};

struct kmod_hash_bucket {
	struct kmod_hash_entry *entries;
	unsigned int used;
	unsigned int total;
};

struct kmod_hash {
	unsigned int count;
	unsigned int step;
	unsigned int n_buckets;
	void (*free_value)(void *value);
	struct kmod_hash_bucket buckets[];
};

struct kmod_hash *kmod_hash_new(unsigned int n_buckets,
					void (*free_value)(void *value))
{
	struct kmod_hash *hash = calloc(1, sizeof(struct kmod_hash) +
				n_buckets * sizeof(struct kmod_hash_bucket));
	if (hash == NULL)
		return NULL;
	hash->n_buckets = n_buckets;
	hash->free_value = free_value;
	hash->step = n_buckets / 32;
	if (hash->step == 0)
		hash->step = 4;
	else if (hash->step > 64)
		hash->step = 64;
	return hash;
}

void kmod_hash_free(struct kmod_hash *hash)
{
	struct kmod_hash_bucket *bucket, *bucket_end;
	bucket = hash->buckets;
	bucket_end = bucket + hash->n_buckets;
	for (; bucket < bucket_end; bucket++) {
		if (hash->free_value) {
			struct kmod_hash_entry *entry, *entry_end;
			entry = bucket->entries;
			entry_end = entry + bucket->used;
			for (; entry < entry_end; entry++)
				hash->free_value((void *)entry->value);
		}
		free(bucket->entries);
	}
	free(hash);
}

static inline unsigned int hash_superfast(const char *key, unsigned int len)
{
	/* Paul Hsieh (http://www.azillionmonkeys.com/qed/hash.html)
	 * used by WebCore (http://webkit.org/blog/8/hashtables-part-2/)
	 * EFL's eina and possible others.
	 */
	unsigned int tmp, hash = len, rem = len & 3;
	const unsigned short *itr = (const unsigned short *)key;

	len /= 4;

	/* Main loop */
	for (; len > 0; len--) {
		hash += itr[0];
		tmp = (itr[1] << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		itr += 2;
		hash += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
	case 3:
		hash += *itr;
		hash ^= hash << 16;
		hash ^= key[2] << 18;
		hash += hash >> 11;
		break;

	case 2:
		hash += *itr;
		hash ^= hash << 11;
		hash += hash >> 17;
		break;

	case 1:
		hash += *(const char *)itr;
		hash ^= hash << 10;
		hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

/*
 * add or replace key in hash map.
 *
 * none of key or value are copied, just references are remembered as is,
 * make sure they are live while pair exists in hash!
 */
int kmod_hash_add(struct kmod_hash *hash, const char *key, const void *value)
{
	unsigned int keylen = strlen(key);
	unsigned int hashval = hash_superfast(key, keylen);
	unsigned int pos = hashval % hash->n_buckets;
	struct kmod_hash_bucket *bucket = hash->buckets + pos;
	struct kmod_hash_entry *entry, *entry_end;

	if (bucket->used + 1 >= bucket->total) {
		unsigned new_total = bucket->total + hash->step;
		size_t size = new_total * sizeof(struct kmod_hash_entry);
		struct kmod_hash_entry *tmp = realloc(bucket->entries, size);
		if (tmp == NULL)
			return -errno;
		bucket->entries = tmp;
		bucket->total = new_total;
	}

	entry = bucket->entries;
	entry_end = entry + bucket->used;
	for (; entry < entry_end; entry++) {
		int c = strcmp(key, entry->key);
		if (c == 0) {
			hash->free_value((void *)entry->value);
			entry->value = value;
			return 0;
		} else if (c < 0) {
			memmove(entry + 1, entry,
				(entry_end - entry) * sizeof(struct kmod_hash_entry));
			break;
		}
	}

	entry->key = key;
	entry->value = value;
	bucket->used++;
	hash->count++;
	return 0;
}

static int kmod_hash_entry_cmp(const void *pa, const void *pb)
{
	const struct kmod_hash_entry *a = pa;
	const struct kmod_hash_entry *b = pb;
	return strcmp(a->key, b->key);
}

void *kmod_hash_find(const struct kmod_hash *hash, const char *key)
{
	unsigned int keylen = strlen(key);
	unsigned int hashval = hash_superfast(key, keylen);
	unsigned int pos = hashval % hash->n_buckets;
	const struct kmod_hash_bucket *bucket = hash->buckets + pos;
	const struct kmod_hash_entry se = {
		.key = key,
		.value = NULL
	};
	const struct kmod_hash_entry *entry = bsearch(
		&se, bucket->entries, bucket->used,
		sizeof(struct kmod_hash_entry), kmod_hash_entry_cmp);
	if (entry == NULL)
		return NULL;
	return (void *)entry->value;
}

int kmod_hash_del(struct kmod_hash *hash, const char *key)
{
	unsigned int keylen = strlen(key);
	unsigned int hashval = hash_superfast(key, keylen);
	unsigned int pos = hashval % hash->n_buckets;
	unsigned int steps_used, steps_total;
	struct kmod_hash_bucket *bucket = hash->buckets + pos;
	struct kmod_hash_entry *entry, *entry_end;
	const struct kmod_hash_entry se = {
		.key = key,
		.value = NULL
	};

	entry = bsearch(&se, bucket->entries, bucket->used,
		sizeof(struct kmod_hash_entry), kmod_hash_entry_cmp);
	if (entry == NULL)
		return -ENOENT;

	entry_end = bucket->entries + bucket->used;
	memmove(entry, entry + 1,
		(entry_end - entry) * sizeof(struct kmod_hash_entry));

	bucket->used--;
	hash->count--;

	steps_used = bucket->used / hash->step;
	steps_total = bucket->total / hash->step;
	if (steps_used + 1 < steps_total) {
		size_t size = (steps_used + 1) *
			hash->step * sizeof(struct kmod_hash_entry);
		struct kmod_hash_entry *tmp = realloc(bucket->entries, size);
		if (tmp) {
			bucket->entries = tmp;
			bucket->total = (steps_used + 1) * hash->step;
		}
	}

	return 0;
}
