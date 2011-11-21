/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011  ProFUSION embedded systems
 * Copyright (C) 2011  Lucas De Marchi <lucas.de.marchi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation version 2.1.
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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "libkmod.h"
#include "libkmod-private.h"

/**
 * SECTION:libkmod
 * @short_description: libkmod context
 *
 * The context contains the default values for the library user,
 * and is passed to all library operations.
 */

/**
 * kmod_ctx:
 *
 * Opaque object representing the library context.
 */
struct kmod_ctx {
	int refcount;
	void (*log_fn)(struct kmod_ctx *ctx,
			int priority, const char *file, int line,
			const char *fn, const char *format, va_list args);
	void *userdata;
	int log_priority;
};

void kmod_log(struct kmod_ctx *ctx,
		int priority, const char *file, int line, const char *fn,
		const char *format, ...)
{
	va_list args;

	va_start(args, format);
	ctx->log_fn(ctx, priority, file, line, fn, format, args);
	va_end(args);
}

static void log_stderr(struct kmod_ctx *ctx,
			int priority, const char *file, int line,
			const char *fn, const char *format, va_list args)
{
	fprintf(stderr, "libkmod: %s: ", fn);
	vfprintf(stderr, format, args);
}

/**
 * kmod_get_userdata:
 * @ctx: kmod library context
 *
 * Retrieve stored data pointer from library context. This might be useful
 * to access from callbacks like a custom logging function.
 *
 * Returns: stored userdata
 **/
KMOD_EXPORT void *kmod_get_userdata(struct kmod_ctx *ctx)
{
	if (ctx == NULL)
		return NULL;
	return ctx->userdata;
}

/**
 * kmod_set_userdata:
 * @ctx: kmod library context
 * @userdata: data pointer
 *
 * Store custom @userdata in the library context.
 **/
KMOD_EXPORT void kmod_set_userdata(struct kmod_ctx *ctx, void *userdata)
{
	if (ctx == NULL)
		return;
	ctx->userdata = userdata;
}

static int log_priority(const char *priority)
{
	char *endptr;
	int prio;

	prio = strtol(priority, &endptr, 10);
	if (endptr[0] == '\0' || isspace(endptr[0]))
		return prio;
	if (strncmp(priority, "err", 3) == 0)
		return LOG_ERR;
	if (strncmp(priority, "info", 4) == 0)
		return LOG_INFO;
	if (strncmp(priority, "debug", 5) == 0)
		return LOG_DEBUG;
	return 0;
}

/**
 * kmod_new:
 *
 * Create kmod library context. This reads the kmod configuration
 * and fills in the default values.
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the kmod library context.
 *
 * Returns: a new kmod library context
 **/
KMOD_EXPORT struct kmod_ctx *kmod_new(void)
{
	const char *env;
	struct kmod_ctx *ctx;

	ctx = calloc(1, sizeof(struct kmod_ctx));
	if (!ctx)
		return NULL;

	ctx->refcount = 1;
	ctx->log_fn = log_stderr;
	ctx->log_priority = LOG_ERR;

	/* environment overwrites config */
	env = getenv("KMOD_LOG");
	if (env != NULL)
		kmod_set_log_priority(ctx, log_priority(env));

	info(ctx, "ctx %p created\n", ctx);
	dbg(ctx, "log_priority=%d\n", ctx->log_priority);

	return ctx;
}

/**
 * kmod_ref:
 * @ctx: kmod library context
 *
 * Take a reference of the kmod library context.
 *
 * Returns: the passed kmod library context
 **/
KMOD_EXPORT struct kmod_ctx *kmod_ref(struct kmod_ctx *ctx)
{
	if (ctx == NULL)
		return NULL;
	ctx->refcount++;
	return ctx;
}

/**
 * kmod_unref:
 * @ctx: kmod library context
 *
 * Drop a reference of the kmod library context. If the refcount
 * reaches zero, the resources of the context will be released.
 *
 **/
KMOD_EXPORT struct kmod_ctx *kmod_unref(struct kmod_ctx *ctx)
{
	if (ctx == NULL)
		return NULL;
	ctx->refcount--;
	if (ctx->refcount > 0)
		return ctx;
	info(ctx, "context %p released\n", ctx);
	free(ctx);
	return NULL;
}

/**
 * kmod_set_log_fn:
 * @ctx: kmod library context
 * @log_fn: function to be called for logging messages
 *
 * The built-in logging writes to stderr. It can be
 * overridden by a custom function, to plug log messages
 * into the user's logging functionality.
 *
 **/
KMOD_EXPORT void kmod_set_log_fn(struct kmod_ctx *ctx,
					void (*log_fn)(struct kmod_ctx *ctx,
						int priority, const char *file,
						int line, const char *fn,
						const char *format, va_list args))
{
	ctx->log_fn = log_fn;
	info(ctx, "custom logging function %p registered\n", log_fn);
}

/**
 * kmod_get_log_priority:
 * @ctx: kmod library context
 *
 * Returns: the current logging priority
 **/
KMOD_EXPORT int kmod_get_log_priority(struct kmod_ctx *ctx)
{
	return ctx->log_priority;
}

/**
 * kmod_set_log_priority:
 * @ctx: kmod library context
 * @priority: the new logging priority
 *
 * Set the current logging priority. The value controls which messages
 * are logged.
 **/
KMOD_EXPORT void kmod_set_log_priority(struct kmod_ctx *ctx, int priority)
{
	ctx->log_priority = priority;
}

struct kmod_list_entry;
struct kmod_list_entry *kmod_list_entry_get_next(struct kmod_list_entry *list_entry);
