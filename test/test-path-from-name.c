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
	struct kmod_module *mod;
	const char *path, *modname;
	int err;

	if (argc < 2) {
		fprintf(stderr, "ERR: Provide an alias name\n");
		return EXIT_FAILURE;
	}

	modname = argv[1];

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	printf("libkmod version %s\n", VERSION);

	err = kmod_module_new_from_name(ctx, modname, &mod);
	if (err < 0) {
		fprintf(stderr, "error creating module: '%s'\n", strerror(-err));
		goto fail;
	}

	path = kmod_module_get_path(mod);

	printf("modname: '%s'  path: '%s'\n", modname, path);
	kmod_module_unref(mod);
	kmod_unref(ctx);

	return EXIT_SUCCESS;

fail:
	kmod_unref(ctx);
	return EXIT_FAILURE;
}
