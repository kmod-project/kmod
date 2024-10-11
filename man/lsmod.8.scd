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

# COPYRIGHT

This manual page originally Copyright 2002, Rusty Russell, IBM Corporation.

# SEE ALSO

*insmod*(8), *modprobe*(8), *modinfo*(8), *depmod*(8)

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
