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

cimport libc.stdint as _stdint


cdef extern from *:
    ctypedef char* const_char_ptr 'const char *'
    ctypedef char* const_char_const_ptr 'const char const *'
    ctypedef void* const_void_ptr 'const void *'


cdef extern from 'stdbool.h':
    ctypedef struct bool:
        pass


cdef extern from 'libkmod/libkmod.h':
    # library user context - reads the config and system
    # environment, user variables, allows custom logging
    cdef struct kmod_ctx:
        pass

    kmod_ctx *kmod_new(
        const_char_ptr dirname, const_char_const_ptr config_paths)
    kmod_ctx *kmod_ref(kmod_ctx *ctx)
    kmod_ctx *kmod_unref(kmod_ctx *ctx)

    # Management of libkmod's resources
    int kmod_load_resources(kmod_ctx *ctx)
    void kmod_unload_resources(kmod_ctx *ctx)

    # access to kmod generated lists
    cdef struct kmod_list:
        pass
    ctypedef kmod_list* const_kmod_list_ptr 'const struct kmod_list *'
    kmod_list *kmod_list_next(
        const_kmod_list_ptr list, const_kmod_list_ptr curr)
    kmod_list *kmod_list_prev(
        const_kmod_list_ptr list, const_kmod_list_ptr curr)
    kmod_list *kmod_list_last(const_kmod_list_ptr list)

    # Operate on kernel modules
    cdef struct kmod_module:
        pass
    ctypedef kmod_module* const_kmod_module_ptr 'const struct kmod_module *'
    int kmod_module_new_from_name(
        kmod_ctx *ctx, const_char_ptr name, kmod_module **mod)
    int kmod_module_new_from_lookup(
        kmod_ctx *ctx, const_char_ptr given_alias, kmod_list **list)
    int kmod_module_new_from_loaded(kmod_ctx *ctx, kmod_list **list)

    kmod_module *kmod_module_ref(kmod_module *mod)
    kmod_module *kmod_module_unref(kmod_module *mod)
    int kmod_module_unref_list(kmod_list *list)
    kmod_module *kmod_module_get_module(kmod_list *entry)

    # Flags to kmod_module_probe_insert_module
    # codes below can be used in return value, too
    enum: KMOD_PROBE_APPLY_BLACKLIST

    #ctypedef int (*install_callback_t)(
    #    kmod_module *m, const_char_ptr cmdline, const_void_ptr data)
    #ctypedef void (*print_action_callback_t)(
    #    kmod_module *m, bool install, const_char_ptr options)

    int kmod_module_remove_module(
        kmod_module *mod, unsigned int flags)
    int kmod_module_insert_module(
        kmod_module *mod, unsigned int flags, const_char_ptr options)
    int kmod_module_probe_insert_module(
        kmod_module *mod, unsigned int flags, const_char_ptr extra_options,
        int (*run_install)(
            kmod_module *m, const_char_ptr cmdline, void *data),
        const_void_ptr data,
        void (*print_action)(
            kmod_module *m, bool install, const_char_ptr options),
        )

    const_char_ptr kmod_module_get_name(const_kmod_module_ptr mod)
    const_char_ptr kmod_module_get_path(const_kmod_module_ptr mod)
    const_char_ptr kmod_module_get_options(const_kmod_module_ptr mod)
    const_char_ptr kmod_module_get_install_commands(const_kmod_module_ptr mod)
    const_char_ptr kmod_module_get_remove_commands(const_kmod_module_ptr mod)

    # Information regarding "live information" from module's state, as
    # returned by kernel
    int kmod_module_get_refcnt(const_kmod_module_ptr mod)
    long kmod_module_get_size(const_kmod_module_ptr mod)

    # Information retrieved from ELF headers and section
    int kmod_module_get_info(const_kmod_module_ptr mod, kmod_list **list)
    const_char_ptr kmod_module_info_get_key(const_kmod_list_ptr entry)
    const_char_ptr kmod_module_info_get_value(const_kmod_list_ptr entry)
    void kmod_module_info_free_list(kmod_list *list)

    int kmod_module_get_versions(const_kmod_module_ptr mod, kmod_list **list)
    const_char_ptr kmod_module_version_get_symbol(const_kmod_list_ptr entry)
    _stdint.uint64_t kmod_module_version_get_crc(const_kmod_list_ptr entry)
    void kmod_module_versions_free_list(kmod_list *list)
