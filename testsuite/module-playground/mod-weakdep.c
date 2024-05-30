#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>

#ifndef MODULE_WEAKDEP
#define MODULE_WEAKDEP(_weakdep) MODULE_INFO(weakdep, _weakdep)
#endif

static int __init weakdep_init(void)
{
	return 0;
}

module_init(weakdep_init);

MODULE_AUTHOR("Jose Ignacio Tornos Martinez <jtornosm@redhat.com>");
MODULE_LICENSE("LGPL");
MODULE_WEAKDEP("mod-simple");
