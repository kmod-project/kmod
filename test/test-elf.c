#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <libkmod.h>
#include <getopt.h>

int main(int argc, char *argv[])
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	struct kmod_list *list, *l;
	int err;

	printf("libkmod version %s\n", VERSION);

	if (argc != 2) {
		fprintf(stderr, "Usage:\n\t%s <module-path.ko>\n", argv[0]);
		return EXIT_FAILURE;
	}

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		return EXIT_FAILURE;

	err = kmod_module_new_from_path(ctx, argv[1], &mod);
	if (err < 0) {
		fprintf(stderr, "ERROR: could not load %s: %s\n",
			argv[1], strerror(-errno));
		goto module_error;
	}

	list = NULL;
	err = kmod_module_get_info(mod, &list);
	if (err <= 0)
		printf("no information! (%s)\n", strerror(-err));
	else {
		puts("info:");
		kmod_list_foreach(l, list) {
			const char *key, *val;
			key = kmod_module_info_get_key(l);
			val = kmod_module_info_get_value(l);
			printf("\t%s: %s\n", key, val ? val : "");
		}
		kmod_module_info_free_list(list);
	}

	list = NULL;
	err = kmod_module_get_versions(mod, &list);
	if (err <= 0)
		printf("no modversions! (%s)\n", strerror(-err));
	else {
		puts("modversions:");
		kmod_list_foreach(l, list) {
			const char *symbol;
			uint64_t crc;
			symbol = kmod_module_version_get_symbol(l);
			crc = kmod_module_version_get_crc(l);
			printf("\t%s: %#"PRIx64"\n", symbol, crc);
		}
		kmod_module_versions_free_list(list);
	}

	list = NULL;
	err = kmod_module_get_symbols(mod, &list);
	if (err <= 0)
		printf("no symbols! (%s)\n", strerror(-err));
	else {
		puts("symbols:");
		kmod_list_foreach(l, list) {
			const char *symbol;
			uint64_t crc;
			symbol = kmod_module_symbol_get_symbol(l);
			crc = kmod_module_symbol_get_crc(l);
			printf("\t%s: %#"PRIx64"\n", symbol, crc);
		}
		kmod_module_symbols_free_list(list);
	}

	list = NULL;
	err = kmod_module_get_dependency_symbols(mod, &list);
	if (err <= 0)
		printf("no dependency symbols! (%s)\n", strerror(-err));
	else {
		puts("dependency symbols:");
		kmod_list_foreach(l, list) {
			const char *symbol;
			uint8_t bind;
			uint64_t crc;
			symbol = kmod_module_dependency_symbol_get_symbol(l);
			bind = kmod_module_dependency_symbol_get_bind(l);
			crc = kmod_module_dependency_symbol_get_crc(l);
			printf("\t%s %c: %#"PRIx64"\n", symbol, bind, crc);
		}
		kmod_module_dependency_symbols_free_list(list);
	}

	kmod_module_unref(mod);
module_error:
	kmod_unref(ctx);

	return (err < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
