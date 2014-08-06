from tkinter import *
import laxcmd
import laxutil
import laxparser
import time
import subprocess

class App:
    def __init__(self, master):
        self.frame1 = Frame(master)
        self.frame2 = Frame(master)

        self.widget_create()
        self.widget_layout()

        master.bind("<Return>", self.cmd_exe)
        master.bind("<Up>", self.cmd_history_up)
        master.bind("<Down>", self.cmd_history_down)

        self.cmd_parser = laxparser.CmdParser()
        self.cmd_parser.set_handler(self)

        self.cmd_history = []
        self.cmd_history_cursor = -1
        self.cmd_ata_feature = 0
        self.last_cmd_time = 0


    # UI related
    def widget_create(self):
        self.btn_reset    = Button(self.frame1, text="reset", fg="red", command=self.act_reset)
        self.btn_cmp      = Button(self.frame1, text="cmp", fg="green", command=self.act_cmp)
        self.btn_rncq     = Button(self.frame1, text="rncq", command=self.act_rncq)
        self.btn_wncq     = Button(self.frame1, text="wncq", command=self.act_wncq)
        self.btn_rext     = Button(self.frame1, text="rext", command=self.act_rext)
        self.btn_wext     = Button(self.frame1, text="wext", command=self.act_wext)
        self.btn_rdmaext  = Button(self.frame1, text="rdmaext", command=self.act_rdmaext)
        self.btn_wdmaext  = Button(self.frame1, text="wdmaext", command=self.act_wdmaext)

        self.txt_log = Text(self.frame2)
        self.txt_log.config(state=DISABLED)
        self.label_lba = Label(self.frame2, text="LBA")
        self.label_blk = Label(self.frame2, text="BLK")
        self.label_pat = Label(self.frame2, text="PAT")
        self.label_cmd = Label(self.frame2, text="CMD")
        self.entry_lba = Entry(self.frame2)
        self.entry_blk = Entry(self.frame2)
        self.entry_pat = Entry(self.frame2)
        self.entry_cmd = Entry(self.frame2)

        self.entry_lba.insert(0, "0x0")
        self.entry_blk.insert(0, "0x8")
        self.entry_pat.insert(0, "0x55")


    def widget_layout(self):
        self.frame1.grid(row = 0, column = 0, sticky = N)
        self.frame2.grid(row = 0, column = 1)

        self.label_lba.grid(row = 0, column = 0)
        self.entry_lba.grid(row = 0, column = 1)
        self.label_blk.grid(row = 0, column = 2)
        self.entry_blk.grid(row = 0, column = 3)
        self.label_pat.grid(row = 0, column = 4)
        self.entry_pat.grid(row = 0, column = 5)
        self.txt_log.grid(row = 1, column = 0, columnspan = 12, rowspan = 15, sticky = N+S+E+W)
        self.label_cmd.grid(row = 16, column = 0, sticky = N+S+E+W)
        self.entry_cmd.grid(row = 16, column = 1, columnspan = 11, sticky = N+S+E+W)

        self.btn_reset.grid(row = 0, column = 0, columnspan = 2, sticky = N+S+E+W)
        self.btn_cmp.grid(row = 1, column = 0, columnspan = 2, sticky = N+S+E+W)
        self.btn_rncq.grid(row = 2, column = 0, sticky = N+S+E+W)
        self.btn_wncq.grid(row = 2, column = 1, sticky = N+S+E+W)
        self.btn_rext.grid(row = 3, column = 0, sticky = N+S+E+W)
        self.btn_wext.grid(row = 3, column = 1, sticky = N+S+E+W)
        self.btn_rdmaext.grid(row = 4, column = 0, sticky = N+S+E+W)
        self.btn_wdmaext.grid(row = 4, column = 1, sticky = N+S+E+W)

        self.entry_cmd.focus()

    # Utility
    def pre_cmd_act(self):
        # reset the port if last command is more than 10 seconds ago
        #  device could be rebooted
        now = time.time()
        if int(now - self.last_cmd_time) > 10:
            result = laxcmd.reset_port()
            print("Reset the port");

        self.last_cmd_time = now


    def validate_lba_blk(self):
        result1, lba = self.validate_lba()
        result2, blk = self.validate_blk()
        return (result1 and result2), lba, blk


    def validate_lba(self):
        result, lba = laxutil.validate_lba(self.entry_lba.get())
        if not result:
            messagebox.showwarning("Data Input", "Not valid LBA");
            return False, 0, 0
        return True, lba


    def validate_blk(self):
        result, blk = laxutil.validate_blk(self.entry_blk.get())
        if not result:
            messagebox.showwarning("Data Input", "Not valid block number");
            return False, 0

        return True, blk


    def validate_pat(self):
        result, pat = laxutil.validate_pat(self.entry_pat.get())
        if result == False:
            messagebox.showwarning("Data Input", "Not valid pattern");

        return result, pat


    def log(self, txt):
        self.txt_log.config(state = NORMAL)
        self.txt_log.insert(END,txt + "\n")
        self.txt_log.config(state = DISABLED)
        self.txt_log.see(END)


    def log_clear(self):
        self.txt_log.config(state = NORMAL)
        self.txt_log.delete("0.0", END)
        self.txt_log.config(state = DISABLED)
        self.txt_log.see(END)


    def cmd_history_append(self, cmd_text):
        try:
            # in case we already have same one in the history
            self.cmd_history.remove(cmd_text)
        except ValueError:
            pass

        self.cmd_history.append(cmd_text)
        self.cmd_history_cursor = len(self.cmd_history)



    def error_msg(self, result):
        if result == False:
            return laxcmd.error_msg()
        else:
            return ""


    # Actions
    def act_cmp(self):
        result, lba, blk = self.validate_lba_blk()
        if not result:
            return
        diff_pos = laxcmd.cmp(blk)
        if diff_pos < (0):
            message = "same"
        else:
            message = "difference from sector " + hex(diff_pos)
        self.log(message);


    def act_reset(self):
        self.log_clear()
        print("Resetting Port ....")
        result = laxcmd.reset_port()
        print("Resetting Done!")
        self.log("port reset" + " " + self.error_msg(result) )


    def act_rncq(self):
        self.pre_cmd_act()
        result, lba, blk = self.validate_lba_blk()
        if not result:
            return
        result = laxcmd.rncq(lba, blk)
        self.log("rncq lba " + hex(lba) + " blk " + hex(blk) + " " + self.error_msg(result) )


    def act_wncq(self):
        self.pre_cmd_act()
        result, lba, blk = self.validate_lba_blk()
        if not result:
            return
        result, pat = self.validate_pat()
        if not result:
            return
        laxcmd.set_pat(pat)
        result = laxcmd.wncq(lba, blk)
        self.log("wncq lba " + hex(lba) + " blk " + hex(blk) + " " + self.error_msg(result) )


    def act_rext(self):
        self.pre_cmd_act()
        result, lba, blk = self.validate_lba_blk()
        if not result:
            return
        result = laxcmd.rext(lba, blk)
        self.log("rext lba " + hex(lba) + " blk " + hex(blk) + " " + self.error_msg(result) )


    def act_wext(self):
        self.pre_cmd_act()
        result, lba, blk = self.validate_lba_blk()
        if not result:
            return
        result, pat = self.validate_pat()
        if not result:
            return
        laxcmd.set_pat(pat)
        result = laxcmd.wext(lba, blk)
        self.log("wext lba " + hex(lba) + " blk " + hex(blk) + " " + self.error_msg(result) )

    def act_rdmaext(self):
        self.pre_cmd_act()
        result, lba, blk = self.validate_lba_blk()
        if not result:
            return
        laxcmd.set_lba(lba)
        laxcmd.set_blk(blk)
        result = laxcmd.rdmaext()
        self.log("rdmaext lba " + hex(lba) + " blk " + hex(blk) + " " + self.error_msg(result) )


    def act_wdmaext(self):
        self.pre_cmd_act()
        result, lba, blk = self.validate_lba_blk()
        if not result:
            return
        result, pat = self.validate_pat()
        if not result:
            return
        laxcmd.set_lba(lba)
        laxcmd.set_blk(blk)
        laxcmd.set_pat(pat)
        result = laxcmd.wdmaext()
        self.log("wdmaext lba " + hex(lba) + " blk " + hex(blk) + " " + self.error_msg(result) )


    def act_dump(self):
        result, blk = self.validate_blk()
        if not result:
            self.log("not valid block number");
            return

        laxcmd.dump_wbuf("./dump_wbuf.bin", blk)
        laxcmd.dump_rbuf("./dump_rbuf.bin", blk)
        self.log("saving done, check dump_rbuf.bin and dump_wbuf.bin")
        subprocess.call(["ghex dump_rbuf.bin dump_wbuf.bin &"], shell = True)

        """
        # old implementation, keep it for reference
        #  who knows how to turn a LP_c_ubyte (POINTER of c_ubyte) into a bytes?
        with open("./dump_rbuf.bin", "wb") as f:
            for i in range(blk * 512):
                f.write(bytes([rbuf[i]]))

        with open("./dump_wbuf.bin", "wb") as f:
            for i in range(blk * 512):
                f.write(bytes([wbuf[i]]))
        """


    # binded to the up/down key
    def cmd_history_up(self, event):
        if len(self.cmd_history) == 0:
            return

        if self.cmd_history_cursor <= 0:
            return

        self.cmd_history_cursor -= 1
        self.entry_cmd.delete(0, END)
        self.entry_cmd.insert(0, self.cmd_history[self.cmd_history_cursor])


    # binded to the up/down key
    def cmd_history_down(self, event):
        if len(self.cmd_history) == 0:
            return

        if self.cmd_history_cursor >= (len(self.cmd_history) - 1):
            return

        self.cmd_history_cursor += 1
        self.entry_cmd.delete(0, END)
        self.entry_cmd.insert(0, self.cmd_history[self.cmd_history_cursor])


    # binded to the return key
    def cmd_exe(self, event):
        cmd_text = self.entry_cmd.get()
        result = self.cmd_parser.exec(cmd_text)

        if result != laxparser.CmdParser.ERR_NONE:
            self.log(laxparser.CmdParser.ERR_MSG[result])
            return

        self.cmd_history_append(cmd_text)
        self.entry_cmd.delete(0, END)


    # parser cmd handlers methods
    def cmd_ldr(self, path):
        laxcmd.load_rbuf(path)
        self.log("loaded " + path + " to read buffer" )


    def cmd_ldw(self, path):
        laxcmd.load_wbuf(path)
        self.log("loaded " + path + " to write buffer" )

    def cmd_feature(self, feature):
        self.cmd_ata_feature = feature
        self.log("set ata command feature " + hex(feature))


    def cmd_ata_cmd(self, cmd):
        laxcmd.ata_direct(self.cmd_ata_feature, 0 , 0, cmd, 0, LAX_CMD_TYPE_NO_DATA)
        self.log("ata cmd " + hex(cmd) + " with feature " + hex(self.cmd_ata_feature))
        self.log("Not implemented yet")


    def cmd_lba(self, lba):
        self.entry_lba.delete(0, END)
        self.entry_lba.insert(0, hex(lba))


    def cmd_blk(self, blk):
        self.entry_blk.delete(0, END)
        self.entry_blk.insert(0, hex(blk))


    def cmd_pat(self, pat):
        self.entry_pat.delete(0, END)
        self.entry_pat.insert(0, hex(pat))
        result1, blk = self.validate_blk()
        result2, lba = self.validate_lba()
        if result1 and result2:
            laxcmd.wbuf_update(lba, blk, pat)
        else:
            self.log("cannot set pat due to invalid block number or lba");


    def cmd_clsr(self):
        laxcmd.clear_rbuf()
        self.log("read buffer cleared to zero")


    def cmd_retry(self, onoff):
        cmd = 0xEF
        if(onoff == "off"):
            feature = 0x33
        else:
            feature = 0x99
        laxcmd.ata_direct(feature, 0, 0, cmd, 0, laxcmd.LAX_CMD_TYPE_NO_DATA)
        self.log("retry: cmd " +  hex(cmd) + " executed with feature " + hex(feature))


    def cmd_rext(self):
        self.act_rext()


    def cmd_rdma(self):
        self.act_rdmaext()


    def cmd_rncq(self):
        self.act_rncq()


    def cmd_wext(self):
        self.act_wext()


    def cmd_wdma(self):
        self.act_wdmaext()


    def cmd_wncq(self):
        self.act_wncq()


    def cmd_cls(self):
        self.log_clear()


    def cmd_dump(self):
        self.act_dump()


    def cmd_cmp(self):
        self.act_cmp()


    def cmd_dldd(self):
        self.pre_cmd_act()
        laxcmd.download_microcode()
        self.log("Microcode downloading done!")

    def cmd_reset(self):
        self.act_reset()


# Main function
def main():
    if laxcmd.lax_open() == 0:
        print("Cannot open LAX device file, check if LAX module is loaded")
        return

    root = Tk()
    root.title("LAX")
    img = PhotoImage(file='lax_icon.png')
    root.tk.call('wm', 'iconphoto', root._w, img)

    app = App(root)
    root.mainloop()

    laxcmd.lax_close()


# Entry point
if __name__ == "__main__":
    main()

