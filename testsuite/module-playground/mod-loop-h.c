#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "mod-loop.h"

static int __init test_module_init(void)
{
	printH();
	printI();

	return 0;
}
module_init(test_module_init);

void printH(void)
{
	pr_warn("Hello, world H\n");
}
EXPORT_SYMBOL(printH);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
