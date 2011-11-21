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

#ifndef _LIBABC_H_
#define _LIBABC_H_

#include <stdarg.h>

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
int kmod_get_log_priority(struct kmod_ctx *ctx);
void kmod_set_log_priority(struct kmod_ctx *ctx, int priority);
void *kmod_get_userdata(struct kmod_ctx *ctx);
void kmod_set_userdata(struct kmod_ctx *ctx, void *userdata);

/*
 * kmod_list
 *
 * access to kmod generated lists
 */
struct kmod_list_entry;
struct kmod_list_entry *kmod_list_entry_get_next(struct kmod_list_entry *list_entry);
const char *kmod_list_entry_get_name(struct kmod_list_entry *list_entry);
const char *kmod_list_entry_get_value(struct kmod_list_entry *list_entry);
#define kmod_list_entry_foreach(list_entry, first_entry) \
	for (list_entry = first_entry; \
		list_entry != NULL; \
		list_entry = kmod_list_entry_get_next(list_entry))

/*
 * kmod_thing
 *
 * access to things of kmod
 */
struct kmod_thing;
struct kmod_thing *kmod_thing_ref(struct kmod_thing *thing);
struct kmod_thing *kmod_thing_unref(struct kmod_thing *thing);
struct kmod_ctx *kmod_thing_get_ctx(struct kmod_thing *thing);
int kmod_thing_new_from_string(struct kmod_ctx *ctx, const char *string, struct kmod_thing **thing);
struct kmod_list_entry *kmod_thing_get_some_list_entry(struct kmod_thing *thing);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
