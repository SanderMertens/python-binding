import ctypes
import os

CORTO_HOME = os.environ.get('CORTO_HOME', '/usr/local/')

libcorto = ctypes.CDLL("%s/lib/corto/0.2/libraries/libcorto.so" % (CORTO_HOME))

def start():
    libcorto.corto_start()

def stop():
    libcorto.corto_stop()
