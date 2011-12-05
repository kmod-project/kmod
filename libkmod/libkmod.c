/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011  ProFUSION embedded systems
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
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <fnmatch.h>
#include <string.h>
#include <ctype.h>
#include <sys/utsname.h>

#include "libkmod.h"
#include "libkmod-private.h"
#include "libkmod-index.h"

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
	int log_priority;
	void (*log_fn)(struct kmod_ctx *ctx,
			int priority, const char *file, int line,
			const char *fn, const char *format, va_list args);
	const void *userdata;
	char *dirname;
	struct kmod_config *config;
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

const char *kmod_get_dirname(const struct kmod_ctx *ctx)
{
	return ctx->dirname;
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
KMOD_EXPORT void *kmod_get_userdata(const struct kmod_ctx *ctx)
{
	if (ctx == NULL)
		return NULL;
	return (void *)ctx->userdata;
}

/**
 * kmod_set_userdata:
 * @ctx: kmod library context
 * @userdata: data pointer
 *
 * Store custom @userdata in the library context.
 **/
KMOD_EXPORT void kmod_set_userdata(struct kmod_ctx *ctx, const void *userdata)
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

static const char *dirname_default_prefix = "/lib/modules";

static char *get_kernel_release(const char *dirname)
{
	struct utsname u;
	char *p;

	if (dirname != NULL)
		return strdup(dirname);

	if (uname(&u) < 0)
		return NULL;

	if (asprintf(&p, "%s/%s", dirname_default_prefix, u.release) < 0)
		return NULL;

	return p;
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
KMOD_EXPORT struct kmod_ctx *kmod_new(const char *dirname)
{
	const char *env;
	struct kmod_ctx *ctx;
	int err;

	ctx = calloc(1, sizeof(struct kmod_ctx));
	if (!ctx)
		return NULL;

	ctx->refcount = 1;
	ctx->log_fn = log_stderr;
	ctx->log_priority = LOG_ERR;

	ctx->dirname = get_kernel_release(dirname);

	/* environment overwrites config */
	env = getenv("KMOD_LOG");
	if (env != NULL)
		kmod_set_log_priority(ctx, log_priority(env));

	err = kmod_config_new(ctx, &ctx->config);
	if (err < 0) {
		ERR(ctx, "could not create config");
		free(ctx->dirname);
		free(ctx);
		return NULL;
	}

	INFO(ctx, "ctx %p created\n", ctx);
	DBG(ctx, "log_priority=%d\n", ctx->log_priority);

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

	if (--ctx->refcount > 0)
		return ctx;
	INFO(ctx, "context %p released\n", ctx);
	free(ctx->dirname);
	if (ctx->config)
		kmod_config_free(ctx->config);
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
	INFO(ctx, "custom logging function %p registered\n", log_fn);
}

/**
 * kmod_get_log_priority:
 * @ctx: kmod library context
 *
 * Returns: the current logging priority
 **/
KMOD_EXPORT int kmod_get_log_priority(const struct kmod_ctx *ctx)
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


static int kmod_lookup_alias_from_alias_bin(struct kmod_ctx *ctx,
						const char *file,
						const char *name,
						struct kmod_list **list)
{
	char *fn;
	int err, nmatch = 0;
	struct index_file *idx;
	struct index_value *realnames, *realname;

	if (asprintf(&fn, "%s/%s.bin", ctx->dirname, file) < 0)
		return -ENOMEM;

	DBG(ctx, "file=%s name=%s\n", fn, name);

	idx = index_file_open(fn);
	if (idx == NULL) {
		free(fn);
		return -ENOSYS;
	}

	realnames = index_searchwild(idx, name);
	for (realname = realnames; realname; realname = realnames->next) {
		struct kmod_module *mod;

		err = kmod_module_new_from_name(ctx, realname->value, &mod);
		if (err < 0) {
			ERR(ctx, "%s\n", strerror(-err));
			goto fail;
		}

		*list = kmod_list_append(*list, mod);
		nmatch++;
	}

	index_values_free(realnames);
	index_file_close(idx);
	free(fn);

	return nmatch;

fail:
	*list = kmod_list_remove_n_latest(*list, nmatch);
	return err;

}

static const char *symbols_file = "modules.symbols";

int kmod_lookup_alias_from_symbols_file(struct kmod_ctx *ctx, const char *name,
						struct kmod_list **list)
{
	if (!startswith(name, "symbol:"))
		return 0;

	return kmod_lookup_alias_from_alias_bin(ctx, symbols_file, name, list);
}


static const char *aliases_file = "modules.alias";

int kmod_lookup_alias_from_aliases_file(struct kmod_ctx *ctx, const char *name,
						struct kmod_list **list)
{
	return kmod_lookup_alias_from_alias_bin(ctx, aliases_file, name, list);
}

static const char *moddep_file = "modules.dep";

char *kmod_search_moddep(struct kmod_ctx *ctx, const char *name)
{
	struct index_file *idx;
	char fn[PATH_MAX];
	char *line;

	snprintf(fn, sizeof(fn), "%s/%s.bin", ctx->dirname, moddep_file);

	DBG(ctx, "file=%s modname=%s\n", fn, name);

	idx = index_file_open(fn);
	if (idx == NULL) {
		ERR(ctx, "Could not open moddep file '%s'", fn);
		return NULL;
	}

	line = index_search(idx, name);
	index_file_close(idx);

	return line;
}

int kmod_lookup_alias_from_moddep_file(struct kmod_ctx *ctx, const char *name,
						struct kmod_list **list)
{
	char *line;
	int n = 0;

	/*
	 * Module names do not contain ':'. Return early if we know it will
	 * not be found.
	 */
	if (strchr(name, ':'))
		return 0;

	line = kmod_search_moddep(ctx, name);
	if (line != NULL) {
		struct kmod_module *mod;

		n = kmod_module_new_from_name(ctx, name, &mod);
		if (n < 0) {
			ERR(ctx, "%s\n", strerror(-n));
			goto finish;
		}

		*list = kmod_list_append(*list, mod);
		kmod_module_parse_depline(mod, line);
	}

finish:
	free(line);

	return n;
}

int kmod_lookup_alias_from_config(struct kmod_ctx *ctx, const char *name,
						struct kmod_list **list)
{
	struct kmod_config *config = ctx->config;
	struct kmod_list *l;
	int err, nmatch = 0;

	kmod_list_foreach(l, config->aliases) {
		const char *aliasname = kmod_alias_get_name(l);
		const char *modname = kmod_alias_get_modname(l);

		if (fnmatch(aliasname, name, 0) == 0) {
			struct kmod_module *mod;

			err = kmod_module_new_from_name(ctx, modname, &mod);
			if (err < 0) {
				ERR(ctx, "%s", strerror(-err));
				goto fail;
			}

			*list = kmod_list_append(*list, mod);
			nmatch++;
		}
	}

	return nmatch;

fail:
	*list = kmod_list_remove_n_latest(*list, nmatch);
	return err;
}
