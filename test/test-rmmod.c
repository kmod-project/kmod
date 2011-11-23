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
	const char *modname = NULL;
	struct kmod_ctx *ctx;
	struct kmod_loaded *mod;
	struct kmod_list *list, *itr;
	int err;

	if (argc == 2)
		modname = argv[1];

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

	kmod_list_foreach(itr, list) {
		const char *name;
		int use_count;

		kmod_loaded_get_module_info(itr, &name, NULL, &use_count,
								NULL, NULL);

		if ((modname && !strcmp(modname, name)) ||
					(modname == NULL && use_count == 0)) {
			printf("Trying to remove '%s'\n", name);
			kmod_loaded_remove_module(mod, itr, 0);
			break;
		}
	}

	kmod_loaded_unref(mod);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
