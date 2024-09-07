## kmod - Linux kernel module handling

OVERVIEW
========

kmod is a set of tools to handle common tasks with Linux kernel modules like
insert, remove, list, check properties, resolve dependencies and aliases.

These tools are designed on top of libkmod, a library that is shipped with
kmod. See libkmod/README for more details on this library and how to use it.
The aim is to be compatible with tools, configurations and indexes from
module-init-tools project.


Links
=====
- Mailing list (no subscription needed): linux-modules@vger.kernel.org
- Mailing list archives: https://lore.kernel.org/linux-modules/

- Signed packages: http://www.kernel.org/pub/linux/utils/kernel/kmod/

- Git:
  - Official: https://git.kernel.org/pub/scm/utils/kernel/kmod/kmod.git
  - Mirror: https://github.com/kmod-project/kmod
  - Mirror: https://kernel.googlesource.com/pub/scm/utils/kernel/kmod/kmod.git

- License:
  - LGPLv2.1+ for libkmod, testsuite and helper libraries
  - GPLv2+ for tools/*

- Irc: `#kmod` on irc.oftc.net

- Issues: https://github.com/kmod-project/kmod/issues


Compilation and installation
============================

In order to compile the source code you need the following software packages:
	- GCC/CLANG compiler
	- GNU C library / musl / uClibc
	- rsync


Optional dependencies:
	- ZLIB library
	- LZMA library
	- ZSTD library
	- OPENSSL library (signature handling in modinfo)

Typical configuration:
	./configure CFLAGS="-g -O2" --prefix=/usr \
			--sysconfdir=/etc --libdir=/usr/lib

Configure automatically searches for all required components and packages.

To compile and install run:
	make && make install

Hacking
=======

Run 'autogen.sh' script before configure. If you want to accept the recommended
flags, you just need to run `autogen.sh c`.

Make sure to read the CODING-STYLE file and the other READMEs: libkmod/README
and testsuite/README.

Compatibility with module-init-tools
====================================

kmod replaced module-init-tools, which was EOL'ed in 2011. All the tools were
rewritten on top of libkmod and they can be used as drop in replacements.
Along the years there were a few behavior changes and new features implemented,
following feedback from Linux kernel community and distros.
