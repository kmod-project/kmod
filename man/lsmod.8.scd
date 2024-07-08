LSMOD(8) "kmod" "lsmod"

# NAME

lsmod - Show the status of modules in the Linux Kernel

# SYNOPSIS
*lsmod*

# DESCRIPTION

*lsmod* is a trivial program which nicely formats the contents of the
/proc/modules, showing what kernel modules are currently loaded.

# COPYRIGHT

This manual page originally Copyright 2002, Rusty Russell, IBM Corporation.

# SEE ALSO

*insmod*(8), *modprobe*(8), *modinfo*(8), *depmod*(8)

# AUTHORS

Numerous contributions have come from the linux-modules mailing list
<linux-modules@vger.kernel.org> and Github. If you have a clone of kmod.git
itself, the output of *git-shortlog*(1) and *git-blame*(1) can show you the
authors for specific parts of the project.

*Lucas De Marchi* <lucas.de.marchi@gmail.com> is the current maintainer of the
project.
