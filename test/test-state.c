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
	const char *name;
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	int err;
	const char *state;

	if (argc < 2) {
		fprintf(stderr, "Provide a module name\n");
		return EXIT_FAILURE;
	}

	name = argv[1];

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	printf("libkmod version %s\n", VERSION);

	err = kmod_module_new_from_name(ctx, name, &mod);
	if (err < 0) {
		kmod_unref(ctx);
		exit(EXIT_FAILURE);
	}

	state = kmod_module_initstate_str(kmod_module_get_initstate(mod));
	if (state == NULL)
		printf("Module '%s' not found.\n", name);
	else
		printf("Module '%s' is in '%s' state.\n", name, state);


	kmod_module_unref(mod);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
