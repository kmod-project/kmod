from distutils.core import setup, Extension

libkmod = Extension('kmod',
                    sources = ['libkmod.c'],
                    libraries= ['kmod'])

setup (name = 'kmod',
       version = '1.0',
       description = 'Python binding for kmod',
       ext_modules = [libkmod])
