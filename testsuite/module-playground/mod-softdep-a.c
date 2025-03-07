#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>

#ifndef MODULE_SOFTDEP
#define MODULE_SOFTDEP(_softdep) MODULE_INFO(softdep, _softdep)
#endif // #ifndef MODULE_SOFTDEP

static int __init softdep_init(void)
{
	return 0;
}
module_init(softdep_init);

MODULE_AUTHOR("Dan He <dan.h.he@intel.com>");
MODULE_LICENSE("LGPL");
MODULE_SOFTDEP("pre: mod-foo-a mod-foo-b post: mod-foo-c");
