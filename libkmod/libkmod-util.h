#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#include <shared/macro.h>

#define KMOD_EXT_UNC 0

extern const struct kmod_ext {
	const char *ext;
	size_t len;
} kmod_exts[];

int alias_normalize(const char *alias, char buf[PATH_MAX], size_t *len) _must_check_ __attribute__((nonnull(1,2)));
char *modname_normalize(const char *modname, char buf[PATH_MAX], size_t *len) __attribute__((nonnull(1, 2)));
char *path_to_modname(const char *path, char buf[PATH_MAX], size_t *len) __attribute__((nonnull(2)));
bool path_ends_with_kmod_ext(const char *path, size_t len) __attribute__((nonnull(1)));
