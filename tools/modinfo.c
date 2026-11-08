// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * kmod-modinfo - query kernel module information using libkmod.
 *
 * Copyright (C) 2011-2013 ProFUSION embedded systems
 * Copyright (C) 2026 Emil Velikov
 */

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <shared/strbuf.h>
#include <shared/util.h>

#include <libkmod/libkmod.h>

#include "kmod.h"

static char separator = '\n';
static const char *field;

struct param {
	const char *name;
	const char *desc;
	const char *type;
	int namelen;
};

enum parm_info {
	parm_desc,
	parm_type,
};

static void add_param(const char *name, int namelen, enum parm_info parm_info,
		      const char *value, struct param *params, unsigned int params_count)
{
	/* We are guaranteed to have a match, or at least one empty entry */
	for (unsigned int i = 0; i < params_count; i++) {
		struct param *it = &params[i];

		if (it->name != NULL &&
		    (it->namelen != namelen || memcmp(it->name, name, namelen) != 0)) {
			continue;
		}

		it->name = name;
		it->namelen = namelen;

		switch (parm_info) {
		case (parm_desc):
			it->desc = value;
			break;
		case (parm_type):
			it->type = value;
			break;
		}
		break;
	}
}

static void process_parm(enum parm_info parm_info, const char *value,
			 struct param *params, unsigned int params_count)
{
	const char *name;
	int namelen;
	const char *colon = strchr(value, ':');

	if (colon == NULL) {
		ERR("Missing ':' in value \"%s\"\n", value);
		return;
	}

	if (colon == value) {
		ERR("Missing param name in value \"%s\"\n", value);
		return;
	}

	name = value;
	namelen = (int)(colon - value);
	add_param(name, namelen, parm_info, colon + 1, params, params_count);
}

static void print_line(const char *key, const char *value)
{
	if (key == NULL) {
		printf("%s%c", value, separator);
		return;
	}

	if (separator == '\0') {
		printf("%s=%s%c", key, value, separator);
	} else {
		size_t keylen = strlen(key);
		if (keylen > 15)
			keylen = 15;
		printf("%s:%-*s%s%c", key, 15 - (int)keylen, "", value, separator);
	}
}

static int modinfo_do(struct kmod_module *mod)
{
	DECLARE_STRBUF_WITH_STACK(buf, 256);
	const bool print_all = field == NULL;
	const bool print_parm = !print_all && streq(field, "parm");
	struct kmod_list *l, *list = NULL;
	struct param *params = NULL;
	unsigned int params_count = 0;
	int err, is_builtin;
	const char *filename = kmod_module_get_path(mod);

	is_builtin = (filename == NULL);

	/* TODO: align builtin vs not wrt listing "name:" via kmod_module_get_info() */
	if (is_builtin) {
		if (print_all)
			print_line("name", kmod_module_get_name(mod));
		else if (streq(field, "name")) {
			print_line(NULL, kmod_module_get_name(mod));
			return 0;
		}
		filename = "(builtin)";
	}

	if (print_all)
		print_line("filename", filename);
	else if (streq(field, "filename")) {
		print_line(NULL, filename);
		return 0;
	}

	err = kmod_module_get_info(mod, &list);
	if (err < 0) {
		if (is_builtin && err == -ENOENT) {
			/*
			 * This is an old kernel that does not have a file
			 * with information about built-in modules.
			 */
			return 0;
		}
		ERR("could not get modinfo from '%s': %s\n", kmod_module_get_name(mod),
		    strerror(-err));
		return err;
	}

	if (print_all || print_parm) {
		size_t count = 0;
		size_t longest_desc = 0;
		size_t longest_type = 0;
		bool can_reserve;

		/*
		 * Usually parm/parmtype come in pairs, where we print once per pair.
		 *
		 * Get worst case scenario & longest entry, for sufficiently large buffers.
		 */
		kmod_list_foreach(l, list) {
			const char *key = kmod_module_info_get_key(l);
			size_t *longest_value;

			if (streq(key, "parm"))
				longest_value = &longest_desc;
			else if (streq(key, "parmtype"))
				longest_value = &longest_type;
			else
				continue;

			const char *value = kmod_module_info_get_value(l);
			size_t len = strlen(value);

			count++;

			if (len > *longest_value)
				*longest_value = len;
		}

		/* XXX: do we want to emit a warning/error here? */
		params_count = (count > UINT_MAX) ? UINT_MAX : (unsigned int)count;

		if (params_count) {
			params = calloc(params_count, sizeof(*params));

			/*
			 * The "name:" exists in both parm&parmtype, so don't worry if we
			 * overallocate.
			 */
			can_reserve =
				strbuf_reserve_extra(&buf, longest_desc + strlen(" ()") +
								   longest_type);
			if (params == NULL || !can_reserve) {
				err = -ENOMEM;
				goto end;
			}
		}
	}

	kmod_list_foreach(l, list) {
		const char *key = kmod_module_info_get_key(l);
		const char *value = kmod_module_info_get_value(l);
		enum parm_info parm_info;

		if (!print_all && !print_parm) {
			if (streq(field, key))
				print_line(NULL, value);
			continue;
		}

		if (streq(key, "parm")) {
			parm_info = parm_desc;
		} else if (streq(key, "parmtype")) {
			parm_info = parm_type;
		} else {
			if (print_all)
				print_line(key, value);
			continue;
		}

		if (strlen(value) > INT_MAX) {
			ERR("%s's value is longer than INT_MAX\n",
			    parm_info == parm_desc ? "parm" : "parmtype");
			continue;
		}
		process_parm(parm_info, value, params, params_count);
	}

	for (unsigned int i = 0; i < params_count; i++) {
		struct param *p = &params[i];
		if (p->name == NULL)
			continue;

		strbuf_clear(&buf);
		strbuf_pushmem(&buf, p->name, p->namelen);
		strbuf_pushchar(&buf, ':');

		if (p->desc != NULL)
			strbuf_pushchars(&buf, p->desc);

		if (p->type != NULL) {
			strbuf_pushchars(&buf, " (");
			strbuf_pushchars(&buf, p->type);
			strbuf_pushchars(&buf, ")");
		}

		print_line(print_parm ? NULL : "parm", strbuf_str(&buf));
	}

end:
	free(params);
	kmod_module_info_free_list(list);

	return err;
}

static int modinfo_path_do(struct kmod_ctx *ctx, const char *path)
{
	struct kmod_module *mod;
	int err = kmod_module_new_from_path(ctx, path, &mod);
	if (err < 0) {
		ERR("Module file %s: %s.\n", path, strerror(-err));
		return err;
	}
	err = modinfo_do(mod);
	kmod_module_unref(mod);
	return err;
}

static int modinfo_name_do(struct kmod_ctx *ctx, const char *name)
{
	struct kmod_module *mod = NULL;
	int err;

	err = kmod_module_new_from_name_lookup(ctx, name, &mod);
	if (err < 0 || mod == NULL) {
		err = err < 0 ? err : -ENOENT;
		ERR("Module name %s: %s.\n", name, strerror(-err));
		return err;
	}

	err = modinfo_do(mod);
	kmod_module_unref(mod);

	return err;
}

static int modinfo_alias_do(struct kmod_ctx *ctx, const char *alias)
{
	struct kmod_list *l, *list = NULL;
	int err = kmod_module_new_from_lookup(ctx, alias, &list);
	if (err < 0 || list == NULL) {
		err = err < 0 ? err : -ENOENT;
		ERR("Module alias %s: %s.\n", alias, strerror(-err));
		return err;
	}

	kmod_list_foreach(l, list) {
		struct kmod_module *mod = kmod_module_get_module(l);
		int r = modinfo_do(mod);
		kmod_module_unref(mod);
		if (r < 0)
			err = r;
	}
	kmod_module_unref_list(list);
	return err;
}

static const char cmdopts_s[] = "adlpn0mF:k:b:Vh";
static const struct option cmdopts[] = {
	{ "author", no_argument, NULL, 'a' },
	{ "description", no_argument, NULL, 'd' },
	{ "license", no_argument, NULL, 'l' },
	{ "parameters", no_argument, NULL, 'p' },
	{ "filename", no_argument, NULL, 'n' },
	{ "null", no_argument, NULL, '0' },
	{ "modname", no_argument, NULL, 'm' },
	{ "field", required_argument, NULL, 'F' },
	{ "set-version", required_argument, NULL, 'k' },
	{ "basedir", required_argument, NULL, 'b' },
	{ "version", no_argument, NULL, 'V' },
	{ "help", no_argument, NULL, 'h' },
	{},
};

static void help(void)
{
	printf("Usage:\n"
	       "\t%s [options] <modulename|filename> [args]\n"
	       "Options:\n"
	       "\t-a, --author                Print only 'author'\n"
	       "\t-d, --description           Print only 'description'\n"
	       "\t-l, --license               Print only 'license'\n"
	       "\t-p, --parameters            Print only 'parm'\n"
	       "\t-n, --filename              Print only 'filename'\n"
	       "\t-0, --null                  Use \\0 instead of \\n\n"
	       "\t-m, --modname               Handle argument as module name instead of alias or filename\n"
	       "\t-F, --field FIELD           Print only provided FIELD\n"
	       "\t-k, --set-version VERSION   Use VERSION instead of $(uname -r)\n"
	       "\t-b, --basedir DIR           Use DIR as filesystem root for " MODULE_DIRECTORY
	       "\n"
	       "\t-V, --version               Show version\n"
	       "\t-h, --help                  Show this help\n",
	       program_invocation_short_name);
}

static bool is_module_filename(const char *name)
{
	struct stat st;

	if (path_ends_with_kmod_ext(name, strlen(name)) && stat(name, &st) == 0 &&
	    S_ISREG(st.st_mode))
		return true;

	return false;
}

static int do_modinfo(int argc, char *argv[])
{
	struct kmod_ctx *ctx;
	char dirname_buf[PATH_MAX];
	const char *dirname = NULL;
	const char *kversion = NULL;
	const char *root = NULL;
	const char *null_config = NULL;
	bool arg_is_modname = false;
	int i, err, c;

	opterr = 0;
	while ((c = getopt_long(argc, argv, cmdopts_s, cmdopts, NULL)) != -1) {
		switch (c) {
		case 'a':
			field = "author";
			break;
		case 'd':
			field = "description";
			break;
		case 'l':
			field = "license";
			break;
		case 'p':
			field = "parm";
			break;
		case 'n':
			field = "filename";
			break;
		case '0':
			separator = '\0';
			break;
		case 'm':
			arg_is_modname = true;
			break;
		case 'F':
			field = optarg;
			break;
		case 'k':
			kversion = optarg;
			break;
		case 'b':
			root = optarg;
			break;
		case 'h':
			help();
			return EXIT_SUCCESS;
		case 'V':
			kmod_version();
			return EXIT_SUCCESS;
		case '?':
			ERR("unrecognised option \'%s\'\n\n", argv[optind - 1]);
			help();
			return EXIT_FAILURE;
		default:
			ERR("unexpected getopt_long() value '%c'.\n", c);
			return EXIT_FAILURE;
		}
	}

	if (optind >= argc) {
		ERR("missing module or filename.\n");
		return EXIT_FAILURE;
	}

	if (root != NULL || kversion != NULL) {
		struct utsname u;
		int n;
		if (root == NULL)
			root = "";
		if (kversion == NULL) {
			if (uname(&u) < 0) {
				ERR("uname() failed: %m\n");
				return EXIT_FAILURE;
			}
			kversion = u.release;
		}

		n = snprintf(dirname_buf, sizeof(dirname_buf),
			     "%s" MODULE_DIRECTORY "/%s", root, kversion);
		if (n >= (int)sizeof(dirname_buf)) {
			ERR("bad directory %s" MODULE_DIRECTORY "/%s: path too long\n",
			    root, kversion);
			return EXIT_FAILURE;
		}
		dirname = dirname_buf;
	}

	ctx = kmod_new(dirname, &null_config);
	if (!ctx) {
		ERR("kmod_new() failed!\n");
		return EXIT_FAILURE;
	}

	err = 0;
	for (i = optind; i < argc; i++) {
		const char *name = argv[i];
		int r;

		if (arg_is_modname)
			r = modinfo_name_do(ctx, name);
		else if (is_module_filename(name))
			r = modinfo_path_do(ctx, name);
		else
			r = modinfo_alias_do(ctx, name);

		if (r < 0)
			err = r;
	}

	kmod_unref(ctx);
	return err >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

const struct kmod_cmd kmod_cmd_compat_modinfo = {
	.name = "modinfo",
	.cmd = do_modinfo,
	.help = "compat modinfo command",
};
