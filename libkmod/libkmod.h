/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#pragma once
#ifndef _LIBKMOD_H_
#define _LIBKMOD_H_

#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
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
struct kmod_ctx *kmod_new(const char *dirname, const char * const *config_paths);
struct kmod_ctx *kmod_ref(struct kmod_ctx *ctx);
struct kmod_ctx *kmod_unref(struct kmod_ctx *ctx);
void kmod_set_log_fn(struct kmod_ctx *ctx,
			void (*log_fn)(void *log_data,
					int priority, const char *file, int line,
					const char *fn, const char *format,
					va_list args),
			const void *data);
int kmod_get_log_priority(const struct kmod_ctx *ctx);
void kmod_set_log_priority(struct kmod_ctx *ctx, int priority);
void *kmod_get_userdata(const struct kmod_ctx *ctx);
void kmod_set_userdata(struct kmod_ctx *ctx, const void *userdata);

const char *kmod_get_dirname(const struct kmod_ctx *ctx);

/*
 * Management of libkmod's resources
 */
int kmod_load_resources(struct kmod_ctx *ctx);
void kmod_unload_resources(struct kmod_ctx *ctx);

enum kmod_resources {
	KMOD_RESOURCES_OK = 0,
	KMOD_RESOURCES_MUST_RELOAD = 1,
	KMOD_RESOURCES_MUST_RECREATE = 2,
};
int kmod_validate_resources(struct kmod_ctx *ctx);

enum kmod_index {
	KMOD_INDEX_MODULES_DEP = 0,
	KMOD_INDEX_MODULES_ALIAS,
	KMOD_INDEX_MODULES_SYMBOL,
	KMOD_INDEX_MODULES_BUILTIN_ALIAS,
	KMOD_INDEX_MODULES_BUILTIN,
	/* Padding to make sure enum is not mapped to char */
	_KMOD_INDEX_PAD = 1U << 31,
};
int kmod_dump_index(struct kmod_ctx *ctx, enum kmod_index type, int fd);



/**
 * SECTION:libkmod-list
 * @short_description: general purpose list
 */

/*
 * kmod_list
 *
 * access to kmod generated lists
 */
struct kmod_list;

#define kmod_list_foreach(list_entry, first_entry) \
	for (list_entry = first_entry; \
		list_entry != NULL; \
		list_entry = kmod_list_next(first_entry, list_entry))

#define kmod_list_foreach_reverse(list_entry, first_entry) \
	for (list_entry = kmod_list_last(first_entry); \
		list_entry != NULL; \
		list_entry = kmod_list_prev(first_entry, list_entry))

/**
 * kmod_list_last:
 * @list: the head of the list
 *
 * Get the last element of the @list. As @list is a circular list,
 * this is a cheap operation O(1) with the last element being the
 * previous element.
 *
 * If the list has a single element it will return the list itself (as
 * expected, and this is what differentiates from kmod_list_prev()).
 *
 * Returns: last node at @list or NULL if the list is empty.
 */
struct kmod_list *kmod_list_last(const struct kmod_list *list);

/**
 * kmod_list_next:
 * @list: the head of the list
 * @curr: the current node in the list
 *
 * Get the next node in @list relative to @curr as if @list was not a circular
 * list. I.e. calling this function in the last node of the list returns
 * NULL.. It can be used to iterate a list by checking for NULL return to know
 * when all elements were iterated.
 *
 * Returns: node next to @curr or NULL if either this node is the last of or
 * list is empty.
 */
struct kmod_list *kmod_list_next(const struct kmod_list *list,
						const struct kmod_list *curr);

/**
 * kmod_list_prev:
 * @list: the head of the list
 * @curr: the current node in the list
 *
 * Get the previous node in @list relative to @curr as if @list was not a
 * circular list. I.e.: the previous of the head is NULL. It can be used to
 * iterate a list by checking for NULL return to know when all elements were
 * iterated.
 *
 * Returns: node previous to @curr or NULL if either this node is the head of
 * the list or the list is empty.
 */
struct kmod_list *kmod_list_prev(const struct kmod_list *list,
						const struct kmod_list *curr);



/**
 * SECTION:libkmod-config
 * @short_description: retrieve current libkmod configuration
 */

/*
 * kmod_config_iter
 *
 * access to configuration lists - it allows to get each configuration's
 * key/value stored by kmod
 */
struct kmod_config_iter;

/**
 * kmod_config_get_blacklists:
 * @ctx: kmod library context
 *
 * Retrieve an iterator to deal with the blacklist maintained inside the
 * library. See kmod_config_iter_get_key(), kmod_config_iter_get_value() and
 * kmod_config_iter_next(). At least one call to kmod_config_iter_next() must
 * be made to initialize the iterator and check if it's valid.
 *
 * Returns: a new iterator over the blacklists or NULL on failure. Free it
 * with kmod_config_iter_free_iter().
 */
struct kmod_config_iter *kmod_config_get_blacklists(const struct kmod_ctx *ctx);

/**
 * kmod_config_get_install_commands:
 * @ctx: kmod library context
 *
 * Retrieve an iterator to deal with the install commands maintained inside the
 * library. See kmod_config_iter_get_key(), kmod_config_iter_get_value() and
 * kmod_config_iter_next(). At least one call to kmod_config_iter_next() must
 * be made to initialize the iterator and check if it's valid.
 *
 * Returns: a new iterator over the install commands or NULL on failure. Free
 * it with kmod_config_iter_free_iter().
 */
struct kmod_config_iter *kmod_config_get_install_commands(const struct kmod_ctx *ctx);

/**
 * kmod_config_get_remove_commands:
 * @ctx: kmod library context
 *
 * Retrieve an iterator to deal with the remove commands maintained inside the
 * library. See kmod_config_iter_get_key(), kmod_config_iter_get_value() and
 * kmod_config_iter_next(). At least one call to kmod_config_iter_next() must
 * be made to initialize the iterator and check if it's valid.
 *
 * Returns: a new iterator over the remove commands or NULL on failure. Free
 * it with kmod_config_iter_free_iter().
 */
struct kmod_config_iter *kmod_config_get_remove_commands(const struct kmod_ctx *ctx);

/**
 * kmod_config_get_aliases:
 * @ctx: kmod library context
 *
 * Retrieve an iterator to deal with the aliases maintained inside the
 * library. See kmod_config_iter_get_key(), kmod_config_iter_get_value() and
 * kmod_config_iter_next(). At least one call to kmod_config_iter_next() must
 * be made to initialize the iterator and check if it's valid.
 *
 * Returns: a new iterator over the aliases or NULL on failure. Free it with
 * kmod_config_iter_free_iter().
 */
struct kmod_config_iter *kmod_config_get_aliases(const struct kmod_ctx *ctx);

/**
 * kmod_config_get_options:
 * @ctx: kmod library context
 *
 * Retrieve an iterator to deal with the options maintained inside the
 * library. See kmod_config_iter_get_key(), kmod_config_iter_get_value() and
 * kmod_config_iter_next(). At least one call to kmod_config_iter_next() must
 * be made to initialize the iterator and check if it's valid.
 *
 * Returns: a new iterator over the options or NULL on failure. Free it with
 * kmod_config_iter_free_iter().
 */
struct kmod_config_iter *kmod_config_get_options(const struct kmod_ctx *ctx);

/**
 * kmod_config_get_softdeps:
 * @ctx: kmod library context
 *
 * Retrieve an iterator to deal with the softdeps maintained inside the
 * library. See kmod_config_iter_get_key(), kmod_config_iter_get_value() and
 * kmod_config_iter_next(). At least one call to kmod_config_iter_next() must
 * be made to initialize the iterator and check if it's valid.
 *
 * Returns: a new iterator over the softdeps or NULL on failure. Free it with
 * kmod_config_iter_free_iter().
 */
struct kmod_config_iter *kmod_config_get_softdeps(const struct kmod_ctx *ctx);

/**
 * kmod_config_get_weakdeps:
 * @ctx: kmod library context
 *
 * Retrieve an iterator to deal with the weakdeps maintained inside the
 * library. See kmod_config_iter_get_key(), kmod_config_iter_get_value() and
 * kmod_config_iter_next(). At least one call to kmod_config_iter_next() must
 * be made to initialize the iterator and check if it's valid.
 *
 * Returns: a new iterator over the weakdeps or NULL on failure. Free it with
 * kmod_config_iter_free_iter().
 */
struct kmod_config_iter *kmod_config_get_weakdeps(const struct kmod_ctx *ctx);

/**
 * kmod_config_iter_get_key:
 * @iter: iterator over a certain configuration
 *
 * When using a new allocated iterator, user must perform a call to
 * kmod_config_iter_next() to initialize iterator's position and check if it's
 * valid.
 *
 * Returns: the key of the current configuration pointed by @iter.
 */
const char *kmod_config_iter_get_key(const struct kmod_config_iter *iter);

/**
 * kmod_config_iter_get_value:
 * @iter: iterator over a certain configuration
 *
 * When using a new allocated iterator, user must perform a call to
 * kmod_config_iter_next() to initialize iterator's position and check if it's
 * valid.
 *
 * Returns: the value of the current configuration pointed by @iter.
 */
const char *kmod_config_iter_get_value(const struct kmod_config_iter *iter);

/**
 * kmod_config_iter_next:
 * @iter: iterator over a certain configuration
 *
 * Make @iter point to the next item of a certain configuration. It's an
 * automatically recycling iterator. When it reaches the end, false is
 * returned; then if user wants to iterate again, it's sufficient to call this
 * function once more.
 *
 * Returns: true if next position of @iter is valid or false if its end is
 * reached.
 */
bool kmod_config_iter_next(struct kmod_config_iter *iter);

/**
 * kmod_config_iter_free_iter:
 * @iter: iterator over a certain configuration
 *
 * Free resources used by the iterator.
 */
void kmod_config_iter_free_iter(struct kmod_config_iter *iter);

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
int kmod_module_new_from_lookup(struct kmod_ctx *ctx, const char *given_alias,
						struct kmod_list **list);
int kmod_module_new_from_name_lookup(struct kmod_ctx *ctx,
				     const char *modname,
				     struct kmod_module **mod);
int kmod_module_new_from_loaded(struct kmod_ctx *ctx,
						struct kmod_list **list);

struct kmod_module *kmod_module_ref(struct kmod_module *mod);
struct kmod_module *kmod_module_unref(struct kmod_module *mod);
int kmod_module_unref_list(struct kmod_list *list);
struct kmod_module *kmod_module_get_module(const struct kmod_list *entry);


/* Removal flags */
enum kmod_remove {
	KMOD_REMOVE_FORCE = O_TRUNC,
	KMOD_REMOVE_NOWAIT = O_NONBLOCK, /* always set */
	/* libkmod-only defines, not passed to kernel */
	KMOD_REMOVE_NOLOG = 1,
};

/* Insertion flags */
enum kmod_insert {
	KMOD_INSERT_FORCE_VERMAGIC = 0x1,
	KMOD_INSERT_FORCE_MODVERSION = 0x2,
};

/* Flags to kmod_module_probe_insert_module() */
enum kmod_probe {
	KMOD_PROBE_FORCE_VERMAGIC =		0x00001,
	KMOD_PROBE_FORCE_MODVERSION =		0x00002,
	KMOD_PROBE_IGNORE_COMMAND =		0x00004,
	KMOD_PROBE_IGNORE_LOADED =		0x00008,
	KMOD_PROBE_DRY_RUN =			0x00010,
	KMOD_PROBE_FAIL_ON_LOADED =		0x00020,

	/* codes below can be used in return value, too */
	KMOD_PROBE_APPLY_BLACKLIST_ALL =	0x10000,
	KMOD_PROBE_APPLY_BLACKLIST =		0x20000,
	KMOD_PROBE_APPLY_BLACKLIST_ALIAS_ONLY =	0x40000,
};

/* Flags to kmod_module_apply_filter() */
enum kmod_filter {
	KMOD_FILTER_BLACKLIST = 0x00001,
	KMOD_FILTER_BUILTIN = 0x00002,
};

int kmod_module_remove_module(struct kmod_module *mod, unsigned int flags);
int kmod_module_insert_module(struct kmod_module *mod, unsigned int flags,
							const char *options);
int kmod_module_probe_insert_module(struct kmod_module *mod,
			unsigned int flags, const char *extra_options,
			int (*run_install)(struct kmod_module *m,
						const char *cmdline, void *data),
			const void *data,
			void (*print_action)(struct kmod_module *m, bool install,
						const char *options));


const char *kmod_module_get_name(const struct kmod_module *mod);
const char *kmod_module_get_path(const struct kmod_module *mod);
const char *kmod_module_get_options(const struct kmod_module *mod);
const char *kmod_module_get_install_commands(const struct kmod_module *mod);
const char *kmod_module_get_remove_commands(const struct kmod_module *mod);
struct kmod_list *kmod_module_get_dependencies(const struct kmod_module *mod);
int kmod_module_get_softdeps(const struct kmod_module *mod,
				struct kmod_list **pre, struct kmod_list **post);
int kmod_module_get_weakdeps(const struct kmod_module *mod,
				struct kmod_list **weak);
int kmod_module_get_filtered_blacklist(const struct kmod_ctx *ctx,
					const struct kmod_list *input,
					struct kmod_list **output) __attribute__ ((deprecated));
int kmod_module_apply_filter(const struct kmod_ctx *ctx,
					enum kmod_filter filter_type,
					const struct kmod_list *input,
					struct kmod_list **output);



/*
 * Information regarding "live information" from module's state, as returned
 * by kernel
 */

enum kmod_module_initstate {
	KMOD_MODULE_BUILTIN = 0,
	KMOD_MODULE_LIVE,
	KMOD_MODULE_COMING,
	KMOD_MODULE_GOING,
	/* Padding to make sure enum is not mapped to char */
	_KMOD_MODULE_PAD = 1U << 31,
};
const char *kmod_module_initstate_str(enum kmod_module_initstate state);
int kmod_module_get_initstate(const struct kmod_module *mod);
int kmod_module_get_refcnt(const struct kmod_module *mod);
struct kmod_list *kmod_module_get_holders(const struct kmod_module *mod);
struct kmod_list *kmod_module_get_sections(const struct kmod_module *mod);
const char *kmod_module_section_get_name(const struct kmod_list *entry);
unsigned long kmod_module_section_get_address(const struct kmod_list *entry);
void kmod_module_section_free_list(struct kmod_list *list);
long kmod_module_get_size(const struct kmod_module *mod);



/*
 * Information retrieved from ELF headers and sections
 */

int kmod_module_get_info(const struct kmod_module *mod, struct kmod_list **list);
const char *kmod_module_info_get_key(const struct kmod_list *entry);
const char *kmod_module_info_get_value(const struct kmod_list *entry);
void kmod_module_info_free_list(struct kmod_list *list);

int kmod_module_get_versions(const struct kmod_module *mod, struct kmod_list **list);
const char *kmod_module_version_get_symbol(const struct kmod_list *entry);
uint64_t kmod_module_version_get_crc(const struct kmod_list *entry);
void kmod_module_versions_free_list(struct kmod_list *list);

int kmod_module_get_symbols(const struct kmod_module *mod, struct kmod_list **list);
const char *kmod_module_symbol_get_symbol(const struct kmod_list *entry);
uint64_t kmod_module_symbol_get_crc(const struct kmod_list *entry);
void kmod_module_symbols_free_list(struct kmod_list *list);

enum kmod_symbol_bind {
	KMOD_SYMBOL_NONE = '\0',
	KMOD_SYMBOL_LOCAL = 'L',
	KMOD_SYMBOL_GLOBAL = 'G',
	KMOD_SYMBOL_WEAK = 'W',
	KMOD_SYMBOL_UNDEF = 'U'
};

int kmod_module_get_dependency_symbols(const struct kmod_module *mod, struct kmod_list **list);
const char *kmod_module_dependency_symbol_get_symbol(const struct kmod_list *entry);
int kmod_module_dependency_symbol_get_bind(const struct kmod_list *entry);
uint64_t kmod_module_dependency_symbol_get_crc(const struct kmod_list *entry);
void kmod_module_dependency_symbols_free_list(struct kmod_list *list);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
