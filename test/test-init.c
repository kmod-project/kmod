#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <libkmod.h>


int main(int argc, char *argv[])
{
	struct kmod_ctx *ctx;

	ctx = kmod_new();
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	printf("libkmod version %s\n", VERSION);

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
