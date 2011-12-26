#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <libkmod.h>
#include <getopt.h>

static const char cmdoptions_short[] = "lh";
static const struct option cmdoptions[] = {
	{"load-resources", no_argument, 0, 'l'},
	{"help", no_argument, 0, 'h'},
	{NULL, 0, 0, 0}
};

static void help(const char *progname)
{
	const struct option *itr_opt;
	const char *itr_short;
	printf("Usage:\n"
	       "\t%s [options] <module> [ module_options ]\n"
	       "Options:\n",
	       progname);
	for (itr_opt = cmdoptions, itr_short = cmdoptions_short;
	     itr_opt->name != NULL; itr_opt++, itr_short++)
		printf("\t-%c, --%s\n", *itr_short, itr_opt->name);
}

int main(int argc, char *argv[])
{
	const char *alias;
	const char *opt;
	struct kmod_ctx *ctx;
	struct kmod_list *list = NULL, *l;
	int load_resources = 0;
	int err;

	printf("libkmod version %s\n", VERSION);

	for (;;) {
		int c, idx = 0;
		c = getopt_long(argc, argv, cmdoptions_short, cmdoptions, &idx);
		if (c == -1)
			break;
		switch (c) {
		case 'l':
			load_resources = 1;
			break;
		case 'h':
			help(argv[0]);
			return 0;
		case '?':
			return -1;
		default:
			fprintf(stderr,
				"ERR: unexpected getopt_long() value %c\n", c);
			return -1;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "ERR: Provide an alias name\n");
		return EXIT_FAILURE;
	}

	alias = argv[optind++];

	if (optind < argc)
		opt = argv[optind];
	else
		opt = NULL;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL) {
		kmod_unref(ctx);
		exit(EXIT_FAILURE);
	}

	if (load_resources) {
		err = kmod_load_resources(ctx);
		if (err < 0) {
			printf("Could not load resources: %s\n",
			       strerror(-err));
			kmod_unref(ctx);
			exit(EXIT_FAILURE);
		}
	}

	err = kmod_module_new_from_lookup(ctx, alias, &list);
	if (err < 0)
		exit(EXIT_FAILURE);

	if (list == NULL)
		printf("No module matches '%s'\n", alias);
	else
		printf("Alias: '%s'\nModules matching:\n", alias);

	kmod_list_foreach(l, list) {
		struct kmod_module *mod = kmod_module_get_module(l);

		printf("\t%s", kmod_module_get_name(mod));

		err = kmod_module_probe_insert_module(mod, 0, opt, NULL, NULL);
		if (err >=0 )
			printf(": inserted ok\n");
		else
			printf(": failed to insert\n");

		kmod_module_unref(mod);
	}

	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
