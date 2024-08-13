DEPMOD.D(5) "kmod" "depmod.d"

# NAME

depmod.d - Configuration directory for depmod

# SYNOPSIS

@SYSCONFDIR@/depmod.d/\*.conf

/run/depmod.d/\*.conf

/usr/local/lib/depmod.d/\*.conf

@DISTCONFDIR@/depmod.d/\*.conf

/lib/depmod.d/\*.conf

# DESCRIPTION

On execution *depmod* reads the configuration files from the above location and
based on that it processes the available modules and their dependencies. For
example: one can change the search order, exclude folders, override specific
module's location and more.

This is typically useful in cases where built-in kernel modules are complemented
by custom built versions of the same and the user wishes to affect the priority
of processing in order to override the module version supplied by the kernel.

# CONFIGURATION FORMAT

The configuration files contain one command per line, with blank lines and lines
starting with '#' ignored (useful for adding comments). A '\\' at the end of a
line causes it to continue on the next line, which makes the files a bit neater.

See the COMMANDS section below for more.

# CONFIGURATION DIRECTORIES AND PRECEDENCE

Configuration files are read from directories in listed in SYNOPSIS in that
order of precedence. Once a file of a given filename is loaded, any file of the
same name in subsequent directories is ignored.

All configuration files are sorted in lexicographic order, regardless of the
directory they reside in. Configuration files can either be completely replaced
(by having a new configuration file with the same name in a directory of higher
priority) or partially replaced (by having a configuration file that is ordered
later).

# COMMANDS

search _subdirectory..._
	This allows you to specify the order in which @MODULE_DIRECTORY@ (or
	other configured module location) subdirectories will be processed by
	*depmod*. Directories are listed in order, with the highest priority
	given to the first listed directory and the lowest priority given to the
	last directory listed. The special keyword *built-in* refers to the
	standard module directories installed by the kernel. Another special
	keyword *external* refers to the list of external directories, defined
	by the *external* command.

	By default, depmod will give a higher priority to a directory with the
	name *updates* using this built-in search string: "updates built-in" but
	more complex arrangements are possible and are used in several popular
	distributions.

override _modulename_ _kernelversion_ _modulesubdirectory_
	This command allows you to override which version of a specific module
	will be used when more than one module sharing the same name is
	processed by the *depmod* command. It is possible to specify one kernel
	or all kernels using the \* wildcard. _modulesubdirectory_ is the name
	of the subdirectory under @MODULE_DIRECTORY@ (or other module location)
	where the target module is installed.

	For example, it is possible to override the priority of an updated test
	module called *kmod* by specifying the following command: "override
	kmod \* extra". This will ensure that any matching module name installed
	under the *extra* subdirectory within @MODULE_DIRECTORY@ (or other
	module location) will take priority over any likenamed module already
	provided by the kernel.

external _kernelversion_ _absolutemodulesdirectory..._
	This specifies a list of directories, which will be checked according to
	the priorities in the *search* command. The order matters also, the
	first directory has the higher priority.

	The _kernelversion_ is a POSIX regular expression or \* wildcard, like
	in the *override*.

exclude _excludedir_
	This specifies the trailing directories that will be excluded during the
	search for kernel modules.

	The _excludedir_ is the trailing directory to exclude.

# COPYRIGHT

This manual page Copyright 2006-2010, Jon Masters, Red Hat, Inc.

# SEE ALSO

*depmod*(8)

# AUTHORS

Numerous contributions have come from the linux-modules mailing list
<linux-modules@vger.kernel.org> and Github. If you have a clone of kmod.git
itself, the output of *git-shortlog*(1) and *git-blame*(1) can show you the
authors for specific parts of the project.

*Lucas De Marchi* <lucas.de.marchi@gmail.com> is the current maintainer of the
project.
