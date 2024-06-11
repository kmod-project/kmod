RMMOD(8) "kmod" "rmmod"

# NAME

rmmod - Simple program to remove a module from the Linux Kernel

# SYNOPSIS

*rmmod* [*-f*] [*-s*] [*-v*] [_modulename_]

# DESCRIPTION

*rmmod* is a trivial program to remove a module (when module unloading support
is provided) from the kernel. Most users will want to use *modprobe*(8) with the
*-r* option instead since it removes unused dependent modules as well.

# OPTIONS

*-v*, *--verbose*
	Print messages about what the program is doing. Usually *rmmod* prints
	messages only if something goes wrong.

*-f*, *--force*
	This option can be extremely dangerous: it has no effect unless
	CONFIG_MODULE_FORCE_UNLOAD was set when the kernel was compiled. With
	this option, you can remove modules which are being used, or which are
	not designed to be removed, or have been marked as unsafe (see *lsmod*(8)).

*-s*, *--syslog*
	Send errors to syslog instead of standard error.

*-V --version*
	Show version of program and exit.

# COPYRIGHT

This manual page originally Copyright 2002, Rusty Russell, IBM Corporation.
Maintained by Jon Masters and others.

# SEE ALSO

*modprobe*(8), *insmod*(8), *lsmod*(8), *modinfo*(8) *depmod*(8)

# AUTHORS

*Jon Masters* <jcm@jonmasters.org>
	Developer

*Lucas De Marchi* <lucas.de.marchi@gmail.com>
	Developer
