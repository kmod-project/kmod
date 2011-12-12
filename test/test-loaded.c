#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <libkmod.h>


int main(int argc, char *argv[])
{
	struct kmod_ctx *ctx;
	struct kmod_list *list, *itr;
	int err;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	printf("libkmod version %s\n", VERSION);

	err = kmod_module_new_from_loaded(ctx, &list);
	if (err < 0) {
		fprintf(stderr, "%s\n", strerror(-err));
		kmod_unref(ctx);
		exit(EXIT_FAILURE);
	}

	printf("Module                  Size  Used by\n");

	kmod_list_foreach(itr, list) {
		struct kmod_module *mod = kmod_module_get_module(itr);
		const char *name = kmod_module_get_name(mod);
		int use_count = kmod_module_get_refcnt(mod);
		long size = kmod_module_get_size(mod);
		struct kmod_list *holders, *hitr;
		int first = 1;

		printf("%-19s %8ld  %d ", name, size, use_count);
		holders = kmod_module_get_holders(mod);
		kmod_list_foreach(hitr, holders) {
			struct kmod_module *hm = kmod_module_get_module(hitr);

			if (!first)
				putchar(',');
			else
				first = 0;

			fputs(kmod_module_get_name(hm), stdout);
			kmod_module_unref(hm);
		}
		putchar('\n');
		kmod_module_unref_list(holders);
		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
