#ifndef _LIBKMOD_PRIVATE_H_
#define _LIBKMOD_PRIVATE_H_

#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>
#include <limits.h>

#include "macro.h"
#include "libkmod.h"

static __always_inline __printf_format(2, 3) void
	kmod_log_null(struct kmod_ctx *ctx, const char *format, ...) {}

#define kmod_log_cond(ctx, prio, arg...) \
	do { \
		if (kmod_get_log_priority(ctx) >= prio) \
		kmod_log(ctx, prio, __FILE__, __LINE__, __FUNCTION__, ## arg);\
	} while (0)

#ifdef ENABLE_LOGGING
#  ifdef ENABLE_DEBUG
#    define DBG(ctx, arg...) kmod_log_cond(ctx, LOG_DEBUG, ## arg)
#  else
#    define DBG(ctx, arg...) kmod_log_null(ctx, ## arg)
#  endif
#  define INFO(ctx, arg...) kmod_log_cond(ctx, LOG_INFO, ## arg)
#  define ERR(ctx, arg...) kmod_log_cond(ctx, LOG_ERR, ## arg)
#else
#  define DBG(ctx, arg...) kmod_log_null(ctx, ## arg)
#  define INFO(ctx, arg...) kmod_log_null(ctx, ## arg)
#  define ERR(ctx, arg...) kmod_log_null(ctx, ## arg)
#endif

#define KMOD_EXPORT __attribute__ ((visibility("default")))

#define KCMD_LINE_SIZE 4096

void kmod_log(const struct kmod_ctx *ctx,
		int priority, const char *file, int line, const char *fn,
		const char *format, ...) __attribute__((format(printf, 6, 7))) __attribute__((nonnull(1, 3, 5)));

struct list_node {
	struct list_node *next, *prev;
};

struct kmod_list {
	struct list_node node;
	void *data;
};

struct kmod_list *kmod_list_append(struct kmod_list *list, const void *data) __must_check __attribute__((nonnull(2)));
struct kmod_list *kmod_list_prepend(struct kmod_list *list, const void *data) __must_check __attribute__((nonnull(2)));
struct kmod_list *kmod_list_remove(struct kmod_list *list) __must_check;
struct kmod_list *kmod_list_remove_data(struct kmod_list *list,
					const void *data) __must_check __attribute__((nonnull(2)));
struct kmod_list *kmod_list_remove_n_latest(struct kmod_list *list,
						unsigned int n) __must_check;
struct kmod_list *kmod_list_insert_after(struct kmod_list *list, const void *data) __attribute__((nonnull(2)));
struct kmod_list *kmod_list_insert_before(struct kmod_list *list, const void *data) __attribute__((nonnull(2)));
struct kmod_list *kmod_list_append_list(struct kmod_list *list1, struct kmod_list *list2) __must_check;

#undef kmod_list_foreach
#define kmod_list_foreach(list_entry, first_entry) \
	for (list_entry = ((first_entry) == NULL) ? NULL : (first_entry); \
		list_entry != NULL; \
		list_entry = (list_entry->node.next == &((first_entry)->node)) ? NULL : \
		container_of(list_entry->node.next, struct kmod_list, node))

/* libkmod.c */
const char *kmod_get_dirname(const struct kmod_ctx *ctx) __attribute__((nonnull(1)));

int kmod_lookup_alias_from_config(struct kmod_ctx *ctx, const char *name, struct kmod_list **list) __attribute__((nonnull(1, 2, 3)));
int kmod_lookup_alias_from_symbols_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list) __attribute__((nonnull(1, 2, 3)));
int kmod_lookup_alias_from_aliases_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list) __attribute__((nonnull(1, 2, 3)));
int kmod_lookup_alias_from_moddep_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list) __attribute__((nonnull(1, 2, 3)));

char *kmod_search_moddep(struct kmod_ctx *ctx, const char *name) __attribute__((nonnull(1,2)));

struct kmod_module *kmod_pool_get_module(struct kmod_ctx *ctx, const char *name) __attribute__((nonnull(1,2)));
void kmod_pool_add_module(struct kmod_ctx *ctx, struct kmod_module *mod) __attribute__((nonnull(1,2)));
void kmod_pool_del_module(struct kmod_ctx *ctx, struct kmod_module *mod) __attribute__((nonnull(1,2)));

const struct kmod_list *kmod_get_options(const struct kmod_ctx *ctx) __must_check __attribute__((nonnull(1)));
const struct kmod_list *kmod_get_install_commands(const struct kmod_ctx *ctx) __must_check __attribute__((nonnull(1)));
const struct kmod_list *kmod_get_remove_commands(const struct kmod_ctx *ctx) __must_check __attribute__((nonnull(1)));


/* libkmod-config.c */
struct kmod_config {
	struct kmod_ctx *ctx;
	struct kmod_list *aliases;
	struct kmod_list *blacklists;
	struct kmod_list *options;
	struct kmod_list *remove_commands;
	struct kmod_list *install_commands;
};
int kmod_config_new(struct kmod_ctx *ctx, struct kmod_config **config, const char * const *config_paths) __attribute__((nonnull(1, 2,3)));
void kmod_config_free(struct kmod_config *config) __attribute__((nonnull(1)));
const char *kmod_alias_get_name(const struct kmod_list *l) __attribute__((nonnull(1)));
const char *kmod_alias_get_modname(const struct kmod_list *l) __attribute__((nonnull(1)));
const char *kmod_option_get_options(const struct kmod_list *l) __attribute__((nonnull(1)));
const char *kmod_option_get_modname(const struct kmod_list *l) __attribute__((nonnull(1)));
const char *kmod_command_get_command(const struct kmod_list *l) __attribute__((nonnull(1)));
const char *kmod_command_get_modname(const struct kmod_list *l) __attribute__((nonnull(1)));


/* libkmod-module.c */
int kmod_module_new_from_alias(struct kmod_ctx *ctx, const char *alias, const char *name, struct kmod_module **mod);
char *modname_normalize(const char *modname, char buf[NAME_MAX], size_t *len)  __attribute__((nonnull(1, 2)));
int kmod_module_parse_depline(struct kmod_module *mod, char *line) __attribute__((nonnull(1, 2)));

/* libkmod-hash.c */
struct kmod_hash;
struct kmod_hash *kmod_hash_new(unsigned int n_buckets, void (*free_value)(void *value));
void kmod_hash_free(struct kmod_hash *hash);
int kmod_hash_add(struct kmod_hash *hash, const char *key, const void *value);
int kmod_hash_del(struct kmod_hash *hash, const char *key);
void *kmod_hash_find(const struct kmod_hash *hash, const char *key);


/* util functions */
char *getline_wrapped(FILE *fp, unsigned int *linenum) __attribute__((nonnull(1)));
char *underscores(struct kmod_ctx *ctx, char *s) __attribute__((nonnull(1, 2)));
#define streq(a, b) (strcmp((a), (b)) == 0)
bool startswith(const char *s, const char *prefix) __attribute__((nonnull(1, 2)));
void *memdup(const void *p, size_t n) __attribute__((nonnull(1)));

ssize_t read_str_safe(int fd, char *buf, size_t buflen) __must_check __attribute__((nonnull(2)));
int read_str_long(int fd, long *value, int base) __must_check __attribute__((nonnull(2)));
int read_str_ulong(int fd, unsigned long *value, int base) __must_check __attribute__((nonnull(2)));
char *strchr_replace(char *s, int c, char r);
bool path_is_absolute(const char *p) __must_check __attribute__((nonnull(1)));
char *path_make_absolute_cwd(const char *p) __must_check __attribute__((nonnull(1)));
int alias_normalize(const char *alias, char buf[NAME_MAX], size_t *len) __must_check __attribute__((nonnull(1,2)));


#endif
