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
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "libkmod.h"
#include "libkmod-private.h"

static const char *config_files[] = {
	"/run/modprobe.d",
	"/etc/modprobe.d",
	"/lib/modprobe.d",
};

struct kmod_alias {
	char *name;
	char modname[];
};

const char *kmod_alias_get_name(const struct kmod_list *l) {
	const struct kmod_alias *alias = l->data;
	return alias->name;
}

const char *kmod_alias_get_modname(const struct kmod_list *l) {
	const struct kmod_alias *alias = l->data;
	return alias->modname;
}

static int kmod_config_add_alias(struct kmod_config *config,
				const char *name, const char *modname)
{
	struct kmod_alias *alias;
	struct kmod_list *list;
	size_t namelen = strlen(name) + 1, modnamelen = strlen(modname) + 1;

	DBG(config->ctx, "name=%s modname=%s\n", name, modname);

	alias = malloc(sizeof(*alias) + namelen + modnamelen);
	if (!alias)
		goto oom_error_init;
	alias->name = sizeof(*alias) + modnamelen + (char *)alias;

	memcpy(alias->modname, modname, modnamelen);
	memcpy(alias->name, name, namelen);

	list = kmod_list_append(config->aliases, alias);
	if (!list)
		goto oom_error;
	config->aliases = list;
	return 0;

oom_error:
	free(alias);
oom_error_init:
	ERR(config->ctx, "out-of-memory name=%s modname=%s\n", name, modname);
	return -ENOMEM;
}

static void kmod_config_free_alias(struct kmod_config *config, struct kmod_list *l)
{
	struct kmod_alias *alias = l->data;

	free(alias);

	config->aliases = kmod_list_remove(l);
}

static int kmod_config_add_blacklist(struct kmod_config *config,
					const char *modname)
{
	char *p;
	struct kmod_list *list;

	DBG(config->ctx, "modname=%s\n", modname);

	p = strdup(modname);
	if (!p)
		goto oom_error_init;

	list = kmod_list_append(config->blacklists, p);
	if (!list)
		goto oom_error;
	config->blacklists = list;
	return 0;

oom_error:
	free(p);
oom_error_init:
	ERR(config->ctx, "out-of-memory modname=%s\n", modname);
	return -ENOMEM;
}

static void kmod_config_free_blacklist(struct kmod_config *config,
							struct kmod_list *l)
{
	free(l->data);
	config->blacklists = kmod_list_remove(l);
}

static int kmod_config_parse(struct kmod_config *config, const char *filename)
{
	struct kmod_ctx *ctx = config->ctx;
	char *line;
	FILE *fp;
	unsigned int linenum;

	DBG(ctx, "%s\n", filename);

	fp = fopen(filename, "r");
	if (fp == NULL)
		return errno;

	while ((line = getline_wrapped(fp, &linenum)) != NULL) {
		char *cmd, *saveptr;

		if (line[0] == '\0' || line[0] == '#')
			goto done_next;

		cmd = strtok_r(line, "\t ", &saveptr);
		if (cmd == NULL)
			goto done_next;

		if (!strcmp(cmd, "alias")) {
			char *alias = strtok_r(NULL, "\t ", &saveptr);
			char *modname = strtok_r(NULL, "\t ", &saveptr);

			if (alias == NULL || modname == NULL)
				goto syntax_error;

			kmod_config_add_alias(config,
						underscores(ctx, alias),
						underscores(ctx, modname));
		} else if (!strcmp(cmd, "blacklist")) {
			char *modname = strtok_r(NULL, "\t ", &saveptr);

			if (modname == NULL)
				goto syntax_error;

			kmod_config_add_blacklist(config,
						underscores(ctx, modname));
		} else if (!strcmp(cmd, "include") || !strcmp(cmd, "options")
				|| !strcmp(cmd, "install")
				|| !strcmp(cmd, "remove")
				|| !strcmp(cmd, "softdep")
				|| !strcmp(cmd, "config")) {
			INFO(ctx, "%s: command %s not implemented yet\n",
								filename, cmd);
		} else {
syntax_error:
			ERR(ctx, "%s line %u: ignoring bad line starting with '%s'\n",
						filename, linenum, cmd);
		}

done_next:
		free(line);
	}

	fclose(fp);

	return 0;
}

void kmod_config_free(struct kmod_config *config)
{
	while (config->aliases)
		kmod_config_free_alias(config, config->aliases);

	while (config->blacklists)
		kmod_config_free_blacklist(config, config->blacklists);

	free(config);
}

static bool conf_files_filter(struct kmod_ctx *ctx, const char *path,
							const char *fn)
{
	size_t len = strlen(fn);

	if (fn[0] == '.')
		return 1;

	if (len < 6 || (strcmp(&fn[len - 5], ".conf") != 0
				&& strcmp(&fn[len - 6], ".alias"))) {
		INFO(ctx, "All config files need .conf: %s/%s, "
				"it will be ignored in a future release\n",
				path, fn);
		return 1;
	}

	return 0;
}

static int conf_files_list(struct kmod_ctx *ctx, struct kmod_list **list,
						const char *path, size_t *n)
{
	struct stat st;
	DIR *d;
	int err;

	if (stat(path, &st) < 0)
		return -ENOENT;

	if (!S_ISDIR(st.st_mode)) {
		*list = kmod_list_append(*list, (void *)path);
		*n += 1;
		return 0;
	}

	d = opendir(path);
	if (d == NULL) {
		err = errno;
		ERR(ctx, "%m\n");
		return -errno;
	}

	for (;;) {
		struct dirent ent, *entp;
		char *p;

		err = readdir_r(d, &ent, &entp);
		if (err != 0) {
			err = -err;
			goto finish;
		}

		if (entp == NULL)
			break;

		if (conf_files_filter(ctx, path, entp->d_name) == 1)
			continue;

		if (asprintf(&p, "%s/%s", path, entp->d_name) < 0) {
			err = -ENOMEM;
			goto finish;
		}

		DBG(ctx, "%s\n", p);

		*list = kmod_list_append(*list, p);
		*n += 1;
	}

finish:
	closedir(d);
	return err;
}

static int base_cmp(const void *a, const void *b)
{
        const char *s1, *s2;

        s1 = *(char * const *)a;
        s2 = *(char * const *)b;

	return strcmp(basename(s1), basename(s2));
}

int kmod_config_new(struct kmod_ctx *ctx, struct kmod_config **p_config)
{
	struct kmod_config *config;
	size_t i, n = 0;
	const char **files;
	int err = 0;
	struct kmod_list *list = NULL, *l;

	*p_config = config = calloc(1, sizeof(struct kmod_config));
	if (config == NULL)
		return -ENOMEM;

	config->ctx = ctx;

	for (i = 0; i < ARRAY_SIZE(config_files); i++)
		conf_files_list(ctx, &list, config_files[i], &n);

	files = malloc(sizeof(char *) * n);
	if (files == NULL) {
		err = -ENOMEM;
		goto finish;
	}

	i = 0;
	kmod_list_foreach(l, list) {
		files[i] = l->data;
		i++;
	}

	qsort(files, n, sizeof(char *), base_cmp);

	for (i = 0; i < n; i++)
		kmod_config_parse(config, files[i]);

finish:
	free(files);

	while (list) {
		free(list->data);
		list = kmod_list_remove(list);
	}

	return err;
}
