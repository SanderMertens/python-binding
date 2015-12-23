import glob
import os

from distutils.core import setup
from distutils.core import Extension


def get_corto_include_dirs():
    return [
        '/usr/local/include/corto/0.2/packages/corto/lang',
        '/usr/local/include/corto/0.2/packages',
    ]


def get_corto_libraries():
    return [
        'corto',
    ]


def get_corto_library_dirs():
    return [
        '/usr/local/lib/corto/0.2/libraries',
    ]


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
