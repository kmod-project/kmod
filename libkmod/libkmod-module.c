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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>

#include "libkmod.h"
#include "libkmod-private.h"
//#include "libkmod-index.h"

/**
 * kmod_module:
 *
 * Opaque object representing a module.
 */
struct kmod_module {
	struct kmod_ctx *ctx;
	int refcount;
	const char *path;
	const char *name;
};

static char *path_to_modname(const char *path)
{
	char *modname;
	char *c;

	modname = basename(path);
	if (modname == NULL || modname[0] == '\0')
		return NULL;

	modname = strdup(modname);
	for (c = modname; *c != '\0' && *c != '.'; c++) {
		if (*c == '-')
			*c = '_';
	}

	*c = '\0';
	return modname;
}

static const char *get_modname(struct kmod_module *mod)
{
	if (mod->name == NULL)
		mod->name = path_to_modname(mod->path);

	return mod->name;
}

KMOD_EXPORT int kmod_module_new_from_name(struct kmod_ctx *ctx,
						const char *name,
						struct kmod_module **mod)
{
	struct kmod_module *m;

	if (ctx == NULL || name == NULL)
		return -ENOENT;

	m = calloc(1, sizeof(*m));
	if (m == NULL) {
		free(m);
		return -ENOMEM;
	}

	m->ctx = kmod_ref(ctx);
	m->name = strdup(name);

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

	if (ctx == NULL || path == NULL)
		return -ENOENT;

	err = stat(path, &st);
	if (err < 0)
		return -errno;

	m = calloc(1, sizeof(*m));
	if (m == NULL) {
		free(m);
		return -ENOMEM;
	}

	m->ctx = kmod_ref(ctx);
	m->path = strdup(path);

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

	kmod_unref(mod->ctx);
	free((char *) mod->path);
	free((char *) mod->name);
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

KMOD_EXPORT int kmod_module_new_from_lookup(struct kmod_ctx *ctx,
						const char *alias,
						struct kmod_list **list)
{

	int err;

	if (ctx == NULL || alias == NULL)
		return -ENOENT;


	if (list == NULL || *list != NULL) {
		ERR(ctx, "An empty list is needed to create lookup\n");
		return -ENOSYS;
	}

	err = kmod_lookup_alias_from_config(ctx, alias, list);

	if (err < 0) {
		kmod_module_unref_list(*list);
		*list = NULL;
	}

	return err;
}

KMOD_EXPORT int kmod_module_unref_list(struct kmod_list *list)
{
	for (; list != NULL; list = kmod_list_remove(list))
		kmod_module_unref(list->data);

	return 0;
}

KMOD_EXPORT struct kmod_module *kmod_module_get_module(struct kmod_list *l)
{
	struct kmod_module *mod = l->data;
	return kmod_module_ref(mod);
}

KMOD_EXPORT const char *kmod_module_get_name(struct kmod_module *mod)
{
	// FIXME calculate name if name == NULL
	return mod->name;
}

KMOD_EXPORT const char *kmod_module_get_path(struct kmod_module *mod)
{
	// FIXME calculate path if path == NULL
	return mod->path;
}


extern long delete_module(const char *name, unsigned int flags);

KMOD_EXPORT int kmod_module_remove_module(struct kmod_module *mod,
							unsigned int flags)
{
	int err;
	const char *modname;

	if (mod == NULL)
		return -ENOENT;

	/* Filter out other flags */
	flags &= (KMOD_REMOVE_FORCE | KMOD_REMOVE_NOWAIT);

	modname = get_modname(mod);
	err = delete_module(modname, flags);
	if (err != 0) {
		ERR(mod->ctx, "Removing '%s': %s\n", modname,
							strerror(-err));
		return err;
	}

	return 0;
}

extern long init_module(void *mem, unsigned long len, const char *args);

KMOD_EXPORT int kmod_module_insert_module(struct kmod_module *mod,
							unsigned int flags)
{
	int err;
	void *mmaped_file;
	struct stat st;
	int fd;
	const char *args = "";

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

	stat(mod->path, &st);

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
