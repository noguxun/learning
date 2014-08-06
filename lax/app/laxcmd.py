from ctypes import *
from os.path import expanduser

LAX_LOG_LEVEL_QUIET        = 0x01
LAX_LOG_LEVEL_VERBOSE      = 0x02

LAX_CMD_TYPE_NO_DATA       = 0x00
LAX_CMD_TYPE_PIO_IN        = 0x01
LAX_CMD_TYPE_PIO_OUT       = 0x02
LAX_CMD_TYPE_DMA_IN        = 0x03
LAX_CMD_TYPE_DMA_OUT       = 0x04


log_level = LAX_LOG_LEVEL_QUIET
stacked_tfd = 0
preset_lba = 0
preset_blk = 8

preset_microcode = expanduser("~") + "/mnt/SRC/Tool/dD.bin"
lib_path = "./__output/liblaxcore.so"

max_lba = 0

lib = cdll.LoadLibrary(lib_path)

def lax_open():
    return lib.lax_open()


def lax_close():
    lib.lax_close()

def get_mlba():
    global max_lba
    if max_lba != 0 :
        return max_lba

    # need to send identify command
    ata_direct(0, 0, 0, 0xEC, 1, LAX_CMD_TYPE_PIO_IN )
    r = rbuf()
    max_lba = (r[207] << 56) + (r[206] << 48) + (r[205] << 40) + (r[204] << 32) + \
              (r[203] << 24) + (r[202] << 16) + (r[201] <<  8) + (r[200])
    print("Sending ID command to get max_lba as " + hex(max_lba))
    return max_lba

def set_lba(lba_v):
    global preset_lba
    preset_lba = lba_v


def log_quiet():
    global log_level
    log_level = LAX_LOG_LEVEL_QUIET


def log_verbose():
    global log_level
    log_level = LAX_LOG_LEVEL_VERBOSE


def set_blk(b_v):
    global preset_blk
    preset_blk = b_v


def set_pat(pat):
    lib.lax_wbuf_set_pat(c_ubyte(pat))


def clear_rbuf():
    lib.lax_rbuf_clear()



def wbuf_update(lba, blk, pat):
    lib.lax_wbuf_set_pat_id(c_ulonglong(lba), c_long(blk), c_ubyte(pat))


def reset_port():
    tfd = lib.lax_command_port_reset()
    return is_tfd_ok(tfd)


def ata_direct(feature, block, lba, ata_cmd, buf_len, ata_cmd_type):
    tfd = lib.lax_command_ata_direct(c_ushort(feature), c_ushort(block), \
                                     c_ulonglong(lba), c_ubyte(ata_cmd),\
                                     c_ulong(buf_len), c_ubyte(ata_cmd_type))
    return is_tfd_ok(tfd)


def dump_rbuf(path, blk):
    if blk == 0:
        blk = 0x10000
    lib.lax_dump_rbuf(c_char_p(bytes(path,"UTF-8")), blk * 512)


def dump_wbuf(path, blk):
    if blk == 0:
        blk = 0x10000
    lib.lax_dump_wbuf(c_char_p(bytes(path,"UTF-8")), blk * 512)


def get_lba_blk(*args):
    global preset_lba
    global preset_blk
    if(len(args) == 2):
        lba = args[0]
        blk = args[1]
    else:
        lba = preset_lba
        blk = preset_blk
    return lba, blk


def get_microcode(*args):
    global preset_microcode
    if(len(args) == 1):
        path = arg[0]
    else:
        path = preset_microcode

    return path


def rbuf():
    lib.lax_get_rbuf.restype = POINTER(c_ubyte)
    return lib.lax_get_rbuf()


def wbuf():
    lib.lax_get_wbuf.restype = POINTER(c_ubyte)
    return lib.lax_get_wbuf()


def rext(*args):
    lba, blk = get_lba_blk(*args)
    tfd = lib.lax_cmd_rext_pio(c_ulonglong(lba), c_long(blk))
    if log_level != LAX_LOG_LEVEL_QUIET:
        print("rext " + hex(lba) +  " " + hex(blk) + " " + hex(tfd));
    return is_tfd_ok(tfd)


def wext(*args):
    lba, blk = get_lba_blk(*args)
    tfd = lib.lax_cmd_wext_pio(c_ulonglong(lba), c_long(blk))
    if log_level != LAX_LOG_LEVEL_QUIET:
        print("wext " + hex(lba) +  " " + hex(blk) + " " + hex(tfd));
    return is_tfd_ok(tfd)


def rdmaext(*args):
    lba, blk = get_lba_blk(*args)
    tfd = lib.lax_cmd_rext_dma(c_ulonglong(lba), c_long(blk))
    if log_level != LAX_LOG_LEVEL_QUIET:
        print("rdmaext " + hex(lba) +  " " + hex(blk) + " " + hex(tfd));
    return is_tfd_ok(tfd)


def wdmaext(*args):
    lba, blk = get_lba_blk(*args)
    tfd = lib.lax_cmd_wext_dma(c_ulonglong(lba), c_long(blk))
    if log_level != LAX_LOG_LEVEL_QUIET:
        print("wdmaext " + hex(lba) +  " " + hex(blk) + " " + hex(tfd));
    return is_tfd_ok(tfd)


def rncq(*args):
    lba, blk = get_lba_blk(*args)
    tfd = lib.lax_cmd_r_ncq(c_ulonglong(lba), c_long(blk))
    if log_level != LAX_LOG_LEVEL_QUIET:
        print("rncq " + hex(lba) +  " " + hex(blk) + " " + hex(tfd));
    return is_tfd_ok(tfd)


def wncq(*args):
    lba, blk = get_lba_blk(*args)
    tfd = lib.lax_cmd_w_ncq(c_ulonglong(lba), c_long(blk))
    if log_level != LAX_LOG_LEVEL_QUIET:
        print("wncq " + hex(lba) +  " " + hex(blk) + " " + hex(tfd));
    return is_tfd_ok(tfd)


def cmp(blk):
    lib.lax_cmp_rw_buf.restype = c_int
    result = lib.lax_cmp_rw_buf(c_long(blk))
    return result;


def util_printk_regs():
    if log_level != LAX_LOG_LEVEL_QUIET:
        print("check the output at: \" tail -f /var/log/kern.log \"");
    lib.lax_command_port_reg_print()


def download_microcode(*args):
    path = get_microcode(*args)
    print(path)
    lib.lax_download_microcode(c_char_p(bytes(path, "UTF-8")))


def load_rbuf(path):
    print("Loading file \"" + path + "\" to read buffer")
    lib.lax_load_data_to_rbuf(c_char_p(bytes(path, "UTF-8")))


def load_wbuf(path):
    print("Loading file \"" + path + "\" to read buffer")
    lib.lax_load_data_to_wbuf(c_char_p(bytes(path, "UTF-8")))


def is_tfd_ok(tfd):
    global stacked_tfd

    # stacked the tfd information
    stacked_tfd = tfd

    if stacked_tfd & 0xff00 != 0:
        lib.lax_command_port_reset()
        return False
    else:
        return True

def error_msg():
    global stacked_tfd

    # format AHCI spec, 3.3.8, Port x Task File Data
    err_msg = ""
    if stacked_tfd & 0xff00 != 0:
        err_msg = "err:"
    if stacked_tfd & 0x8000:
        err_msg = err_msg + " CRC"
    if stacked_tfd & 0x4000:
        err_msg = err_msg + " UNC"
    if stacked_tfd & 0x2000:
        err_msg = err_msg + " MC"
    if stacked_tfd & 0x1000:
        err_msg = err_msg + " IDNF"
    if stacked_tfd & 0x0800:
        err_msg = err_msg + " MCR"
    if stacked_tfd & 0x0400:
        err_msg = err_msg + " ABRT"
    if stacked_tfd & 0x0200:
        err_msg = err_msg + " TK0NF"
    if stacked_tfd & 0x0100:
        err_msg = err_msg + " AMNF"

    return err_msg;
