#include <linux/init.h>
#include <linux/module.h>

extern void dummy_export(void);

static int __init test_module_init(void)
{
	dummy_export();
	return 0;
}

static void test_module_exit(void)
{
}
module_init(test_module_init);
module_exit(test_module_exit);

MODULE_AUTHOR("Lucas De Marchi <lucas.demarchi@intel.com>");
MODULE_LICENSE("LGPL");

MODULE_ALIAS("pci:v0000103Cd0000323Asv0000103Csd00003241bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Asv0000103Csd00003243bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Asv0000103Csd00003245bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Asv0000103Csd00003247bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Asv0000103Csd00003249bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Asv0000103Csd0000324Abc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Asv0000103Csd0000324Bbc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Asv0000103Csd00003233bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Bsv0000103Csd00003350bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Bsv0000103Csd00003351bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Bsv0000103Csd00003352bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Bsv0000103Csd00003353bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Bsv0000103Csd00003354bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Bsv0000103Csd00003355bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd0000323Bsv0000103Csd00003356bc*sc*i*");
MODULE_ALIAS("pci:v0000103Cd*sv*sd*bc01sc04i*");
