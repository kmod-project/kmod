/*
  libabc - something with abc

  Copyright (C) 2011 Someone <someone@example.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.¶

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "libabc.h"
#include "libabc-private.h"

/**
 * SECTION:libabc
 * @short_description: libabc context
 *
 * The context contains the default values for the library user,
 * and is passed to all library operations.
 */

/**
 * abc_ctx:
 *
 * Opaque object representing the library context.
 */
struct abc_ctx {
	int refcount;
	void (*log_fn)(struct abc_ctx *ctx,
		       int priority, const char *file, int line, const char *fn,
		       const char *format, va_list args);
	void *userdata;
	int log_priority;
};

void abc_log(struct abc_ctx *ctx,
	   int priority, const char *file, int line, const char *fn,
	   const char *format, ...)
{
	va_list args;

	va_start(args, format);
	ctx->log_fn(ctx, priority, file, line, fn, format, args);
	va_end(args);
}

static void log_stderr(struct abc_ctx *ctx,
		       int priority, const char *file, int line, const char *fn,
		       const char *format, va_list args)
{
	fprintf(stderr, "libabc: %s: ", fn);
	vfprintf(stderr, format, args);
}

/**
 * abc_get_userdata:
 * @ctx: abc library context
 *
 * Retrieve stored data pointer from library context. This might be useful
 * to access from callbacks like a custom logging function.
 *
 * Returns: stored userdata
 **/
ABC_EXPORT void *abc_get_userdata(struct abc_ctx *ctx)
{
	if (ctx == NULL)
		return NULL;
	return ctx->userdata;
}

/**
 * abc_set_userdata:
 * @ctx: abc library context
 * @userdata: data pointer
 *
 * Store custom @userdata in the library context.
 **/
ABC_EXPORT void abc_set_userdata(struct abc_ctx *ctx, void *userdata)
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
 * abc_new:
 *
 * Create abc library context. This reads the abc configuration
 * and fills in the default values.
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the abc library context.
 *
 * Returns: a new abc library context
 **/
ABC_EXPORT int abc_new(struct abc_ctx **ctx)
{
	const char *env;
	struct abc_ctx *c;

	c = calloc(1, sizeof(struct abc_ctx));
	if (!c)
		return -ENOMEM;

	c->refcount = 1;
	c->log_fn = log_stderr;
	c->log_priority = LOG_ERR;

	/* environment overwrites config */
	env = getenv("ABC_LOG");
	if (env != NULL)
		abc_set_log_priority(c, log_priority(env));

	info(c, "ctx %p created\n", c);
	dbg(c, "log_priority=%d\n", c->log_priority);
	*ctx = c;
	return 0;
}

/**
 * abc_ref:
 * @ctx: abc library context
 *
 * Take a reference of the abc library context.
 *
 * Returns: the passed abc library context
 **/
ABC_EXPORT struct abc_ctx *abc_ref(struct abc_ctx *ctx)
{
	if (ctx == NULL)
		return NULL;
	ctx->refcount++;
	return ctx;
}

/**
 * abc_unref:
 * @ctx: abc library context
 *
 * Drop a reference of the abc library context. If the refcount
 * reaches zero, the resources of the context will be released.
 *
 **/
ABC_EXPORT struct abc_ctx *abc_unref(struct abc_ctx *ctx)
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
 * abc_set_log_fn:
 * @ctx: abc library context
 * @log_fn: function to be called for logging messages
 *
 * The built-in logging writes to stderr. It can be
 * overridden by a custom function, to plug log messages
 * into the user's logging functionality.
 *
 **/
ABC_EXPORT void abc_set_log_fn(struct abc_ctx *ctx,
			      void (*log_fn)(struct abc_ctx *ctx,
					     int priority, const char *file,
					     int line, const char *fn,
					     const char *format, va_list args))
{
	ctx->log_fn = log_fn;
	info(ctx, "custom logging function %p registered\n", log_fn);
}

/**
 * abc_get_log_priority:
 * @ctx: abc library context
 *
 * Returns: the current logging priority
 **/
ABC_EXPORT int abc_get_log_priority(struct abc_ctx *ctx)
{
	return ctx->log_priority;
}

/**
 * abc_set_log_priority:
 * @ctx: abc library context
 * @priority: the new logging priority
 *
 * Set the current logging priority. The value controls which messages
 * are logged.
 **/
ABC_EXPORT void abc_set_log_priority(struct abc_ctx *ctx, int priority)
{
	ctx->log_priority = priority;
}

struct abc_list_entry;
struct abc_list_entry *abc_list_entry_get_next(struct abc_list_entry *list_entry);
const char *abc_list_entry_get_name(struct abc_list_entry *list_entry);
const char *abc_list_entry_get_value(struct abc_list_entry *list_entry);

struct abc_thing {
	struct abc_ctx *ctx;
	int refcount;
};

ABC_EXPORT struct abc_thing *abc_thing_ref(struct abc_thing *thing)
{
	if (!thing)
		return NULL;
	thing->refcount++;
	return thing;
}

ABC_EXPORT struct abc_thing *abc_thing_unref(struct abc_thing *thing)
{
	if (thing == NULL)
		return NULL;
	thing->refcount--;
	if (thing->refcount > 0)
		return thing;
	dbg(thing->ctx, "context %p released\n", thing);
	free(thing);
	return NULL;
}

ABC_EXPORT struct abc_ctx *abc_thing_get_ctx(struct abc_thing *thing)
{
	return thing->ctx;
}

ABC_EXPORT int abc_thing_new_from_string(struct abc_ctx *ctx, const char *string, struct abc_thing **thing)
{
	struct abc_thing *t;

	t = calloc(1, sizeof(struct abc_thing));
	if (!t)
		return -ENOMEM;

	t->refcount = 1;
	t->ctx = ctx;
	*thing = t;
	return 0;
}

ABC_EXPORT struct abc_list_entry *abc_thing_get_some_list_entry(struct abc_thing *thing)
{
	return NULL;
}
