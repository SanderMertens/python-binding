import ctypes
import os


CORTO_HOME = os.environ.get('CORTO_HOME', '/usr/local/')


libcorto = ctypes.CDLL("%s/lib/corto/0.2/libraries/libcorto.so" % (CORTO_HOME))


ROOT_O = ctypes.c_void_p.in_dll(libcorto, "root_o")


def start():
    libcorto.corto_start()


def stop():
    libcorto.corto_stop()


def resolve(scope, name):
    return libcorto.corto_resolve(scope, name)


def parentof(o):
    return libcorto.corto_parentof(o)


def nameof(o):
    return libcorto.corto_nameof(o)
