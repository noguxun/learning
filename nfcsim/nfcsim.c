/*
 * For kernel 3.13
 */

#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/vmalloc.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_bch.h>
#include <linux/mtd/partitions.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>


static uint log            = 0;
module_param(log,            uint, 0400);
MODULE_PARM_DESC(log,            "Perform logging if not zero");

struct nfcs_info{

};

struct nfcs_info nfc;
struct mtd_info mtd;
struct nand_chip chip;


#define PK(fmt, args...)  printk(KERN_INFO "nfc: " fmt, ## args )
#define PKL(fmt, args...) printk(KERN_INFO "nfc: " fmt "\n", ## args )


static int nfc_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int oob_required, int page)
{
	return 0;
}

static int nfc_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf, int oob_required)
{
	return 0;
}

static int nfc_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
	return 0;
}

static void nfc_select_chip(struct mtd_info *mtd, int chip)
{
	return;
}

static void nfc_cmdfunc(struct mtd_info *mtd, unsigned command,	int column, int page_addr)
{
	return;
}

static u16 nfc_read_word(struct mtd_info *mtd)
{
	return 0;
}

static uint8_t nfc_read_byte(struct mtd_info *mtd)
{
	return 0;
}

static void nfc_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	return;
}

static void nfc_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	return;
}

static int __init nfc_init_module(void)
{
	PKL("module init");

	mtd.priv = &nfc;
	mtd.owner = THIS_MODULE;

	chip.ecc.read_page = nfc_read_page_hwecc;
	chip.ecc.write_page = nfc_write_page_hwecc;
	chip.waitfunc = nfc_waitfunc;
	chip.select_chip = nfc_select_chip;
	chip.cmdfunc = nfc_cmdfunc;
	chip.read_word = nfc_read_word;
	chip.read_byte = nfc_read_byte;
	chip.read_buf = nfc_read_buf;
	chip.write_buf = nfc_write_buf;

	return 0;
}
module_init(nfc_init_module);


static void __exit nfc_cleanup_module(void)
{
	PKL("module exit");
}
module_exit(nfc_cleanup_module);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Xun Gu");
MODULE_DESCRIPTION ("The NAND flash controller simulator");
