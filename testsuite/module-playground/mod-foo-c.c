#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

static int __init foo_init(void)
{
	return 0;
}
module_init(foo_init);

void print_fooC(void)
{
	pr_warn("fooC\n");
}
EXPORT_SYMBOL(print_fooC);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
