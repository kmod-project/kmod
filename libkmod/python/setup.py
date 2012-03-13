from distutils.core import setup, Extension

libkmod = Extension('kmod',
                    sources = ['libkmod.c'],
                    libraries= ['kmod'])

setup (name = 'kmod',
       version = '0.1',
       description = 'Python binding for kmod',
       ext_modules = [libkmod])
