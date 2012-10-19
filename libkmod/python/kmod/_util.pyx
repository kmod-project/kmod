import sys as _sys

cimport _libkmod_h


cdef object char_ptr_to_str(_libkmod_h.const_char_ptr char_ptr):
    if char_ptr is NULL:
        return None
    if _sys.version_info >= (3,):  # Python 3
        return str(char_ptr, 'ascii')
    # Python 2
    return unicode(char_ptr, 'ascii')
