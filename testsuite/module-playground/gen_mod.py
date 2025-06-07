#! /usr/bin/python3
# This script generates source code for fake kernel modules,
# with dependencies defined by functions exported and called.

class Module:
    # expansions in f-strings can't contain backslashes
    _nl = "\n"
    _nlt = "\n\t"

    # module file name prefix
    _prefix = "ex"

    # Constructor
    # name: module name
    # provides: list of exported "functions"
    # calls: list of called "functions"
    def __init__(self, name, provides=[], calls=[], int_provides=[], int_calls=[]):
        self._name = name
        self._symname = self._name.replace("-", "_")
        self._provides = provides
        self._calls = calls
        self._int_provides = int_provides
        self._int_calls = int_calls

    # symbol names must be mangled to avoid clashes with the kernel
    def mangled(self, name):
        return name + "_mangled"

    # void f(void): declaration, body, call
    def void_decl(self, name):
        return f"void {self.mangled(name)}(void);"

    def void_body(self, name):
        return f"""\
void {self.mangled(name)}(void)
{{
        pr_warn("%s\\n", __func__);
}}
EXPORT_SYMBOL({self.mangled(name)});"""

    def void_call(self, name):
        return f"{self.mangled(name)}();"

    # void f(int): declaration, body, call
    def int_decl(self, name):
        return f"void {self.mangled(name)}(int);"

    def int_body(self, name):
        return f"""\
void {self.mangled(name)}(int x)
{{
        pr_warn("%s: %d\\n", __func__, x);
}}
EXPORT_SYMBOL({self.mangled(name)});"""

    def int_call(self, name):
        return f"{self.mangled(name)}(0);"

    # str() creates the C source code.
    def __str__(self):
        return f"""\
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

{self._nl.join(self.void_decl(x) for x in self._calls + self._provides)}
{self._nl.join(self.int_decl(x) for x in self._int_calls + self._int_provides)}
{self._nl.join(self.void_body(x) for x in self._provides)}
{self._nl.join(self.int_body(x) for x in self._int_provides)}

static int __init {self._symname}_init(void)
{{
        {self._nlt.join(self.void_call(x) for x in self._calls)}
        {self._nlt.join(self.int_call(x) for x in self._int_calls)}
        return 0;
}}
module_init({self._symname}_init);

MODULE_AUTHOR("Martin Wilck <mwilck@suse.com>");
MODULE_LICENSE("GPL");
"""

    # write() writes the C source code to a suitably named C file
    def write(self):
        with open(f"{self._prefix}-{self._name}.c", "w") as out:
            out.write(str(self))


# The following example creates a set of modules inspired by some real-world
# block and SCSI modules.
if __name__ == "__main__":
    import sys

    if len(sys.argv) == 2 and sys.argv[1] == "other":
        q_mods = [
            Module("qla2xxx",
                   int_provides = ["qlt_stop_phase2"],
                   calls = ["fc_attach_transport", "nvme_fc_set_remoteport_devloss",
                            "scsi_remove_host"]),
            Module("tcm_qla2xxx",
                   int_calls = [ "qlt_stop_phase2" ],
                   calls=["target_execute_cmd",
                          "scsi_host_get", "fc_vport_create"])
            ]
    else:
        q_mods = [
            Module("qla2xxx",
                   provides = ["qlt_stop_phase2"],
                   calls = ["fc_attach_transport", "nvme_fc_set_remoteport_devloss",
                            "scsi_remove_host"]),
            Module("tcm_qla2xxx",
                   calls=["target_execute_cmd", "qlt_stop_phase2",
                          "scsi_host_get", "fc_vport_create"])
            ]

    mods = [
        Module("scsi_transport_fc",
               provides = ["fc_attach_transport", "fc_vport_create"],
               calls = ["scsi_target_block"]),
        Module("scsi_mod",
               provides = ["scsi_target_block", "scsi_host_get",
                           "scsi_dma_map", "scsi_mode_sense",
                           "scsi_is_sdev_device", "scsi_print_sense_hdr",
                           "__scsi_iterate_devices", "scsi_dh_attach",
                           "scsi_remove_host"]),
        Module("nvme-fc",
               provides = ["nvme_fc_set_remoteport_devloss"],
               calls=["nvme_uninit_ctrl",  "nvmf_fail_nonready_command"]),
        Module("nvme-fabrics",
               provides = ["nvmf_fail_nonready_command"],
               calls=["nvme_complete_rq"]),
        Module("nvme-core",
               provides = ["nvme_complete_rq", "nvme_uninit_ctrl"],
               calls=["t10_pi_type1_crc"]),
        Module("t10-pi",
               provides = ["t10_pi_type1_crc"]),
        Module("scsi_dh_alua",
               calls=["scsi_print_sense_hdr"]),
        Module("target_core_mod",
               provides = ["target_execute_cmd", "target_backend_unregister"],
               calls=["config_group_init"]),
        Module("target_core_iblock",
               calls=["target_backend_unregister"]),
        Module("configfs",
               provides = ["config_group_init"]),
        Module("mpt3sas",
               calls=["sas_enable_tlr", "scsi_dma_map", "raid_class_attach"]),
        Module("scsi_transport_sas",
               provides = ["sas_enable_tlr", "scsi_is_sas_rphy"],
               calls=["scsi_mode_sense"]),
        Module("raid_class",
               provides = ["raid_class_attach"],
               calls=["scsi_is_sdev_device"]),
        Module("sd_mod",
               calls=["scsi_print_sense_hdr", "t10_pi_type1_crc"]),
        Module("sr_mod",
               calls=["scsi_mode_sense", "register_cdrom"]),
        Module("cdrom",
               provides = ["register_cdrom"]),
        Module("ses",
               calls=["__scsi_iterate_devices", "scsi_is_sas_rphy",
                      "enclosure_remove_device"]),
        Module("enclosure",
               provides = ["enclosure_remove_device"]),
        Module("dm-multipath",
               provides = ["dm_unregister_path_selector"],
               calls=["dm_table_run_md_queue_async", "scsi_dh_attach"]),
        Module("dm-service-time",
               calls=["dm_unregister_path_selector"]),
        Module("dm-mod",
               provides = ["dm_table_run_md_queue_async"]),
    ]

    for x in mods + q_mods:
        x.write()

    with open("gen_mod.mak", "w") as out:
        out.write("".join(f"obj-m += {x._prefix}-{x._name}.o\n"
                          for x in mods + q_mods))
