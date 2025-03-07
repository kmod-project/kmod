// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2025 Intel Corporation
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>

#ifndef MODULE_SOFTDEP
#define MODULE_SOFTDEP(_softdep) MODULE_INFO(softdep, _softdep)
#endif

static int __init softdep_init(void)
{
	return 0;
}
module_init(softdep_init);

MODULE_AUTHOR("Dan He <dan.h.he@intel.com>");
MODULE_LICENSE("LGPL");
MODULE_SOFTDEP("pre: mod-foo-a mod-foo-b post: mod-foo-c");
