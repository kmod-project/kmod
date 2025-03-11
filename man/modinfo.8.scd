MODINFO(8) "kmod" "modinfo"

# NAME

modinfo - Show information about a Linux Kernel module

# SYNOPSIS

*modinfo* [*-0*] [*-F* _field_] [*-k* _kernel_] [modulename|filename...]

*modinfo* *-V*

*modinfo* *-h*

# DESCRIPTION

*modinfo* extracts information from the Linux Kernel modules given on the
command line. If the module name is not a filename, then the @MODULE_DIRECTORY@/
_version_ directory is searched, as is also done by *modprobe*(8) when loading
kernel modules.

*modinfo* by default lists each attribute of the module in form _fieldname_ :
_value_, for easy reading. The filename is listed the same way (although it's
not really an attribute).

This version of *modinfo* can understand modules of any Linux Kernel
architecture.

# OPTIONS

*-V*, *--version*
	Print the *modinfo* version.

*-F* _field_, *--field* _field_
	Only print this _field_ value, one per line. This is most useful for
	scripts. Field names are case-insensitive. Common fields (which may not
	be in every module) include author, description, license, parm, depends,
	and alias. There are often multiple parm, alias and depends fields. The
	special _field_ filename lists the filename of the module.

*-b* _basedir_, *--basedir* _basedir_
	Root directory for modules, / by default.

*-k* _kernel_
	Provide information about a kernel other than the running one. This is
	particularly useful for distributions needing to extract information
	from a newly installed (but not yet running) set of kernel modules. For
	example, you wish to find which firmware files are needed by various
	modules in a new kernel for which you must make an initrd/initramfs
	image prior to booting.

*-0*, *--null*
	Use the ASCII zero character to separate _field_ values, instead of a new
	line. This is useful for scripts, since a new line can theoretically
	appear inside a _field_.

*-a* *--author*, *-d* *--description*, *-l* *--license*, *-p* *--parameters*, *-n* *--filename*
	These are shortcuts for the *--field* flag's author, description,
	license, parm and filename arguments, to ease the transition from the
	old modutils *modinfo*.

*-h*, *--help*
	Print the help message and exit.

# COPYRIGHT

This manual page originally Copyright 2003, Rusty Russell, IBM Corporation.

# SEE ALSO

*modprobe*(8)

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
