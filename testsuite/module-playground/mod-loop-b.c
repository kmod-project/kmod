#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "mod-loop.h"

static int __init test_module_init(void)
{
	printB();
	printC();
	printF();
	printG();

	return 0;
}
module_init(test_module_init);

void printB(void)
{
	pr_warn("Hello, world B\n");
}
EXPORT_SYMBOL(printB);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
MODULE_DESCRIPTION("dummy test module");
