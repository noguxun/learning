from ctypes import *
from laxcmd import *

# Init: Open LAX
file_id = lax_open()
if file_id == 0:
    print("lax open failed")
    quit()

reset_port()

################################################
# Serious stuff starts from here
################################################
log_verbose()

util_printk_regs()

################################################
# Serious stuff ends
################################################

# Close LAX
lax_close()



