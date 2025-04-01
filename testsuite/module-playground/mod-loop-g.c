#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "mod-loop.h"

static int __init test_module_init(void)
{
	printG();

	return 0;
}
module_init(test_module_init);

void printG(void)
{
	pr_warn("Hello, world G\n");
}
EXPORT_SYMBOL(printG);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
