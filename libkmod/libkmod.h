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
struct kmod_ctx;

/**
 * kmod_new:
 * @dirname: what to consider as linux module's directory, if NULL
 *           defaults to $MODULE_DIRECTORY/`uname -r`. If it's relative,
 *           it's treated as relative to the current working directory.
 *           Otherwise, give an absolute dirname.
 * @config_paths: ordered array of paths (directories or files) where
 *                to load from user-defined configuration parameters such as
 *                alias, blacklists, commands (install, remove). If NULL
 *                defaults to /etc/modprobe.d, /run/modprobe.d,
 *                /usr/local/lib/modprobe.d, DISTCONFDIR/modprobe.d, and
 *                /lib/modprobe.d. Give an empty vector if configuration should
 *                not be read. This array must be null terminated.
 *
 * Create kmod library context. This reads the kmod configuration
 * and fills in the default values.
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the kmod library context.
 *
 * Returns: a new kmod library context
 */
struct kmod_ctx *kmod_new(const char *dirname, const char * const *config_paths);

/**
 * kmod_ref:
 * @ctx: kmod library context
 *
 * Take a reference of the kmod library context.
 *
 * Returns: the passed kmod library context
 */
struct kmod_ctx *kmod_ref(struct kmod_ctx *ctx);

/**
 * kmod_unref:
 * @ctx: kmod library context
 *
 * Drop a reference of the kmod library context. If the refcount
 * reaches zero, the resources of the context will be released.
 *
 * Returns: the passed kmod library context or NULL if it's freed
 */
struct kmod_ctx *kmod_unref(struct kmod_ctx *ctx);


/**
 * kmod_load_resources:
 * @ctx: kmod library context
 *
 * Load indexes and keep them open in @ctx. This way it's faster to lookup
 * information within the indexes. If this function is not called before a
 * search, the necessary index is always opened and closed.
 *
 * If user will do more than one or two lookups, insertions, deletions, most
 * likely it's good to call this function first. Particularly in a daemon like
 * udev that on boot issues hundreds of calls to lookup the index, calling
 * this function will speedup the searches.
 *
 * Returns: 0 on success or < 0 otherwise.
 */
int kmod_load_resources(struct kmod_ctx *ctx);

/**
 * kmod_unload_resources:
 * @ctx: kmod library context
 *
 * Unload all the indexes. This will free the resources to maintain the index
 * open and all subsequent searches will need to open and close the index.
 *
 * User is free to call kmod_load_resources() and kmod_unload_resources() as
 * many times as wanted during the lifecycle of @ctx. For example, if a daemon
 * knows that when starting up it will lookup a lot of modules, it could call
 * kmod_load_resources() and after the first burst of searches is gone, it
 * could free the resources by calling kmod_unload_resources().
 *
 * Returns: 0 on success or < 0 otherwise.
 */
void kmod_unload_resources(struct kmod_ctx *ctx);

/**
 * kmod_resources:
 * @KMOD_RESOURCES_OK: resources are valid
 * @KMOD_RESOURCES_MUST_RELOAD: resources are not valid; to resolve call
 * kmod_unload_resources() and kmod_load_resources()
 * @KMOD_RESOURCES_MUST_RECREATE: resources are not valid; to resolve @ctx must
 * be re-created.
 *
 * The validity state of the current libkmod resources, returned by
 * kmod_validate_resources().
 */
enum kmod_resources {
	KMOD_RESOURCES_OK = 0,
	KMOD_RESOURCES_MUST_RELOAD = 1,
	KMOD_RESOURCES_MUST_RECREATE = 2,
};

/**
 * kmod_validate_resources:
 * @ctx: kmod library context
 *
 * Check if indexes and configuration files changed on disk and the current
 * context is not valid anymore.
 *
 * Returns: the resources state, valid states are #kmod_resources.
 */
int kmod_validate_resources(struct kmod_ctx *ctx);

/**
 * kmod_index:
 * @KMOD_INDEX_MODULES_DEP: index of module dependencies
 * @KMOD_INDEX_MODULES_ALIAS: index of module aliases
 * @KMOD_INDEX_MODULES_SYMBOL: index of symbol aliases
 * @KMOD_INDEX_MODULES_BUILTIN_ALIAS: index of builtin module aliases
 * @KMOD_INDEX_MODULES_BUILTIN: index of builtin module
 * @_KMOD_INDEX_PAD: DO NOT USE; padding to make sure enum is not mapped to char
 *
 * The (module) index type, used by kmod_dump_index().
 */
enum kmod_index {
	KMOD_INDEX_MODULES_DEP = 0,
	KMOD_INDEX_MODULES_ALIAS,
	KMOD_INDEX_MODULES_SYMBOL,
	KMOD_INDEX_MODULES_BUILTIN_ALIAS,
	KMOD_INDEX_MODULES_BUILTIN,
	_KMOD_INDEX_PAD = 1U << 31,
};

/**
 * kmod_dump_index:
 * @ctx: kmod library context
 * @type: index to dump, valid indexes are #kmod_index
 * @fd: file descriptor to dump index to
 *
 * Dump index to file descriptor. Note that this function doesn't use stdio.h
 * so call fflush() before calling this function to be sure data is written in
 * order.
 *
 * Returns: 0 on success or < 0 otherwise.
 */
int kmod_dump_index(struct kmod_ctx *ctx, enum kmod_index type, int fd);


/**
 * kmod_set_log_priority:
 * @ctx: kmod library context
 * @priority: the new logging priority
 *
 * Set the current logging priority, as defined in syslog.h(0P). The value
 * controls which messages are logged.
 */
void kmod_set_log_priority(struct kmod_ctx *ctx, int priority);

/**
 * kmod_get_log_priority:
 * @ctx: kmod library context
 *
 * Get the current logging priority, as defined in syslog.h(0P).
 *
 * Returns: the current logging priority
 */
int kmod_get_log_priority(const struct kmod_ctx *ctx);

/**
 * kmod_set_log_fn:
 * @ctx: kmod library context
 * @log_fn: function to be called for logging messages
 * @data: data to pass to log function
 *
 * The built-in logging writes to stderr. It can be
 * overridden by a custom function, to plug log messages
 * into the user's logging functionality.
 */
void kmod_set_log_fn(struct kmod_ctx *ctx,
			void (*log_fn)(void *log_data,
					int priority, const char *file, int line,
					const char *fn, const char *format,
					va_list args),
			const void *data);

/**
 * kmod_set_userdata:
 * @ctx: kmod library context
 * @userdata: data pointer
 *
 * Store custom @userdata in the library context.
 */
void kmod_set_userdata(struct kmod_ctx *ctx, const void *userdata);

/**
 * kmod_get_userdata:
 * @ctx: kmod library context
 *
 * Retrieve stored data pointer from library context. This might be useful
 * to access from callbacks.
 *
 * Returns: stored userdata
 */
void *kmod_get_userdata(const struct kmod_ctx *ctx);

/**
 * kmod_get_dirname:
 * @ctx: kmod library context
 *
 * Retrieve the absolute path used for linux modules in this context. The path
 * is computed from the arguments to kmod_new().
 */
const char *kmod_get_dirname(const struct kmod_ctx *ctx);



/**
 * SECTION:libkmod-list
 * @short_description: general purpose list
 *
 * Access to kmod generated lists.
 */

/**
 * kmod_list:
 *
 * Opaque object for a circular (doubly linked) list.
 */
struct kmod_list;

/**
 * kmod_list_foreach:
 * @curr: the current node in the list
 * @list: the head of the list
 *
 * Iterate over the list @list.
 */
#define kmod_list_foreach(curr, list) \
	for (curr = list; \
		curr != NULL; \
		curr = kmod_list_next(list, curr))

/**
 * kmod_list_foreach_reverse:
 * @curr: the current node in the list
 * @list: the head of the list
 *
 * Iterate in reverse over the list @list.
 */
#define kmod_list_foreach_reverse(curr, list) \
	for (curr = kmod_list_last(list); \
		curr != NULL; \
		curr = kmod_list_prev(list, curr))

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
 *
 * Access to configuration lists - it allows to get each configuration's
 * key/value stored by kmod.
 */

/**
 * kmod_config_iter:
 *
 * Opaque object for iterating and retrieving configuration information.
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


/**
 * SECTION:libkmod-module
 * @short_description: operate on kernel modules
 *
 * Wide range of function to operate on kernel modules - loading, unloading,
 * reference counting, retrieving a list of module dependencies and more.
 */

/**
 * kmod_module:
 *
 * Opaque object representing a module.
 */
struct kmod_module;

/**
 * kmod_module_new_from_lookup:
 * @ctx: kmod library context
 * @given_alias: alias to look for
 * @list: an empty list where to save the list of modules matching
 * @given_alias
 *
 * Create a new list of kmod modules using an alias or module name and lookup
 * libkmod's configuration files and indexes in order to find the module.
 * Once it's found in one of the places, it stops searching and create the
 * list of modules that is saved in @list.
 *
 * The search order is: 1. aliases in configuration file; 2. module names in
 * modules.dep index; 3. symbol aliases in modules.symbols index; 4. aliases
 * from install commands; 5. builtin indexes from kernel.
 *
 * The initial refcount is 1, and needs to be decremented to release the
 * resources of the kmod_module. The returned @list must be released by
 * calling kmod_module_unref_list(). Since libkmod keeps track of all
 * kmod_modules created, they are all released upon @ctx destruction too. Do
 * not unref @ctx before all the desired operations with the returned list are
 * completed.
 *
 * Returns: 0 on success or < 0 otherwise. It fails if any of the lookup
 * methods failed, which is basically due to memory allocation fail. If module
 * is not found, it still returns 0, but @list is an empty list.
 */
int kmod_module_new_from_lookup(struct kmod_ctx *ctx, const char *given_alias,
						struct kmod_list **list);

/**
 * kmod_module_new_from_name_lookup:
 * @ctx: kmod library context
 * @modname: module name to look for
 * @mod: returned module on success
 *
 * Lookup by module name, without considering possible aliases. This is similar
 * to kmod_module_new_from_lookup(), but don't consider as source indexes and
 * configurations that work with aliases. When successful, this always resolves
 * to one and only one module.
 *
 * The search order is: 1. module names in modules.dep index;
 * 2. builtin indexes from kernel.
 *
 * The initial refcount is 1, and needs to be decremented to release the
 * resources of the kmod_module. Since libkmod keeps track of all
 * kmod_modules created, they are all released upon @ctx destruction too. Do
 * not unref @ctx before all the desired operations with the returned list are
 * completed.
 *
 * Returns: 0 on success or < 0 otherwise. It fails if any of the lookup
 * methods failed, which is basically due to memory allocation failure. If
 * module is not found, it still returns 0, but @mod is left untouched.
 */
int kmod_module_new_from_name_lookup(struct kmod_ctx *ctx,
				     const char *modname,
				     struct kmod_module **mod);

/**
 * kmod_module_new_from_name:
 * @ctx: kmod library context
 * @name: name of the module
 * @mod: where to save the created struct kmod_module
 *
 * Create a new struct kmod_module using the module name. @name can not be an
 * alias, file name or anything else; it must be a module name. There's no
 * check if the module exists in the system.
 *
 * This function is also used internally by many others that return a new
 * struct kmod_module or a new list of modules.
 *
 * The initial refcount is 1, and needs to be decremented to release the
 * resources of the kmod_module. Since libkmod keeps track of all
 * kmod_modules created, they are all released upon @ctx destruction too. Do
 * not unref @ctx before all the desired operations with the returned
 * kmod_module are done.
 *
 * Returns: 0 on success or < 0 otherwise. It fails if name is not a valid
 * module name or if memory allocation failed.
 */
int kmod_module_new_from_name(struct kmod_ctx *ctx, const char *name,
						struct kmod_module **mod);

/**
 * kmod_module_new_from_path:
 * @ctx: kmod library context
 * @path: path where to find the given module
 * @mod: where to save the created struct kmod_module
 *
 * Create a new struct kmod_module using the module path. @path must be an
 * existent file with in the filesystem and must be accessible to libkmod.
 *
 * The initial refcount is 1, and needs to be decremented to release the
 * resources of the kmod_module. Since libkmod keeps track of all
 * kmod_modules created, they are all released upon @ctx destruction too. Do
 * not unref @ctx before all the desired operations with the returned
 * kmod_module are done.
 *
 * If @path is relative, it's treated as relative to the current working
 * directory. Otherwise, give an absolute path.
 *
 * Returns: 0 on success or < 0 otherwise. It fails if file does not exist, if
 * it's not a valid file for a kmod_module or if memory allocation failed.
 */
int kmod_module_new_from_path(struct kmod_ctx *ctx, const char *path,
						struct kmod_module **mod);


/**
 * kmod_module_ref:
 * @mod: kmod module
 *
 * Take a reference of the kmod module, incrementing its refcount.
 *
 * Returns: the passed @module with its refcount incremented.
 */
struct kmod_module *kmod_module_ref(struct kmod_module *mod);

/**
 * kmod_module_unref:
 * @mod: kmod module
 *
 * Drop a reference of the kmod module. If the refcount reaches zero, its
 * resources are released.
 *
 * Returns: NULL if @mod is NULL or if the module was released. Otherwise it
 * returns the passed @mod with its refcount decremented.
 */
struct kmod_module *kmod_module_unref(struct kmod_module *mod);

/**
 * kmod_module_unref_list:
 * @list: list of kmod modules
 *
 * Drop a reference of each kmod module in @list and releases the resources
 * taken by the list itself.
 *
 * Returns: 0
 */
int kmod_module_unref_list(struct kmod_list *list);


/**
 * kmod_insert:
 * @KMOD_INSERT_FORCE_VERMAGIC: ignore kernel version magic
 * @KMOD_INSERT_FORCE_MODVERSION: ignore symbol version hashes
 *
 * Insertion flags, used by kmod_module_insert_module().
 */
enum kmod_insert {
	KMOD_INSERT_FORCE_VERMAGIC = 0x1,
	KMOD_INSERT_FORCE_MODVERSION = 0x2,
};

/**
 * kmod_module_insert_module:
 * @mod: kmod module
 * @flags: flags are not passed to the kernel, but instead they dictate the
 * behavior of this function, valid flags #kmod_insert
 * @options: module's options to pass to the kernel.
 *
 * Insert a module in the kernel. It opens the file pointed by @mod,
 * mmap'ing it and passing to kernel.
 *
 * Returns: 0 on success or < 0 on failure. If module is already loaded it
 * returns -EEXIST.
 */
int kmod_module_insert_module(struct kmod_module *mod, unsigned int flags,
							const char *options);

/**
 * kmod_probe:
 * @KMOD_PROBE_FORCE_VERMAGIC: ignore kernel version magic
 * @KMOD_PROBE_FORCE_MODVERSION: ignore symbol version hashes
 * @KMOD_PROBE_IGNORE_COMMAND: ignore install commands and softdeps configured
 * in the system
 * @KMOD_PROBE_IGNORE_LOADED: do not check whether the module is already
 * live in the kernel or not
 * @KMOD_PROBE_DRY_RUN: dry run, do not insert module, just call the
 * associated callback function
 * @KMOD_PROBE_FAIL_ON_LOADED: probe will fail if KMOD_PROBE_IGNORE_LOADED is
 * not specified and the module is already live in the kernel
 * @KMOD_PROBE_APPLY_BLACKLIST_ALL: prior to probe, apply KMOD_FILTER_BLACKLIST
 * filter to this module and its dependencies. If any of them are blacklisted
 * and the blacklisted module is not live in the kernel, the function returns
 * early with thus enum
 * @KMOD_PROBE_APPLY_BLACKLIST: probe will return early with this enum, if the
 * module is blacklisted
 * @KMOD_PROBE_APPLY_BLACKLIST_ALIAS_ONLY: probe will return early with this
 * enum, if the module is an alias and is blacklisted
 *
 * Bitmask which defines the behaviour of kmod_module_probe_insert_module().
 */
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

/**
 * kmod_module_probe_insert_module:
 * @mod: kmod module
 * @flags: flags are not passed to the kernel, but instead they dictate the
 * behavior of this function, valid flags are #kmod_probe
 * @extra_options: module's options to pass to the kernel. It applies only
 * to @mod, not to its dependencies.
 * @run_install: function to run when @mod is backed by an install command.
 * @data: data to give back to @run_install callback
 * @print_action: function to call with the action being taken (install or
 * insmod). It's useful for tools like modprobe when running with verbose
 * output or in dry-run mode.
 *
 * Insert a module in the kernel resolving dependencies, soft dependencies,
 * install commands and applying blacklist.
 *
 * If @run_install is NULL, this function will fork and exec by calling
 * system(3). Don't pass a NULL argument in @run_install if your binary is
 * setuid/setgid (see warning in system(3)). If you need control over the
 * execution of an install command, give a callback function instead.
 *
 * Returns: 0 on success, > 0 if stopped by a reason given in @flags or < 0 on
 * failure.
 */
int kmod_module_probe_insert_module(struct kmod_module *mod,
			unsigned int flags, const char *extra_options,
			int (*run_install)(struct kmod_module *m,
						const char *cmdline, void *data),
			const void *data,
			void (*print_action)(struct kmod_module *m, bool install,
						const char *options));

/**
 * kmod_remove:
 * @KMOD_REMOVE_FORCE: force remove module regardless if it's still in
 * use by a kernel subsystem or other process; passed directly to the kernel
 * @KMOD_REMOVE_NOWAIT: always set, pass O_NONBLOCK to delete_module(2);
 * passed directly to the kernel
 * @KMOD_REMOVE_NOLOG: when module removal fails, do not log anything; not
 * passed to the kernel
 *
 * Removal flags, used by kmod_module_remove_module().
 */
enum kmod_remove {
	KMOD_REMOVE_FORCE = O_TRUNC,
	KMOD_REMOVE_NOWAIT = O_NONBLOCK,
	KMOD_REMOVE_NOLOG = 1,
};

/**
 * kmod_module_remove_module:
 * @mod: kmod module
 * @flags: flags used when removing the module, valid flags are #kmod_remove
 *
 * Remove a module from the kernel.
 *
 * Returns: 0 on success or < 0 on failure.
 */
int kmod_module_remove_module(struct kmod_module *mod, unsigned int flags);


/**
 * kmod_module_get_module:
 * @entry: an entry in a list of kmod modules.
 *
 * Get the kmod module of this @entry in the list, increasing its refcount.
 * After it's used, unref it. Since the refcount is incremented upon return,
 * you still have to call kmod_module_unref_list() to release the list of kmod
 * modules.
 *
 * Returns: NULL on failure or the kmod_module contained in this list entry
 * with its refcount incremented.
 */
struct kmod_module *kmod_module_get_module(const struct kmod_list *entry);

/**
 * kmod_module_get_dependencies:
 * @mod: kmod module
 *
 * Search the modules.dep index to find the dependencies of the given @mod.
 * The result is cached in @mod, so subsequent calls to this function will
 * return the already searched list of modules.
 *
 * Returns: NULL on failure. Otherwise it returns a list of kmod modules
 * that can be released by calling kmod_module_unref_list().
 */
struct kmod_list *kmod_module_get_dependencies(const struct kmod_module *mod);

/**
 * kmod_module_get_softdeps:
 * @mod: kmod module
 * @pre: where to save the list of preceding soft dependencies.
 * @post: where to save the list of post soft dependencies.
 *
 * Get soft dependencies for this kmod module. Soft dependencies come
 * from configuration file and are not cached in @mod because it may include
 * dependency cycles that would make we leak kmod_module. Any call
 * to this function will search for this module in configuration, allocate a
 * list and return the result.
 *
 * Both @pre and @post are newly created list of kmod_module and
 * should be unreferenced with kmod_module_unref_list().
 *
 * Returns: 0 on success or < 0 otherwise.
 */
int kmod_module_get_softdeps(const struct kmod_module *mod,
				struct kmod_list **pre, struct kmod_list **post);

/**
 * kmod_module_get_weakdeps:
 * @mod: kmod module
 * @weak: where to save the list of weak dependencies.
 *
 * Get weak dependencies for this kmod module. Weak dependencies come
 * from configuration file and are not cached in @mod because it may include
 * dependency cycles that would make we leak kmod_module. Any call
 * to this function will search for this module in configuration, allocate a
 * list and return the result.
 *
 * @weak is newly created list of kmod_module and
 * should be unreferenced with kmod_module_unref_list().
 *
 * Returns: 0 on success or < 0 otherwise.
 */
int kmod_module_get_weakdeps(const struct kmod_module *mod,
				struct kmod_list **weak);

/**
 * kmod_filter:
 * @KMOD_FILTER_BLACKLIST: filter modules in blacklist out
 * @KMOD_FILTER_BUILTIN: filter builtin modules out
 *
 * Bitmask defining what gets filtered out, used by kmod_module_apply_filter().
 */
enum kmod_filter {
	KMOD_FILTER_BLACKLIST = 0x00001,
	KMOD_FILTER_BUILTIN = 0x00002,
};

/**
 * kmod_module_apply_filter:
 * @ctx: kmod library context
 * @filter_type: bitmask to filter modules out, valid types are #kmod_filter
 * @input: list of kmod_module to be filtered
 * @output: where to save the new list
 *
 * Given a list @input, this function filter it out by the filter mask
 * and save it in @output.
 *
 * Returns: 0 on success or < 0 otherwise. @output is saved with the updated
 * list.
 */
int kmod_module_apply_filter(const struct kmod_ctx *ctx,
					enum kmod_filter filter_type,
					const struct kmod_list *input,
					struct kmod_list **output);

/**
 * kmod_module_get_filtered_blacklist:
 * @ctx: kmod library context
 * @input: list of kmod_module to be filtered with blacklist
 * @output: where to save the new list
 *
 * This function should not be used. Use kmod_module_apply_filter instead.
 *
 * Given a list @input, this function filter it out with config's blacklist
 * and save it in @output.
 *
 * Returns: 0 on success or < 0 otherwise. @output is saved with the updated
 * list.
 */
int kmod_module_get_filtered_blacklist(const struct kmod_ctx *ctx,
					const struct kmod_list *input,
					struct kmod_list **output) __attribute__ ((deprecated));

/**
 * kmod_module_get_install_commands:
 * @mod: kmod module
 *
 * Get install commands for this kmod module. Install commands come from the
 * configuration file and are cached in @mod. The first call to this function
 * will search for this module in configuration and subsequent calls return
 * the cached string. The install commands are returned as they were in the
 * configuration, concatenated by ';'. No other processing is made in this
 * string.
 *
 * Returns: a string with all install commands separated by semicolons. This
 * string is owned by @mod, do not free it.
 */
const char *kmod_module_get_install_commands(const struct kmod_module *mod);

/**
 * kmod_module_get_remove_commands:
 * @mod: kmod module
 *
 * Get remove commands for this kmod module. Remove commands come from the
 * configuration file and are cached in @mod. The first call to this function
 * will search for this module in configuration and subsequent calls return
 * the cached string. The remove commands are returned as they were in the
 * configuration, concatenated by ';'. No other processing is made in this
 * string.
 *
 * Returns: a string with all remove commands separated by semicolons. This
 * string is owned by @mod, do not free it.
 */
const char *kmod_module_get_remove_commands(const struct kmod_module *mod);

/**
 * kmod_module_get_name:
 * @mod: kmod module
 *
 * Get the name of this kmod module. Name is always available, independently
 * if it was created by kmod_module_new_from_name() or another function and
 * it's always normalized (dashes are replaced with underscores).
 *
 * Returns: the name of this kmod module.
 */
const char *kmod_module_get_name(const struct kmod_module *mod);

/**
 * kmod_module_get_options:
 * @mod: kmod module
 *
 * Get options of this kmod module. Options come from the configuration file
 * and are cached in @mod. The first call to this function will search for
 * this module in configuration and subsequent calls return the cached string.
 *
 * Returns: a string with all the options separated by spaces. This string is
 * owned by @mod, do not free it.
 */
const char *kmod_module_get_options(const struct kmod_module *mod);

/**
 * kmod_module_get_path:
 * @mod: kmod module
 *
 * Get the path of this kmod module. If this kmod module was not created by
 * path, it can search the modules.dep index in order to find out the module
 * under context's dirname.
 *
 * Returns: the path of this kmod module or NULL if such information is not
 * available.
 */
const char *kmod_module_get_path(const struct kmod_module *mod);


/**
 * kmod_module_get_dependency_symbols:
 * @mod: kmod module
 * @list: where to return list of module dependency_symbols
 *
 * Get a list of entries in ELF section ".symtab" or "__ksymtab_strings".
 *
 * The structure contained in this list is internal to libkmod and its fields
 * can be obtainsed by calling kmod_module_dependency_symbol_get_crc() and
 * kmod_module_dependency_symbol_get_symbol().
 *
 * After use, free the @list by calling
 * kmod_module_dependency_symbols_free_list().
 *
 * Returns: 0 on success or < 0 otherwise.
 */
int kmod_module_get_dependency_symbols(const struct kmod_module *mod, struct kmod_list **list);

/**
 * kmod_symbol_bind:
 * @KMOD_SYMBOL_NONE: no or unknown symbol type
 * @KMOD_SYMBOL_LOCAL: local symbol, accessible only within the module
 * @KMOD_SYMBOL_GLOBAL: global symbol, accessible by all modules
 * @KMOD_SYMBOL_WEAK: weak symbol, a lower precedence global symbols
 * @KMOD_SYMBOL_UNDEF: undefined or not yet resolved symbol
 *
 * The symbol bind type, see kmod_module_dependency_symbol_get_bind().
 */
enum kmod_symbol_bind {
	KMOD_SYMBOL_NONE = '\0',
	KMOD_SYMBOL_LOCAL = 'L',
	KMOD_SYMBOL_GLOBAL = 'G',
	KMOD_SYMBOL_WEAK = 'W',
	KMOD_SYMBOL_UNDEF = 'U',
};

/**
 * kmod_module_dependency_symbol_get_bind:
 * @entry: a list entry representing a kmod module dependency_symbol
 *
 * Get the bind type of a kmod module dependency_symbol.
 *
 * Returns: the bind of this kmod module dependency_symbol on success,
 * or < 0 on failure. Valid bind types are #kmod_symbol_bind.
 */
int kmod_module_dependency_symbol_get_bind(const struct kmod_list *entry);

/**
 * kmod_module_dependency_symbol_get_crc:
 * @entry: a list entry representing a kmod module dependency_symbol
 *
 * Get the crc of a kmod module dependency_symbol.
 *
 * Returns: the crc of this kmod module dependency_symbol if available, otherwise default to 0.
 */
uint64_t kmod_module_dependency_symbol_get_crc(const struct kmod_list *entry);

/**
 * kmod_module_dependency_symbol_get_symbol:
 * @entry: a list entry representing a kmod module dependency_symbols
 *
 * Get the dependency symbol of a kmod module
 *
 * Returns: the symbol of this kmod module dependency_symbols on success or NULL
 * on failure. The string is owned by the dependency_symbols, do not free it.
 */
const char *kmod_module_dependency_symbol_get_symbol(const struct kmod_list *entry);

/**
 * kmod_module_dependency_symbols_free_list:
 * @list: kmod module dependency_symbols list
 *
 * Release the resources taken by @list
 */
void kmod_module_dependency_symbols_free_list(struct kmod_list *list);


/**
 * kmod_module_get_sections:
 * @mod: kmod module
 *
 * Get a list of kmod sections of this @mod, as returned by the kernel.
 *
 * The structure contained in this list is internal to libkmod and its fields
 * can be obtained by calling kmod_module_section_get_name() and
 * kmod_module_section_get_address().
 *
 * After use, free the @list by calling kmod_module_section_free_list().
 *
 * Returns: a new list of kmod module sections on success or NULL on failure.
 */
struct kmod_list *kmod_module_get_sections(const struct kmod_module *mod);

/**
 * kmod_module_section_get_address:
 * @entry: a list entry representing a kmod module section
 *
 * Get the address of a kmod module section.
 *
 * Returns: the address of this kmod module section on success or ULONG_MAX
 * on failure.
 */
unsigned long kmod_module_section_get_address(const struct kmod_list *entry);

/**
 * kmod_module_section_get_name:
 * @entry: a list entry representing a kmod module section
 *
 * Get the name of a kmod module section.
 *
 * Returns: the name of this kmod module section on success or NULL on
 * failure. The string is owned by the section, do not free it.
 */
const char *kmod_module_section_get_name(const struct kmod_list *entry);

/**
 * kmod_module_section_free_list:
 * @list: kmod module section list
 *
 * Release the resources taken by @list
 */
void kmod_module_section_free_list(struct kmod_list *list);


/**
 * kmod_module_get_symbols:
 * @mod: kmod module
 * @list: where to return list of module symbols
 *
 * Get a list of entries in ELF section ".symtab" or "__ksymtab_strings".
 *
 * The structure contained in this list is internal to libkmod and its fields
 * can be obtainsed by calling kmod_module_symbol_get_crc() and
 * kmod_module_symbol_get_symbol().
 *
 * After use, free the @list by calling kmod_module_symbols_free_list().
 *
 * Returns: 0 on success or < 0 otherwise.
 */
int kmod_module_get_symbols(const struct kmod_module *mod, struct kmod_list **list);

/**
 * kmod_module_symbol_get_crc:
 * @entry: a list entry representing a kmod module symbol
 *
 * Get the crc of a kmod module symbol.
 *
 * Returns: the crc of this kmod module symbol if available, otherwise default to 0.
 */
uint64_t kmod_module_symbol_get_crc(const struct kmod_list *entry);

/**
 * kmod_module_symbol_get_symbol:
 * @entry: a list entry representing a kmod module symbols
 *
 * Get the symbol of a kmod module symbols.
 *
 * Returns: the symbol of this kmod module symbols on success or NULL
 * on failure. The string is owned by the symbols, do not free it.
 */
const char *kmod_module_symbol_get_symbol(const struct kmod_list *entry);

/**
 * kmod_module_symbols_free_list:
 * @list: kmod module symbols list
 *
 * Release the resources taken by @list
 */
void kmod_module_symbols_free_list(struct kmod_list *list);


/**
 * kmod_module_get_versions:
 * @mod: kmod module
 * @list: where to return list of module versions
 *
 * Get a list of entries in ELF section "__versions".
 *
 * The structure contained in this list is internal to libkmod and its fields
 * can be obtainsed by calling kmod_module_version_get_crc() and
 * kmod_module_version_get_symbol().
 *
 * After use, free the @list by calling kmod_module_versions_free_list().
 *
 * Returns: 0 on success or < 0 otherwise.
 */
int kmod_module_get_versions(const struct kmod_module *mod, struct kmod_list **list);

/**
 * kmod_module_version_get_crc:
 * @entry: a list entry representing a kmod module version
 *
 * Get the crc of a kmod module version.
 *
 * Returns: the crc of this kmod module version if available, otherwise default to 0.
 */
uint64_t kmod_module_version_get_crc(const struct kmod_list *entry);

/**
 * kmod_module_version_get_symbol:
 * @entry: a list entry representing a kmod module versions
 *
 * Get the symbol of a kmod module versions.
 *
 * Returns: the symbol of this kmod module versions on success or NULL
 * on failure. The string is owned by the versions, do not free it.
 */
const char *kmod_module_version_get_symbol(const struct kmod_list *entry);

/**
 * kmod_module_versions_free_list:
 * @list: kmod module versions list
 *
 * Release the resources taken by @list
 */
void kmod_module_versions_free_list(struct kmod_list *list);


/**
 * kmod_module_get_info:
 * @mod: kmod module
 * @list: where to return list of module information
 *
 * Get a list of entries in ELF section ".modinfo", these contain
 * alias, license, depends, vermagic and other keys with respective
 * values. If the module is signed (CONFIG_MODULE_SIG), information
 * about the module signature is included as well: signer,
 * sig_key and sig_hashalgo.
 *
 * The structure contained in this list is internal to libkmod and its fields
 * can be obtainsed by calling kmod_module_info_get_key() and
 * kmod_module_info_get_value().
 *
 * After use, free the @list by calling kmod_module_info_free_list().
 *
 * Returns: number of entries in @list on success or < 0 otherwise.
 */
int kmod_module_get_info(const struct kmod_module *mod, struct kmod_list **list);

/**
 * kmod_module_info_get_key:
 * @entry: a list entry representing a kmod module info
 *
 * Get the key of a kmod module info.
 *
 * Returns: the key of this kmod module info on success or NULL on
 * failure. The string is owned by the info, do not free it.
 */
const char *kmod_module_info_get_key(const struct kmod_list *entry);

/**
 * kmod_module_info_get_value:
 * @entry: a list entry representing a kmod module info
 *
 * Get the value of a kmod module info.
 *
 * Returns: the value of this kmod module info on success or NULL on
 * failure. The string is owned by the info, do not free it.
 */
const char *kmod_module_info_get_value(const struct kmod_list *entry);

/**
 * kmod_module_info_free_list:
 * @list: kmod module info list
 *
 * Release the resources taken by @list
 */
void kmod_module_info_free_list(struct kmod_list *list);



/**
 * SECTION:libkmod-loaded
 * @short_description: currently loaded modules
 *
 * Information about currently loaded modules, as reported by the kernel.
 * These information are not cached by libkmod and are always read from /sys
 * and /proc/modules.
 */

/**
 * kmod_module_new_from_loaded:
 * @ctx: kmod library context
 * @list: where to save the list of loaded modules
 *
 * Create a new list of kmod modules with all modules currently loaded in
 * kernel. It uses /proc/modules to get the names of loaded modules and to
 * create kmod modules by calling kmod_module_new_from_name() in each of them.
 * They are put in @list in no particular order.
 *
 * The initial refcount is 1, and needs to be decremented to release the
 * resources of the kmod_module. The returned @list must be released by
 * calling kmod_module_unref_list(). Since libkmod keeps track of all
 * kmod_modules created, they are all released upon @ctx destruction too. Do
 * not unref @ctx before all the desired operations with the returned list are
 * completed.
 *
 * Returns: 0 on success or < 0 on error.
 */
int kmod_module_new_from_loaded(struct kmod_ctx *ctx,
						struct kmod_list **list);

/**
 * kmod_module_initstate:
 * @KMOD_MODULE_BUILTIN: module is builtin
 * @KMOD_MODULE_LIVE: module is live in the kernel
 * @KMOD_MODULE_COMING: module is being loaded
 * @KMOD_MODULE_GOING: module is being unloaded
 * @_KMOD_MODULE_PAD: DO NOT USE; padding to make sure enum is not mapped to char
 *
 * The module "live information" as reported by the kernel, see
 * kmod_module_get_initstate().
 */
enum kmod_module_initstate {
	KMOD_MODULE_BUILTIN = 0,
	KMOD_MODULE_LIVE,
	KMOD_MODULE_COMING,
	KMOD_MODULE_GOING,
	_KMOD_MODULE_PAD = 1U << 31,
};

/**
 * kmod_module_get_initstate:
 * @mod: kmod module
 *
 * Get the initstate of this @mod, as returned by the kernel, by reading
 * /sys filesystem.
 *
 * Returns: < 0 on error or module state if module is found in the kernel, valid
 * states are #kmod_module_initstate.
 */
int kmod_module_get_initstate(const struct kmod_module *mod);

/**
 * kmod_module_initstate_str:
 * @state: the state as returned by kmod_module_get_initstate()
 *
 * Translate a initstate to a string.
 *
 * Returns: the string associated to the @state. This string is statically
 * allocated, do not free it.
 */
const char *kmod_module_initstate_str(enum kmod_module_initstate state);

/**
 * kmod_module_get_size:
 * @mod: kmod module
 *
 * Get the size of this kmod module as returned by the kernel. If supported,
 * the size is read from the coresize attribute in /sys/module. For older
 * kernels, this falls back on /proc/modules and searches for the specified
 * module to get its size.
 *
 * Returns: the size of this kmod module.
 */
long kmod_module_get_size(const struct kmod_module *mod);

/**
 * kmod_module_get_refcnt:
 * @mod: kmod module
 *
 * Get the ref count of this @mod, as returned by the kernel, by reading
 * /sys filesystem.
 *
 * Returns: the reference count on success or < 0 on failure.
 */
int kmod_module_get_refcnt(const struct kmod_module *mod);

/**
 * kmod_module_get_holders:
 * @mod: kmod module
 *
 * Get a list of kmod modules that are holding this @mod, as returned by Linux
 * Kernel. After use, free the @list by calling kmod_module_unref_list().
 *
 * Returns: a new list of kmod modules on success or NULL on failure.
 */
struct kmod_list *kmod_module_get_holders(const struct kmod_module *mod);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
