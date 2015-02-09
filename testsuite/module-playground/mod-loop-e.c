#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "mod-loop.h"

static int __init test_module_init(void)
{
	printE();
	printD();

	return 0;
}
module_init(test_module_init);

void printE(void)
{
	pr_warn("Hello, world E\n");
}
EXPORT_SYMBOL(printE);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
