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
	       "\t%s [options] <name-to-lookup>\n"
	       "Options:\n",
	       progname);
	for (itr_opt = cmdoptions, itr_short = cmdoptions_short;
	     itr_opt->name != NULL; itr_opt++, itr_short++)
		printf("\t-%c, --%s\n", *itr_short, itr_opt->name);
}

int main(int argc, char *argv[])
{
	const char *alias = NULL;
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

	alias = argv[optind];

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
		struct kmod_list *d, *pre = NULL, *post = NULL;
		struct kmod_module *mod = kmod_module_get_module(l);
		const char *str;

		printf("\t%s\n", kmod_module_get_name(mod));
		str = kmod_module_get_options(mod);
		if (str)
			printf("\t\toptions: '%s'\n", str);
		str = kmod_module_get_install_commands(mod);
		if (str)
			printf("\t\tinstall commands: '%s'\n", str);
		str = kmod_module_get_remove_commands(mod);
		if (str)
			printf("\t\tremove commands: '%s'\n", str);

		err = kmod_module_get_softdeps(mod, &pre, &post);
		if (err == 0) {
			if (pre != NULL || post != NULL)
				puts("\t\tsoft dependencies:");
			if (pre != NULL) {
				fputs("\t\t\tpre:", stdout);
				kmod_list_foreach(d, pre) {
					struct kmod_module *dm = kmod_module_get_module(d);
					printf(" %s", kmod_module_get_name(dm));
					kmod_module_unref(dm);
				}
				putchar('\n');
				kmod_module_unref_list(pre);
			}
			if (post != NULL) {
				fputs("\t\t\tpost:", stdout);
				kmod_list_foreach(d, post) {
					struct kmod_module *dm = kmod_module_get_module(d);
					printf(" %s", kmod_module_get_name(dm));
					kmod_module_unref(dm);
				}
				putchar('\n');
				kmod_module_unref_list(post);
			}
		}

		pre = NULL;
		err = kmod_module_get_info(mod, &pre);
		if (err > 0) {
			puts("\t\tmodinfo:");
			kmod_list_foreach(d, pre) {
				const char *key, *val;
				key = kmod_module_info_get_key(d);
				val = kmod_module_info_get_value(d);
				printf("\t\t\t%s: %s\n", key, val ? val : "");
			}
			kmod_module_info_free_list(pre);
		}

		pre = NULL;
		err = kmod_module_get_versions(mod, &pre);
		if (err > 0) {
			puts("\t\tmodversions:");
			kmod_list_foreach(d, pre) {
				const char *symbol;
				uint64_t crc;
				symbol = kmod_module_version_get_symbol(d);
				crc = kmod_module_version_get_crc(d);
				printf("\t\t\t%s: %#"PRIx64"\n", symbol, crc);
			}
			kmod_module_versions_free_list(pre);
		}

		pre = NULL;
		err = kmod_module_get_symbols(mod, &pre);
		if (err > 0) {
			puts("\t\tsymbols:");
			kmod_list_foreach(d, pre) {
				const char *symbol;
				uint64_t crc;
				symbol = kmod_module_symbol_get_symbol(d);
				crc = kmod_module_symbol_get_crc(d);
				printf("\t\t\t%s: %#"PRIx64"\n", symbol, crc);
			}
			kmod_module_symbols_free_list(pre);
		}

		kmod_module_unref(mod);
	}

	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
