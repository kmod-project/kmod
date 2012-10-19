import sys as _sys

cimport _libkmod_h
from error import KmodError as _KmodError
cimport list as _list
import list as _list


cdef class Module (object):
    "Wrap a struct kmod_module* item"
    def __cinit__(self):
        self.module = NULL

    def __dealloc__(self):
        self._cleanup()

    def _cleanup(self):
        if self.module is not NULL:
            _libkmod_h.kmod_module_unref(self.module)
            self.module = NULL

    cpdef from_mod_list_item(self, _list.ModListItem item):
        self._cleanup()
        self.module = _libkmod_h.kmod_module_get_module(item.list)

    def _name_get(self):
        name = _libkmod_h.kmod_module_get_name(self.module)
        if _sys.version_info >= (3,):  # Python 3
            name = str(name, 'ascii')
        else:  # Python 2
            name = unicode(name, 'ascii')
        return name
    name = property(fget=_name_get)

    def _size_get(self):
        return _libkmod_h.kmod_module_get_size(self.module)
    size = property(fget=_size_get)

    def insert(self, flags=0, extra_options=None, install_callback=None,
               data=None, print_action_callback=None):
        cdef char *opt = NULL
        #cdef _libkmod_h.install_callback_t install = NULL
        cdef int (*install)(
            _libkmod_h.kmod_module *, _libkmod_h.const_char_ptr, void *)
        install = NULL
        cdef void *d = NULL
        #cdef _libkmod_h.print_action_callback_t print_action = NULL
        cdef void (*print_action)(
            _libkmod_h.kmod_module *, _libkmod_h.bool,
            _libkmod_h.const_char_ptr)
        print_action = NULL
        if extra_options:
            opt = extra_options
        # TODO: convert callbacks and data from Python object to C types
        err = _libkmod_h.kmod_module_probe_insert_module(
            self.module, flags, opt, install, d, print_action)
        if err == -_libkmod_h.EEXIST:
            raise _KmodError('Module already loaded')
        elif err < 0:
            raise _KmodError('Could not load module')

    def remove(self, flags=0):
        err = _libkmod_h.kmod_module_remove_module(self.module, flags)
        if err < 0:
            raise _KmodError('Could not remove module')
