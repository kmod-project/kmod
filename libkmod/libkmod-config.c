/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011-2012  ProFUSION embedded systems
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

struct kmod_alias {
	char *name;
	char modname[];
};

struct kmod_options {
	char *options;
	char modname[];
};

struct kmod_command {
	char *command;
	char modname[];
};

struct kmod_softdep {
	char *name;
	const char **pre;
	const char **post;
	unsigned int n_pre;
	unsigned int n_post;
};

const char *kmod_blacklist_get_modname(const struct kmod_list *l)
{
	return l->data;
}

const char *kmod_alias_get_name(const struct kmod_list *l) {
	const struct kmod_alias *alias = l->data;
	return alias->name;
}

const char *kmod_alias_get_modname(const struct kmod_list *l) {
	const struct kmod_alias *alias = l->data;
	return alias->modname;
}

const char *kmod_option_get_options(const struct kmod_list *l) {
	const struct kmod_options *alias = l->data;
	return alias->options;
}

const char *kmod_option_get_modname(const struct kmod_list *l) {
	const struct kmod_options *alias = l->data;
	return alias->modname;
}

const char *kmod_command_get_command(const struct kmod_list *l) {
	const struct kmod_command *alias = l->data;
	return alias->command;
}

const char *kmod_command_get_modname(const struct kmod_list *l) {
	const struct kmod_command *alias = l->data;
	return alias->modname;
}

const char *kmod_softdep_get_name(const struct kmod_list *l) {
	const struct kmod_softdep *dep = l->data;
	return dep->name;
}

const char * const *kmod_softdep_get_pre(const struct kmod_list *l, unsigned int *count) {
	const struct kmod_softdep *dep = l->data;
	*count = dep->n_pre;
	return dep->pre;
}

const char * const *kmod_softdep_get_post(const struct kmod_list *l, unsigned int *count) {
	const struct kmod_softdep *dep = l->data;
	*count = dep->n_post;
	return dep->post;
}

static int kmod_config_add_command(struct kmod_config *config,
						const char *modname,
						const char *command,
						const char *command_name,
						struct kmod_list **list)
{
	struct kmod_command *cmd;
	struct kmod_list *l;
	size_t modnamelen = strlen(modname) + 1;
	size_t commandlen = strlen(command) + 1;

	DBG(config->ctx, "modname='%s' cmd='%s %s'\n", modname, command_name,
								command);

	cmd = malloc(sizeof(*cmd) + modnamelen + commandlen);
	if (cmd == NULL)
		goto oom_error_init;

	cmd->command = sizeof(*cmd) + modnamelen + (char *)cmd;
	memcpy(cmd->modname, modname, modnamelen);
	memcpy(cmd->command, command, commandlen);

	l = kmod_list_append(*list, cmd);
	if (l == NULL)
		goto oom_error;

	*list = l;
	return 0;

oom_error:
	free(cmd);
oom_error_init:
	ERR(config->ctx, "out-of-memory\n");
	return -ENOMEM;
}

static void kmod_config_free_command(struct kmod_config *config,
					struct kmod_list *l,
					struct kmod_list **list)
{
	struct kmod_command *cmd = l->data;

	free(cmd);
	*list = kmod_list_remove(l);
}

static int kmod_config_add_options(struct kmod_config *config,
				const char *modname, const char *options)
{
	struct kmod_options *opt;
	struct kmod_list *list;
	size_t modnamelen = strlen(modname) + 1;
	size_t optionslen = strlen(options) + 1;

	DBG(config->ctx, "modname='%s' options='%s'\n", modname, options);

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

static void kmod_config_free_options(struct kmod_config *config,
							struct kmod_list *l)
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

static void kmod_config_free_alias(struct kmod_config *config,
							struct kmod_list *l)
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

static int kmod_config_add_softdep(struct kmod_config *config,
							const char *modname,
							const char *line)
{
	struct kmod_list *list;
	struct kmod_softdep *dep;
	const char *s, *p;
	char *itr;
	unsigned int n_pre = 0, n_post = 0;
	size_t modnamelen = strlen(modname) + 1;
	size_t buflen = 0;
	bool was_space = false;
	enum { S_NONE, S_PRE, S_POST } mode = S_NONE;

	DBG(config->ctx, "modname=%s\n", modname);

	/* analyze and count */
	for (p = s = line; ; s++) {
		size_t plen;

		if (*s != '\0') {
			if (!isspace(*s)) {
				was_space = false;
				continue;
			}

			if (was_space) {
				p = s + 1;
				continue;
			}
			was_space = true;

			if (p >= s)
				continue;
		}
		plen = s - p;

		if (plen == sizeof("pre:") - 1 &&
				memcmp(p, "pre:", sizeof("pre:") - 1) == 0)
			mode = S_PRE;
		else if (plen == sizeof("post:") - 1 &&
				memcmp(p, "post:", sizeof("post:") - 1) == 0)
			mode = S_POST;
		else if (*s != '\0' || (*s == '\0' && !was_space)) {
			if (mode == S_PRE) {
				buflen += plen + 1;
				n_pre++;
			} else if (mode == S_POST) {
				buflen += plen + 1;
				n_post++;
			}
		}
		p = s + 1;
		if (*s == '\0')
			break;
	}

	DBG(config->ctx, "%u pre, %u post\n", n_pre, n_post);

	dep = malloc(sizeof(struct kmod_softdep) + modnamelen +
		     n_pre * sizeof(const char *) +
		     n_post * sizeof(const char *) +
		     buflen);
	if (dep == NULL) {
		ERR(config->ctx, "out-of-memory modname=%s\n", modname);
		return -ENOMEM;
	}
	dep->n_pre = n_pre;
	dep->n_post = n_post;
	dep->pre = (const char **)((char *)dep + sizeof(struct kmod_softdep));
	dep->post = dep->pre + n_pre;
	dep->name = (char *)(dep->post + n_post);

	memcpy(dep->name, modname, modnamelen);

	/* copy strings */
	itr = dep->name + modnamelen;
	n_pre = 0;
	n_post = 0;
	mode = S_NONE;
	for (p = s = line; ; s++) {
		size_t plen;

		if (*s != '\0') {
			if (!isspace(*s)) {
				was_space = false;
				continue;
			}

			if (was_space) {
				p = s + 1;
				continue;
			}
			was_space = true;

			if (p >= s)
				continue;
		}
		plen = s - p;

		if (plen == sizeof("pre:") - 1 &&
				memcmp(p, "pre:", sizeof("pre:") - 1) == 0)
			mode = S_PRE;
		else if (plen == sizeof("post:") - 1 &&
				memcmp(p, "post:", sizeof("post:") - 1) == 0)
			mode = S_POST;
		else if (*s != '\0' || (*s == '\0' && !was_space)) {
			if (mode == S_PRE) {
				dep->pre[n_pre] = itr;
				memcpy(itr, p, plen);
				itr[plen] = '\0';
				itr += plen + 1;
				n_pre++;
			} else if (mode == S_POST) {
				dep->post[n_post] = itr;
				memcpy(itr, p, plen);
				itr[plen] = '\0';
				itr += plen + 1;
				n_post++;
			}
		}
		p = s + 1;
		if (*s == '\0')
			break;
	}

	list = kmod_list_append(config->softdeps, dep);
	if (list == NULL) {
		free(dep);
		return -ENOMEM;
	}
	config->softdeps = list;

	return 0;
}

static void kmod_config_free_softdep(struct kmod_config *config,
							struct kmod_list *l)
{
	free(l->data);
	config->softdeps = kmod_list_remove(l);
}

static void kcmdline_parse_result(struct kmod_config *config, char *modname,
						char *param, char *value)
{
	if (modname == NULL || param == NULL || value == NULL)
		return;

	DBG(config->ctx, "%s %s\n", modname, param);

	if (streq(modname, "modprobe") && !strncmp(param, "blacklist=", 10)) {
		for (;;) {
			char *t = strsep(&value, ",");
			if (t == NULL)
				break;

			kmod_config_add_blacklist(config, t);
		}
	} else {
		kmod_config_add_options(config,
				underscores(config->ctx, modname), param);
	}
}

static int kmod_config_parse_kcmdline(struct kmod_config *config)
{
	char buf[KCMD_LINE_SIZE];
	int fd, err;
	char *p, *modname,  *param = NULL, *value = NULL;

	fd = open("/proc/cmdline", O_RDONLY|O_CLOEXEC);
	if (fd < 0) {
		err = -errno;
		DBG(config->ctx, "could not open '/proc/cmdline' for reading: %m\n");
		return err;
	}

	err = read_str_safe(fd, buf, sizeof(buf));
	close(fd);
	if (err < 0) {
		ERR(config->ctx, "could not read from '/proc/cmdline': %s\n",
							strerror(-err));
		return err;
	}

	for (p = buf, modname = buf; *p != '\0' && *p != '\n'; p++) {
		switch (*p) {
		case ' ':
			*p = '\0';
			kcmdline_parse_result(config, modname, param, value);
			param = value = NULL;
			modname = p + 1;
			break;
		case '.':
			*p = '\0';
			param = p + 1;
			break;
		case '=':
			if (param != NULL)
				value = p + 1;
			break;
		}
	}

	*p = '\0';
	kcmdline_parse_result(config, modname, param, value);

	return 0;
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
	unsigned int linenum = 0;
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
			char *options = strtok_r(NULL, "\0", &saveptr);

			if (modname == NULL || options == NULL)
				goto syntax_error;

			kmod_config_add_options(config,
						underscores(ctx, modname),
						options);
		} else if (streq(cmd, "install")) {
			char *modname = strtok_r(NULL, "\t ", &saveptr);
			char *installcmd = strtok_r(NULL, "\0", &saveptr);

			if (modname == NULL || installcmd == NULL)
				goto syntax_error;

			kmod_config_add_command(config,
					underscores(ctx, modname),
					installcmd,
					cmd, &config->install_commands);
		} else if (streq(cmd, "remove")) {
			char *modname = strtok_r(NULL, "\t ", &saveptr);
			char *removecmd = strtok_r(NULL, "\0", &saveptr);

			if (modname == NULL || removecmd == NULL)
				goto syntax_error;

			kmod_config_add_command(config,
					underscores(ctx, modname),
					removecmd,
					cmd, &config->remove_commands);
		} else if (streq(cmd, "softdep")) {
			char *modname = strtok_r(NULL, "\t ", &saveptr);
			char *softdeps = strtok_r(NULL, "\0", &saveptr);

			if (modname == NULL || softdeps == NULL)
				goto syntax_error;

			kmod_config_add_softdep(config,
					underscores(ctx, modname),
					softdeps);
		} else if (streq(cmd, "include")
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

	while (config->install_commands) {
		kmod_config_free_command(config, config->install_commands,
						&config->install_commands);
	}

	while (config->remove_commands) {
		kmod_config_free_command(config, config->remove_commands,
						&config->remove_commands);
	}

	while (config->softdeps)
		kmod_config_free_softdep(config, config->softdeps);

	for (; config->paths != NULL;
				config->paths = kmod_list_remove(config->paths))
		free(config->paths->data);

	free(config);
}

static bool conf_files_filter_out(struct kmod_ctx *ctx, DIR *d,
					const char *path, const char *fn)
{
	size_t len = strlen(fn);
	struct stat st;

	if (fn[0] == '.')
		return true;

	if (len < 6 || (!streq(&fn[len - 5], ".conf")
				&& !streq(&fn[len - 6], ".alias")))
		return true;

	fstatat(dirfd(d), fn, &st, 0);

	if (S_ISDIR(st.st_mode)) {
		ERR(ctx, "Directories inside directories are not supported: "
							"%s/%s\n", path, fn);
		return true;
	}

	return false;
}

struct conf_file {
	const char *path;
	bool is_single;
	char name[];
};

static int conf_files_insert_sorted(struct kmod_ctx *ctx,
					struct kmod_list **list,
					const char *path, const char *name)
{
	struct kmod_list *lpos, *tmp;
	struct conf_file *cf;
	size_t namelen;
	int cmp = -1;
	bool is_single = false;

	if (name == NULL) {
		name = basename(path);
		is_single = true;
	}

	kmod_list_foreach(lpos, *list) {
		cf = lpos->data;

		if ((cmp = strcmp(name, cf->name)) <= 0)
			break;
	}

	if (cmp == 0) {
		DBG(ctx, "Ignoring duplicate config file: %s/%s\n", path,
									name);
		return -EEXIST;
	}

	namelen = strlen(name);
	cf = malloc(sizeof(*cf) + namelen + 1);
	if (cf == NULL)
		return -ENOMEM;

	memcpy(cf->name, name, namelen + 1);
	cf->path = path;
	cf->is_single = is_single;

	if (lpos == NULL)
		tmp = kmod_list_append(*list, cf);
	else if (lpos == *list)
		tmp = kmod_list_prepend(*list, cf);
	else
		tmp = kmod_list_insert_before(lpos, cf);

	if (tmp == NULL) {
		free(cf);
		return -ENOMEM;
	}

	if (lpos == NULL || lpos == *list)
		*list = tmp;

	return 0;
}

/*
 * Insert configuration files in @list, ignoring duplicates
 */
static int conf_files_list(struct kmod_ctx *ctx, struct kmod_list **list,
						const char *path,
						unsigned long long *path_stamp)
{
	DIR *d;
	int err;
	struct stat st;

	if (stat(path, &st) != 0) {
		err = -errno;
		DBG(ctx, "could not stat '%s': %m\n", path);
		return err;
	}

	*path_stamp = ts_usec(&st.st_mtim);

	if (S_ISREG(st.st_mode)) {
		conf_files_insert_sorted(ctx, list, path, NULL);
		return 0;
	} if (!S_ISDIR(st.st_mode)) {
		ERR(ctx, "unsupported file mode %s: %#x\n",
							path, st.st_mode);
		return -EINVAL;
	}

	d = opendir(path);
	if (d == NULL) {
		ERR(ctx, "%m\n");
		return -EINVAL;
	}

	for (;;) {
		struct dirent ent, *entp;

		err = readdir_r(d, &ent, &entp);
		if (err != 0) {
			ERR(ctx, "reading entry %s\n", strerror(-err));
			goto fail_read;
		}

		if (entp == NULL)
			break;

		if (conf_files_filter_out(ctx, d, path, entp->d_name))
			continue;

		conf_files_insert_sorted(ctx, list, path, entp->d_name);
	}

	closedir(d);
	return 0;

fail_read:
	closedir(d);
	return err;
}

int kmod_config_new(struct kmod_ctx *ctx, struct kmod_config **p_config,
					const char * const *config_paths)
{
	struct kmod_config *config;
	struct kmod_list *list = NULL;
	struct kmod_list *path_list = NULL;
	size_t i;

	for (i = 0; config_paths[i] != NULL; i++) {
		const char *path = config_paths[i];
		unsigned long long path_stamp = 0;
		size_t pathlen;
		struct kmod_list *tmp;
		struct kmod_config_path *cf;

		if (conf_files_list(ctx, &list, path, &path_stamp) < 0)
			continue;

		pathlen = strlen(path) + 1;
		cf = malloc(sizeof(*cf) + pathlen);
		if (cf == NULL)
			goto oom;

		cf->stamp = path_stamp;
		memcpy(cf->path, path, pathlen);

		tmp = kmod_list_append(path_list, cf);
		if (tmp == NULL)
			goto oom;
		path_list = tmp;
	}

	*p_config = config = calloc(1, sizeof(struct kmod_config));
	if (config == NULL)
		goto oom;

	config->paths = path_list;
	config->ctx = ctx;

	for (; list != NULL; list = kmod_list_remove(list)) {
		char fn[PATH_MAX];
		struct conf_file *cf = list->data;
		int fd;

		if (cf->is_single)
			strcpy(fn, cf->path);
		else
			snprintf(fn, sizeof(fn),"%s/%s", cf->path,
					cf->name);

		fd = open(fn, O_RDONLY|O_CLOEXEC);
		DBG(ctx, "parsing file '%s' fd=%d\n", fn, fd);

		if (fd >= 0)
			kmod_config_parse(config, fd, fn);

		free(cf);
	}

	kmod_config_parse_kcmdline(config);

	return 0;

oom:
	for (; list != NULL; list = kmod_list_remove(list))
		free(list->data);

	for (; path_list != NULL; path_list = kmod_list_remove(path_list))
		free(path_list->data);

	return -ENOMEM;
}
