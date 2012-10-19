cimport _libkmod_h
cimport list as _list


cdef class Module (object):
    cdef _libkmod_h.kmod_module *module

    cpdef from_mod_list_item(self, _list.ModListItem item)
