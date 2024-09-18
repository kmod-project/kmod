# rmmod(8) completion                                       -*- shell-script -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# SPDX-FileCopyrightText: 2024 Emil Velikov <emil.l.velikov@gmail.com>

# globally disable file completions
complete -c rmmod -f

complete -c rmmod -s f -l force   -d "DANGEROUS: forces a module unload and may crash your machine"
complete -c rmmod -s s -l syslog  -d 'print to syslog, not stderr'
complete -c rmmod -s v -l verbose -d 'enables more messages'
complete -c rmmod -s V -l version -d 'show version'
complete -c rmmod -s h -l help    -d 'show this help'

# provide an exclusive (x) list of required (r) answers (a)
complete -c rmmod -x -ra "(lsmod | cut -f1 -d' ')"
