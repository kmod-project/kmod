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

struct kmod_options {
	char *options;
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

static int kmod_config_add_options(struct kmod_config *config,
				const char *modname, const char *options)
{
	struct kmod_options *opt;
	struct kmod_list *list;
	size_t modnamelen = strlen(modname) + 1;
	size_t optionslen = strlen(options) + 1;

	DBG(config->ctx, "modname'%s' options='%s'\n", modname, options);

	opt = malloc(sizeof(*opt) + modnamelen + optionslen);
	if (opt == NULL)
		goto oom_error_init;

	opt->options = sizeof(*opt) + modnamelen + (char *)opt;

	memcpy(opt->modname, modname, modnamelen);
	memcpy(opt->options, options, optionslen);
	strchr_replace(opt->options, '\t', ' ');

	list = kmod_list_append(config->options, opt);
	if (list == NULL)
		goto oom_error;

	config->options = list;
	return 0;

oom_error:
	free(opt);
oom_error_init:
	ERR(config->ctx, "out-of-memory\n");
	return -ENOMEM;
}

static void kmod_config_free_options(struct kmod_config *config, struct kmod_list *l)
{
	struct kmod_options *opt = l->data;

	free(opt);

	config->options = kmod_list_remove(l);
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

/*
 * Take an fd and own it. It will be closed on return. filename is used only
 * for debug messages
 */
static int kmod_config_parse(struct kmod_config *config, int fd,
							const char *filename)
{
	struct kmod_ctx *ctx = config->ctx;
	char *line;
	FILE *fp;
	unsigned int linenum;
	int err;

	fp = fdopen(fd, "r");
	if (fp == NULL) {
		err = -errno;
		ERR(config->ctx, "fd %d: %m", fd);
		close(fd);
		return err;
	}

	while ((line = getline_wrapped(fp, &linenum)) != NULL) {
		char *cmd, *saveptr;

		if (line[0] == '\0' || line[0] == '#')
			goto done_next;

		cmd = strtok_r(line, "\t ", &saveptr);
		if (cmd == NULL)
			goto done_next;

		if (streq(cmd, "alias")) {
			char *alias = strtok_r(NULL, "\t ", &saveptr);
			char *modname = strtok_r(NULL, "\t ", &saveptr);

			if (alias == NULL || modname == NULL)
				goto syntax_error;

			kmod_config_add_alias(config,
						underscores(ctx, alias),
						underscores(ctx, modname));
		} else if (streq(cmd, "blacklist")) {
			char *modname = strtok_r(NULL, "\t ", &saveptr);

			if (modname == NULL)
				goto syntax_error;

			kmod_config_add_blacklist(config,
						underscores(ctx, modname));
		} else if (streq(cmd, "options")) {
			char *modname = strtok_r(NULL, "\t ", &saveptr);

			if (modname == NULL)
				goto syntax_error;

			kmod_config_add_options(config,
						underscores(ctx, modname),
						strtok_r(NULL, "\0", &saveptr));
		} else if (streq(cmd, "include")
				|| streq(cmd, "install")
				|| streq(cmd, "remove")
				|| streq(cmd, "softdep")
				|| streq(cmd, "config")) {
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

	while (config->options)
		kmod_config_free_options(config, config->options);

	free(config);
}

static bool conf_files_filter_out(struct kmod_ctx *ctx, const char *path,
								const char *fn)
{
	size_t len = strlen(fn);

	if (fn[0] == '.')
		return 1;

	if (len < 6 || (!streq(&fn[len - 5], ".conf")
				&& !streq(&fn[len - 6], ".alias"))) {
		INFO(ctx, "All config files need .conf: %s/%s, "
				"it will be ignored in a future release\n",
				path, fn);
		return 1;
	}

	return 0;
}

static DIR *conf_files_list(struct kmod_ctx *ctx, struct kmod_list **list,
							const char *path)
{
	struct stat st;
	DIR *d;
	int err;

	if (stat(path, &st) < 0)
		return NULL;

	if (!S_ISDIR(st.st_mode)) {
		*list = kmod_list_append(*list, path);
		return NULL;
	}

	d = opendir(path);
	if (d == NULL) {
		err = errno;
		ERR(ctx, "%m\n");
		return NULL;
	}

	for (;;) {
		struct dirent ent, *entp;
		struct kmod_list *l, *tmp;
		const char *dname;

		err = readdir_r(d, &ent, &entp);
		if (err != 0) {
			ERR(ctx, "reading entry %s\n", strerror(-err));
			goto fail_read;
		}

		if (entp == NULL)
			break;

		if (conf_files_filter_out(ctx, path, entp->d_name) == 1)
			continue;

		/* insert sorted */
		kmod_list_foreach(l, *list) {
			if (strcmp(entp->d_name, l->data) < 0)
				break;
		}

		dname = strdup(entp->d_name);
		if (dname == NULL)
			goto fail_oom;

		if (l == NULL)
			tmp = kmod_list_append(*list, dname);
		else if (l == *list)
			tmp = kmod_list_prepend(*list, dname);
		else
			tmp = kmod_list_insert_before(l, dname);

		if (tmp == NULL)
			goto fail_oom;

		if (l == NULL || l == *list)
			*list = tmp;
	}

	return d;

fail_oom:
	ERR(ctx, "out of memory while scanning '%s'\n", path);
fail_read:
	for (; *list != NULL; *list = kmod_list_remove(*list))
		free((*list)->data);
	closedir(d);
	return NULL;
}

int kmod_config_new(struct kmod_ctx *ctx, struct kmod_config **p_config)
{
	struct kmod_config *config;
	size_t i;

	*p_config = config = calloc(1, sizeof(struct kmod_config));
	if (config == NULL)
		return -ENOMEM;

	config->ctx = ctx;

	for (i = 0; i < ARRAY_SIZE(config_files); i++) {
		struct kmod_list *list = NULL;
		DIR *d;
		int fd;

		d = conf_files_list(ctx, &list, config_files[i]);

		/* there's no entry */
		if (list == NULL)
			continue;

		/* there's only one entry, and it's a file */
		if (d == NULL) {
			DBG(ctx, "parsing file '%s'\n", config_files[i]);
			list = kmod_list_remove(list);
			fd = open(config_files[i], O_RDONLY);
			if (fd >= 0)
				kmod_config_parse(config, fd, config_files[i]);

			continue;
		}

		/* treat all the entries in that dir */
		for (; list != NULL; list = kmod_list_remove(list)) {
			DBG(ctx, "parsing file '%s/%s'\n", config_files[i],
							(char *) list->data);
			fd = openat(dirfd(d), list->data, O_RDONLY);
			if (fd >= 0)
				kmod_config_parse(config, fd, list->data);

			free(list->data);
		}

		closedir(d);
	}

	return 0;
}
