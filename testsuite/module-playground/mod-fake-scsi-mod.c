#include <linux/init.h>
#include <linux/module.h>

static int __init test_module_init(void)
{
	return 0;
}

static void test_module_exit(void)
{
}
module_init(test_module_init);
module_exit(test_module_exit);

void dummy_export(void)
{
}
EXPORT_SYMBOL(dummy_export);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");
