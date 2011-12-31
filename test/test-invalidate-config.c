#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <libkmod.h>

static const char *config[] = {
	NULL,
	NULL,
};

int main(int argc, char *argv[])
{
	struct kmod_ctx *ctx;
	int r;
	char cmd[4096];

	if (argc < 2) {
		fprintf(stderr, "Provide a path to config\n");
		return EXIT_FAILURE;
	}

	config[0] = argv[1];

	ctx = kmod_new(NULL, config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	r = kmod_validate_resources(ctx);
	if (r != KMOD_RESOURCES_OK) {
		fprintf(stderr, "ERR: return should be 'resources ok'\n");
		return EXIT_FAILURE;
	}

	snprintf(cmd, sizeof(cmd), "touch %s", config[0]);
	system(cmd);
	r = kmod_validate_resources(ctx);
	if (r != KMOD_RESOURCES_MUST_RECREATE) {
		fprintf(stderr, "ERR: return should be 'must recreate'\n");
		return EXIT_FAILURE;
	}

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
