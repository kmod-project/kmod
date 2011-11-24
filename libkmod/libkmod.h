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

#ifndef _LIBKMOD_H_
#define _LIBKMOD_H_

#include <fcntl.h>
#include <stdarg.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * kmod_ctx
 *
 * library user context - reads the config and system
 * environment, user variables, allows custom logging
 */
struct kmod_ctx;
struct kmod_ctx *kmod_new(void);
struct kmod_ctx *kmod_ref(struct kmod_ctx *ctx);
struct kmod_ctx *kmod_unref(struct kmod_ctx *ctx);
void kmod_set_log_fn(struct kmod_ctx *ctx,
			void (*log_fn)(struct kmod_ctx *ctx,
				int priority, const char *file, int line,
				const char *fn, const char *format,
				va_list args));
int kmod_get_log_priority(const struct kmod_ctx *ctx);
void kmod_set_log_priority(struct kmod_ctx *ctx, int priority);
void *kmod_get_userdata(const struct kmod_ctx *ctx);
void kmod_set_userdata(struct kmod_ctx *ctx, void *userdata);

/*
 * kmod_list
 *
 * access to kmod generated lists
 */
struct kmod_list;
struct kmod_list *kmod_list_next(struct kmod_list *first_entry,
						struct kmod_list *list_entry);
#define kmod_list_foreach(list_entry, first_entry) \
	for (list_entry = first_entry; \
		list_entry != NULL; \
		list_entry = kmod_list_next(first_entry, list_entry))

#ifdef __cplusplus
} /* extern "C" */
#endif

struct kmod_loaded;
int kmod_loaded_new(struct kmod_ctx *ctx, struct kmod_loaded **mod);
struct kmod_loaded *kmod_loaded_ref(struct kmod_loaded *mod);
struct kmod_loaded *kmod_loaded_unref(struct kmod_loaded *mod);
int kmod_loaded_get_list(struct kmod_loaded *mod, struct kmod_list **list);
int kmod_loaded_get_module_info(const struct kmod_list *entry,
				const char **name, long *size, int *use_count,
				const char **deps, uintptr_t *addr);

enum kmod_remove {
	KMOD_REMOVE_FORCE = O_TRUNC,
	KMOD_REMOVE_NOWAIT = O_NONBLOCK,
};

int kmod_loaded_remove_module(struct kmod_loaded *kmod,
				struct kmod_list *entry, unsigned int flags);
#endif
