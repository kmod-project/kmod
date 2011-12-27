#ifndef _LIBKMOD_HASH_H_
#define _LIBKMOD_HASH_H_

struct hash;
struct hash *hash_new(unsigned int n_buckets, void (*free_value)(void *value));
void hash_free(struct hash *hash);
int hash_add(struct hash *hash, const char *key, const void *value);
int hash_add_unique(struct hash *hash, const char *key, const void *value);
int hash_del(struct hash *hash, const char *key);
void *hash_find(const struct hash *hash, const char *key);
unsigned int hash_get_count(const struct hash *hash);

#endif
