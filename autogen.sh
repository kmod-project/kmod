#!/bin/sh

set -e

oldpwd=$(pwd)

if [ -n "$MESON_DIST_ROOT" ]; then
    topdir="$MESON_DIST_ROOT"
    gtkdocize_args="--copy"
    autoreconf_args=
else
    topdir=$(dirname $0)
    gtkdocize_args=
    autoreconf_args="--symlink"
fi

cd $topdir

gtkdocize ${gtkdocize_args} --docdir libkmod/docs || NO_GTK_DOC="yes"
if [ "x$NO_GTK_DOC" = "xyes" ]
then
	for f in libkmod/docs/gtk-doc.make m4/gtk-doc.m4
	do
		rm -f $f
		touch $f
	done
fi

autoreconf --force --install ${autoreconf_args}

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
