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
    tfd = lib.lax_command_simple(c_int(LAX_CMD_TST_PORT_RESET), c_long(0))
    return stsmsg(tfd)


def rext(lba, sn):
    tfd = lib.lax_cmd_rext_pio(c_long(lba), c_long(sn))
    return stsmsg(tfd)


def wext(lba, sn):
    tfd = lib.lax_cmd_wext_pio(c_long(lba), c_long(sn))
    return stsmsg(tfd)


def rdmaext(lba, sn):
    tfd = lib.lax_cmd_rext_dma(c_long(lba), c_long(sn))
    return stsmsg(tfd)


def wdmaext(lba, sn):
    tfd = lib.lax_cmd_wext_dma(c_long(lba), c_long(sn))
    return stsmsg(tfd)


def rncq(lba, sn):
    tfd = lib.lax_cmd_r_ncq(c_long(lba), c_long(sn))
    return stsmsg(tfd)


def wncq(lba, sn):
    tfd = lib.lax_cmd_w_ncq(c_long(lba), c_long(sn))
    return stsmsg(tfd)


def stsmsg(tfd):
    # format AHCI spec, 3.3.8, Port x Task File Data
    err_msg = ""
    if tfd & 0xff00 != 0:
        err_msg = "err:"
    if tfd & 0x8000:
        err_msg = err_msg + " CRC"
    if tfd & 0x4000:
        err_msg = err_msg + " UNC"
    if tfd & 0x2000:
        err_msg = err_msg + " MC"
    if tfd & 0x1000:
        err_msg = err_msg + " IDNF"
    if tfd & 0x0800:
        err_msg = err_msg + " MCR"
    if tfd & 0x0400:
        err_msg = err_msg + " ABRT"
    if tfd & 0x0200:
        err_msg = err_msg + " TK0NF"
    if tfd & 0x0100:
        err_msg = err_msg + " AMNF"

    return err_msg;
