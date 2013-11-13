
from ctypes import *
from laxcmd import *

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./liblaxcore.so')

lib.lax_open()

# Send out a PIO read ext command
block = ( 0x2 )
lba = 0x1
buf_size = block * 512


lib.lax_cmd_rext_pio.restype = POINTER(c_ubyte * (block * 512))

buf = lib.lax_cmd_wext_dma(c_long(lba), c_long(block))

lib.lax_close()


