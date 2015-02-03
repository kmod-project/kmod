#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

void print_fooA(void);
void print_fooB(void);
void print_fooC(void);

static int __init foo_init(void)
{
	print_fooA();
	print_fooB();
	print_fooC();

	return 0;
}

module_init(foo_init);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
