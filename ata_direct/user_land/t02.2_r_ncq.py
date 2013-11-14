from ctypes import *
from laxcmd import *

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./liblaxcore.so')

lib.lax_open()

# Send out a PIO read ext command
block = ( 0x2 )
lba = 0x100
buf_size = block * 512


lib.lax_cmd_rext_pio.restype = POINTER(c_ubyte * (block * 512))

lib.lax_command_simple(c_int(LAX_CMD_TST_PORT_RESET), c_long(0))
buf = lib.lax_cmd_r_ncq(c_long(lba), c_long(block))
buf = lib.lax_cmd_r_ncq(c_long(lba), c_long(block))

lib.lax_command_simple(c_int(LAX_CMD_TST_PRINT_REGS), c_long(0))


print("\noutput contents\n");
lib.lax_rbuf_dump(c_long(block * 512));
print("\nend of data\n");

lib.lax_close()



