RMMOD(8) "kmod" "rmmod"

# NAME

rmmod - Simple program to remove a module from the Linux Kernel

# SYNOPSIS

*rmmod* [_OPTIONS_] [_list of modulenames_]

# DESCRIPTION

*rmmod* is a trivial program to remove a module or a list of modules from the
kernel (when module unloading support is provided). Most users will want to use
*modprobe*(8) with the *-r* option instead since it removes unused dependent
modules as well.

When a list of modules is provided, the program will process them one at a time.
If a module is not found, *rmmod* will immediately exit with an error code.
Should the module removal fail, the program will log an error AND continue with
the next module. This behaviour is NOT controlled by the *--force* option.

# OPTIONS

*-f*, *--force*
	This option can be extremely dangerous: it has no effect unless
	CONFIG_MODULE_FORCE_UNLOAD was set when the kernel was compiled. With
	this option, you can remove modules which are being used, or which are
	not designed to be removed, or have been marked as unsafe (see *lsmod*(8)).

*-s*, *--syslog*
	Send errors to syslog instead of standard error.

*-v*, *--verbose*
	Print messages about what the program is doing. Usually *rmmod* prints
	messages only if something goes wrong.

*-V*, *--version*
	Show version of program and exit.

*-h*, *--help*
	Print the help message and exit.

# COPYRIGHT

This manual page originally Copyright 2002, Rusty Russell, IBM Corporation.

# SEE ALSO

*modprobe*(8), *insmod*(8), *lsmod*(8), *modinfo*(8), *depmod*(8)

# BUGS

Please direct any bug reports to kmod's issue tracker at
https://github.com/kmod-project/kmod/issues/ alongside with version used, steps
to reproduce the problem and the expected outcome.

# AUTHORS

Numerous contributions have come from the linux-modules mailing list
<linux-modules@vger.kernel.org> and Github. If you have a clone of kmod.git
itself, the output of *git-shortlog*(1) and *git-blame*(1) can show you the
authors for specific parts of the project.

*Lucas De Marchi* <lucas.de.marchi@gmail.com> is the current maintainer of the
project.
