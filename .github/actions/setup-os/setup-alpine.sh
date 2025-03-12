#!/bin/sh
# SPDX-FileCopyrightText: 2024 Emil Velikov <emil.l.velikov@gmail.com>
# SPDX-FileCopyrightText: 2024 Lucas De Marchi <lucas.de.marchi@gmail.com>
#
# SPDX-License-Identifier: LGPL-2.1-or-later

apk update
apk add \
    bash \
    build-base \
    coreutils \
    clang \
    git \
    gtk-doc \
    linux-edge-dev \
    meson \
    openssl-dev \
    scdoc \
    tar \
    xz-dev \
    zlib-dev \
    zstd-dev
