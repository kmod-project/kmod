#ifndef _LIBKMOD_TESTSUITE_
#define _LIBKMOD_TESTSUITE_

#include <stdbool.h>
#include <stdarg.h>

struct test;
typedef int (*testfunc)(const struct test *t);

enum test_config {
	TC_ROOTFS = 0,
	TC_UNAME_R,
	TC_INIT_MODULE_RETCODES,
	TC_DELETE_MODULE_RETCODES,
	_TC_LAST,
};

#define S_TC_ROOTFS "TESTSUITE_ROOTFS"
#define S_TC_UNAME_R "TESTSUITE_UNAME_R"
#define S_TC_INIT_MODULE_RETCODES "TESTSUITE_INIT_MODULE_RETCODES"
#define S_TC_DELETE_MODULE_RETCODES "TESTSUITE_DELETE_MODULE_RETCODES"


struct test {
	const char *name;
	const char *description;
	struct {
		const char *stdout;
		const char *stderr;
	} output;
	testfunc func;
	const char *config[_TC_LAST];
	bool need_spawn;
};


const struct test *test_find(const struct test *tests[], const char *name);
int test_init(int argc, char *const argv[], const struct test *tests[]);
int test_spawn_prog(const char *prog, const char *const args[]);

int test_run(const struct test *t);

#define TS_EXPORT __attribute__ ((visibility("default")))

#define _LOG(prefix, fmt, ...) printf("TESTSUITE: " prefix fmt, ## __VA_ARGS__)
#define LOG(fmt, ...) _LOG("", fmt, ## __VA_ARGS__)
#define WARN(fmt, ...) _LOG("WARN: ", fmt, ## __VA_ARGS__)
#define ERR(fmt, ...) _LOG("ERR: ", fmt, ## __VA_ARGS__)

/* Test definitions */
#define DEFINE_TEST(_name) \
	struct test s_name = { \
		.name = #_name, \
		.func = _name, \
	}

#endif
