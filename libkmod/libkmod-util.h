#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#include <shared/macro.h>

#define KMOD_EXTENSION_UNCOMPRESSED ".ko"

char *modname_normalize(const char *modname, char buf[PATH_MAX], size_t *len) __attribute__((nonnull(1, 2)));
char *path_to_modname(const char *path, char buf[PATH_MAX], size_t *len) __attribute__((nonnull(2)));
bool path_ends_with_kmod_ext(const char *path, size_t len) __attribute__((nonnull(1)));
