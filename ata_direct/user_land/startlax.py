from tkinter import *
from tkinter import messagebox
import laxcmd

class App:
    def __init__(self, master):
        self.frame1 = Frame(master)
        self.frame2 = Frame(master)

        self.widget_create()
        self.widget_layout()


    # UI related
    def widget_create(self):
        self.btn_tmp = Button(self.frame1, text="tmp", command=self.act_tmp)

        self.btn_reset    = Button(self.frame1, text="reset", fg="red", command=self.act_reset)
        self.btn_rncq     = Button(self.frame1, text="rncq", command=self.act_rncq)
        self.btn_wncq     = Button(self.frame1, text="wncq", command=self.act_wncq)
        self.btn_rext     = Button(self.frame1, text="rext", command=self.act_rext)
        self.btn_wext     = Button(self.frame1, text="wext", command=self.act_wext)
        self.btn_rdmaext  = Button(self.frame1, text="rdmaext", command=self.act_rdmaext)
        self.btn_wdmaext  = Button(self.frame1, text="wdmaext", command=self.act_wdmaext)

        self.txt_log = Text(self.frame2)
        self.txt_log.config(state=DISABLED)
        self.label_lba = Label(self.frame2, text="LBA")
        self.label_sn = Label(self.frame2, text="SN")
        self.entry_lba = Entry(self.frame2)
        self.entry_sn = Entry(self.frame2)
        self.entry_lba.insert(0, "0")
        self.entry_sn.insert(0, "1")


    def widget_layout(self):
        self.frame1.grid(row = 0, column = 0, sticky = N)
        self.frame2.grid(row = 0, column = 1)

        self.label_lba.grid(row = 0, column = 0)
        self.entry_lba.grid(row = 0, column = 1)
        self.label_sn.grid(row = 0, column = 2)
        self.entry_sn.grid(row = 0, column = 3)
        self.txt_log.grid(row = 1, column = 0, columnspan = 12, rowspan = 15)

        self.btn_reset.grid(row = 0, column = 0, columnspan = 2, sticky = N+S+E+W)
        self.btn_rncq.grid(row = 1, column = 0, sticky = N+S+E+W)
        self.btn_wncq.grid(row = 1, column = 1, sticky = N+S+E+W)
        self.btn_rext.grid(row = 2, column = 0, sticky = N+S+E+W)
        self.btn_wext.grid(row = 2, column = 1, sticky = N+S+E+W)
        self.btn_rdmaext.grid(row = 3, column = 0, sticky = N+S+E+W)
        self.btn_wdmaext.grid(row = 3, column = 1, sticky = N+S+E+W)

    # Utility
    def validate_lba_sn(self):
        result = True
        lba = 0
        sn = 1
        try:
            lba = int(self.entry_lba.get())
            sn = int(self.entry_sn.get())
            if lba < 0 or sn < 0 or sn > 0x10000:
                result = False
        except ValueError:
            result = False

        if result == False:
            messagebox.showwarning("Data Input", "Not valid LBA or Sector Number");

        return result, lba, sn

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


    # Actions
    def act_tmp(self):
        # we can do some test here
        print("place holder for test");


    def act_reset(self):
        self.log_clear()
        msg = laxcmd.reset()
        self.log("port reset" + " " + msg )


    def act_rncq(self):
        res, lba, sn = self.validate_lba_sn()
        if res == False:
            return
        msg = laxcmd.rncq(lba, sn)
        self.log("rncq lba " + hex(lba) + " sn " + hex(sn) + " " + msg )


    def act_wncq(self):
        res, lba, sn = self.validate_lba_sn()
        if res == False:
            return
        msg = laxcmd.wncq(lba, sn)
        self.log("wncq lba " + hex(lba) + " sn " + hex(sn) + " " + msg )


    def act_rext(self):
        res, lba, sn = self.validate_lba_sn()
        if res == False:
            return
        msg = laxcmd.rext(lba, sn)
        self.log("rext lba " + hex(lba) + " sn " + hex(sn) + " " + msg )


    def act_wext(self):
        res, lba, sn = self.validate_lba_sn()
        if res == False:
            return
        msg = laxcmd.wext(lba, sn)
        self.log("wext lba " + hex(lba) + " sn " + hex(sn) + " " + msg )

    def act_rdmaext(self):
        res, lba, sn = self.validate_lba_sn()
        if res == False:
            return
        msg = laxcmd.rdmaext(lba, sn)
        self.log("rdmaext lba " + hex(lba) + " sn " + hex(sn) + " " + msg )


    def act_wdmaext(self):
        res, lba, sn = self.validate_lba_sn()
        if res == False:
            return
        msg = laxcmd.wdmaext(lba, sn)
        self.log("wdmaext lba " + hex(lba) + " sn " + hex(sn) + " " + msg )







# Main function
def main():
    laxcmd.open()

    root = Tk()
    root.title("LAX")

    app = App(root)
    root.mainloop()

    laxcmd.close()


# Entry point
if __name__ == "__main__":
    main()

