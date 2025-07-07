// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2025 Intel Corporation
 */
#include <linux/init.h>
#include <linux/module.h>

static int __init softdep_init(void)
{
	return 0;
}
module_init(softdep_init);

MODULE_AUTHOR("Dan He <dan.h.he@intel.com>");
MODULE_LICENSE("LGPL");
MODULE_DESCRIPTION("dummy test module");
