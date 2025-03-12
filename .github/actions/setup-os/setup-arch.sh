#!/bin/bash
# SPDX-FileCopyrightText: 2024 Emil Velikov <emil.l.velikov@gmail.com>
# SPDX-FileCopyrightText: 2024 Lucas De Marchi <lucas.de.marchi@gmail.com>
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# Semi-regularly the packager key may have (temporarily) expired. Somewhat
# robust solution is to wipe the local keyring and regenerate/reinstall it
# prior to any other packages on the system.
rm -rf /etc/pacman.d/gnupg
pacman-key --init
pacman-key --populate
pacman --noconfirm -Sy archlinux-keyring

pacman --noconfirm -Su \
    clang \
    git \
    gtk-doc \
    linux-headers \
    lld \
    meson \
    scdoc
