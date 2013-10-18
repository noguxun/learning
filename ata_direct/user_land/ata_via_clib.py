from ctypes import *

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./liblaxcore.so')

lib.lax_open()

loop_c = 100
while True:
    ome_int = lib.lax_command(c_int(0xFF), c_long(0x2B))
    loop_c -= 1
    if loop_c <= 0:
        break;

lib.close()



