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
	const char *alias;
	struct kmod_ctx *ctx;
	struct kmod_list *list = NULL, *l;
	int err;

	printf("libkmod version %s\n", VERSION);

	if (argc < 2) {
		fprintf(stderr, "ERR: Provide an alias name\n");
		return EXIT_FAILURE;
	}

	alias = argv[1];

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_lookup(ctx, alias, &list);
	if (err < 0)
		goto fail_lookup;

	if (list == NULL)
		printf("No module matches '%s'\n", alias);
	else
		printf("Alias: '%s'\nModules matching:\n", alias);

	kmod_list_foreach(l, list) {
		struct kmod_module *mod = kmod_module_get_module(l);
		printf("\t%s\n", kmod_module_get_name(mod));
		kmod_module_unref(mod);
	}

	if (list != NULL) {
		struct kmod_list *filtered;
		err = kmod_module_get_filtered_blacklist(ctx, list, &filtered);
		if (err < 0) {
			printf("Could not filter: %s\n", strerror(-err));
			goto fail;
		}
		if (filtered == NULL)
			printf("All modules were filtered out!\n");
		else
			printf("Modules remaining after filter:\n");

		kmod_list_foreach(l, filtered) {
		struct kmod_module *mod = kmod_module_get_module(l);
		printf("\t%s\n", kmod_module_get_name(mod));
		kmod_module_unref(mod);
		}
		kmod_module_unref_list(filtered);
	}

	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return EXIT_SUCCESS;

fail:
	kmod_module_unref_list(list);
fail_lookup:
	kmod_unref(ctx);
	return EXIT_FAILURE;
}
