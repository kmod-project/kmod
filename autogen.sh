#!/bin/sh

set -e

oldpwd=$(pwd)
topdir=$(dirname $0)
cd $topdir

gtkdocize --docdir libkmod/docs || touch libkmod/docs/gtk-doc.make
autoreconf --force --install --symlink

libdir() {
        echo $(cd "$1/$(gcc -print-multi-os-directory)"; pwd)
}

args="\
--prefix=/usr \
--sysconfdir=/etc \
--libdir=$(libdir /usr/lib) \
"

if [ -f "$topdir/.config.args" ]; then
    args="$args $(cat $topdir/.config.args)"
fi

cd $oldpwd

hackargs="\
--enable-debug \
--enable-gtk-doc \
--with-zstd \
--with-xz \
--with-zlib \
--with-openssl \
"

if [ "x$1" = "xc" ]; then
        shift
        $topdir/configure CFLAGS='-g -O2' $args $hackargs "$@"
        make clean
elif [ "x$1" = "xg" ]; then
        shift
        $topdir/configure CFLAGS='-g -Og' $args "$@"
        make clean
elif [ "x$1" = "xl" ]; then
        shift
        $topdir/configure CC=clang CXX=clang++ $args "$@"
        make clean
elif [ "x$1" = "xa" ]; then
        shift
        $topdir/configure CFLAGS='-g -O2 -Wsuggest-attribute=pure -Wsuggest-attribute=const' $args "$@"
        make clean
elif [ "x$1" = "xs" ]; then
        shift
        scan-build $topdir/configure CFLAGS='-g -O0 -std=gnu11' $args "$@"
        scan-build make
else
        echo
        echo "----------------------------------------------------------------"
        echo "Initialized build system. For a common configuration please run:"
        echo "----------------------------------------------------------------"
        echo
        echo "$topdir/configure CFLAGS='-g -O2' $args"
        echo
        echo If you are debugging or hacking on kmod, consider configuring
        echo like below:
        echo
        echo "$topdir/configure CFLAGS='-g -O2' $args $hackargs"
fi
