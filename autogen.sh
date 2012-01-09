#!/bin/sh -e

gtkdocize --docdir libkmod/docs
autoreconf --install --symlink

libdir() {
	echo $(cd $1/$(gcc -print-multi-os-directory); pwd)
}

args="--prefix=/usr \
--sysconfdir=/etc \
--libdir=$(libdir /usr/lib)"

echo
echo "----------------------------------------------------------------"
echo "Initialized build system. For a common configuration please run:"
echo "----------------------------------------------------------------"
echo
echo "# ./configure CFLAGS='-g -O2' \\"
echo "		$args"
echo

echo "If you are debugging or hacking on kmod, consider configuring"
echo "like below:"
echo
echo "# ./configure CFLAGS='-g -O2 -Werror' \\"
echo "		$args \\"
echo "		--enable-debug"
echo
