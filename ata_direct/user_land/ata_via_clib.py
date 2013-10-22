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


# Send out a PIO read ext command
block = 1
lba = 123
array_size = block * 512
rext_data = (c_ubyte * array_size )()
lib.lax_command_rext_pio(c_long(lba), c_short(block), rext_data)

for index in range(array_size):
    output = "{0:2x} ".format(rext_data[index])
    print(output,end="")
    if (index % 0x10 == 0x0F):
        print("\n", end="")

lib.close()



