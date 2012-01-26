/*
 * Copyright (C) 2012  ProFUSION embedded systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* kmod_elf_get_section() is not exported, we need the private header */
#include <libkmod-private.h>

/* FIXME: hack, change name so we don't clash */
#undef ERR
#include "testsuite.h"
#include "stripped-module.h"

struct mod {
	struct mod *next;
	int ret;
	int errcode;
	char name[];
};

static struct mod *modules;
static bool need_init = true;

static void parse_retcodes(struct mod *_modules, const char *s)
{
	const char *p;

	if (s == NULL)
		return;

	for (p = s;;) {
		struct mod *mod;
		const char *modname;
		char *end;
		size_t modnamelen;
		int ret, errcode;
		long l;

		modname = p;
		if (modname == NULL || modname[0] == '\0')
			break;

		modnamelen = strcspn(s, ":");
		if (modname[modnamelen] != ':')
			break;

		p = modname + modnamelen + 1;
		if (p == NULL)
			break;

		l = strtol(p, &end, 0);
		if (end == p || *end != ':')
			break;
		ret = (int) l;
		p = end + 1;

		l = strtol(p, &end, 0);
		if (*end == ':')
			p = end + 1;
		else if (*end != '\0')
			break;

		errcode = (int) l;

		mod = malloc(sizeof(*mod) + modnamelen + 1);
		if (mod == NULL)
			break;

		memcpy(mod->name, modname, modnamelen);
		mod->name[modnamelen] = '\0';
		mod->ret = ret;
		mod->errcode = errcode;
		mod->next = _modules;
		_modules = mod;
	}
}

static struct mod *find_module(struct mod *_modules, const char *modname)
{
	struct mod *mod;

	for (mod = _modules; mod != NULL; mod = mod->next) {
		if (strcmp(mod->name, modname))
			return mod;
	}

	return NULL;
}

static void init_retcodes(void)
{
	const char *s;

	if (!need_init)
		return;

	need_init = false;
	s = getenv(S_TC_INIT_MODULE_RETCODES);
	if (s == NULL) {
		fprintf(stderr, "TRAP init_module(): missing export %s?\n",
						S_TC_INIT_MODULE_RETCODES);
	}

	parse_retcodes(modules, s);
}

TS_EXPORT long init_module(void *mem, unsigned long len, const char *args);

/*
 * FIXME: change /sys/module/<modname> to fake-insert a module
 *
 * Default behavior is to exit successfully. If this is not the intended
 * behavior, set TESTSUITE_INIT_MODULE_RETCODES env var.
 */
long init_module(void *mem, unsigned long len, const char *args)
{
	const char *modname;
	struct kmod_elf *elf;
	struct mod *mod;
	const void *buf;
	uint64_t bufsize;
	int err;

	init_retcodes();

	elf = kmod_elf_new(mem, len);
	if (elf == NULL)
		return 0;

	err = kmod_elf_get_section(elf, ".gnu.linkonce.this_module", &buf,
								&bufsize);
	kmod_elf_unref(elf);

	/*
	 * We couldn't find the module's name inside the ELF file. Just exit
	 * as if it was successful
	 */
	if (err < 0)
		return 0;

	modname = (char *)buf + offsetof(struct module, name);
	mod = find_module(modules, modname);
	if (mod == NULL)
		return 0;

	errno = mod->errcode;
	return mod->ret;
}

/* the test is going away anyway, but lets keep valgrind happy */
void free_resources(void) __attribute__((destructor));
void free_resources(void)
{
	while (modules) {
		struct mod *mod = modules->next;
		free(modules);
		modules = mod;
	}
}
