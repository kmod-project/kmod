#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

void print_fooB(void);

static int __init foo_init(void)
{
	return 0;
}
module_init(foo_init);

void print_fooB(void)
{
	pr_warn("fooB\n");
}
EXPORT_SYMBOL(print_fooB);

MODULE_AUTHOR("Lucas De Marchi <demarchi@kernel.org>");
MODULE_LICENSE("LGPL");
MODULE_DESCRIPTION("dummy test module");
