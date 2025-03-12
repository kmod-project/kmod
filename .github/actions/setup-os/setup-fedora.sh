#!/bin/bash
# SPDX-FileCopyrightText: 2024 Emil Velikov <emil.l.velikov@gmail.com>
# SPDX-FileCopyrightText: 2024 Lucas De Marchi <lucas.de.marchi@gmail.com>
#
# SPDX-License-Identifier: LGPL-2.1-or-later

dnf update -y
dnf install -y \
    clang \
    compiler-rt \
    gcc \
    git \
    gtk-doc \
    kernel-devel \
    libasan \
    libhwasan \
    liblsan \
    libtsan \
    libubsan \
    libzstd-devel \
    make \
    meson \
    openssl-devel \
    scdoc \
    xz-devel \
    zlib-devel
    # CI builds with KDIR pointing to /usr/lib/modules/*/build
    # so just a foo/build pointing to the right place, assuming
    # just one kernel installed
    mkdir -p /usr/lib/modules/foo/
    ln -s /usr/src/kernels/* /usr/lib/modules/foo/build
