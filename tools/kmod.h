/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#pragma once

#include <stdio.h>

#include <shared/macro.h>

struct kmod_cmd {
	const char *name;
	int (*cmd)(int argc, char *argv[]);
	const char *help;
};

extern const struct kmod_cmd kmod_cmd_compat_lsmod;
extern const struct kmod_cmd kmod_cmd_compat_rmmod;
extern const struct kmod_cmd kmod_cmd_compat_insmod;
extern const struct kmod_cmd kmod_cmd_compat_modinfo;
extern const struct kmod_cmd kmod_cmd_compat_modprobe;
extern const struct kmod_cmd kmod_cmd_compat_depmod;

extern const struct kmod_cmd kmod_cmd_insert;
extern const struct kmod_cmd kmod_cmd_list;
extern const struct kmod_cmd kmod_cmd_static_nodes;
extern const struct kmod_cmd kmod_cmd_remove;

static inline void kmod_version(void)
{
	puts(PACKAGE " version " VERSION);
	puts(KMOD_FEATURES);
}

#include "log.h"
