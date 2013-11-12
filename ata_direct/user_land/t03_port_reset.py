from ctypes import *
from laxcmd import *

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./liblaxcore.so')


file_id = lib.lax_open()
if file_id == 0:
    print("lax open failed")


# Send out a UV command
lib.lax_command_simple(c_int(LAX_CMD_TST_PRINT_REGS), c_long(0))
lib.lax_command_simple(c_int(LAX_CMD_TST_PORT_RESET), c_long(0))
lib.lax_command_simple(c_int(LAX_CMD_TST_PRINT_REGS), c_long(0))


lib.lax_close()



