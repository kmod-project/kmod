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
	"/usr/local/lib/modprobe",
	"/lib/modprobe.d",
};

struct kmod_alias {
	char *name;
	char *modname;
};

const char *kmod_alias_get_name(const struct kmod_list *l) {
	struct kmod_alias *alias = l->data;
	return alias->name;
}

const char *kmod_alias_get_modname(const struct kmod_list *l) {
	struct kmod_alias *alias = l->data;
	return alias->modname;
}

static struct kmod_list *add_alias(struct kmod_ctx *ctx,
					struct kmod_list *aliases,
					const char *name, const char *modname)
{
	struct kmod_alias *alias;

	DBG(ctx, "name=%s modname=%s\n", name, modname);

	alias = malloc(sizeof(*alias));
	alias->name = strdup(name);
	alias->modname = strdup(modname);

	return kmod_list_append(aliases, alias);
}

static struct kmod_list *free_alias(struct kmod_ctx *ctx, struct kmod_list *l)
{
	struct kmod_alias *alias = l->data;

	free(alias->modname);
	free(alias->name);
	free(alias);

	return kmod_list_remove(l);
}

static struct kmod_list *add_blacklist(struct kmod_ctx *ctx,
					struct kmod_list *blacklists,
					const char *modname)
{
	struct kmod_blacklist *blacklist;
	char *p;

	DBG(ctx, "modname=%s\n", modname);

	p = strdup(modname);

	return kmod_list_append(blacklists, p);
}

static struct kmod_list *free_blacklist(struct kmod_ctx *ctx,
							struct kmod_list *l)
{
	free(l->data);
	return kmod_list_remove(l);
}


int kmod_parse_config_file(struct kmod_ctx *ctx, const char *filename,
						struct kmod_config *config)
{
	char *line;
	FILE *fp;
	unsigned int linenum;

	DBG(ctx, "%s\n", filename);

	fp = fopen(filename, "r");
	if (fp == NULL)
		return errno;

	while ((line = getline_wrapped(fp, &linenum)) != NULL) {
		char *cmd;

		if (line[0] == '\0' || line[0] == '#')
			goto done_next;

		cmd = strtok(line, "\t ");
		if (cmd == NULL)
			goto done_next;

		if (!strcmp(cmd, "alias")) {
			char *alias = strtok(NULL, "\t ");
			char *modname = strtok(NULL, "\t ");

			if (alias == NULL || modname == NULL)
				goto syntax_error;

			config->aliases = add_alias(ctx, config->aliases,
						underscores(ctx, alias),
						underscores(ctx, modname));
		} else if (!strcmp(cmd, "blacklist")) {
			char *modname = strtok(NULL, "\t ");

			if (modname == NULL)
				goto syntax_error;

			config->blacklists = add_blacklist(ctx,
						config->blacklists,
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

void kmod_free_config(struct kmod_ctx *ctx, struct kmod_config *config)
{
	while (config->aliases)
		config->aliases =  free_alias(ctx, config->aliases);

	while (config->blacklists)
		config->blacklists = free_blacklist(ctx, config->blacklists);
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

int kmod_parse_config(struct kmod_ctx *ctx, struct kmod_config *config)
{

	size_t i, n = 0;
	char **files = NULL;
	int err = 0;
	struct kmod_list *list = NULL, *l;

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
		kmod_parse_config_file(ctx, files[i], config);

finish:
	free(files);

	while (list) {
		free(list->data);
		list = kmod_list_remove(list);
	}

	return err;
}
