def validate_num(string):
    if len(string) == 0:
        return False, 0

    if string[-1].lower() == 'h':
        string = "0x" + string[:-1]

    try:
        num = int(string, 0)
        return True, num
    except ValueError:
        return False, 0


def validate_retry_setting(string):
    if string.lower() == "on":
        return True, "on"

    if string.lower() == "off":
        return True, "off"

    return False, ""


def validate_path(string):
    try:
        open(string, "rb")
        valid = True
    except:
        valid = False

    return valid, string


def validate_lba(string):
    result, lba = validate_num(string)
    if(result and lba >=0 ):
        return result, lba
    else:
        return False, 0


def validate_feature(string):
    result, feature = validate_num(string)
    if(result and feature >=0 ):
        return result, feature
    else:
        return False, 0


def validate_ata_cmd(string):
    result, ata_cmd = validate_num(string)
    if(result and ata_cmd >=0 ):
        return result, ata_cmd
    else:
        return False, 0



def validate_blk(string):
    result, blk = validate_num(string)
    if(result and blk >=0 and blk <= 0x10000 ):
        return result, blk
    else:
        return False, 0


def validate_pat(string):
    result, pat = validate_num(string)
    if(result and pat >=0 and pat <= 0xFF):
        return result, pat
    else:
        return False, 0


