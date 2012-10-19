# Copyright (C) 2012 Red Hat, Inc. All rights reserved.
#               2012 W. Trevor King
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU Lesser General Public License v.2.1.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

from distutils.core import setup
from distutils.extension import Extension as _Extension
import os as _os
import sys as _sys

from Cython.Distutils import build_ext as _build_ext


package_name = 'kmod'

# read version from local kmod/version.py without pulling in
# kmod/__init__.py
_sys.path.insert(0, package_name)
from version import __version__


_this_dir = _os.path.dirname(__file__)

ext_modules = []
for filename in sorted(_os.listdir(package_name)):
    basename,extension = _os.path.splitext(filename)
    if extension == '.pyx':
        ext_modules.append(
            _Extension(
                '{}.{}'.format(package_name, basename),
                [_os.path.join(package_name, filename)],
                libraries=['kmod'],
                ))

setup(
    name=package_name,
    version=__version__,
    description='Python binding for kmod',
    packages=[package_name],
    provides=[package_name],
    cmdclass = {'build_ext': _build_ext},
    ext_modules=ext_modules,
    )
