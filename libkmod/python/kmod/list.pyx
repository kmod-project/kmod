# Copyright (C) 2012 Red Hat, Inc. All rights reserved.
#               2012 W. Trevor King
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU Lesser General Public License v.2.1.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
