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

cimport _libkmod_h


cdef class ModListItem (object):
    "Wrap a struct kmod_list* list item"
    def __cinit__(self):
        self.list = NULL


cdef class ModList (ModListItem):
    "Wrap a struct kmod_list* list with iteration"
    def __cinit__(self):
        self._next = NULL

    def __dealloc__(self):
        if self.list is not NULL:
            _libkmod_h.kmod_module_unref_list(self.list)

    def __iter__(self):
        self._next = self.list
        return self

    def __next__(self):
        if self._next is NULL:
            raise StopIteration()
        mli = ModListItem()
        mli.list = self._next
        self._next = _libkmod_h.kmod_list_next(self.list, self._next)
        return mli
