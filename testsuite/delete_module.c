// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <shared/util.h>

#include "testsuite.h"

struct mod {
	struct mod *next;
	int ret;
	int errcode;
	char name[];
};

static struct mod *modules;
static bool need_init = true;

static void parse_retcodes(struct mod **_modules, const char *s)
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

		modnamelen = strcspn(p, ":");
		if (modname[modnamelen] != ':')
			break;

		p = modname + modnamelen + 1;
		if (p == NULL)
			break;

		l = strtol(p, &end, 0);
		if (end == p || *end != ':')
			break;

		ret = (int)l;
		p = end + 1;

		l = strtol(p, &end, 0);
		if (*end == ':')
			p = end + 1;
		else if (*end != '\0')
			break;

		errcode = (int)l;

		mod = malloc(sizeof(*mod) + modnamelen + 1);
		if (mod == NULL)
			break;

		memcpy(mod->name, modname, modnamelen);
		mod->name[modnamelen] = '\0';
		mod->ret = ret;
		mod->errcode = errcode;
		mod->next = *_modules;
		*_modules = mod;
	}
}

static struct mod *find_module(struct mod *_modules, const char *modname)
{
	struct mod *mod;

	for (mod = _modules; mod != NULL; mod = mod->next) {
		if (streq(mod->name, modname))
			return mod;
	}

	return NULL;
}

static void init_retcodes(void)
{
	const char *s;
	struct mod *mod;

	if (!need_init)
		return;

	need_init = false;
	s = getenv(S_TC_DELETE_MODULE_RETCODES);
	if (s == NULL) {
		ERR("TRAP delete_module(): missing export %s?\n",
		    S_TC_DELETE_MODULE_RETCODES);
	}

	parse_retcodes(&modules, s);

	for (mod = modules; mod != NULL; mod = mod->next) {
		LOG("Added module to test delete_module:\n");
		LOG("\tname=%s ret=%d errcode=%d\n", mod->name, mod->ret, mod->errcode);
	}
}

TS_EXPORT long delete_module(const char *name, unsigned int flags);

/*
 * FIXME: change /sys/module/<modname> to fake-remove a module
 *
 * Default behavior is to exit successfully. If this is not the intended
 * behavior, set TESTSUITE_DELETE_MODULE_RETCODES env var.
 */

static int remove_directory(const char *path)
{
	struct stat st;
	DIR *dir;
	struct dirent *entry;
	char full_path[PATH_MAX];

	if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
		LOG("Directory %s not found, skip remove.\n", path);
		return 0;
	}

	dir = opendir(path);
	if (!dir) {
		ERR("Failed to open directory %s: %s (errno: %d)\n", path,
		    strerror(errno), errno);
		return -1;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

		if (entry->d_type == DT_DIR) {
			if (remove_directory(full_path) != 0) {
				closedir(dir);
				ERR("Failed to remove directory %s: %s (errno: %d)\n",
				    full_path, strerror(errno), errno);
				return -1;
			}
		} else {
			if (remove(full_path) != 0) {
				closedir(dir);
				ERR("Failed to remove file %s: %s (errno: %d)\n",
				    full_path, strerror(errno), errno);
				return -1;
			}
		}
	}

	closedir(dir);
	if (rmdir(path) != 0) {
		ERR("Failed to remove directory %s: %s (errno: %d)\n", path,
		    strerror(errno), errno);
		return -1;
	}
	return 0;
}

long delete_module(const char *modname, unsigned int flags)
{
	struct mod *mod;
	int ret = 0;
	char buf[PATH_MAX];
	const char *sysfsmod = "/sys/module/";
	int len = strlen(sysfsmod);

	init_retcodes();
	mod = find_module(modules, modname);
	if (mod == NULL)
		return 0;

	memcpy(buf, sysfsmod, len);
	strcpy(buf + len, modname);
	ret = remove_directory(buf);
	if (ret != 0)
		return ret;

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
