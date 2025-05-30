INSMOD(8) "kmod" "insmod"

# NAME

insmod - Simple program to insert a module into the Linux Kernel

# SYNOPSIS

*insmod* [_OPTIONS_] [_filename_] [_module options_]

# DESCRIPTION

*insmod* is a trivial program to insert a module into the kernel. Most users
will want to use *modprobe*(8) instead, which is more clever and can handle
module dependencies.

Only the most general of error messages are reported: as the work of trying to
link the module is now done inside the kernel, the *dmesg*(1) usually gives more
information about errors.

# OPTIONS

*-f*, *--force*
	This option can be extremely dangerous: it tells the kernel to ignore
	the module version and vermagic fields when loading. With this option,
	you can load modules build locally or by third parties, although this
	can lead to memory corruption, system crashes and data loss. This is
	the same as using both *--force-vermagic* and *--force-modversion*.

*--force-modversion*
	When modules are compiled with CONFIG_MODVERSIONS set, a section
	detailing the versions of every interfaced used by (or supplied by) the
	module is created. If a module fails to load and the kernel complains
	that the module disagrees about a version of some interface, you can use
	*--force-modversion* to remove the version information altogether.

*--force-vermagic*
	Every module contains a small string containing important information,
	such as the kernel and compiler versions. If a module fails to load and
	the kernel complains that the "version magic" doesn't match, you can use
	this option to remove it. Naturally, this check is there for your
	protection, so using this option is dangerous unless you know what
	you're doing.

*-s*, *--syslog*
	Send errors to syslog instead of standard error.

*-v*, *--verbose*
	Print messages about what the program is doing. Usually *insmod* prints
	messages only if something goes wrong.

*-V*, *--version*
	Show version of program and exit.

*-h*, *--help*
	Print the help message and exit.

# COPYRIGHT

This manual page originally Copyright 2002, Rusty Russell, IBM Corporation.

# SEE ALSO

*modprobe*(8), *rmmod*(8), *lsmod*(8), *modinfo*(8), *depmod*(8)

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
