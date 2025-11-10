LSMOD(8) "kmod" "lsmod"

# NAME

lsmod - Show the status of modules in the Linux Kernel

# SYNOPSIS

*lsmod* [_OPTIONS_]

# DESCRIPTION

*lsmod* is a trivial program which nicely formats the contents of the
_/proc/modules_, showing what kernel modules are currently loaded.

# OPTIONS

*-s*, *--syslog*
	Send errors to syslog instead of standard error.

*-v*, *--verbose*
	Print messages about what the program is doing. Usually *lsmod* prints
	messages only if something goes wrong.

*-V*, *--version*
	Show version of program and exit.

*-h*, *--help*
	Print the help message and exit.

# SEE ALSO

*insmod*(8), *modprobe*(8), *modinfo*(8), *depmod*(8)
