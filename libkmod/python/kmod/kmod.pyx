# Copyright (C) 2012 Red Hat, Inc.
#                    W. Trevor King <wking@tremily.us>
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

"Define the Kmod class"

cimport cython as _cython
cimport _libkmod_h
from error import KmodError as _KmodError
cimport module as _module
import module as _module
cimport list as _list
import list as _list


cdef class Kmod (object):
    "Wrap a struct kmod_ctx* item"
    def __cinit__(self):
        self._kmod_ctx = NULL
        self.mod_dir = None

    def __dealloc__(self):
        self._cleanup()

    def __init__(self, mod_dir=None):
        self.set_mod_dir(mod_dir=mod_dir)

    def set_mod_dir(self, mod_dir=None):
        self.mod_dir = mod_dir
        self._setup()

    def _setup(self):
        cdef char *mod_dir = NULL
        self._cleanup()
        if self.mod_dir:
            mod_dir = self.mod_dir
        self._kmod_ctx = _libkmod_h.kmod_new(mod_dir, NULL);
        if self._kmod_ctx is NULL:
            raise _KmodError('Could not initialize')
        _libkmod_h.kmod_load_resources(self._kmod_ctx)

    def _cleanup(self):
        if self._kmod_ctx is not NULL:
            _libkmod_h.kmod_unload_resources(self._kmod_ctx);
            self._kmod_ctx = NULL

    def loaded(self):
        "iterate through currently loaded modules"
        cdef _list.ModList ml = _list.ModList()
        cdef _list.ModListItem mli
        err = _libkmod_h.kmod_module_new_from_loaded(self._kmod_ctx, &ml.list)
        if err < 0:
            raise _KmodError('Could not get loaded modules')
        for item in ml:
            mli = <_list.ModListItem> item
            mod = _module.Module()
            mod.from_mod_list_item(item)
            yield mod

    def lookup(self, alias_name, flags=_libkmod_h.KMOD_PROBE_APPLY_BLACKLIST):
        "iterate through modules matching `alias_name`"
        cdef _list.ModList ml = _list.ModList()
        cdef _list.ModListItem mli
        if hasattr(alias_name, 'encode'):
            alias_name = alias_name.encode('ascii')
        err = _libkmod_h.kmod_module_new_from_lookup(
            self._kmod_ctx, alias_name, &ml.list)
        if err < 0:
            raise _KmodError('Could not modprobe')
        for item in ml:
            mli = <_list.ModListItem> item
            mod = _module.Module()
            mod.from_mod_list_item(item)
            yield mod

    @_cython.always_allow_keywords(True)
    def module_from_name(self, name):
        cdef _module.Module mod = _module.Module()
        if hasattr(name, 'encode'):
            name = name.encode('ascii')
        err = _libkmod_h.kmod_module_new_from_name(
            self._kmod_ctx, name, &mod.module)
        if err < 0:
            raise _KmodError('Could not get module')
        return mod

    def list(self):
        "iterate through currently loaded modules and sizes"
        for mod in self.loaded():
            yield (mod.name, mod.size)

    def modprobe(self, name, quiet=False, *args, **kwargs):
        """
        Load a module (or alias) and all modules on which it depends.
        The 'quiet' option defaults to False; set to True to mimic the behavior
        of the '--quiet' commandline option.
        """
        mods = list(self.lookup(alias_name=name))

        if not mods and not quiet:
            raise _KmodError('Could not modprobe %s' % name)

        for mod in mods:
            mod.insert(*args, **kwargs)

    def rmmod(self, module_name, *args, **kwargs):
       """
       remove module from current tree
       e.g. km.rmmod("thinkpad_acpi")
       """
       mod = self.module_from_name(name=module_name)
       mod.remove(*args, **kwargs)
