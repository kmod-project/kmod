#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void usage(const char *progname)
{
	printf("%s <path> <relative-to-path>\n", progname);
}

/*
 * path must be absolute path, as the result of calling realpath()
 */
static void split_path(const char *path, char d[PATH_MAX], size_t *dlen,
						char n[NAME_MAX], size_t *nlen)
{
	size_t dirnamelen, namelen;
	char *p;

	p = strrchr(path, '/'); /* p is never NULL since path is abs */
	dirnamelen = p - path;
	if (dirnamelen > 0)
		memcpy(d, path, dirnamelen);
	d[dirnamelen] = '\0';

	namelen = strlen(p + 1);
	if (namelen > 0)
		memcpy(n, p + 1, namelen + 1);

	if (dlen)
		*dlen = dirnamelen;
	if (nlen)
		*nlen = namelen;
}

int main(int argc, char *argv[])
{
	size_t sympathlen, namelen, pathlen, i, j;
	char name[NAME_MAX], path[PATH_MAX];
	char sympath[PATH_MAX];
	char buf[PATH_MAX];
	char *p;

	if (argc < 3) {
		usage(basename(argv[0]));
		return EXIT_FAILURE;
	}

	p = realpath(argv[1], buf);
	if (p == NULL) {
		fprintf(stderr, "could not get realpath of %s: %m\n", argv[1]);
		return EXIT_FAILURE;
	}
	split_path(buf, path, &pathlen, name, &namelen);

	p = realpath(argv[2], sympath);
	if (p == NULL) {
		fprintf(stderr, "could not get realpath of %s: %m\n", argv[2]);
		return EXIT_FAILURE;
	}
	sympathlen = strlen(sympath);

	for (i = 1; i < sympathlen && i < pathlen; i++) {
		if (sympath[i] == path[i])
			continue;
		break;
	}

	if (i < sympathlen) {
		while (sympath[i - 1] != '/')
			i--;
	}

	p = &path[i];

	j = 0;
	if (i < sympathlen) {
		memcpy(buf + j, "../", 3);
		j += 3;
	}

	for (; i < sympathlen; i++) {
		if (sympath[i] != '/')
			continue;

		memcpy(buf + j, "../", 3);
		j += 3;
	}

	if (p != path + pathlen) {
		memcpy(buf + j, p, path + pathlen - p);
		j += path + pathlen - p;;
		buf[j++] = '/';
	}
	memcpy(buf + j, name, namelen + 1);

	printf("%s\n", buf);
	return 0;
}
