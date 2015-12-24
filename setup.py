import glob
import os

from distutils.core import setup
from distutils.core import Extension


HAS_CORTO_INSTALLATION = False
CORTO_HOME = '/usr/local' if HAS_CORTO_INSTALLATION else os.path.join(os.getenv('HOME'), '.corto')


def get_corto_include_dirs():
    include_dirs = [
        'include/corto/0.2/packages/corto/lang',
        'include/corto/0.2/packages',
    ]
    include_dirs = [
        os.path.join(CORTO_HOME, include_dir)
        for include_dir in include_dirs
    ]
    for include_dir in include_dirs:
        assert os.path.exists(include_dir)
    return include_dirs


def get_corto_libraries():
    libraries = [
        'corto',
    ]
    return libraries


def get_corto_library_dirs():
    library_dirs = [
        'lib/corto/0.2/libraries',
    ]
    library_dirs = [
        os.path.join(CORTO_HOME, library_dir)
        for library_dir in library_dirs
    ]
    for library_dir in library_dirs:
        assert os.path.exists(library_dir)
    return library_dirs


def get_sources():
    return glob.glob("src/*.c")


corto_module = Extension(
    'cortopy',
    include_dirs=get_corto_include_dirs(),
    libraries=get_corto_libraries(),
    library_dirs=get_corto_library_dirs(),
    sources=get_sources(),
)


setup(
    name='cortopy',
    version='0.1',
    ext_modules=[corto_module],
    author='Johnny Lee',
    author_email='jleeothon@gmail.com',
    license='MIT',
    description='Python binding for Corto',
    url='https://github.com/cortoproject/python-binding',
    classifiers=[
        'Environment :: Console',
        'Environment :: Web Environment',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'License :: OSI Approved :: MIT License',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: C',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3.5',
        'Topic :: Internet',
    ],
)
