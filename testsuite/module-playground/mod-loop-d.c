#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "mod-loop.h"

static int __init test_module_init(void)
{
	printD();
	printE();

	return 0;
}
module_init(test_module_init);

void printD(void)
{
	pr_warn("Hello, world D\n");
}
EXPORT_SYMBOL(printD);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
