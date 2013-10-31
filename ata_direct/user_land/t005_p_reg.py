from ctypes import *

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./liblaxcore.so')

LAX_CMD_PORT_INIT = 0x11
LAX_CMD_VU_2B = (0x11 + 1)
LAX_CMD_PRINT_REGS = (0x11 + 2)
LAX_CMD_PORT_RESET = (0x11 + 3)


file_id = lib.lax_open()
if file_id == 0:
    print("lax open failed")


# Send out a UV command
lib.lax_command_simple(c_int(LAX_CMD_PRINT_REGS), c_long(0))
#lib.lax_command_simple(c_int(LAX_CMD_PORT_INIT), c_long(0))
#lib.lax_command_simple(c_int(LAX_CMD_VU_2B), c_long(0))
#lib.lax_command_simple(c_int(LAX_CMD_PORT_RESET), c_long(0))
#lib.lax_command_simple(c_int(LAX_CMD_PRINT_REGS), c_long(0))


lib.lax_close()



