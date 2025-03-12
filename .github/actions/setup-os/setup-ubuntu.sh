#!/bin/bash
# SPDX-FileCopyrightText: 2024 Emil Velikov <emil.l.velikov@gmail.com>
# SPDX-FileCopyrightText: 2024 Lucas De Marchi <lucas.de.marchi@gmail.com>
#
# SPDX-License-Identifier: LGPL-2.1-or-later

export DEBIAN_FRONTEND=noninteractive
export TZ=Etc/UTC
apt-get update
apt-get install --yes \
    build-essential \
    clang \
    gcc-multilib \
    gcovr \
    git \
    gtk-doc-tools \
    liblzma-dev \
    libssl-dev \
    libzstd-dev \
    linux-headers-generic \
    meson \
    scdoc \
    zlib1g-dev \
    zstd
