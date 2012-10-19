cimport _libkmod_h


cdef class ModListItem (object):
    cdef _libkmod_h.kmod_list *list


cdef class ModList (ModListItem):
    cdef _libkmod_h.kmod_list *_next
