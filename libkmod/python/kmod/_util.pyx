# Copyright (C) 2012 W. Trevor King <wking@tremily.us>
#
# This file is part of python-kmod.
#
# python-kmod is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License version 2.1 as published
# by the Free Software Foundation.
#
# python-kmod is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with python-kmod.  If not, see <http://www.gnu.org/licenses/>.

import sys as _sys

cimport _libkmod_h


cdef object char_ptr_to_str(_libkmod_h.const_char_ptr char_ptr):
    if char_ptr is NULL:
        return None
    if _sys.version_info >= (3,):  # Python 3
        return str(char_ptr, 'ascii')
    # Python 2
    return unicode(char_ptr, 'ascii')
