# lsmod(8) completion                                       -*- shell-script -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# SPDX-FileCopyrightText: 2024 Emil Velikov <emil.l.velikov@gmail.com>

# globally disable file completions
complete -c lsmod -f

complete -c lsmod -s s -l syslog  -d 'print to syslog, not stderr'
complete -c lsmod -s v -l verbose -d 'enables more messages'
complete -c lsmod -s V -l version -d 'show version'
complete -c lsmod -s h -l help    -d 'show this help'
