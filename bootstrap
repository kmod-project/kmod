#!/bin/sh -e

gtkdocize --docdir libkmod/docs || touch libkmod/docs/gtk-doc.make
autoreconf --install --symlink

libdir() {
	(cd "$1/$(gcc -print-multi-os-directory)"; pwd)
}

args="--prefix=/usr \
--sysconfdir=/etc \
--libdir=$(libdir /usr/lib)"

hackargs="--enable-debug --enable-python --with-xz --with-zlib"

cat <<EOC

----------------------------------------------------------------
Initialized build system. For a common configuration please run:
----------------------------------------------------------------

./configure CFLAGS='-g -O2' $args

If you are debugging or hacking on kmod, consider configuring
like below:

./configure CFLAGS="-g -O2" $args $hackargs

EOC
