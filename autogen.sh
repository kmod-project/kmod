#!/bin/sh -e

autoreconf --install --symlink

MYCFLAGS="-g -Wall -Wextra \
-Wmissing-declarations -Wmissing-prototypes \
-Wnested-externs -Wpointer-arith \
-Wpointer-arith -Wsign-compare -Wchar-subscripts \
-Wstrict-prototypes -Wshadow \
-Wformat-security -Wtype-limits \
-Wformat=2 -Wuninitialized -Winit-self -Wundef \
-Wmissing-include-dirs -Wold-style-definition \
-Wfloat-equal -Wredundant-decls -Wendif-labels \
-Wcast-align -Wstrict-aliasing -Wwrite-strings \
-Wno-unused-parameter"

libdir() {
	echo $(cd $1/$(gcc -print-multi-os-directory); pwd)
}

args="--prefix=/usr \
--sysconfdir=/etc \
--libdir=$(libdir /usr/lib)"

if [ -z "$NOCONFIGURE" ]; then
	./configure $args CFLAGS="${MYCFLAGS} ${CFLAGS}" $@
fi
