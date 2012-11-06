/*
 * kmod-rmmod - remove modules from linux kernel using libkmod.
 *
 * Copyright (C) 2011-2012  ProFUSION embedded systems
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include "libkmod.h"
#include "macro.h"

#include "kmod.h"

#define DEFAULT_VERBOSE LOG_ERR
static int verbose = DEFAULT_VERBOSE;
static int use_syslog;

static const char cmdopts_s[] = "fsvVwh";
static const struct option cmdopts[] = {
	{"force", no_argument, 0, 'f'},
	{"syslog", no_argument, 0, 's'},
	{"verbose", no_argument, 0, 'v'},
	{"version", no_argument, 0, 'V'},
	{"wait", no_argument, 0, 'w'},
	{"help", no_argument, 0, 'h'},
	{NULL, 0, 0, 0}
};

static void help(void)
{
	fprintf(stderr,
		"Usage:\n"
		"\t%s [options] modulename ...\n"
		"Options:\n"
		"\t-f, --force       forces a module unload and may crash your\n"
		"\t                  machine. This requires Forced Module Removal\n"
		"\t                  option in your kernel. DANGEROUS\n"
		"\t-s, --syslog      print to syslog, not stderr\n"
		"\t-v, --verbose     enables more messages\n"
		"\t-V, --version     show version\n"
		"\t-h, --help        show this help\n",
		binname);
}

static void log_rmmod(void *data, int priority, const char *file, int line,
			const char *fn, const char *format, va_list args)
{
	const char *prioname = prio_to_str(priority);
	char *str;

	if (vasprintf(&str, format, args) < 0)
		return;

	if (use_syslog > 1) {
#ifdef ENABLE_DEBUG
		syslog(priority, "%s: %s:%d %s() %s", prioname, file, line,
		       fn, str);
#else
		syslog(priority, "%s: %s", prioname, str);
#endif
	} else {
#ifdef ENABLE_DEBUG
		fprintf(stderr, "rmmod: %s: %s:%d %s() %s", prioname, file,
			line, fn, str);
#else
		fprintf(stderr, "rmmod: %s: %s", prioname, str);
#endif
	}

	free(str);
	(void)data;
}

static void _log(int prio, const char *fmt, ...)
{
	const char *prioname;
	char *msg;
	va_list args;

	if (prio > verbose)
		return;

	va_start(args, fmt);
	if (vasprintf(&msg, fmt, args) < 0)
		msg = NULL;
	va_end(args);
	if (msg == NULL)
		return;

	prioname = prio_to_str(prio);

	if (use_syslog > 1)
		syslog(prio, "%s: %s", prioname, msg);
	else
		fprintf(stderr, "rmmod: %s: %s", prioname, msg);
	free(msg);

	if (prio <= LOG_CRIT)
		exit(EXIT_FAILURE);
}
#define ERR(...) _log(LOG_ERR, __VA_ARGS__)

static int check_module_inuse(struct kmod_module *mod) {
	struct kmod_list *holders;

	if (kmod_module_get_initstate(mod) == -ENOENT) {
		ERR("Module %s is not currently loaded\n",
				kmod_module_get_name(mod));
		return -ENOENT;
	}

	holders = kmod_module_get_holders(mod);
	if (holders != NULL) {
		struct kmod_list *itr;

		ERR("Module %s is in use by:", kmod_module_get_name(mod));

		kmod_list_foreach(itr, holders) {
			struct kmod_module *hm = kmod_module_get_module(itr);
			fprintf(stderr, " %s", kmod_module_get_name(hm));
			kmod_module_unref(hm);
		}
		fputc('\n', stderr);

		kmod_module_unref_list(holders);
		return -EBUSY;
	}

	if (kmod_module_get_refcnt(mod) != 0) {
		ERR("Module %s is in use\n", kmod_module_get_name(mod));
		return -EBUSY;
	}

	return 0;
}

static int do_rmmod(int argc, char *argv[])
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	int flags = KMOD_REMOVE_NOWAIT;
	int i, err, r = 0;

	for (;;) {
		int c, idx = 0;
		c = getopt_long(argc, argv, cmdopts_s, cmdopts, &idx);
		if (c == -1)
			break;
		switch (c) {
		case 'f':
			flags |= KMOD_REMOVE_FORCE;
			break;
		case 's':
			use_syslog = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'w':
			ERR("'Wait' behavior is targeted for removal from kernel.\nWe will now sleep for 10s, and then continue.\n");
			sleep(10);
			flags &= ~KMOD_REMOVE_NOWAIT;
			break;
		case 'h':
			help();
			return EXIT_SUCCESS;
		case 'V':
			puts(PACKAGE " version " VERSION);
			return EXIT_SUCCESS;
		case '?':
			return EXIT_FAILURE;
		default:
			ERR("unexpected getopt_long() value '%c'.\n", c);
			return EXIT_FAILURE;
		}
	}

	if (use_syslog) {
		openlog("rmmod", LOG_CONS, LOG_DAEMON);
		use_syslog++;
	}

	if (optind >= argc) {
		ERR("missing module name.\n");
		r = EXIT_FAILURE;
		goto done;
	}

	ctx = kmod_new(NULL, &null_config);
	if (!ctx) {
		ERR("kmod_new() failed!\n");
		r = EXIT_FAILURE;
		goto done;
	}

	kmod_set_log_priority(ctx, kmod_get_log_priority(ctx));
	kmod_set_log_fn(ctx, log_rmmod, NULL);

	for (i = optind; i < argc; i++) {
		struct kmod_module *mod;
		const char *arg = argv[i];
		struct stat st;
		if (stat(arg, &st) == 0)
			err = kmod_module_new_from_path(ctx, arg, &mod);
		else
			err = kmod_module_new_from_name(ctx, arg, &mod);

		if (err < 0) {
			ERR("could not use module %s: %s\n", arg,
			    strerror(-err));
			break;
		}

		if (!(flags & KMOD_REMOVE_FORCE) && (flags & KMOD_REMOVE_NOWAIT))
			if (check_module_inuse(mod) < 0) {
				r++;
				goto next;
			}

		err = kmod_module_remove_module(mod, flags);
		if (err < 0) {
			ERR("could not remove module %s: %s\n", arg,
			    strerror(-err));
			r++;
		}
next:
		kmod_module_unref(mod);
	}

	kmod_unref(ctx);

done:
	if (use_syslog > 1)
		closelog();

	return r == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

const struct kmod_cmd kmod_cmd_compat_rmmod = {
	.name = "rmmod",
	.cmd = do_rmmod,
	.help = "compat rmmod command",
};
