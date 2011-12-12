#!/bin/sh -e

autoreconf --install --symlink

MYCFLAGS="-g"

libdir() {
	echo $(cd $1/$(gcc -print-multi-os-directory); pwd)
}

args="--prefix=/usr \
--sysconfdir=/etc \
--libdir=$(libdir /usr/lib)"

if [ -z "$NOCONFIGURE" ]; then
	exec ./configure $args CFLAGS="${MYCFLAGS} ${CFLAGS}" "$@"
fi
