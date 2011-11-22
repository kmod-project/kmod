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
	struct kmod_loaded *mod;
	struct kmod_list *list, *itr;
	int err;

	ctx = kmod_new();
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	printf("libkmod version %s\n", VERSION);

	err = kmod_loaded_new(ctx, &mod);
	if (err < 0)
		exit(EXIT_FAILURE);

	err = kmod_loaded_get_list(mod, &list);
	if (err < 0) {
		fprintf(stderr, "%s\n", strerror(-err));
		exit(EXIT_FAILURE);
	}

	printf("Module                  Size  Used by\n");

	kmod_list_foreach(itr, list) {
		const char *name, *deps;
		long size;
		int use_count;
		kmod_loaded_get_module_info(itr, &name, &size, &use_count,
								&deps, NULL);
		printf("%-19s %8ld  %d %s\n", name, size,
					use_count, deps ? deps : "");
	}

	kmod_loaded_unref(mod);

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
