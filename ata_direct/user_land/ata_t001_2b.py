from ctypes import *

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./liblaxcore.so')

lib.lax_open()

# Send out a UV command
loop_c = 1
while True:
    ome_int = lib.lax_command_vu_2b(c_long(0x2B))
    loop_c -= 1
    if loop_c <= 0:
        break;

lib.close()



