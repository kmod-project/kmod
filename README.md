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
- Mailing list (no subscription needed): <linux-modules@vger.kernel.org>
- Mailing list archives: <https://lore.kernel.org/linux-modules/>

- Signed packages: <http://www.kernel.org/pub/linux/utils/kernel/kmod/>

- Git:
    - Official: <https://git.kernel.org/pub/scm/utils/kernel/kmod/kmod.git>
    - Mirror: <https://github.com/kmod-project/kmod>
    - Mirror: <https://kernel.googlesource.com/pub/scm/utils/kernel/kmod/kmod.git>

- License:
    - LGPLv2.1+ for libkmod, testsuite and helper libraries
    - GPLv2+ for tools/*

- Irc: `#kmod` on irc.oftc.net

- Issues: <https://github.com/kmod-project/kmod/issues>


Compilation and installation
============================

In order to compile the source code you need:

- C11 compiler, supporting a range of GNU extensions - GCC 8+, Clang 6+
- POSIX.1-2008 C runtime library - Bionic, GNU C library, musl

Optional dependencies, required with the default build configuration:

- ZLIB library
- LZMA library
- ZSTD library
- OPENSSL library (signature handling in modinfo)

Typical configuration and installation

    meson setup builddir/
    meson compile -C builddir/
    sudo meson install -C builddir/

For end-user and distributions builds, it's recommended to use:

    meson setup --buildtype release builddir/

Hacking
=======

When working on kmod, use the included `build-dev.ini` file, as:

    meson setup --native-file build-dev.ini builddir/

The testsuite can be executed with:

    meson test -C builddir

It builds test kernel modules, so kernel headers need to be pre-installed. By
default it tries to use the kernel header for the currently running kernel.
`KDIR=any` environment variable can be used to tell it to use any installed
kernel header or `KDIR=/path/to/specific/headers` when a specific one is
needed. Example:

    KDIR=any meson test -C builddir

Make sure to read [our contributing guide](CONTRIBUTING.md) and the other
READMEs: [libkmod](libkmod/README) and [testsuite](testsuite/README).

Compatibility with module-init-tools
====================================

kmod replaced module-init-tools, which was EOL'ed in 2011. All the tools were
rewritten on top of libkmod and they can be used as drop in replacements.
Along the years there were a few behavior changes and new features implemented,
following feedback from Linux kernel community and distros.
