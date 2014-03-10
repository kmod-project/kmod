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

import collections as _collections

cimport libc.errno as _errno

cimport _libkmod_h
from error import KmodError as _KmodError
cimport list as _list
import list as _list
cimport _util
import _util


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
        return _util.char_ptr_to_str(
            _libkmod_h.kmod_module_get_name(self.module))
    name = property(fget=_name_get)

    def _path_get(self):
        return _util.char_ptr_to_str(
            _libkmod_h.kmod_module_get_path(self.module))
    path = property(fget=_path_get)

    def _options_get(self):
        return _util.char_ptr_to_str(
            _libkmod_h.kmod_module_get_options(self.module))
    options = property(fget=_options_get)

    def _install_commands_get(self):
        return _util.char_ptr_to_str(
            _libkmod_h.kmod_module_get_install_commands(self.module))
    install_commands = property(fget=_install_commands_get)

    def _remove_commands_get(self):
        return _util.char_ptr_to_str(
            _libkmod_h.kmod_module_get_remove_commands(self.module))
    remove_commands = property(fget=_remove_commands_get)

    def _refcnt_get(self):
        return _libkmod_h.kmod_module_get_refcnt(self.module)
    refcnt = property(fget=_refcnt_get)

    def _size_get(self):
        return _libkmod_h.kmod_module_get_size(self.module)
    size = property(fget=_size_get)

    def _info_get(self):
        cdef _list.ModList ml = _list.ModList()
        cdef _list.ModListItem mli
        err = _libkmod_h.kmod_module_get_info(self.module, &ml.list)
        if err < 0:
            raise _KmodError('Could not get info')
        info = _collections.OrderedDict()
        try:
            for item in ml:
                mli = <_list.ModListItem> item
                key = _util.char_ptr_to_str(
                    _libkmod_h.kmod_module_info_get_key(mli.list))
                value = _util.char_ptr_to_str(
                    _libkmod_h.kmod_module_info_get_value(mli.list))
                info[key] = value
        finally:
            _libkmod_h.kmod_module_info_free_list(ml.list)
            ml.list = NULL
        return info
    info = property(fget=_info_get)

    def _versions_get(self):
        cdef _list.ModList ml = _list.ModList()
        cdef _list.ModListItem mli
        err = _libkmod_h.kmod_module_get_versions(self.module, &ml.list)
        if err < 0:
            raise _KmodError('Could not get versions')
        try:
            for item in ml:
                mli = <_list.ModListItem> item
                symbol = _util.char_ptr_to_str(
                    _libkmod_h.kmod_module_version_get_symbol(mli.list))
                crc = _libkmod_h.kmod_module_version_get_crc(mli.list)
                yield {'symbol': symbol, 'crc': crc}
        finally:
            _libkmod_h.kmod_module_versions_free_list(ml.list)
            ml.list = NULL
    versions = property(fget=_versions_get)

    def insert(self, flags=0, extra_options=None, install_callback=None,
               data=None, print_action_callback=None):
        """
        insert module to current tree. 
        e.g.
        km = kmod.Kmod()
        tp = km.module_from_name("thinkpad_acpi")
        tp.insert(extra_options='fan_control=1')
        """
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
        if err == -_errno.EEXIST:
            raise _KmodError('Module already loaded')
        elif err < 0:
            raise _KmodError('Could not load module')

    def remove(self, flags=0):
        """
        remove module from current tree
        e.g.
        km = kmod.Kmod()
        tp = km.module_from_name("thinkpad_acpi")
        tp.remove()
        """
        err = _libkmod_h.kmod_module_remove_module(self.module, flags)
        if err < 0:
            raise _KmodError('Could not remove module')
