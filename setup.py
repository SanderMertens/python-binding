import glob
import os

from distutils.core import setup
from distutils.core import Extension

version = os.getenv("PYTHON_BINDING_VERSION")

if not version:
    raise Exception("python-binding version not set; please do `rake build`")


HAS_CORTO_INSTALLATION = False
CORTO_HOME = '/usr/local' if HAS_CORTO_INSTALLATION else os.path.join(os.getenv('HOME'), '.corto')


def get_corto_include_dirs():
    include_dirs = glob.glob(os.path.join(CORTO_HOME, 'include/corto/0.2/packages'))
    for include_dir in include_dirs:
        assert os.path.exists(include_dir), '"{}" doesn\'t exist'.format(include_dir)
    return include_dirs


def get_corto_libraries():
    libraries = [
        'corto',
    ]
    for library in libraries:
        search_locations = [
            os.path.join(d, "lib" + library + ".so")
            for d in get_corto_library_dirs()
        ]
        assert any(
            map(os.path.exists, search_locations)
        ), "did not find {}, tried {}".format(library, search_locations)
    return libraries


def get_corto_library_dirs():
    library_dirs = glob.glob(os.path.join(CORTO_HOME, "lib/corto/0.2/packages/*"))
    for library_dir in library_dirs:
        assert os.path.exists(library_dir), '"{}" doesn\'t exist'.format(library_dir)
    return library_dirs


def get_sources():
    return glob.glob("src/*.c")


def get_runtime_library_dirs():
    return get_corto_library_dirs()


corto_module = Extension(
    'cortopy',
    include_dirs=get_corto_include_dirs(),
    libraries=get_corto_libraries(),
    library_dirs=get_corto_library_dirs(),
    sources=get_sources(),
    runtime_library_dirs=get_runtime_library_dirs(),
)


setup(
    name='cortopy',
    version=version,
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
