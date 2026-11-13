#!/bin/bash
# SPDX-FileCopyrightText: 2024 Emil Velikov <emil.l.velikov@gmail.com>
# SPDX-FileCopyrightText: 2024 Lucas De Marchi <demarchi@kernel.org>
#
# SPDX-License-Identifier: LGPL-2.1-or-later

export DEBIAN_FRONTEND=noninteractive
export TZ=Etc/UTC

apt-get update
apt-get install --yes \
    bash \
    build-essential \
    curl \
    clang \
    gcc-multilib \
    gcovr \
    git \
    gtk-doc-tools \
    liblz4-dev \
    liblzma-dev \
    libmbedtls-dev \
    libssl-dev \
    libzstd-dev \
    linux-headers-generic \
    lz4 \
    meson \
    scdoc \
    zlib1g-dev \
    zstd
