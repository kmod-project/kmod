// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"

enum kmod_module_builtin {
	KMOD_MODULE_BUILTIN_UNKNOWN,
	KMOD_MODULE_BUILTIN_NO,
	KMOD_MODULE_BUILTIN_YES,
};

struct kmod_module {
	struct kmod_ctx *ctx;
	char *hashkey;
	char *name;
	char *path;
	struct kmod_list *dep;
	char *options;
	const char *install_commands; /* owned by kmod_config */
	const char *remove_commands; /* owned by kmod_config */
	char *alias; /* only set if this module was created from an alias */
	struct kmod_file *file;
	int n_dep;
	int refcount;
	struct {
		bool dep : 1;
		bool options : 1;
		bool install_commands : 1;
		bool remove_commands : 1;
	} init;

	/*
	 * mark if module is builtin, i.e. it's present on modules.builtin
	 * file. This is set as soon as it is needed or as soon as we know
	 * about it, i.e. the module was created from builtin lookup.
	 */
	enum kmod_module_builtin builtin;

	/*
	 * private field used by kmod_module_get_probe_list() to detect
	 * dependency loops
	 */
	bool visited : 1;

	/*
	 * set by kmod_module_get_probe_list: indicates for probe_insert()
	 * whether the module's command and softdep should be ignored
	 */
	bool ignorecmd : 1;

	/*
	 * set by kmod_module_get_probe_list: indicates whether this is the
	 * module the user asked for or its dependency, or whether this
	 * is a softdep only
	 */
	bool required : 1;
};

static inline const char *path_join(const char *path, size_t prefixlen, char buf[PATH_MAX])
{
	size_t pathlen;

	if (path[0] == '/')
		return path;

	pathlen = strlen(path);
	if (prefixlen + pathlen + 1 >= PATH_MAX)
		return NULL;

	memcpy(buf + prefixlen, path, pathlen + 1);
	return buf;
}

static inline bool module_is_inkernel(struct kmod_module *mod)
{
	int state = kmod_module_get_initstate(mod);

	if (state == KMOD_MODULE_LIVE || state == KMOD_MODULE_BUILTIN)
		return true;

	return false;
}

int kmod_module_parse_depline(struct kmod_module *mod, char *line)
{
	struct kmod_ctx *ctx = mod->ctx;
	struct kmod_list *list = NULL;
	const char *dirname;
	char buf[PATH_MAX];
	char *p, *saveptr;
	int err = 0, n = 0;
	size_t dirnamelen;

	if (mod->init.dep)
		return mod->n_dep;
	assert(mod->dep == NULL);
	mod->init.dep = true;

	p = strchr(line, ':');
	if (p == NULL)
		return 0;

	*p = '\0';
	dirname = kmod_get_dirname(mod->ctx);
	dirnamelen = strlen(dirname);
	if (dirnamelen + 2 >= PATH_MAX)
		return 0;

	memcpy(buf, dirname, dirnamelen);
	buf[dirnamelen] = '/';
	dirnamelen++;
	buf[dirnamelen] = '\0';

	if (mod->path == NULL) {
		const char *str = path_join(line, dirnamelen, buf);
		if (str == NULL)
			return 0;
		mod->path = strdup(str);
		if (mod->path == NULL)
			return 0;
	}

	p++;
	for (p = strtok_r(p, " \t", &saveptr); p != NULL;
	     p = strtok_r(NULL, " \t", &saveptr)) {
		struct kmod_module *depmod = NULL;
		const char *path;

		path = path_join(p, dirnamelen, buf);
		if (path == NULL) {
			ERR(ctx, "could not join path '%s' and '%s'.\n", dirname, p);
			goto fail;
		}

		err = kmod_module_new_from_path(ctx, path, &depmod);
		if (err < 0) {
			ERR(ctx, "ctx=%p path=%s error=%s\n", ctx, path, strerror(-err));
			goto fail;
		}

		DBG(ctx, "add dep: %s\n", path);

		list = kmod_list_prepend(list, depmod);
		n++;
	}

	DBG(ctx, "%d dependencies for %s\n", n, mod->name);

	mod->dep = list;
	mod->n_dep = n;
	return n;

fail:
	kmod_module_unref_list(list);
	mod->init.dep = false;
	return err;
}

void kmod_module_set_visited(struct kmod_module *mod, bool visited)
{
	mod->visited = visited;
}

void kmod_module_set_builtin(struct kmod_module *mod, bool builtin)
{
	mod->builtin = builtin ? KMOD_MODULE_BUILTIN_YES : KMOD_MODULE_BUILTIN_NO;
}

void kmod_module_set_required(struct kmod_module *mod, bool required)
{
	mod->required = required;
}

bool kmod_module_is_builtin(struct kmod_module *mod)
{
	if (mod->builtin == KMOD_MODULE_BUILTIN_UNKNOWN) {
		kmod_module_set_builtin(mod, kmod_lookup_alias_is_builtin(mod->ctx,
									  mod->name));
	}

	return mod->builtin == KMOD_MODULE_BUILTIN_YES;
}
/*
 * Memory layout with alias:
 *
 * struct kmod_module {
 *        hashkey -----.
 *        alias -----. |
 *        name ----. | |
 * }               | | |
 * name <----------' | |
 * alias <-----------' |
 * name\alias <--------'
 *
 * Memory layout without alias:
 *
 * struct kmod_module {
 *        hashkey ---.
 *        alias -----|----> NULL
 *        name ----. |
 * }               | |
 * name <----------'-'
 *
 * @key is "name\alias" or "name" (in which case alias == NULL)
 */
static int kmod_module_new(struct kmod_ctx *ctx, const char *key, const char *name,
			   size_t namelen, const char *alias, size_t aliaslen,
			   struct kmod_module **mod)
{
	struct kmod_module *m;
	size_t keylen;

	m = kmod_pool_get_module(ctx, key);
	if (m != NULL) {
		*mod = kmod_module_ref(m);
		return 0;
	}

	if (alias == NULL)
		keylen = namelen;
	else
		keylen = namelen + aliaslen + 1;

	m = malloc(sizeof(*m) + (alias == NULL ? 1 : 2) * (keylen + 1));
	if (m == NULL)
		return -ENOMEM;

	memset(m, 0, sizeof(*m));

	m->ctx = kmod_ref(ctx);
	m->name = (char *)m + sizeof(*m);
	memcpy(m->name, key, keylen + 1);
	if (alias == NULL) {
		m->hashkey = m->name;
		m->alias = NULL;
	} else {
		m->name[namelen] = '\0';
		m->alias = m->name + namelen + 1;
		m->hashkey = m->name + keylen + 1;
		memcpy(m->hashkey, key, keylen + 1);
	}

	m->refcount = 1;
	kmod_pool_add_module(ctx, m, m->hashkey);
	*mod = m;

	return 0;
}

KMOD_EXPORT int kmod_module_new_from_name(struct kmod_ctx *ctx, const char *name,
					  struct kmod_module **mod)
{
	size_t namelen;
	char name_norm[PATH_MAX];

	if (ctx == NULL || name == NULL || mod == NULL)
		return -ENOENT;

	modname_normalize(name, name_norm, &namelen);

	return kmod_module_new(ctx, name_norm, name_norm, namelen, NULL, 0, mod);
}

int kmod_module_new_from_alias(struct kmod_ctx *ctx, const char *alias, const char *name,
			       struct kmod_module **mod)
{
	int err;
	char key[PATH_MAX];
	size_t namelen = strlen(name);
	size_t aliaslen = strlen(alias);

	if (namelen + aliaslen + 2 > PATH_MAX)
		return -ENAMETOOLONG;

	memcpy(key, name, namelen);
	memcpy(key + namelen + 1, alias, aliaslen + 1);
	key[namelen] = '\\';

	err = kmod_module_new(ctx, key, name, namelen, alias, aliaslen, mod);
	if (err < 0)
		return err;

	return 0;
}

KMOD_EXPORT int kmod_module_new_from_path(struct kmod_ctx *ctx, const char *path,
					  struct kmod_module **mod)
{
	struct kmod_module *m;
	int err;
	struct stat st;
	char name[PATH_MAX];
	char *abspath;
	size_t namelen;

	if (ctx == NULL || path == NULL || mod == NULL)
		return -ENOENT;

	abspath = path_make_absolute_cwd(path);
	if (abspath == NULL) {
		DBG(ctx, "no absolute path for %s\n", path);
		return -ENOMEM;
	}

	err = stat(abspath, &st);
	if (err < 0) {
		err = -errno;
		DBG(ctx, "stat %s: %s\n", path, strerror(errno));
		free(abspath);
		return err;
	}

	if (path_to_modname(path, name, &namelen) == NULL) {
		DBG(ctx, "could not get modname from path %s\n", path);
		free(abspath);
		return -ENOENT;
	}

	err = kmod_module_new(ctx, name, name, namelen, NULL, 0, &m);
	if (err < 0) {
		free(abspath);
		return err;
	}
	if (m->path == NULL)
		m->path = abspath;
	else if (streq(m->path, abspath))
		free(abspath);
	else {
		kmod_module_unref(m);
		ERR(ctx,
		    "kmod_module '%s' already exists with different path: new-path='%s' old-path='%s'\n",
		    name, abspath, m->path);
		free(abspath);
		return -EEXIST;
	}

	m->builtin = KMOD_MODULE_BUILTIN_NO;
	*mod = m;

	return 0;
}

KMOD_EXPORT struct kmod_module *kmod_module_unref(struct kmod_module *mod)
{
	if (mod == NULL)
		return NULL;

	if (--mod->refcount > 0)
		return mod;

	DBG(mod->ctx, "kmod_module %p released\n", mod);

	kmod_pool_del_module(mod->ctx, mod, mod->hashkey);
	kmod_module_unref_list(mod->dep);

	if (mod->file)
		kmod_file_unref(mod->file);

	kmod_unref(mod->ctx);
	free(mod->options);
	free(mod->path);
	free(mod);
	return NULL;
}

KMOD_EXPORT struct kmod_module *kmod_module_ref(struct kmod_module *mod)
{
	if (mod == NULL)
		return NULL;

	mod->refcount++;

	return mod;
}

typedef _nonnull_all_ int (*lookup_func)(struct kmod_ctx *ctx, const char *name,
					 struct kmod_list **list);

static int __kmod_module_new_from_lookup(struct kmod_ctx *ctx, const lookup_func lookup[],
					 size_t lookup_count, const char *s,
					 struct kmod_list **list)
{
	unsigned int i;

	for (i = 0; i < lookup_count; i++) {
		int err;

		err = lookup[i](ctx, s, list);
		if (err < 0 && err != -ENOSYS)
			return err;
		else if (*list != NULL)
			return 0;
	}

	return 0;
}

KMOD_EXPORT int kmod_module_new_from_lookup(struct kmod_ctx *ctx, const char *given_alias,
					    struct kmod_list **list)
{
	static const lookup_func lookup[] = {
		kmod_lookup_alias_from_config,
		kmod_lookup_alias_from_moddep_file,
		kmod_lookup_alias_from_symbols_file,
		kmod_lookup_alias_from_commands,
		kmod_lookup_alias_from_aliases_file,
		kmod_lookup_alias_from_builtin_file,
		kmod_lookup_alias_from_kernel_builtin_file,
	};
	char alias[PATH_MAX];
	int err;

	if (ctx == NULL || given_alias == NULL)
		return -ENOENT;

	if (list == NULL || *list != NULL) {
		ERR(ctx, "An empty list is needed to create lookup\n");
		return -ENOSYS;
	}

	if (alias_normalize(given_alias, alias, NULL) < 0) {
		DBG(ctx, "invalid alias: %s\n", given_alias);
		return -EINVAL;
	}

	DBG(ctx, "input alias=%s, normalized=%s\n", given_alias, alias);

	err = __kmod_module_new_from_lookup(ctx, lookup, ARRAY_SIZE(lookup), alias, list);

	DBG(ctx, "lookup=%s found=%d\n", alias, err >= 0 && *list);

	if (err < 0) {
		kmod_module_unref_list(*list);
		*list = NULL;
	}

	return err;
}

KMOD_EXPORT int kmod_module_new_from_name_lookup(struct kmod_ctx *ctx,
						 const char *modname,
						 struct kmod_module **mod)
{
	static const lookup_func lookup[] = {
		kmod_lookup_alias_from_moddep_file,
		kmod_lookup_alias_from_builtin_file,
		kmod_lookup_alias_from_kernel_builtin_file,
	};
	char name_norm[PATH_MAX];
	struct kmod_list *list = NULL;
	int err;

	if (ctx == NULL || modname == NULL || mod == NULL)
		return -ENOENT;

	modname_normalize(modname, name_norm, NULL);

	DBG(ctx, "input modname=%s, normalized=%s\n", modname, name_norm);

	err = __kmod_module_new_from_lookup(ctx, lookup, ARRAY_SIZE(lookup), name_norm,
					    &list);

	DBG(ctx, "lookup=%s found=%d\n", name_norm, err >= 0 && list);

	if (err >= 0 && list != NULL)
		*mod = kmod_module_get_module(list);

	kmod_module_unref_list(list);

	return err;
}

KMOD_EXPORT int kmod_module_unref_list(struct kmod_list *list)
{
	for (; list != NULL; list = kmod_list_remove(list))
		kmod_module_unref(list->data);

	return 0;
}

KMOD_EXPORT int kmod_module_get_filtered_blacklist(const struct kmod_ctx *ctx,
						   const struct kmod_list *input,
						   struct kmod_list **output)
{
	return kmod_module_apply_filter(ctx, KMOD_FILTER_BLACKLIST, input, output);
}

static const struct kmod_list *module_get_dependencies_noref(const struct kmod_module *mod)
{
	if (!mod->init.dep) {
		/* lazy init */
		char *line = kmod_search_moddep(mod->ctx, mod->name);

		if (line == NULL)
			return NULL;

		kmod_module_parse_depline((struct kmod_module *)mod, line);
		free(line);

		if (!mod->init.dep)
			return NULL;
	}

	return mod->dep;
}

KMOD_EXPORT struct kmod_list *kmod_module_get_dependencies(const struct kmod_module *mod)
{
	struct kmod_list *l, *l_new, *list_new = NULL;

	if (mod == NULL)
		return NULL;

	module_get_dependencies_noref(mod);

	kmod_list_foreach(l, mod->dep) {
		l_new = kmod_list_append(list_new, kmod_module_ref(l->data));
		if (l_new == NULL) {
			kmod_module_unref(l->data);
			goto fail;
		}

		list_new = l_new;
	}

	return list_new;

fail:
	ERR(mod->ctx, "out of memory\n");
	kmod_module_unref_list(list_new);
	return NULL;
}

KMOD_EXPORT struct kmod_module *kmod_module_get_module(const struct kmod_list *entry)
{
	if (entry == NULL)
		return NULL;

	return kmod_module_ref(entry->data);
}

KMOD_EXPORT const char *kmod_module_get_name(const struct kmod_module *mod)
{
	if (mod == NULL)
		return NULL;

	return mod->name;
}

KMOD_EXPORT const char *kmod_module_get_path(const struct kmod_module *mod)
{
	char *line;

	if (mod == NULL)
		return NULL;

	DBG(mod->ctx, "name='%s' path='%s'\n", mod->name, mod->path);

	if (mod->path != NULL)
		return mod->path;
	if (mod->init.dep)
		return NULL;

	/* lazy init */
	line = kmod_search_moddep(mod->ctx, mod->name);
	if (line == NULL)
		return NULL;

	kmod_module_parse_depline((struct kmod_module *)mod, line);
	free(line);

	return mod->path;
}

extern long delete_module(const char *name, unsigned int flags);

KMOD_EXPORT int kmod_module_remove_module(struct kmod_module *mod, unsigned int flags)
{
	unsigned int libkmod_flags = flags & 0xff;

	int err;

	if (mod == NULL)
		return -ENOENT;

	/* Filter out other flags and force ONONBLOCK */
	flags &= KMOD_REMOVE_FORCE;
	flags |= KMOD_REMOVE_NOWAIT;

	err = delete_module(mod->name, flags);
	if (err != 0) {
		err = -errno;
		if (!(libkmod_flags & KMOD_REMOVE_NOLOG))
			ERR(mod->ctx, "could not remove '%s': %m\n", mod->name);
	}

	return err;
}

extern long init_module(const void *mem, unsigned long len, const char *args);

static int do_finit_module(struct kmod_module *mod, unsigned int flags, const char *args)
{
	enum kmod_file_compression_type compression, kernel_compression;
	unsigned int kernel_flags = 0;
	int err;

	/*
	 * When module is not compressed or its compression type matches the
	 * one in use by the kernel, there is no need to read the file
	 * in userspace. Otherwise, reuse ENOSYS to trigger the same fallback
	 * as when finit_module() is not supported.
	 */
	compression = kmod_file_get_compression(mod->file);
	kernel_compression = kmod_get_kernel_compression(mod->ctx);
	if (!(compression == KMOD_FILE_COMPRESSION_NONE ||
	      compression == kernel_compression))
		return -ENOSYS;

	if (compression != KMOD_FILE_COMPRESSION_NONE)
		kernel_flags |= MODULE_INIT_COMPRESSED_FILE;

	if (flags & KMOD_INSERT_FORCE_VERMAGIC)
		kernel_flags |= MODULE_INIT_IGNORE_VERMAGIC;
	if (flags & KMOD_INSERT_FORCE_MODVERSION)
		kernel_flags |= MODULE_INIT_IGNORE_MODVERSIONS;

	err = finit_module(kmod_file_get_fd(mod->file), args, kernel_flags);
	if (err < 0)
		err = -errno;

	return err;
}

static int do_init_module(struct kmod_module *mod, unsigned int flags, const char *args)
{
	struct kmod_elf *elf;
	const void *mem;
	off_t size;
	int err;

	if (flags & (KMOD_INSERT_FORCE_VERMAGIC | KMOD_INSERT_FORCE_MODVERSION)) {
		elf = kmod_file_get_elf(mod->file);
		if (elf == NULL) {
			err = -errno;
			return err;
		}

		if (flags & KMOD_INSERT_FORCE_MODVERSION) {
			err = kmod_elf_strip_section(elf, "__versions");
			if (err < 0)
				INFO(mod->ctx, "Failed to strip modversion: %s\n",
				     strerror(-err));
		}

		if (flags & KMOD_INSERT_FORCE_VERMAGIC) {
			err = kmod_elf_strip_vermagic(elf);
			if (err < 0)
				INFO(mod->ctx, "Failed to strip vermagic: %s\n",
				     strerror(-err));
		}

		mem = kmod_elf_get_memory(elf);
	} else {
		err = kmod_file_load_contents(mod->file);
		if (err)
			return err;

		mem = kmod_file_get_contents(mod->file);
	}
	size = kmod_file_get_size(mod->file);

	err = init_module(mem, size, args);
	if (err < 0)
		err = -errno;

	return err;
}

KMOD_EXPORT int kmod_module_insert_module(struct kmod_module *mod, unsigned int flags,
					  const char *options)
{
	int err;
	const char *path;
	const char *args = options ? options : "";

	if (mod == NULL)
		return -ENOENT;

	path = kmod_module_get_path(mod);
	if (path == NULL) {
		ERR(mod->ctx, "could not find module by name='%s'\n", mod->name);
		return -ENOENT;
	}

	if (!mod->file) {
		mod->file = kmod_file_open(mod->ctx, path);
		if (mod->file == NULL) {
			err = -errno;
			return err;
		}
	}

	err = do_finit_module(mod, flags, args);
	if (err == -ENOSYS)
		err = do_init_module(mod, flags, args);

	if (err < 0)
		INFO(mod->ctx, "Failed to insert module '%s': %s\n", path, strerror(-err));

	return err;
}

static bool module_is_blacklisted(const struct kmod_module *mod)
{
	const struct kmod_ctx *ctx = mod->ctx;
	const struct kmod_config *config = kmod_get_config(ctx);
	const struct kmod_list *bl = config->blacklists;
	const struct kmod_list *l;

	kmod_list_foreach(l, bl) {
		const char *modname = kmod_blacklist_get_modname(l);

		if (streq(modname, mod->name))
			return true;
	}

	return false;
}

KMOD_EXPORT int kmod_module_apply_filter(const struct kmod_ctx *ctx,
					 enum kmod_filter filter_type,
					 const struct kmod_list *input,
					 struct kmod_list **output)
{
	const struct kmod_list *li;

	if (ctx == NULL || output == NULL)
		return -ENOENT;

	*output = NULL;
	if (input == NULL)
		return 0;

	kmod_list_foreach(li, input) {
		struct kmod_module *mod = li->data;
		struct kmod_list *node;

		if ((filter_type & KMOD_FILTER_BLACKLIST) && module_is_blacklisted(mod))
			continue;

		if ((filter_type & KMOD_FILTER_BUILTIN) && kmod_module_is_builtin(mod))
			continue;

		node = kmod_list_append(*output, mod);
		if (node == NULL)
			goto fail;

		*output = node;
		kmod_module_ref(mod);
	}

	return 0;

fail:
	kmod_module_unref_list(*output);
	*output = NULL;
	return -ENOMEM;
}

static int command_do(struct kmod_module *mod, const char *type, const char *cmd)
{
	const char *modname = kmod_module_get_name(mod);
	int err;

	DBG(mod->ctx, "%s %s\n", type, cmd);

	setenv("MODPROBE_MODULE", modname, 1);
	err = system(cmd);
	unsetenv("MODPROBE_MODULE");

	if (err == -1) {
		ERR(mod->ctx, "Could not run %s command '%s' for module %s: %m\n", type,
		    cmd, modname);
		return -EINVAL;
	}

	if (WEXITSTATUS(err)) {
		ERR(mod->ctx, "Error running %s command '%s' for module %s: retcode %d\n",
		    type, cmd, modname, WEXITSTATUS(err));
		return -EINVAL;
	}

	return 0;
}

struct probe_insert_cb {
	int (*run_install)(struct kmod_module *m, const char *cmd, void *data);
	void *data;
};

static int module_do_install_commands(struct kmod_module *mod, const char *options,
				      struct probe_insert_cb *cb)
{
	const char *command = kmod_module_get_install_commands(mod);
	char *p;
	_cleanup_free_ char *cmd;
	int err;
	size_t cmdlen, options_len, varlen;

	assert(command);

	if (options == NULL)
		options = "";

	options_len = strlen(options);
	cmdlen = strlen(command);
	varlen = sizeof("$CMDLINE_OPTS") - 1;

	cmd = memdup(command, cmdlen + 1);
	if (cmd == NULL)
		return -ENOMEM;

	while ((p = strstr(cmd, "$CMDLINE_OPTS")) != NULL) {
		size_t prefixlen = p - cmd;
		size_t suffixlen = cmdlen - prefixlen - varlen;
		size_t slen = cmdlen - varlen + options_len;
		char *suffix = p + varlen;
		char *s = malloc(slen + 1);
		if (!s)
			return -ENOMEM;

		memcpy(s, cmd, p - cmd);
		memcpy(s + prefixlen, options, options_len);
		memcpy(s + prefixlen + options_len, suffix, suffixlen);
		s[slen] = '\0';

		free(cmd);
		cmd = s;
		cmdlen = slen;
	}

	if (cb->run_install != NULL)
		err = cb->run_install(mod, cmd, cb->data);
	else
		err = command_do(mod, "install", cmd);

	return err;
}

static char *module_options_concat(const char *opt, const char *xopt)
{
	// TODO: we might need to check if xopt overrides options on opt
	size_t optlen = opt == NULL ? 0 : strlen(opt);
	size_t xoptlen = xopt == NULL ? 0 : strlen(xopt);
	char *r;

	if (optlen == 0 && xoptlen == 0)
		return NULL;

	r = malloc(optlen + xoptlen + 2);

	if (opt != NULL) {
		memcpy(r, opt, optlen);
		r[optlen] = ' ';
		optlen++;
	}

	if (xopt != NULL)
		memcpy(r + optlen, xopt, xoptlen);

	r[optlen + xoptlen] = '\0';

	return r;
}

static int __kmod_module_get_probe_list(struct kmod_module *mod, bool required,
					bool ignorecmd, struct kmod_list **list);

/* re-entrant */
static int __kmod_module_fill_softdep(struct kmod_module *mod, struct kmod_list **list)
{
	struct kmod_list *pre = NULL, *post = NULL, *l;
	int err;

	err = kmod_module_get_softdeps(mod, &pre, &post);
	if (err < 0) {
		ERR(mod->ctx, "could not get softdep: %s\n", strerror(-err));
		goto fail;
	}

	kmod_list_foreach(l, pre) {
		struct kmod_module *m = l->data;
		err = __kmod_module_get_probe_list(m, false, false, list);
		if (err < 0)
			goto fail;
	}

	l = kmod_list_append(*list, kmod_module_ref(mod));
	if (l == NULL) {
		kmod_module_unref(mod);
		err = -ENOMEM;
		goto fail;
	}
	*list = l;
	mod->ignorecmd = (pre != NULL || post != NULL);

	kmod_list_foreach(l, post) {
		struct kmod_module *m = l->data;
		err = __kmod_module_get_probe_list(m, false, false, list);
		if (err < 0)
			goto fail;
	}

fail:
	kmod_module_unref_list(pre);
	kmod_module_unref_list(post);

	return err;
}

/* re-entrant */
static int __kmod_module_get_probe_list(struct kmod_module *mod, bool required,
					bool ignorecmd, struct kmod_list **list)
{
	struct kmod_list *dep, *l;
	int err = 0;

	if (mod->visited) {
		DBG(mod->ctx, "Ignore module '%s': already visited\n", mod->name);
		return 0;
	}
	mod->visited = true;

	dep = kmod_module_get_dependencies(mod);
	if (required) {
		/*
		 * Called from kmod_module_probe_insert_module(); set the
		 * ->required flag on mod and all its dependencies before
		 * they are possibly visited through some softdeps.
		 */
		mod->required = true;
		kmod_list_foreach(l, dep) {
			struct kmod_module *m = l->data;
			m->required = true;
		}
	}

	kmod_list_foreach(l, dep) {
		struct kmod_module *m = l->data;
		err = __kmod_module_fill_softdep(m, list);
		if (err < 0)
			goto finish;
	}

	if (ignorecmd) {
		l = kmod_list_append(*list, kmod_module_ref(mod));
		if (l == NULL) {
			kmod_module_unref(mod);
			err = -ENOMEM;
			goto finish;
		}
		*list = l;
		mod->ignorecmd = true;
	} else
		err = __kmod_module_fill_softdep(mod, list);

finish:
	kmod_module_unref_list(dep);
	return err;
}

static int kmod_module_get_probe_list(struct kmod_module *mod, bool ignorecmd,
				      struct kmod_list **list)
{
	int err;

	assert(mod != NULL);
	assert(list != NULL && *list == NULL);

	/*
	 * Make sure we don't get screwed by previous calls to this function
	 */
	kmod_set_modules_visited(mod->ctx, false);
	kmod_set_modules_required(mod->ctx, false);

	err = __kmod_module_get_probe_list(mod, true, ignorecmd, list);
	if (err < 0) {
		kmod_module_unref_list(*list);
		*list = NULL;
	}

	return err;
}

KMOD_EXPORT int kmod_module_probe_insert_module(
	struct kmod_module *mod, unsigned int flags, const char *extra_options,
	int (*run_install)(struct kmod_module *m, const char *cmd, void *data),
	const void *data,
	void (*print_action)(struct kmod_module *m, bool install, const char *options))
{
	struct kmod_list *list = NULL, *l;
	struct probe_insert_cb cb;
	int err;

	if (mod == NULL)
		return -ENOENT;

	if (!(flags & KMOD_PROBE_IGNORE_LOADED) && module_is_inkernel(mod)) {
		if (flags & KMOD_PROBE_FAIL_ON_LOADED)
			return -EEXIST;
		else
			return 0;
	}

	if (module_is_blacklisted(mod)) {
		if (mod->alias != NULL && (flags & KMOD_PROBE_APPLY_BLACKLIST_ALIAS_ONLY))
			return KMOD_PROBE_APPLY_BLACKLIST_ALIAS_ONLY;

		if (flags & KMOD_PROBE_APPLY_BLACKLIST_ALL)
			return KMOD_PROBE_APPLY_BLACKLIST_ALL;

		if (flags & KMOD_PROBE_APPLY_BLACKLIST)
			return KMOD_PROBE_APPLY_BLACKLIST;
	}

	err = kmod_module_get_probe_list(mod, !!(flags & KMOD_PROBE_IGNORE_COMMAND),
					 &list);
	if (err < 0)
		return err;

	if (flags & KMOD_PROBE_APPLY_BLACKLIST_ALL) {
		struct kmod_list *filtered = NULL;

		err = kmod_module_apply_filter(mod->ctx, KMOD_FILTER_BLACKLIST, list,
					       &filtered);
		if (err < 0)
			return err;

		kmod_module_unref_list(list);
		if (filtered == NULL)
			return KMOD_PROBE_APPLY_BLACKLIST_ALL;

		list = filtered;
	}

	cb.run_install = run_install;
	cb.data = (void *)data;

	kmod_list_foreach(l, list) {
		struct kmod_module *m = l->data;
		const char *moptions = kmod_module_get_options(m);
		const char *cmd = kmod_module_get_install_commands(m);
		char *options;

		if (!(flags & KMOD_PROBE_IGNORE_LOADED) && module_is_inkernel(m)) {
			DBG(mod->ctx, "Ignoring module '%s': already loaded\n", m->name);
			err = -EEXIST;
			goto finish_module;
		}

		options =
			module_options_concat(moptions, m == mod ? extra_options : NULL);

		if (cmd != NULL && !m->ignorecmd) {
			if (print_action != NULL)
				print_action(m, true, options ?: "");

			if (!(flags & KMOD_PROBE_DRY_RUN))
				err = module_do_install_commands(m, options, &cb);
		} else {
			if (print_action != NULL)
				print_action(m, false, options ?: "");

			if (!(flags & KMOD_PROBE_DRY_RUN))
				err = kmod_module_insert_module(m, flags, options);
		}

		free(options);

finish_module:
		/*
		 * Treat "already loaded" error. If we were told to stop on
		 * already loaded and the module being loaded is not a softdep
		 * or dep, bail out. Otherwise, just ignore and continue.
		 *
		 * We need to check here because of race conditions. We
		 * checked first if module was already loaded but it may have
		 * been loaded between the check and the moment we try to
		 * insert it.
		 */
		if (err == -EEXIST && m == mod && (flags & KMOD_PROBE_FAIL_ON_LOADED))
			break;

		/*
		 * Ignore errors from softdeps
		 */
		if (err == -EEXIST || !m->required)
			err = 0;

		else if (err < 0)
			break;
	}

	kmod_module_unref_list(list);
	return err;
}

KMOD_EXPORT const char *kmod_module_get_options(const struct kmod_module *mod)
{
	if (mod == NULL)
		return NULL;

	if (!mod->init.options) {
		/* lazy init */
		struct kmod_module *m = (struct kmod_module *)mod;
		const struct kmod_list *l;
		const struct kmod_config *config;
		char *opts = NULL;
		size_t optslen = 0;

		config = kmod_get_config(mod->ctx);

		kmod_list_foreach(l, config->options) {
			const char *modname = kmod_option_get_modname(l);
			const char *str;
			size_t len;
			void *tmp;

			DBG(mod->ctx, "modname=%s mod->name=%s mod->alias=%s\n", modname,
			    mod->name, mod->alias);
			if (!(streq(modname, mod->name) ||
			      (mod->alias != NULL && streq(modname, mod->alias))))
				continue;

			DBG(mod->ctx, "passed = modname=%s mod->name=%s mod->alias=%s\n",
			    modname, mod->name, mod->alias);
			str = kmod_option_get_options(l);
			len = strlen(str);
			if (len < 1)
				continue;

			tmp = realloc(opts, optslen + len + 2);
			if (tmp == NULL) {
				free(opts);
				goto failed;
			}

			opts = tmp;

			if (optslen > 0) {
				opts[optslen] = ' ';
				optslen++;
			}

			memcpy(opts + optslen, str, len);
			optslen += len;
			opts[optslen] = '\0';
		}

		m->init.options = true;
		m->options = opts;
	}

	return mod->options;

failed:
	ERR(mod->ctx, "out of memory\n");
	return NULL;
}

KMOD_EXPORT const char *kmod_module_get_install_commands(const struct kmod_module *mod)
{
	if (mod == NULL)
		return NULL;

	if (!mod->init.install_commands) {
		/* lazy init */
		struct kmod_module *m = (struct kmod_module *)mod;
		const struct kmod_list *l;
		const struct kmod_config *config;

		config = kmod_get_config(mod->ctx);

		kmod_list_foreach(l, config->install_commands) {
			const char *modname = kmod_command_get_modname(l);

			if (fnmatch(modname, mod->name, 0) != 0)
				continue;

			m->install_commands = kmod_command_get_command(l);

			/*
			 * find only the first command, as modprobe from
			 * module-init-tools does
			 */
			break;
		}

		m->init.install_commands = true;
	}

	return mod->install_commands;
}

void kmod_module_set_install_commands(struct kmod_module *mod, const char *cmd)
{
	mod->init.install_commands = true;
	mod->install_commands = cmd;
}

static struct kmod_list *lookup_dep(struct kmod_ctx *ctx, const char *const *array,
				    unsigned int count)
{
	struct kmod_list *ret = NULL;
	unsigned i;

	for (i = 0; i < count; i++) {
		const char *depname = array[i];
		struct kmod_list *lst = NULL;
		int err;

		err = kmod_module_new_from_lookup(ctx, depname, &lst);
		if (err < 0) {
			ERR(ctx, "failed to lookup dependency '%s', continuing anyway.\n",
			    depname);
			continue;
		} else if (lst != NULL)
			ret = kmod_list_append_list(ret, lst);
	}
	return ret;
}

KMOD_EXPORT int kmod_module_get_softdeps(const struct kmod_module *mod,
					 struct kmod_list **pre, struct kmod_list **post)
{
	const struct kmod_list *l;
	const struct kmod_config *config;

	if (mod == NULL || pre == NULL || post == NULL)
		return -ENOENT;

	assert(*pre == NULL);
	assert(*post == NULL);

	config = kmod_get_config(mod->ctx);

	kmod_list_foreach(l, config->softdeps) {
		const char *modname = kmod_softdep_get_name(l);
		const char *const *array;
		unsigned count;

		if (fnmatch(modname, mod->name, 0) != 0)
			continue;

		array = kmod_softdep_get_pre(l, &count);
		*pre = lookup_dep(mod->ctx, array, count);
		array = kmod_softdep_get_post(l, &count);
		*post = lookup_dep(mod->ctx, array, count);

		/*
		 * find only the first command, as modprobe from
		 * module-init-tools does
		 */
		break;
	}

	return 0;
}

KMOD_EXPORT int kmod_module_get_weakdeps(const struct kmod_module *mod,
					 struct kmod_list **weak)
{
	const struct kmod_list *l;
	const struct kmod_config *config;

	if (mod == NULL || weak == NULL)
		return -ENOENT;

	assert(*weak == NULL);

	config = kmod_get_config(mod->ctx);

	kmod_list_foreach(l, config->weakdeps) {
		const char *modname = kmod_weakdep_get_name(l);
		const char *const *array;
		unsigned count;

		if (fnmatch(modname, mod->name, 0) != 0)
			continue;

		array = kmod_weakdep_get_weak(l, &count);
		*weak = lookup_dep(mod->ctx, array, count);

		/*
		 * find only the first command, as modprobe from
		 * module-init-tools does
		 */
		break;
	}

	return 0;
}

KMOD_EXPORT const char *kmod_module_get_remove_commands(const struct kmod_module *mod)
{
	if (mod == NULL)
		return NULL;

	if (!mod->init.remove_commands) {
		/* lazy init */
		struct kmod_module *m = (struct kmod_module *)mod;
		const struct kmod_list *l;
		const struct kmod_config *config;

		config = kmod_get_config(mod->ctx);

		kmod_list_foreach(l, config->remove_commands) {
			const char *modname = kmod_command_get_modname(l);

			if (fnmatch(modname, mod->name, 0) != 0)
				continue;

			m->remove_commands = kmod_command_get_command(l);

			/*
			 * find only the first command, as modprobe from
			 * module-init-tools does
			 */
			break;
		}

		m->init.remove_commands = true;
	}

	return mod->remove_commands;
}

void kmod_module_set_remove_commands(struct kmod_module *mod, const char *cmd)
{
	mod->init.remove_commands = true;
	mod->remove_commands = cmd;
}

KMOD_EXPORT int kmod_module_new_from_loaded(struct kmod_ctx *ctx, struct kmod_list **list)
{
	struct kmod_list *l = NULL;
	FILE *fp;
	char line[4096];

	if (ctx == NULL || list == NULL)
		return -ENOENT;

	fp = fopen("/proc/modules", "re");
	if (fp == NULL) {
		int err = -errno;
		ERR(ctx, "could not open /proc/modules: %s\n", strerror(errno));
		return err;
	}

	while (fgets(line, sizeof(line), fp)) {
		struct kmod_module *m;
		struct kmod_list *node;
		int err;
		size_t len = strlen(line);
		char *saveptr, *name = strtok_r(line, " \t", &saveptr);

		err = kmod_module_new_from_name(ctx, name, &m);
		if (err < 0) {
			ERR(ctx, "could not get module from name '%s': %s\n", name,
			    strerror(-err));
			goto eat_line;
		}

		node = kmod_list_append(l, m);
		if (node)
			l = node;
		else {
			ERR(ctx, "out of memory\n");
			kmod_module_unref(m);
		}
eat_line:
		while (line[len - 1] != '\n' && fgets(line, sizeof(line), fp))
			len = strlen(line);
	}

	fclose(fp);
	*list = l;

	return 0;
}

KMOD_EXPORT const char *kmod_module_initstate_str(enum kmod_module_initstate state)
{
	switch (state) {
	case KMOD_MODULE_BUILTIN:
		return "builtin";
	case KMOD_MODULE_LIVE:
		return "live";
	case KMOD_MODULE_COMING:
		return "coming";
	case KMOD_MODULE_GOING:
		return "going";
	default:
		return NULL;
	}
}

KMOD_EXPORT int kmod_module_get_initstate(const struct kmod_module *mod)
{
	char path[PATH_MAX], buf[32];
	int fd, err, pathlen;

	if (mod == NULL)
		return -ENOENT;

	/* remove const: this can only change internal state */
	if (kmod_module_is_builtin((struct kmod_module *)mod))
		return KMOD_MODULE_BUILTIN;

	pathlen = snprintf(path, sizeof(path), "/sys/module/%s/initstate", mod->name);
	if (pathlen >= (int)sizeof(path)) {
		/* Too long path was truncated */
		return -ENAMETOOLONG;
	}
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		err = -errno;

		DBG(mod->ctx, "could not open '%s': %s\n", path, strerror(-err));

		if (pathlen > (int)sizeof("/initstate") - 1) {
			struct stat st;
			path[pathlen - (sizeof("/initstate") - 1)] = '\0';
			if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
				return KMOD_MODULE_COMING;
		}

		DBG(mod->ctx, "could not open '%s': %s\n", path, strerror(-err));
		return err;
	}

	err = read_str_safe(fd, buf, sizeof(buf));
	close(fd);
	if (err < 0) {
		ERR(mod->ctx, "could not read from '%s': %s\n", path, strerror(-err));
		return err;
	}

	if (streq(buf, "live\n"))
		return KMOD_MODULE_LIVE;
	else if (streq(buf, "coming\n"))
		return KMOD_MODULE_COMING;
	else if (streq(buf, "going\n"))
		return KMOD_MODULE_GOING;

	ERR(mod->ctx, "unknown %s: '%s'\n", path, buf);
	return -EINVAL;
}

KMOD_EXPORT long kmod_module_get_size(const struct kmod_module *mod)
{
	FILE *fp;
	char line[4096];
	int lineno = 0;
	long size = -ENOENT;
	int dfd, cfd;

	if (mod == NULL)
		return -ENOENT;

	/* try to open the module dir in /sys. If this fails, don't
	 * bother trying to find the size as we know the module isn't
	 * loaded.
	 */
	snprintf(line, sizeof(line), "/sys/module/%s", mod->name);
	dfd = open(line, O_RDONLY | O_CLOEXEC);
	if (dfd < 0)
		return -errno;

	/* available as of linux 3.3.x */
	cfd = openat(dfd, "coresize", O_RDONLY | O_CLOEXEC);
	if (cfd >= 0) {
		if (read_str_long(cfd, &size, 10) < 0)
			ERR(mod->ctx, "failed to read coresize from %s\n", line);
		close(cfd);
		goto done;
	}

	/* fall back on parsing /proc/modules */
	fp = fopen("/proc/modules", "re");
	if (fp == NULL) {
		int err = -errno;
		ERR(mod->ctx, "could not open /proc/modules: %s\n", strerror(errno));
		close(dfd);
		return err;
	}

	while (fgets(line, sizeof(line), fp)) {
		size_t len = strlen(line);
		char *saveptr, *endptr, *tok = strtok_r(line, " \t", &saveptr);
		long value;

		lineno++;
		if (tok == NULL || !streq(tok, mod->name))
			goto eat_line;

		tok = strtok_r(NULL, " \t", &saveptr);
		if (tok == NULL) {
			ERR(mod->ctx, "invalid line format at /proc/modules:%d\n", lineno);
			break;
		}

		value = strtol(tok, &endptr, 10);
		if (endptr == tok || *endptr != '\0') {
			ERR(mod->ctx, "invalid line format at /proc/modules:%d\n", lineno);
			break;
		}

		size = value;
		break;
eat_line:
		while (line[len - 1] != '\n' && fgets(line, sizeof(line), fp))
			len = strlen(line);
	}
	fclose(fp);

done:
	close(dfd);
	return size;
}

KMOD_EXPORT int kmod_module_get_refcnt(const struct kmod_module *mod)
{
	char path[PATH_MAX];
	long refcnt;
	int fd, err;

	if (mod == NULL)
		return -ENOENT;

	snprintf(path, sizeof(path), "/sys/module/%s/refcnt", mod->name);
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		err = -errno;
		DBG(mod->ctx, "could not open '%s': %s\n", path, strerror(errno));
		return err;
	}

	err = read_str_long(fd, &refcnt, 10);
	close(fd);
	if (err < 0) {
		ERR(mod->ctx, "could not read integer from '%s': '%s'\n", path,
		    strerror(-err));
		return err;
	}

	return (int)refcnt;
}

KMOD_EXPORT struct kmod_list *kmod_module_get_holders(const struct kmod_module *mod)
{
	char dname[PATH_MAX];
	struct kmod_list *list = NULL;
	struct dirent *dent;
	DIR *d;

	if (mod == NULL || mod->ctx == NULL)
		return NULL;

	snprintf(dname, sizeof(dname), "/sys/module/%s/holders", mod->name);

	d = opendir(dname);
	if (d == NULL) {
		ERR(mod->ctx, "could not open '%s': %s\n", dname, strerror(errno));
		return NULL;
	}

	for (dent = readdir(d); dent != NULL; dent = readdir(d)) {
		struct kmod_module *holder;
		struct kmod_list *l;
		int err;

		if (dent->d_name[0] == '.') {
			if (dent->d_name[1] == '\0' ||
			    (dent->d_name[1] == '.' && dent->d_name[2] == '\0'))
				continue;
		}

		err = kmod_module_new_from_name(mod->ctx, dent->d_name, &holder);
		if (err < 0) {
			ERR(mod->ctx, "could not create module for '%s': %s\n",
			    dent->d_name, strerror(-err));
			goto fail;
		}

		l = kmod_list_append(list, holder);
		if (l != NULL) {
			list = l;
		} else {
			ERR(mod->ctx, "out of memory\n");
			kmod_module_unref(holder);
			goto fail;
		}
	}

	closedir(d);
	return list;

fail:
	closedir(d);
	kmod_module_unref_list(list);
	return NULL;
}

struct kmod_module_section {
	unsigned long address;
	char name[];
};

static void kmod_module_section_free(struct kmod_module_section *section)
{
	free(section);
}

KMOD_EXPORT struct kmod_list *kmod_module_get_sections(const struct kmod_module *mod)
{
	char dname[PATH_MAX];
	struct kmod_list *list = NULL;
	struct dirent *dent;
	DIR *d;
	int dfd;

	if (mod == NULL)
		return NULL;

	snprintf(dname, sizeof(dname), "/sys/module/%s/sections", mod->name);

	d = opendir(dname);
	if (d == NULL) {
		ERR(mod->ctx, "could not open '%s': %s\n", dname, strerror(errno));
		return NULL;
	}

	dfd = dirfd(d);

	for (dent = readdir(d); dent; dent = readdir(d)) {
		struct kmod_module_section *section;
		struct kmod_list *l;
		unsigned long address;
		size_t namesz;
		int fd, err;

		if (dent->d_name[0] == '.') {
			if (dent->d_name[1] == '\0' ||
			    (dent->d_name[1] == '.' && dent->d_name[2] == '\0'))
				continue;
		}

		fd = openat(dfd, dent->d_name, O_RDONLY | O_CLOEXEC);
		if (fd < 0) {
			ERR(mod->ctx, "could not open '%s/%s': %m\n", dname, dent->d_name);
			goto fail;
		}

		err = read_str_ulong(fd, &address, 16);
		close(fd);
		if (err < 0) {
			ERR(mod->ctx, "could not read long from '%s/%s': %m\n", dname,
			    dent->d_name);
			goto fail;
		}

		namesz = strlen(dent->d_name) + 1;
		section = malloc(sizeof(*section) + namesz);

		if (section == NULL) {
			ERR(mod->ctx, "out of memory\n");
			goto fail;
		}

		section->address = address;
		memcpy(section->name, dent->d_name, namesz);

		l = kmod_list_append(list, section);
		if (l != NULL) {
			list = l;
		} else {
			ERR(mod->ctx, "out of memory\n");
			free(section);
			goto fail;
		}
	}

	closedir(d);
	return list;

fail:
	closedir(d);
	kmod_module_unref_list(list);
	return NULL;
}

KMOD_EXPORT const char *kmod_module_section_get_name(const struct kmod_list *entry)
{
	struct kmod_module_section *section;

	if (entry == NULL)
		return NULL;

	section = entry->data;
	return section->name;
}

KMOD_EXPORT unsigned long kmod_module_section_get_address(const struct kmod_list *entry)
{
	struct kmod_module_section *section;

	if (entry == NULL)
		return (unsigned long)-1;

	section = entry->data;
	return section->address;
}

KMOD_EXPORT void kmod_module_section_free_list(struct kmod_list *list)
{
	while (list) {
		kmod_module_section_free(list->data);
		list = kmod_list_remove(list);
	}
}

static struct kmod_elf *kmod_module_get_elf(const struct kmod_module *mod)
{
	if (mod->file == NULL) {
		const char *path = kmod_module_get_path(mod);

		if (path == NULL) {
			errno = ENOENT;
			return NULL;
		}

		((struct kmod_module *)mod)->file = kmod_file_open(mod->ctx, path);
		if (mod->file == NULL)
			return NULL;
	}

	return kmod_file_get_elf(mod->file);
}

struct kmod_module_info {
	char *key;
	char value[];
};

static struct kmod_module_info *kmod_module_info_new(const char *key, size_t keylen,
						     const char *value, size_t valuelen)
{
	struct kmod_module_info *info;

	info = malloc(sizeof(struct kmod_module_info) + keylen + valuelen + 2);
	if (info == NULL)
		return NULL;

	info->key = (char *)info + sizeof(struct kmod_module_info) + valuelen + 1;
	memcpy(info->key, key, keylen);
	info->key[keylen] = '\0';
	memcpy(info->value, value, valuelen);
	info->value[valuelen] = '\0';
	return info;
}

static void kmod_module_info_free(struct kmod_module_info *info)
{
	free(info);
}

static struct kmod_list *kmod_module_info_append(struct kmod_list **list, const char *key,
						 size_t keylen, const char *value,
						 size_t valuelen)
{
	struct kmod_module_info *info;
	struct kmod_list *n;

	info = kmod_module_info_new(key, keylen, value, valuelen);
	if (info == NULL)
		return NULL;
	n = kmod_list_append(*list, info);
	if (n != NULL)
		*list = n;
	else
		kmod_module_info_free(info);
	return n;
}

static char *kmod_module_hex_to_str(const char *hex, size_t len)
{
	char *str;
	int i;
	int j;
	const size_t line_limit = 20;
	size_t str_len;

	str_len = len * 3; /* XX: or XX\0 */
	str_len += ((str_len + line_limit - 1) / line_limit - 1) * 3; /* \n\t\t */

	str = malloc(str_len);
	if (str == NULL)
		return NULL;

	for (i = 0, j = 0; i < (int)len; i++) {
		j += sprintf(str + j, "%02X", (unsigned char)hex[i]);
		if (i < (int)len - 1) {
			str[j++] = ':';

			if ((i + 1) % line_limit == 0)
				j += sprintf(str + j, "\n\t\t");
		}
	}
	return str;
}

static struct kmod_list *kmod_module_info_append_hex(struct kmod_list **list,
						     const char *key, size_t keylen,
						     const char *value, size_t valuelen)
{
	char *hex;
	struct kmod_list *n;

	if (valuelen > 0) {
		/* Display as 01:12:DE:AD:BE:EF:... */
		hex = kmod_module_hex_to_str(value, valuelen);
		if (hex == NULL)
			goto list_error;
		n = kmod_module_info_append(list, key, keylen, hex, strlen(hex));
		free(hex);
		if (n == NULL)
			goto list_error;
	} else {
		n = kmod_module_info_append(list, key, keylen, NULL, 0);
		if (n == NULL)
			goto list_error;
	}

	return n;

list_error:
	return NULL;
}

KMOD_EXPORT int kmod_module_get_info(const struct kmod_module *mod,
				     struct kmod_list **list)
{
	struct kmod_elf *elf;
	char **strings;
	int i, count, ret = -ENOMEM;
	struct kmod_signature_info sig_info = {};

	if (mod == NULL || list == NULL)
		return -ENOENT;

	assert(*list == NULL);

	/* remove const: this can only change internal state */
	if (kmod_module_is_builtin((struct kmod_module *)mod)) {
		count = kmod_builtin_get_modinfo(mod->ctx, kmod_module_get_name(mod),
						 &strings);
		if (count < 0)
			return count;
	} else {
		elf = kmod_module_get_elf(mod);
		if (elf == NULL)
			return -errno;

		count = kmod_elf_get_strings(elf, ".modinfo", &strings);
		if (count < 0)
			return count;
	}

	for (i = 0; i < count; i++) {
		struct kmod_list *n;
		const char *key, *value;
		size_t keylen, valuelen;

		key = strings[i];
		value = strchr(key, '=');
		if (value == NULL) {
			keylen = strlen(key);
			valuelen = 0;
			value = key;
		} else {
			keylen = value - key;
			value++;
			valuelen = strlen(value);
		}

		n = kmod_module_info_append(list, key, keylen, value, valuelen);
		if (n == NULL)
			goto list_error;
	}

	if (mod->file && kmod_module_signature_info(mod->file, &sig_info)) {
		struct kmod_list *n;

		n = kmod_module_info_append(list, "sig_id", strlen("sig_id"),
					    sig_info.id_type, strlen(sig_info.id_type));
		if (n == NULL)
			goto list_error;
		count++;

		n = kmod_module_info_append(list, "signer", strlen("signer"),
					    sig_info.signer, sig_info.signer_len);
		if (n == NULL)
			goto list_error;
		count++;

		n = kmod_module_info_append_hex(list, "sig_key", strlen("sig_key"),
						sig_info.key_id, sig_info.key_id_len);
		if (n == NULL)
			goto list_error;
		count++;

		n = kmod_module_info_append(list, "sig_hashalgo", strlen("sig_hashalgo"),
					    sig_info.hash_algo,
					    strlen(sig_info.hash_algo));
		if (n == NULL)
			goto list_error;
		count++;

		/*
		 * Omit sig_info.algo for now, as these
		 * are currently constant.
		 */
		n = kmod_module_info_append_hex(list, "signature", strlen("signature"),
						sig_info.sig, sig_info.sig_len);

		if (n == NULL)
			goto list_error;
		count++;
	}
	ret = count;

list_error:
	/* aux structures freed in normal case also */
	kmod_module_signature_info_free(&sig_info);

	if (ret < 0) {
		kmod_module_info_free_list(*list);
		*list = NULL;
	}
	free(strings);
	return ret;
}

KMOD_EXPORT const char *kmod_module_info_get_key(const struct kmod_list *entry)
{
	struct kmod_module_info *info;

	if (entry == NULL)
		return NULL;

	info = entry->data;
	return info->key;
}

KMOD_EXPORT const char *kmod_module_info_get_value(const struct kmod_list *entry)
{
	struct kmod_module_info *info;

	if (entry == NULL)
		return NULL;

	info = entry->data;
	return info->value;
}

KMOD_EXPORT void kmod_module_info_free_list(struct kmod_list *list)
{
	while (list) {
		kmod_module_info_free(list->data);
		list = kmod_list_remove(list);
	}
}

struct kmod_module_version {
	uint64_t crc;
	char symbol[];
};

static struct kmod_module_version *kmod_module_versions_new(uint64_t crc,
							    const char *symbol)
{
	struct kmod_module_version *mv;
	size_t symbollen = strlen(symbol) + 1;

	mv = malloc(sizeof(struct kmod_module_version) + symbollen);
	if (mv == NULL)
		return NULL;

	mv->crc = crc;
	memcpy(mv->symbol, symbol, symbollen);
	return mv;
}

static void kmod_module_version_free(struct kmod_module_version *version)
{
	free(version);
}

KMOD_EXPORT int kmod_module_get_versions(const struct kmod_module *mod,
					 struct kmod_list **list)
{
	struct kmod_elf *elf;
	struct kmod_modversion *versions;
	int i, count, ret = 0;

	if (mod == NULL || list == NULL)
		return -ENOENT;

	assert(*list == NULL);

	elf = kmod_module_get_elf(mod);
	if (elf == NULL)
		return -errno;

	count = kmod_elf_get_modversions(elf, &versions);
	if (count < 0)
		return count;

	for (i = 0; i < count; i++) {
		struct kmod_module_version *mv;
		struct kmod_list *n;

		mv = kmod_module_versions_new(versions[i].crc, versions[i].symbol);
		if (mv == NULL) {
			ret = -errno;
			kmod_module_versions_free_list(*list);
			*list = NULL;
			goto list_error;
		}

		n = kmod_list_append(*list, mv);
		if (n != NULL)
			*list = n;
		else {
			kmod_module_version_free(mv);
			kmod_module_versions_free_list(*list);
			*list = NULL;
			ret = -ENOMEM;
			goto list_error;
		}
	}
	ret = count;

list_error:
	free(versions);
	return ret;
}

KMOD_EXPORT const char *kmod_module_version_get_symbol(const struct kmod_list *entry)
{
	struct kmod_module_version *version;

	if (entry == NULL || entry->data == NULL)
		return NULL;

	version = entry->data;
	return version->symbol;
}

KMOD_EXPORT uint64_t kmod_module_version_get_crc(const struct kmod_list *entry)
{
	struct kmod_module_version *version;

	if (entry == NULL || entry->data == NULL)
		return 0;

	version = entry->data;
	return version->crc;
}

KMOD_EXPORT void kmod_module_versions_free_list(struct kmod_list *list)
{
	while (list) {
		kmod_module_version_free(list->data);
		list = kmod_list_remove(list);
	}
}

struct kmod_module_symbol {
	uint64_t crc;
	char symbol[];
};

static struct kmod_module_symbol *kmod_module_symbols_new(uint64_t crc, const char *symbol)
{
	struct kmod_module_symbol *mv;
	size_t symbollen = strlen(symbol) + 1;

	mv = malloc(sizeof(struct kmod_module_symbol) + symbollen);
	if (mv == NULL)
		return NULL;

	mv->crc = crc;
	memcpy(mv->symbol, symbol, symbollen);
	return mv;
}

static void kmod_module_symbol_free(struct kmod_module_symbol *symbol)
{
	free(symbol);
}

KMOD_EXPORT int kmod_module_get_symbols(const struct kmod_module *mod,
					struct kmod_list **list)
{
	struct kmod_elf *elf;
	struct kmod_modversion *symbols;
	int i, count, ret = 0;

	if (mod == NULL || list == NULL)
		return -ENOENT;

	assert(*list == NULL);

	elf = kmod_module_get_elf(mod);
	if (elf == NULL)
		return -errno;

	count = kmod_elf_get_symbols(elf, &symbols);
	if (count < 0)
		return count;

	for (i = 0; i < count; i++) {
		struct kmod_module_symbol *mv;
		struct kmod_list *n;

		mv = kmod_module_symbols_new(symbols[i].crc, symbols[i].symbol);
		if (mv == NULL) {
			ret = -errno;
			kmod_module_symbols_free_list(*list);
			*list = NULL;
			goto list_error;
		}

		n = kmod_list_append(*list, mv);
		if (n != NULL)
			*list = n;
		else {
			kmod_module_symbol_free(mv);
			kmod_module_symbols_free_list(*list);
			*list = NULL;
			ret = -ENOMEM;
			goto list_error;
		}
	}
	ret = count;

list_error:
	free(symbols);
	return ret;
}

KMOD_EXPORT const char *kmod_module_symbol_get_symbol(const struct kmod_list *entry)
{
	struct kmod_module_symbol *symbol;

	if (entry == NULL || entry->data == NULL)
		return NULL;

	symbol = entry->data;
	return symbol->symbol;
}

KMOD_EXPORT uint64_t kmod_module_symbol_get_crc(const struct kmod_list *entry)
{
	struct kmod_module_symbol *symbol;

	if (entry == NULL || entry->data == NULL)
		return 0;

	symbol = entry->data;
	return symbol->crc;
}

KMOD_EXPORT void kmod_module_symbols_free_list(struct kmod_list *list)
{
	while (list) {
		kmod_module_symbol_free(list->data);
		list = kmod_list_remove(list);
	}
}

struct kmod_module_dependency_symbol {
	uint64_t crc;
	uint8_t bind;
	char symbol[];
};

// clang-format off
static struct kmod_module_dependency_symbol *kmod_module_dependency_symbols_new(uint64_t crc, uint8_t bind, const char *symbol)
// clang-format on
{
	struct kmod_module_dependency_symbol *mv;
	size_t symbollen = strlen(symbol) + 1;

	mv = malloc(sizeof(struct kmod_module_dependency_symbol) + symbollen);
	if (mv == NULL)
		return NULL;

	mv->crc = crc;
	mv->bind = bind;
	memcpy(mv->symbol, symbol, symbollen);
	return mv;
}

static void kmod_module_dependency_symbol_free(
	struct kmod_module_dependency_symbol *dependency_symbol)
{
	free(dependency_symbol);
}

KMOD_EXPORT int kmod_module_get_dependency_symbols(const struct kmod_module *mod,
						   struct kmod_list **list)
{
	struct kmod_elf *elf;
	struct kmod_modversion *symbols;
	int i, count, ret = 0;

	if (mod == NULL || list == NULL)
		return -ENOENT;

	assert(*list == NULL);

	elf = kmod_module_get_elf(mod);
	if (elf == NULL)
		return -errno;

	count = kmod_elf_get_dependency_symbols(elf, &symbols);
	if (count < 0)
		return count;

	for (i = 0; i < count; i++) {
		struct kmod_module_dependency_symbol *mv;
		struct kmod_list *n;

		mv = kmod_module_dependency_symbols_new(symbols[i].crc, symbols[i].bind,
							symbols[i].symbol);
		if (mv == NULL) {
			ret = -errno;
			kmod_module_dependency_symbols_free_list(*list);
			*list = NULL;
			goto list_error;
		}

		n = kmod_list_append(*list, mv);
		if (n != NULL)
			*list = n;
		else {
			kmod_module_dependency_symbol_free(mv);
			kmod_module_dependency_symbols_free_list(*list);
			*list = NULL;
			ret = -ENOMEM;
			goto list_error;
		}
	}
	ret = count;

list_error:
	free(symbols);
	return ret;
}

// clang-format off
KMOD_EXPORT const char *kmod_module_dependency_symbol_get_symbol(const struct kmod_list *entry)
// clang-format on
{
	struct kmod_module_dependency_symbol *dependency_symbol;

	if (entry == NULL || entry->data == NULL)
		return NULL;

	dependency_symbol = entry->data;
	return dependency_symbol->symbol;
}

KMOD_EXPORT uint64_t kmod_module_dependency_symbol_get_crc(const struct kmod_list *entry)
{
	struct kmod_module_dependency_symbol *dependency_symbol;

	if (entry == NULL || entry->data == NULL)
		return 0;

	dependency_symbol = entry->data;
	return dependency_symbol->crc;
}

KMOD_EXPORT int kmod_module_dependency_symbol_get_bind(const struct kmod_list *entry)
{
	struct kmod_module_dependency_symbol *dependency_symbol;

	if (entry == NULL || entry->data == NULL)
		return 0;

	dependency_symbol = entry->data;
	return dependency_symbol->bind;
}

KMOD_EXPORT void kmod_module_dependency_symbols_free_list(struct kmod_list *list)
{
	while (list) {
		kmod_module_dependency_symbol_free(list->data);
		list = kmod_list_remove(list);
	}
}
