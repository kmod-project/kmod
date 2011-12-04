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
	struct kmod_module *mod;
	int err;

	if (argc < 2) {
		fprintf(stderr, "Provide a module name\n");
		return EXIT_FAILURE;
	}

	modname = argv[1];

	ctx = kmod_new(NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	printf("libkmod version %s\n", VERSION);

	err = kmod_module_new_from_name(ctx, modname, &mod);
	if (err < 0) {
		kmod_unref(ctx);
		exit(EXIT_FAILURE);
	}

	printf("Trying to remove '%s'\n", modname);
	kmod_module_remove_module(mod, 0);

	kmod_module_unref(mod);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
