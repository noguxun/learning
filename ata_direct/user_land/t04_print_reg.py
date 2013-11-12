from ctypes import *
from laxcmd import *

lib = cdll.LoadLibrary('./liblaxcore.so')

file_id = lib.lax_open()
if file_id == 0:
    print("lax open failed")


# Send out a UV commanlib.lax_command_simple(c_int(LAX_TST_CMD_PRINT_REGS), c_long(0))
lib.lax_command_simple(c_int(LAX_CMD_TST_PRINT_REGS), c_long(0))

lib.lax_close()



