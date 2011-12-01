#ifndef _LIBKMOD_PRIVATE_H_
#define _LIBKMOD_PRIVATE_H_

#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>

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

void kmod_log(struct kmod_ctx *ctx,
		int priority, const char *file, int line, const char *fn,
		const char *format, ...) __attribute__((format(printf, 6, 7)));

struct list_node {
	struct list_node *next, *prev;
};

struct kmod_list {
	struct list_node node;
	void *data;
};

struct kmod_list *kmod_list_append(struct kmod_list *list, void *data) __must_check;
struct kmod_list *kmod_list_prepend(struct kmod_list *list, void *data) __must_check;
struct kmod_list *kmod_list_remove(struct kmod_list *list);
struct kmod_list *kmod_list_remove_data(struct kmod_list *list,
					const void *data) __must_check;
struct kmod_list *kmod_list_remove_n_latest(struct kmod_list *list,
						unsigned int n) __must_check;

/* libkmod.c */
const char *kmod_get_dirname(struct kmod_ctx *ctx) __attribute__((nonnull(1)));
int kmod_lookup_alias_from_config(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);
int kmod_lookup_alias_from_symbols_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);
int kmod_lookup_alias_from_moddep_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);

/* libkmod-config.c */
struct kmod_config {
	struct kmod_list *aliases;
	struct kmod_list *blacklists;
};
int kmod_parse_config_file(struct kmod_ctx *ctx, const char *filename, struct kmod_config *config);
int kmod_parse_config(struct kmod_ctx *ctx, struct kmod_config *config);
void kmod_free_config(struct kmod_ctx *ctx, struct kmod_config *config);
const char *kmod_alias_get_name(const struct kmod_list *l);
const char *kmod_alias_get_modname(const struct kmod_list *l);

/* util functions */
char *getline_wrapped(FILE *fp, unsigned int *linenum);
char *underscores(struct kmod_ctx *ctx, char *s);
#define streq(a, b) (strcmp((a), (b)) == 0)
bool startswith(const char *s, const char *prefix);

#endif
