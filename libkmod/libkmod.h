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
struct kmod_ctx *kmod_new(const char *dirname);
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
void kmod_set_userdata(struct kmod_ctx *ctx, const void *userdata);

/*
 * kmod_list
 *
 * access to kmod generated lists
 */
struct kmod_list;
struct kmod_list *kmod_list_next(const struct kmod_list *first_entry,
						const struct kmod_list *list_entry);
struct kmod_list *kmod_list_prev(const struct kmod_list *first_entry,
						const struct kmod_list *list_entry);
#define kmod_list_foreach(list_entry, first_entry) \
	for (list_entry = first_entry; \
		list_entry != NULL; \
		list_entry = kmod_list_next(first_entry, list_entry))

int kmod_loaded_get_list(struct kmod_ctx *ctx, struct kmod_list **list);

enum kmod_remove {
	KMOD_REMOVE_FORCE = O_TRUNC,
	KMOD_REMOVE_NOWAIT = O_NONBLOCK,
};

enum kmod_insert {
	KMOD_INSERT_FORCE_VERMAGIC = 0x1,
	KMOD_INSERT_FORCE_MODVERSION = 0x2,
	KMOD_INSERT_HANDLE_DEPENDENCIES = 0x4,
	KMOD_INSERT_IGNORE_CONFIG = 0x8,
};

/*
 * kmod_module
 *
 * Operate on kernel modules
 */
struct kmod_module;
int kmod_module_new_from_name(struct kmod_ctx *ctx, const char *name,
						struct kmod_module **mod);
int kmod_module_new_from_path(struct kmod_ctx *ctx, const char *path,
						struct kmod_module **mod);
int kmod_module_new_from_lookup(struct kmod_ctx *ctx, const char *alias,
						struct kmod_list **list);

struct kmod_module *kmod_module_ref(struct kmod_module *mod);
struct kmod_module *kmod_module_unref(struct kmod_module *mod);
int kmod_module_unref_list(struct kmod_list *list);
struct kmod_module *kmod_module_get_module(const struct kmod_list *entry);
struct kmod_list *kmod_module_get_dependency(const struct kmod_module *mod);

int kmod_module_remove_module(struct kmod_module *mod, unsigned int flags);
int kmod_module_insert_module(struct kmod_module *mod, unsigned int flags);

const char *kmod_module_get_name(const struct kmod_module *mod);
const char *kmod_module_get_path(const struct kmod_module *mod);

enum kmod_module_initstate {
	KMOD_MODULE_BUILTIN = 0,
	KMOD_MODULE_LIVE,
	KMOD_MODULE_COMING,
	KMOD_MODULE_GOING
};
const char *kmod_module_initstate_str(enum kmod_module_initstate initstate);
int kmod_module_get_initstate(const struct kmod_module *mod);
int kmod_module_get_refcnt(const struct kmod_module *mod);
struct kmod_list *kmod_module_get_holders(const struct kmod_module *mod);

struct kmod_list *kmod_module_get_sections(const struct kmod_module *mod);
const char *kmod_module_section_get_name(const struct kmod_list *entry);
unsigned long kmod_module_section_get_address(const struct kmod_list *entry);
void kmod_module_section_free_list(struct kmod_list *list);

long kmod_module_get_size(const struct kmod_module *mod);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
