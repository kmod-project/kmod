#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "mod-loop.h"

static int __init test_module_init(void)
{
	printA();
	printB();
	printF();
	printG();

	return 0;
}
module_init(test_module_init);

void printA(void)
{
	pr_warn("Hello, world A\n");
}
EXPORT_SYMBOL(printA);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
