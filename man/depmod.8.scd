DEPMOD(8) "kmod" "depmod"

# NAME

depmod - Generate modules.dep and map files.

# SYNOPSIS

*depmod* [*-b* _basedir_] [*-m* _moduledir_] [*-o* _outdir_][*-o* _outdir_] [*-e*] [*-E* _Module.symvers_]
\ \ \ \ \ \ \ \[*-F* _System.map_] [*-n*] [*-v*] [*-A*] [*-P* _prefix_] [*-w*] [_version_]

*depmod* [*-e*] [*-E* _Module.symvers_] [*-F* _System.map_] [*-n*] [*-v*] [*-P* _prefix_]
\ \ \ \ \ \ \ \[*-w*] [_version_] [_filename_...]

# DESCRIPTION

Linux kernel modules can provide services (called "symbols") for other modules
to use (using one of the EXPORT_SYMBOL variants in the code). If a second module
uses this symbol, that second module clearly depends on the first module. These
dependencies can get quite complex.

*depmod* creates a list of module dependencies by reading each module under
<BASEDIR>/<MODULEDIR>/_version_. By default <MODULE_DIRECTORY> is
@MODULE_DIRECTORY@ and <BASEDIR> is empty. See options below to override when
needed. It determines what symbols each module exports and needs.  This list is
written to *modules.dep*, and a binary hashed version named modules.dep.bin, in
the same directory. If filenames are given on the command line, only those
modules are examined (which is rarely useful unless all modules are listed).
*depmod* also creates a list of symbols provided by modules in the file named
modules.symbols and its binary hashed version, modules.symbols.bin. Finally,
*depmod* will output a file named modules.devname if modules supply special
device names (devname) that should be populated in /dev on boot (by a utility
such as systemd-tmpfiles).

If a _version_ is provided, then that kernel version's module directory is used
rather than the current kernel version (as returned by *uname -r*).

# OPTIONS

*-a*
*--all*
	Probe all modules. This option is enabled by default if no file names
	are given in the command-line.

*-A*
*--quick*
	This option scans to see if any modules are newer than the
	*modules.dep* file before any work is done: if not, it silently exits
	rather than regenerating the files.

*-b* _basedir_
*--basedir* _basedir_
	Override the base directory <BASEDIR> where modules are located.
	If your modules are not currently in the (normal) directory
	@MODULE_DIRECTORY@/_version_, but in a staging area, you can specify a
	_basedir_ which is prepended to the directory name. This _basedir_ is
	stripped from the resulting *modules.dep* file, so it is ready to be
	moved into the normal location. Use this option if you are a
	distribution vendor who needs to pre-generate the meta-data files rather
	than running *depmod* again later.

	Example:
		depmod -b /my/build/staging/dir/

	This expects all input files under
	_/my/build/staging/dir/@MODULE_DIRECTORY@/$(uname -r)_ and generates
	index files under that same directory.

*-m* _moduledir_
*--moduledir* _moduledir_
	Override the module directory <MODULEDIR>, which defaults to
	@MODULE_DIRECTORY@ prefix set at build time. This is useful when
	building *modules.dep* file in _basedir_ for a system that uses a
	different prefix, e.g. _/usr/lib/modules_ vs _/lib/modules_.

	Example:
		depmod -b /tmp/build -m /kernel-modules

	This expects all input files under
	_/tmp/build/kernel-modules/$(uname -r)_ and generates index files under
	that same directory.

*-o* _outdir_
*--outdir* _outdir_
	Set the output directory where *depmod* will store any generated file.
	_outdir_ serves as a root to that location, similar to how _basedir_ is
	used. Also this setting takes precedence and if used together with
	_basedir_ it will result in the input being that directory, but the output
	being the one set by _outdir_.

	Example:
		depmod -o /my/build/staging/dir/

	This expects all input files under
	_@MODULE_DIRECTORY@/$(uname -r)_ and generates index files under
	_/my/build/staging/dir@MODULE_DIRECTORY@/$(uname -r)_.

*-C*  _file_ _or_ _directory_
*--config* _file_ _or_ _directory_
	This option overrides the default configuration files. See
	*depmod.d*(5).

*-e*
*--errsyms*
	When combined with the *-F* option, this reports any symbols which a
	module needs which are not supplied by other modules or the kernel.
	Normally, any symbols not provided by modules are assumed to be provided
	by the kernel (which should be true in a perfect world), but this
	assumption can break especially when additionally updated third party
	drivers are not correctly installed or were built incorrectly.

*-E* _Module.symvers_
*--symvers* _Module.symvers_
	When combined with the *-e* option, this reports any symbol versions
	supplied by modules that do not match with the symbol versions provided
	by the kernel in its _Module.symvers_. This option is mutually
	incompatible with *-F*.

*-F* _System.map_
*--filesyms* _System.map_
	Supplied with the _System.map_ produced when the kernel was built, this
	allows the *-e* option to report unresolved symbols. This option is
	mutually incompatible with *-E*.

*-h*
*--help*
	Print the help message and exit.

*-n*
*--show*
*--dry-run*
	This sends the resulting *modules.dep* and the various map files to
	standard output rather than writing them into the module directory.

*-P*
	Some architectures prefix symbols with an extraneous character. This
	specifies a prefix character (for example '\_') to ignore.

*-v*
*--verbose*
	In verbose mode, *depmod* will print (to stdout) all the symbols each
	module depends on and the module's file name which provides that symbol.

*-V*
*--version*
	Show version of program and exit. See below for caveats when run on
	older kernels.

*-w*
	Warn on duplicate dependencies, aliases, symbol versions, etc.

# COPYRIGHT

This manual page originally Copyright 2002, Rusty Russell, IBM Corporation.
Portions Copyright Jon Masters, and others.

# SEE ALSO

*depmod.d*(5), *modprobe*(8), *modules.dep*(5)

# AUTHORS

Numerous contributions have come from the linux-modules mailing list
<linux-modules@vger.kernel.org> and Github. If you have a clone of kmod.git
itself, the output of *git-shortlog*(1) and *git-blame*(1) can show you the
authors for specific parts of the project.

*Lucas De Marchi* <lucas.de.marchi@gmail.com> is the current maintainer of the
project.
