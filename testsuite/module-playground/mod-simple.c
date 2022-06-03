#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/module.h>

static struct dentry *debugfs_dir;

static int test_show(struct seq_file *s, void *data)
{
	seq_puts(s, "test");
	return 0;
}

DEFINE_SHOW_ATTRIBUTE(test);

static int __init test_module_init(void)
{
	debugfs_dir = debugfs_create_dir(KBUILD_MODNAME, NULL);
	debugfs_create_file("test", 0444, debugfs_dir, NULL, &test_fops);

	return 0;
}

static void test_module_exit(void)
{
	debugfs_remove_recursive(debugfs_dir);
}

module_init(test_module_init);
module_exit(test_module_exit);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("GPL");
