#!/bin/sh -e

autoreconf --install --symlink

MYCFLAGS="-g -O2 -Werror"

libdir() {
	echo $(cd $1/$(gcc -print-multi-os-directory); pwd)
}

args="--prefix=/usr \
--sysconfdir=/etc \
--libdir=$(libdir /usr/lib) \
--enable-debug"

if [ -z "$NOCONFIGURE" ]; then
	exec ./configure $args CFLAGS="${MYCFLAGS} ${CFLAGS}" "$@"
fi
