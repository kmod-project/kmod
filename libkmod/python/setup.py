from distutils.core import setup, Extension

libkmod = Extension('libkmod',
                    sources = ['libkmod.c'],
                    libraries= ['kmod'])

setup (name = 'libkmod',
       version = '1.0',
       description = 'Python binding for kmod',
       ext_modules = [libkmod])
