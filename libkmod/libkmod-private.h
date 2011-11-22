#ifndef _LIBKMOD_PRIVATE_H_
#define _LIBKMOD_PRIVATE_H_

#include <stdbool.h>
#include <syslog.h>

#include "libkmod.h"

static inline void __attribute__((always_inline, format(printf, 2, 3)))
	kmod_log_null(struct kmod_ctx *ctx, const char *format, ...) {}

#define kmod_log_cond(ctx, prio, arg...) \
	do { \
		if (kmod_get_log_priority(ctx) >= prio) \
		kmod_log(ctx, prio, __FILE__, __LINE__, __FUNCTION__, ## arg);\
	} while (0)

#ifdef ENABLE_LOGGING
#  ifdef ENABLE_DEBUG
#    define dbg(ctx, arg...) kmod_log_cond(ctx, LOG_DEBUG, ## arg)
#  else
#    define dbg(ctx, arg...) kmod_log_null(ctx, ## arg)
#  endif
#  define info(ctx, arg...) kmod_log_cond(ctx, LOG_INFO, ## arg)
#  define err(ctx, arg...) kmod_log_cond(ctx, LOG_ERR, ## arg)
#else
#  define dbg(ctx, arg...) kmod_log_null(ctx, ## arg)
#  define info(ctx, arg...) kmod_log_null(ctx, ## arg)
#  define err(ctx, arg...) kmod_log_null(ctx, ## arg)
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

struct kmod_list *kmod_list_append(struct kmod_list *list, void *data);
struct kmod_list *kmod_list_prepend(struct kmod_list *list, void *data);
struct kmod_list *kmod_list_remove(struct kmod_list *list);
struct kmod_list *kmod_list_remove_data(struct kmod_list *list,
							const void *data);

#endif
