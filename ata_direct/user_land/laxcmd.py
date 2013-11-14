from ctypes import *

LAX_CMD_TST_START          = 0x101
LAX_CMD_TST_PORT_INIT      = 0x102
LAX_CMD_TST_VU_2B          = 0x103
LAX_CMD_TST_PRINT_REGS     = 0x104
LAX_CMD_TST_PORT_RESET     = 0x105
LAX_CMD_TST_ID             = 0x106
LAX_CMD_TST_RW             = 0x106
LAX_CMD_TST_RESTORE_IRQ    = 0x107


lib = cdll.LoadLibrary('./liblaxcore.so')

def open():
    lib.lax_open()

def close():
    lib.lax_close()

def reset():
    lib.lax_command_simple(c_int(LAX_CMD_TST_PORT_RESET), c_long(0))

def rext(lba, sn):
    lib.lax_cmd_rext_pio(c_long(lba), c_long(sn))

def wext(lba, sn):
    lib.lax_cmd_wext_pio(c_long(lba), c_long(sn))

def rdmaext(lba, sn):
    lib.lax_cmd_rext_dma(c_long(lba), c_long(sn))

def wdmaext(lba, sn):
    lib.lax_cmd_wext_dma(c_long(lba), c_long(sn))

def rncq(lba, sn):
    print("rncq xx lba " + hex(lba) + " sn " + hex(sn) )
    lib.lax_cmd_r_ncq(c_long(lba), c_long(sn))

def wncq(lba, sn):
    lib.lax_cmd_w_ncq(c_long(lba), c_long(sn))

