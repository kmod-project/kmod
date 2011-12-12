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
	const char *null_config = NULL;
	struct kmod_ctx *ctx;
	struct kmod_list *list, *itr;
	int err, count = 0;

	if (argc == 2)
		modname = argv[1];

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	printf("libkmod version %s\n", VERSION);

	err = kmod_module_new_from_loaded(ctx, &list);
	if (err < 0) {
		fprintf(stderr, "%s\n", strerror(-err));
		kmod_unref(ctx);
		exit(EXIT_FAILURE);
	}

	kmod_list_foreach(itr, list) {
		struct kmod_module *mod = kmod_module_get_module(itr);
		const char *name = kmod_module_get_name(mod);

		if ((modname && !strcmp(modname, name)) ||
		    (modname == NULL && kmod_module_get_refcnt(mod) < 1)) {
			printf("Trying to remove '%s'\n", name);
			err = kmod_module_remove_module(mod, 0);
			if (err == 0)
				count++;
			else {
				fprintf(stderr, "Error removing %s: %s\n",
					name, strerror(-err));
			}
		}

		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return count > 0;
}
