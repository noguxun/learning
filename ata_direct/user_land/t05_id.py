from ctypes import *
from laxcmd import *

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./liblaxcore.so')


file_id = lib.lax_open()
if file_id == 0:
    print("lax open failed")

lib.lax_command_simple(c_int(LAX_CMD_TST_ID), c_long(0))

lib.lax_close()



