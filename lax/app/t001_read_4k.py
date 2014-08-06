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

lba = 0x0
block = 0x60
count = 100

while count > 0:
    set_lba(lba)
    set_blk(block)

    result = rdmaext()
    if result == False:
        print("rdma error at " + hex(lba) + " " + hex(block) + error_msg());
        util_printk_regs()
        break

    lba += 0x10
    count -= 1

################################################
# Serious stuff ends
################################################

# Close LAX
lax_close()



