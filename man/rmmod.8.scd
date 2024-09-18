RMMOD(8) "kmod" "rmmod"

# NAME

rmmod - Simple program to remove a module from the Linux Kernel

# SYNOPSIS

*rmmod* [_OPTIONS_] [_modulename_]

# DESCRIPTION

*rmmod* is a trivial program to remove a module (when module unloading support
is provided) from the kernel. Most users will want to use *modprobe*(8) with the
*-r* option instead since it removes unused dependent modules as well.

# OPTIONS

*-f*
*--force*
	This option can be extremely dangerous: it has no effect unless
	CONFIG_MODULE_FORCE_UNLOAD was set when the kernel was compiled. With
	this option, you can remove modules which are being used, or which are
	not designed to be removed, or have been marked as unsafe (see *lsmod*(8)).

*-s*
*--syslog*
	Send errors to syslog instead of standard error.

*-v*
*--verbose*
	Print messages about what the program is doing. Usually *rmmod* prints
	messages only if something goes wrong.

*-V*
*--version*
	Show version of program and exit.

*-h*
*--help*
	Print the help message and exit.

# COPYRIGHT

This manual page originally Copyright 2002, Rusty Russell, IBM Corporation.

# SEE ALSO

*modprobe*(8), *insmod*(8), *lsmod*(8), *modinfo*(8), *depmod*(8)

# AUTHORS

Numerous contributions have come from the linux-modules mailing list
<linux-modules@vger.kernel.org> and Github. If you have a clone of kmod.git
itself, the output of *git-shortlog*(1) and *git-blame*(1) can show you the
authors for specific parts of the project.

*Lucas De Marchi* <lucas.de.marchi@gmail.com> is the current maintainer of the
project.
