#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

void print_fooB(void);

static int __init bar_init(void)
{
	print_fooB();

	return 0;
}

module_init(bar_init);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
