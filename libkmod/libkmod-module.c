/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011  ProFUSION embedded systems
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>

#include "libkmod.h"
#include "libkmod-private.h"

/**
 * kmod_module:
 *
 * Opaque object representing a module.
 */
struct kmod_module {
	struct kmod_ctx *ctx;
	char *path;
	struct kmod_list *dep;
	char *options;
	char *install_commands;
	char *remove_commands;
	int n_dep;
	int refcount;
	struct {
		bool dep : 1;
		bool options : 1;
		bool install_commands : 1;
		bool remove_commands : 1;
	} init;
	char name[];
};

inline char *modname_normalize(const char *modname, char buf[NAME_MAX],
								size_t *len)
{
	size_t s;

	for (s = 0; s < NAME_MAX - 1; s++) {
		const char c = modname[s];
		if (c == '-')
			buf[s] = '_';
		else if (c == '\0' || c == '.')
			break;
		else
			buf[s] = c;
	}

	buf[s] = '\0';

	if (len)
		*len = s;

	return buf;
}

static char *path_to_modname(const char *path, char buf[NAME_MAX], size_t *len)
{
	char *modname;

	modname = basename(path);
	if (modname == NULL || modname[0] == '\0')
		return NULL;

	return modname_normalize(modname, buf, len);
}

static inline const char *path_join(const char *path, size_t prefixlen,
							char buf[PATH_MAX])
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
		struct kmod_module *depmod;
		const char *path;

		path = path_join(p, dirnamelen, buf);
		if (path == NULL) {
			ERR(ctx, "could not join path '%s' and '%s'.\n",
			    dirname, p);
			goto fail;
		}

		err = kmod_module_new_from_path(ctx, path, &depmod);
		if (err < 0) {
			ERR(ctx, "ctx=%p path=%s error=%s\n",
						ctx, path, strerror(-err));
			goto fail;
		}

		DBG(ctx, "add dep: %s\n", path);

		list = kmod_list_append(list, depmod);
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

KMOD_EXPORT int kmod_module_new_from_name(struct kmod_ctx *ctx,
						const char *name,
						struct kmod_module **mod)
{
	struct kmod_module *m;
	size_t namelen;
	char name_norm[NAME_MAX];

	if (ctx == NULL || name == NULL)
		return -ENOENT;

	modname_normalize(name, name_norm, &namelen);

	m = kmod_pool_get_module(ctx, name_norm);
	if (m != NULL) {
		*mod = kmod_module_ref(m);
		return 0;
	}

	m = calloc(1, sizeof(*m) + namelen + 1);
	if (m == NULL) {
		free(m);
		return -ENOMEM;
	}

	m->ctx = kmod_ref(ctx);
	memcpy(m->name, name_norm, namelen + 1);
	m->refcount = 1;

	kmod_pool_add_module(ctx, m);

	*mod = m;

	return 0;
}

KMOD_EXPORT int kmod_module_new_from_path(struct kmod_ctx *ctx,
						const char *path,
						struct kmod_module **mod)
{
	struct kmod_module *m;
	int err;
	struct stat st;
	char name[NAME_MAX];
	char *abspath;
	size_t namelen;

	if (ctx == NULL || path == NULL)
		return -ENOENT;

	abspath = path_make_absolute_cwd(path);
	if (abspath == NULL)
		return -ENOMEM;

	err = stat(abspath, &st);
	if (err < 0) {
		free(abspath);
		return -errno;
	}

	path_to_modname(path, name, &namelen);

	m = kmod_pool_get_module(ctx, name);
	if (m != NULL) {
		if (m->path == NULL)
			m->path = abspath;
		else if (streq(m->path, abspath))
			free(abspath);
		else {
			ERR(ctx, "kmod_module '%s' already exists with different path\n",
									name);
			free(abspath);
			return -EEXIST;
		}

		*mod = kmod_module_ref(m);
		return 0;
	}

	m = calloc(1, sizeof(*m) + namelen + 1);
	if (m == NULL)
		return -errno;

	m->ctx = kmod_ref(ctx);
	memcpy(m->name, name, namelen);
	m->path = abspath;
	m->refcount = 1;

	kmod_pool_add_module(ctx, m);

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

	kmod_module_unref_list(mod->dep);
	kmod_unref(mod->ctx);
	free(mod->options);
	free(mod->install_commands);
	free(mod->remove_commands);
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

#define CHECK_ERR_AND_FINISH(_err, _label_err, _list, label_finish)	\
	do {								\
		if ((_err) < 0)						\
			goto _label_err;				\
		if (*(_list) != NULL)					\
			goto finish;					\
	} while (0)

KMOD_EXPORT int kmod_module_new_from_lookup(struct kmod_ctx *ctx,
						const char *given_alias,
						struct kmod_list **list)
{
	int err;
	char alias[NAME_MAX];

	if (ctx == NULL || alias == NULL)
		return -ENOENT;

	if (list == NULL || *list != NULL) {
		ERR(ctx, "An empty list is needed to create lookup\n");
		return -ENOSYS;
	}

	modname_normalize(given_alias, alias, NULL);

	/* Aliases from config file override all the others */
	err = kmod_lookup_alias_from_config(ctx, alias, list);
	CHECK_ERR_AND_FINISH(err, fail, list, finish);

	err = kmod_lookup_alias_from_moddep_file(ctx, alias, list);
	CHECK_ERR_AND_FINISH(err, fail, list, finish);

	err = kmod_lookup_alias_from_symbols_file(ctx, alias, list);
	CHECK_ERR_AND_FINISH(err, fail, list, finish);

	err = kmod_lookup_alias_from_aliases_file(ctx, alias, list);
	CHECK_ERR_AND_FINISH(err, fail, list, finish);

finish:

	return err;
fail:
	kmod_module_unref_list(*list);
	*list = NULL;
	return err;
}
#undef CHECK_ERR_AND_FINISH


KMOD_EXPORT int kmod_module_unref_list(struct kmod_list *list)
{
	for (; list != NULL; list = kmod_list_remove(list))
		kmod_module_unref(list->data);

	return 0;
}

KMOD_EXPORT struct kmod_list *kmod_module_get_dependencies(const struct kmod_module *mod)
{
	struct kmod_list *l, *l_new, *list_new = NULL;

	if (mod == NULL)
		return NULL;

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

KMOD_EXPORT long kmod_module_get_size(const struct kmod_module *mod)
{
	// FIXME TODO: this should be available from /sys/module/foo
	FILE *fp;
	char line[4096];
	int lineno = 0;
	long size = -ENOENT;

	if (mod == NULL)
		return -ENOENT;

	fp = fopen("/proc/modules", "r");
	if (fp == NULL) {
		int err = -errno;
		ERR(mod->ctx,
		    "could not open /proc/modules: %s\n", strerror(errno));
		return err;
	}

	while (fgets(line, sizeof(line), fp)) {
		char *saveptr, *endptr, *tok = strtok_r(line, " \t", &saveptr);
		long value;

		lineno++;
		if (tok == NULL || !streq(tok, mod->name))
			continue;

		tok = strtok_r(NULL, " \t", &saveptr);
		if (tok == NULL) {
			ERR(mod->ctx,
			"invalid line format at /proc/modules:%d\n", lineno);
			break;
		}

		value = strtol(tok, &endptr, 10);
		if (endptr == tok || *endptr != '\0') {
			ERR(mod->ctx,
			"invalid line format at /proc/modules:%d\n", lineno);
			break;
		}

		size = value;
		break;
	}
	fclose(fp);
	return size;
}

KMOD_EXPORT const char *kmod_module_get_name(const struct kmod_module *mod)
{
	return mod->name;
}

KMOD_EXPORT const char *kmod_module_get_path(const struct kmod_module *mod)
{
	char *line;

	DBG(mod->ctx, "name='%s' path='%s'\n", mod->name, mod->path);

	if (mod->path != NULL)
		return mod->path;
	if (mod->init.dep)
		return NULL;

	/* lazy init */
	line = kmod_search_moddep(mod->ctx, mod->name);
	if (line == NULL)
		return NULL;

	kmod_module_parse_depline((struct kmod_module *) mod, line);
	free(line);

	return mod->path;
}


extern long delete_module(const char *name, unsigned int flags);

KMOD_EXPORT int kmod_module_remove_module(struct kmod_module *mod,
							unsigned int flags)
{
	int err;

	if (mod == NULL)
		return -ENOENT;

	/* Filter out other flags */
	flags &= (KMOD_REMOVE_FORCE | KMOD_REMOVE_NOWAIT);

	err = delete_module(mod->name, flags);
	if (err != 0) {
		ERR(mod->ctx, "Removing '%s': %s\n", mod->name,
							strerror(-err));
		return err;
	}

	return 0;
}

extern long init_module(void *mem, unsigned long len, const char *args);

KMOD_EXPORT int kmod_module_insert_module(struct kmod_module *mod,
							unsigned int flags,
							const char *options)
{
	int err;
	void *mmaped_file;
	struct stat st;
	int fd;
	const char *args = options ? options : "";

	if (mod == NULL)
		return -ENOENT;

	if (mod->path == NULL) {
		ERR(mod->ctx, "Not supported to load a module by name yet\n");
		return -ENOSYS;
	}

	if (flags != 0)
		INFO(mod->ctx, "Flags are not implemented yet\n");

	if ((fd = open(mod->path, O_RDONLY)) < 0) {
		err = -errno;
		return err;
	}

	fstat(fd, &st);

	if ((mmaped_file = mmap(0, st.st_size, PROT_READ,
					MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
		close(fd);
		return -errno;
	}

	err = init_module(mmaped_file, st.st_size, args);
	if (err < 0)
		ERR(mod->ctx, "Failed to insert module '%s'\n", mod->path);

	munmap(mmaped_file, st.st_size);
	close(fd);

	return err;
}

KMOD_EXPORT const char *kmod_module_get_options(const struct kmod_module *mod)
{
	if (mod == NULL)
		return NULL;

	if (!mod->init.options) {
		/* lazy init */
		struct kmod_module *m = (struct kmod_module *)mod;
		const struct kmod_list *l, *ctx_options;
		char *opts = NULL;
		size_t optslen = 0;

		ctx_options = kmod_get_options(mod->ctx);

		kmod_list_foreach(l, ctx_options) {
			const char *modname = kmod_option_get_modname(l);
			const char *str;
			size_t len;
			void *tmp;

			if (strcmp(modname, mod->name) != 0)
				continue;

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
		const struct kmod_list *l, *ctx_install_commands;
		char *cmds = NULL;
		size_t cmdslen = 0;

		ctx_install_commands = kmod_get_install_commands(mod->ctx);

		kmod_list_foreach(l, ctx_install_commands) {
			const char *modname = kmod_command_get_modname(l);
			const char *str;
			size_t len;
			void *tmp;

			if (strcmp(modname, mod->name) != 0)
				continue;

			str = kmod_command_get_command(l);
			len = strlen(str);
			if (len < 1)
				continue;

			tmp = realloc(cmds, cmdslen + len + 2);
			if (tmp == NULL) {
				free(cmds);
				goto failed;
			}

			cmds = tmp;

			if (cmdslen > 0) {
				cmds[cmdslen] = ';';
				cmdslen++;
			}

			memcpy(cmds + cmdslen, str, len);
			cmdslen += len;
			cmds[cmdslen] = '\0';
		}

		m->init.install_commands = true;
		m->install_commands = cmds;
	}

	return mod->install_commands;

failed:
	ERR(mod->ctx, "out of memory\n");
	return NULL;
}

KMOD_EXPORT const char *kmod_module_get_remove_commands(const struct kmod_module *mod)
{
	if (mod == NULL)
		return NULL;

	if (!mod->init.remove_commands) {
		/* lazy init */
		struct kmod_module *m = (struct kmod_module *)mod;
		const struct kmod_list *l, *ctx_remove_commands;
		char *cmds = NULL;
		size_t cmdslen = 0;

		ctx_remove_commands = kmod_get_remove_commands(mod->ctx);

		kmod_list_foreach(l, ctx_remove_commands) {
			const char *modname = kmod_command_get_modname(l);
			const char *str;
			size_t len;
			void *tmp;

			if (strcmp(modname, mod->name) != 0)
				continue;

			str = kmod_command_get_command(l);
			len = strlen(str);
			if (len < 1)
				continue;

			tmp = realloc(cmds, cmdslen + len + 2);
			if (tmp == NULL) {
				free(cmds);
				goto failed;
			}

			cmds = tmp;

			if (cmdslen > 0) {
				cmds[cmdslen] = ';';
				cmdslen++;
			}

			memcpy(cmds + cmdslen, str, len);
			cmdslen += len;
			cmds[cmdslen] = '\0';
		}

		m->init.remove_commands = true;
		m->remove_commands = cmds;
	}

	return mod->remove_commands;

failed:
	ERR(mod->ctx, "out of memory\n");
	return NULL;
}

/**
 * SECTION:libkmod-loaded
 * @short_description: currently loaded modules
 *
 * Information about currently loaded modules, as reported by Linux kernel.
 * These information are not cached by libkmod and are always read from /sys
 * and /proc/modules.
 */

/**
 * kmod_module_new_from_loaded:
 * @ctx: kmod library context
 * @list: where to save the list of loaded modules
 *
 * Get a list of all modules currently loaded in kernel. It uses /proc/modules
 * to get the names of loaded modules and to create kmod_module objects by
 * calling kmod_module_new_from_name() in each of them. They are put are put
 * in @list in no particular order.
 *
 * All the returned modules get their refcount incremented (or are created if
 * they do not exist yet). After using the list, release the resources by
 * calling kmod_module_unref_list().
 *
 * Returns: 0 on success or < 0 on error.
 */
KMOD_EXPORT int kmod_module_new_from_loaded(struct kmod_ctx *ctx,
						struct kmod_list **list)
{
	struct kmod_list *l = NULL;
	FILE *fp;
	char line[4096];

	if (ctx == NULL || list == NULL)
		return -ENOENT;

	fp = fopen("/proc/modules", "r");
	if (fp == NULL) {
		int err = -errno;
		ERR(ctx, "could not open /proc/modules: %s\n", strerror(errno));
		return err;
	}

	while (fgets(line, sizeof(line), fp)) {
		struct kmod_module *m;
		struct kmod_list *node;
		int err;
		char *saveptr, *name = strtok_r(line, " \t", &saveptr);

		err = kmod_module_new_from_name(ctx, name, &m);
		if (err < 0) {
			ERR(ctx, "could not get module from name '%s': %s\n",
				name, strerror(-err));
			continue;
		}

		node = kmod_list_append(l, m);
		if (node)
			l = node;
		else {
			ERR(ctx, "out of memory\n");
			kmod_module_unref(m);
		}
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

	pathlen = snprintf(path, sizeof(path),
				"/sys/module/%s/initstate", mod->name);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		err = -errno;

		if (pathlen > (int)sizeof("/initstate") - 1) {
			struct stat st;
			path[pathlen - (sizeof("/initstate") - 1)] = '\0';
			if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
				return KMOD_MODULE_BUILTIN;
		}

		DBG(mod->ctx, "could not open '%s': %s\n",
			path, strerror(-err));
		return err;
	}

	err = read_str_safe(fd, buf, sizeof(buf));
	close(fd);
	if (err < 0) {
		ERR(mod->ctx, "could not read from '%s': %s\n",
			path, strerror(-err));
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

KMOD_EXPORT int kmod_module_get_refcnt(const struct kmod_module *mod)
{
	char path[PATH_MAX];
	long refcnt;
	int fd, err;

	snprintf(path, sizeof(path), "/sys/module/%s/refcnt", mod->name);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		err = -errno;
		ERR(mod->ctx, "could not open '%s': %s\n",
			path, strerror(errno));
		return err;
	}

	err = read_str_long(fd, &refcnt, 10);
	close(fd);
	if (err < 0) {
		ERR(mod->ctx, "could not read integer from '%s': '%s'\n",
			path, strerror(-err));
		return err;
	}

	return (int)refcnt;
}

KMOD_EXPORT struct kmod_list *kmod_module_get_holders(const struct kmod_module *mod)
{
	char dname[PATH_MAX];
	struct kmod_list *list = NULL;
	DIR *d;

	if (mod == NULL)
		return NULL;
	snprintf(dname, sizeof(dname), "/sys/module/%s/holders", mod->name);

	d = opendir(dname);
	if (d == NULL) {
		ERR(mod->ctx, "could not open '%s': %s\n",
						dname, strerror(errno));
		return NULL;
	}

	for (;;) {
		struct dirent de, *entp;
		struct kmod_module *holder;
		struct kmod_list *l;
		int err;

		err = readdir_r(d, &de, &entp);
		if (err != 0) {
			ERR(mod->ctx, "could not iterate for module '%s': %s\n",
						mod->name, strerror(-err));
			goto fail;
		}

		if (entp == NULL)
			break;

		if (de.d_name[0] == '.') {
			if (de.d_name[1] == '\0' ||
			    (de.d_name[1] == '.' && de.d_name[2] == '\0'))
				continue;
		}

		err = kmod_module_new_from_name(mod->ctx, de.d_name, &holder);
		if (err < 0) {
			ERR(mod->ctx, "could not create module for '%s': %s\n",
				de.d_name, strerror(-err));
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
	DIR *d;
	int dfd;

	if (mod == NULL)
		return NULL;

	snprintf(dname, sizeof(dname), "/sys/module/%s/sections", mod->name);

	d = opendir(dname);
	if (d == NULL) {
		ERR(mod->ctx, "could not open '%s': %s\n",
			dname, strerror(errno));
		return NULL;
	}

	dfd = dirfd(d);

	for (;;) {
		struct dirent de, *entp;
		struct kmod_module_section *section;
		struct kmod_list *l;
		unsigned long address;
		size_t namesz;
		int fd, err;

		err = readdir_r(d, &de, &entp);
		if (err != 0) {
			ERR(mod->ctx, "could not iterate for module '%s': %s\n",
						mod->name, strerror(-err));
			goto fail;
		}

		if (de.d_name[0] == '.') {
			if (de.d_name[1] == '\0' ||
			    (de.d_name[1] == '.' && de.d_name[2] == '\0'))
				continue;
		}

		fd = openat(dfd, de.d_name, O_RDONLY);
		if (fd < 0) {
			ERR(mod->ctx, "could not open '%s/%s': %m\n",
							dname, de.d_name);
			goto fail;
		}

		err = read_str_ulong(fd, &address, 16);
		close(fd);
		if (err < 0) {
			ERR(mod->ctx, "could not read long from '%s/%s': %m\n",
							dname, de.d_name);
			goto fail;
		}

		namesz = strlen(de.d_name) + 1;
		section = malloc(sizeof(*section) + namesz);

		if (section == NULL) {
			ERR(mod->ctx, "out of memory\n");
			goto fail;
		}

		section->address = address;
		memcpy(section->name, de.d_name, namesz);

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
