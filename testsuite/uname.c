#include <errno.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "testsuite.h"

TS_EXPORT int uname(struct utsname *u)
{
	static void *nextlib = NULL;
	static int (*nextlib_uname)(struct utsname *u);
	const char *release = getenv(S_TC_UNAME_R);
	int err;
	size_t sz;

	if (release == NULL) {
		fprintf(stderr, "TRAP uname(): missing export %s?\n",
							S_TC_UNAME_R);
		errno = EFAULT;
		return -1;
	}

	if (nextlib == NULL) {
#ifdef RTLD_NEXT
		nextlib = RTLD_NEXT;
#else
		nextlib = dlopen("libc.so.6", RTLD_LAZY);
#endif
		nextlib_uname = dlsym(nextlib, "uname");
	}

	err = nextlib_uname(u);
	if (err < 0)
		return err;

	sz = strlen(release) + 1;
	if (sz > sizeof(u->release)) {
		fprintf(stderr, "uname(): sizeof release (%s) "
				"is greater than available space: %lu",
				release, sizeof(u->release));
		errno = -EFAULT;
		return -1;
	}

	memcpy(u->release, release, sz);
	return 0;
}
