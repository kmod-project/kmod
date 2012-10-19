cimport _libkmod_h

cdef class Kmod (object):
    cdef _libkmod_h.kmod_ctx *_kmod_ctx
    cdef object mod_dir
