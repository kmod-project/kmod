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
	struct kmod_list *list, *l;
	int err;

	printf("libkmod version %s\n", VERSION);

	if (argc < 2) {
		fprintf(stderr, "ERR: Provide a module name\n");
		return EXIT_FAILURE;
	}

	name = argv[1];

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_name(ctx, name, &mod);
	if (err < 0) {
		kmod_unref(ctx);
		exit(EXIT_FAILURE);
	}

	list = kmod_module_get_dependencies(mod);
	printf("Module: %s\nDependency list:\n", name);

	kmod_list_foreach(l, list) {
		struct kmod_module *m = kmod_module_get_module(l);
		printf("\t%s\n", kmod_module_get_name(m));
		kmod_module_unref(m);
	}

	kmod_module_unref_list(list);
	kmod_module_unref(mod);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
