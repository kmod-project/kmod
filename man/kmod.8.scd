KMOD(8) "kmod" "kmod"

# NAME

kmod - Program to manage Linux Kernel modules

# SYNOPSIS

*kmod* [*OPTIONS*...] [_COMMAND_] [*COMMAND_OPTIONS*...]

# DESCRIPTION

*kmod* is a multi-call binary which implements the programs used to control
Linux Kernel modules. Most users will only run it using its other names.

# OPTIONS

*-V --version*
	Show the program version and exit.

*-h --help*
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

This manual page originally Copyright 2014, Marco d'Itri. Maintained by Lucas De
Marchi and others.

# SEE ALSO

*lsmod*(8), *rmmod*(8), *insmod*(8), *modinfo*(8), *modprobe*(8), *depmod*(8)

# AUTHOR

*Lucas De Marchi* <lucas.de.marchi@gmail.com>
	Developer
