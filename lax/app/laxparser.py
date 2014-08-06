import laxutil

class CmdParser:
    ERR_NONE          = 0x00
    ERR_INVALID_ARG   = 0x01
    ERR_MISSING_ARG   = 0x02
    ERR_INVALID_CMD   = 0x03
    ERR_OTHER         = 0x04

    MODE_VERIFTY      = 0x00
    MODE_RUN          = 0x01

    ERR_MSG = {
        ERR_NONE        : "no error",
        ERR_INVALID_ARG : "invalid argument",
        ERR_MISSING_ARG : "missing argument",
        ERR_INVALID_CMD : "invalid command",
        ERR_OTHER       : "not identified error",
    }


    def __init__(self):
       self.mode = CmdParser.MODE_VERIFTY


    def set_handler(self, handler):
        self.cmd_tbl = dict()
        self.cmd_tbl["reset"]    = (handler.cmd_reset, 0 , None)
        self.cmd_tbl["l"]        = \
        self.cmd_tbl["lba"]      = (handler.cmd_lba, 1, laxutil.validate_lba)
        self.cmd_tbl["b"]        = \
        self.cmd_tbl["blk"]      = (handler.cmd_blk, 1, laxutil.validate_blk)
        self.cmd_tbl["p"]        = \
        self.cmd_tbl["pat"]      = (handler.cmd_pat, 1, laxutil.validate_pat)
        self.cmd_tbl["clsr"]     = (handler.cmd_clsr, 0, None)
        self.cmd_tbl["retry"]    = (handler.cmd_retry, 1, laxutil.validate_retry_setting)
        self.cmd_tbl["cls"]      = \
        self.cmd_tbl["clear"]    = (handler.cmd_cls, 0, None)
        self.cmd_tbl["rext"]     = (handler.cmd_rext, 0, None)
        self.cmd_tbl["rdmaext"]  = \
        self.cmd_tbl["rdma"]     = (handler.cmd_rdma, 0, None)
        self.cmd_tbl["rncq"]     = (handler.cmd_rncq, 0, None)
        self.cmd_tbl["wext"]     = (handler.cmd_wext, 0, None)
        self.cmd_tbl["wdmaext"]  = \
        self.cmd_tbl["wdma"]     = (handler.cmd_wdma, 0, None)
        self.cmd_tbl["wncq"]     = (handler.cmd_wncq, 0, None)
        self.cmd_tbl["d"]        = \
        self.cmd_tbl["dump"]     = (handler.cmd_dump, 0, None)
        self.cmd_tbl["cmp"]      = (handler.cmd_cmp, 0, None)
        self.cmd_tbl["feature"]  = \
        self.cmd_tbl["rwpre"]    = (handler.cmd_feature, 1, laxutil.validate_feature)
        self.cmd_tbl["cmd"]      = (handler.cmd_ata_cmd, 1, laxutil.validate_ata_cmd)
        self.cmd_tbl["dldd"]     = (handler.cmd_dldd, 0 , None)
        self.cmd_tbl["ldw"]      = (handler.cmd_ldw, 1, laxutil.validate_path)
        self.cmd_tbl["ldr"]      = (handler.cmd_ldr, 1, laxutil.validate_path)


    def exec(self, cmd_line):
        print("parsing \"" + cmd_line + "\"")
        self.tokens = cmd_line.split()
        start = 0
        end   = len(self.tokens)

        # dry run without calling handler for error check
        self.mode = CmdParser.MODE_VERIFTY
        result = self.expression(start, end)
        if result == CmdParser.ERR_NONE:
            # no error, now parse and calling the handler
            self.mode = CmdParser.MODE_RUN
            result = self.expression(start, end)
            assert(result == CmdParser.ERR_NONE)
        else:
            print(CmdParser.ERR_MSG[result]);

        return result


    def expression(self, start, end):
        if start >= end:
            return CmdParser.ERR_NONE

        token = self.tokens[start]
        if token not in self.cmd_tbl:
            return CmdParser.ERR_INVALID_CMD

        # handler meta info
        handler      = self.cmd_tbl[token][0]
        argc         = self.cmd_tbl[token][1]
        arg_validate = self.cmd_tbl[token][2]

        if argc == 0:
            if self.mode == CmdParser.MODE_RUN:
                handler()
            return self.expression(start + 1, end)
        elif argc == 1:
            if((start + 1) >= end):
                return CmdParser.ERR_MISSING_ARG
            para = self.tokens[start+1]
            result, arg = arg_validate(para)
            if not result:
                return CmdParser.ERR_INVALID_ARG
            if self.mode == CmdParser.MODE_RUN:
                handler(arg)
        else:
            assert(mFalse)

        return self.expression(start+argc+1, end)

