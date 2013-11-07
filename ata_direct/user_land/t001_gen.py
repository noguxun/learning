from ctypes import *

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./liblaxcore.so')

file_id = lib.lax_open()
if file_id == 0:
    print("lax open failed");

lib.lax_tst_mmap()


lib.lax_close()



