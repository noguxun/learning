from ctypes import *

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./liblaxcore.so')

lib.lax_open()

loop_c = 10
while True:
    ome_int = lib.lax_command_vu_2b(c_long(0x2B))
    loop_c -= 1
    if loop_c <= 0:
        break;

block = 10
array_size = block * 512
rext_data = (c_ubyte * array_size )()
lib.lax_command_rext_pio(c_long(0x00), c_short(block), rext_data)
for index in range(array_size):
    print(hex(rext_data[index]),end="")
    print(" ", end="")
    if (index % 0x10 == 0x0F):
        print("\n", end="")

lib.close()



