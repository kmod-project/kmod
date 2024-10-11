KMOD(8) "kmod" "kmod"

# NAME

kmod - Program to manage Linux Kernel modules

# SYNOPSIS

*kmod* [*OPTIONS...*] [_COMMAND_] [*COMMAND_OPTIONS...*]

# DESCRIPTION

*kmod* is a multi-call binary which implements the programs used to control
Linux Kernel modules. Most users will only run it using its other names.

# OPTIONS

*-V*, *--version*
	Show the program version and exit.

*-h*, *--help*
	Show the help message.

# COMMANDS

*help*
	Show the help message.

*list*
	List the currently loaded modules.

*static-nodes*
	Output the static device nodes information provided by the modules of
	the currently running kernel version.

# COPYRIGHT

This manual page originally Copyright 2014, Marco d'Itri.

# SEE ALSO

*lsmod*(8), *rmmod*(8), *insmod*(8), *modinfo*(8), *modprobe*(8), *depmod*(8)

# BUGS

Please direct any bug reports to kmod's issue tracker at
https://github.com/kmod-project/kmod/issues/ alongside with version used, steps
to reproduce the problem and the expected outcome.

# AUTHOR

*kmod* project was started by Lucas De Marchi as a drop-in replacement to
module-init-tools that was maintained by Jon Masters, adding a library (libkmod)
and additional features.

Numerous contributions have come from the linux-modules mailing list
<linux-modules@vger.kernel.org> and Github. If you have a clone of kmod.git
itself, the output of *git-shortlog*(1) and *git-blame*(1) can show you the
authors for specific parts of the project.

*Lucas De Marchi* <lucas.de.marchi@gmail.com> is the current maintainer of the
project.
