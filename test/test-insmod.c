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
	const char *path;
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	int err;

	if (argc < 2) {
		fprintf(stderr, "Provide a path to a module\n");
		return EXIT_FAILURE;
	}

	path = argv[1];

	ctx = kmod_new(NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	printf("libkmod version %s\n", VERSION);

	err = kmod_module_new_from_path(ctx, path, &mod);
	if (err < 0)
		exit(EXIT_FAILURE);

	printf("Trying insmod '%s'\n", path);
	err = kmod_module_insert_module(mod, 0);
	if (err < 0) {
		fprintf(stderr, "%s\n", strerror(-err));

		kmod_module_unref(mod);
		kmod_unref(ctx);
		exit(EXIT_FAILURE);
	}

	kmod_module_unref(mod);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
