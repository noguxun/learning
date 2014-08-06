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

# Send out a PIO read command via ata direct cmd interface
feature = 0x00
block = 0x10
lba = 0
ata_cmd = 0x20
buf_len = 0x10
cmd_type = LAX_CMD_TYPE_PIO_IN

ata_direct(feature, block, lba, ata_cmd, buf_len, cmd_type)


################################################
# Serious stuff ends
################################################

# Close LAX
lax_close()



