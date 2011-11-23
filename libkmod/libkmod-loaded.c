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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "libkmod.h"
#include "libkmod-private.h"

/**
 * SECTION:libkmod-loaded
 * @short_description: currently loaded modules
 *
 * Information about currently loaded modules, as reported by Linux kernel
 */

/**
 * kmod_loaded:
 *
 * Opaque object representing a loaded module.
 */
struct kmod_loaded {
	struct kmod_ctx *ctx;
	int refcount;
	struct kmod_list *modules;
	bool parsed;
};

struct kmod_loaded_module {
	char *name;
	long size;
	int use_count;
	char *deps;
	uintptr_t addr;
};

KMOD_EXPORT int kmod_loaded_new(struct kmod_ctx *ctx, struct kmod_loaded **mod)
{
	struct kmod_loaded *m;

	m = calloc(1, sizeof(*m));
	if (m == NULL)
		return -ENOMEM;

	m->refcount = 1;
	m->ctx = ctx;
	*mod = m;
	return 0;
}

KMOD_EXPORT struct kmod_loaded *kmod_loaded_ref(struct kmod_loaded *mod)
{
	if (mod == NULL)
		return NULL;
	mod->refcount++;
	return mod;
}

static void loaded_modules_free_module(struct kmod_loaded_module *m)
{
	free(m->name);
	free(m->deps);
	free(m);
}

static void loaded_modules_free(struct kmod_loaded *mod)
{
	while (mod->modules != NULL) {
		loaded_modules_free_module(mod->modules->data);
		mod->modules = kmod_list_remove(mod->modules);
	}
}

KMOD_EXPORT struct kmod_loaded *kmod_loaded_unref(struct kmod_loaded *mod)
{
	if (mod == NULL)
		return NULL;
	mod->refcount--;
	dbg(mod->ctx, "kmod_loaded %p released\n", mod);
	loaded_modules_free(mod);
	free(mod);
	return NULL;
}

static int loaded_modules_parse(struct kmod_loaded *mod,
						struct kmod_list **list)
{
	struct kmod_list *l = NULL;
	FILE *fp;
	char line[4096];

	fp = fopen("/proc/modules", "r");
	if (fp == NULL)
		return -errno;

	while (fgets(line, sizeof(line), fp)) {
		char *tok;
		struct kmod_loaded_module *m;

		m = calloc(1, sizeof(*m));
		if (m == NULL)
			goto err;

		tok = strtok(line, " \t");
		m->name = strdup(tok);

		tok = strtok(NULL, " \t\n");
		m->size = atoi(tok);

		/* Null if no module unloading is supported */
		tok = strtok(NULL, " \t\n");
		if (tok == NULL)
			goto done;

		m->use_count = atoi(tok);
		tok = strtok(NULL, "\n");
		if (tok == NULL)
			goto done;

		/* Strip trailing comma */
		if (strchr(tok, ',')) {
			char *end;
			tok = strtok(tok, " \t");
			end = &tok[strlen(tok) - 1];
			if (*end == ',')
				*end = '\0';
			m->deps = strdup(tok);
			tok = &end[2];
		} else if (tok[0] == '-' && tok[1] == '\0')
			goto done;
		else if (tok[0] == '-' && isspace(tok[1]))
			tok = &tok[3];

		tok = strtok(tok, " \t\n");
		if (tok == NULL)
			goto done;

		tok = strtok(NULL, " \t\n");
		if (tok == NULL)
			goto done;

		sscanf(tok, "%" SCNxPTR, &m->addr);

done:
		l = kmod_list_append(l, m);
	}

	fclose(fp);
	mod->parsed = 1;
	*list = l;

	return 0;

err:
	fclose(fp);
	mod->modules = l;
	loaded_modules_free(mod);
	mod->modules = NULL;
	return -ENOMEM;
}

KMOD_EXPORT int kmod_loaded_get_list(struct kmod_loaded *mod,
						struct kmod_list **list)
{
	if (mod == NULL)
		return -ENOENT;

	if (!mod->parsed) {
		int err = loaded_modules_parse(mod, &mod->modules);
		if (err < 0)
			return err;
	}

	*list = mod->modules;

	return 0;
}

KMOD_EXPORT int kmod_loaded_get_module_info(const struct kmod_list *entry,
						const char **name,
						long *size, int *use_count,
						const char **deps,
						uintptr_t *addr)
{
	struct kmod_loaded_module *m;

	if (entry == NULL)
		return -ENOENT;

	m = entry->data;

	if (name)
		*name = m->name;
	if (size)
		*size = m->size;
	if (use_count)
		*use_count = m->use_count;
	if (addr)
		*addr = m->addr;
	if (deps)
		*deps = m->deps;

	return 0;
}

extern long delete_module(const char *name, unsigned int flags);

KMOD_EXPORT int kmod_loaded_remove_module(struct kmod_loaded *mod,
						struct kmod_list *entry,
						unsigned int flags)
{
	struct kmod_loaded_module *m;
	int err;

	if (mod == NULL)
		return -ENOSYS;

	if (entry == NULL)
		return -ENOENT;

	m = entry->data;

	/* Filter out other flags */
	flags &= (KMOD_REMOVE_FORCE | KMOD_REMOVE_NOWAIT);

	err = delete_module(m->name, flags);
	if (err != 0) {
		err(mod->ctx, "Removing '%s': %s\n", m->name,
							strerror(-err));
		return err;
	}

	loaded_modules_free_module(m);
	kmod_list_remove(entry);

	return 0;
}
