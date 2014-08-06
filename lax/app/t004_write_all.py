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

max_lba = get_mlba()
lba = 0x100
step = 0x10000

while ( lba + step ) <= (max_lba + 1):
    set_lba(lba)
    set_blk(step)
    lba += step
    wdmaext()


################################################
# Serious stuff ends
################################################

# Close LAX
lax_close()
