#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "mod-loop.h"

static int __init test_module_init(void)
{
	printK();
	printH();

	return 0;
}
module_init(test_module_init);

void printK(void)
{
	pr_warn("Hello, world K\n");
}
EXPORT_SYMBOL(printK);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
