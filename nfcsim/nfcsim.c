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


#define NFC_FIRST_ID_BYTE  0x98
#define NFC_SECOND_ID_BYTE 0x39
#define NFC_THIRD_ID_BYTE  0xFF /* No byte */
#define NFC_FOURTH_ID_BYTE 0xFF /* No byte */

/* After a command is input, the simulator goes to one of the following states */
#define STATE_CMD_NONE         0x00000000 /* No command state */
#define STATE_CMD_READ0        0x00000001 /* read data from the beginning of page */
#define STATE_CMD_READ1        0x00000002 /* read data from the second half of page */
#define STATE_CMD_READSTART    0x00000003 /* read data second command (large page devices) */
#define STATE_CMD_PAGEPROG     0x00000004 /* start page program */
#define STATE_CMD_READOOB      0x00000005 /* read OOB area */
#define STATE_CMD_ERASE1       0x00000006 /* sector erase first command */
#define STATE_CMD_STATUS       0x00000007 /* read status */
#define STATE_CMD_SEQIN        0x00000009 /* sequential data input */
#define STATE_CMD_READID       0x0000000A /* read ID */
#define STATE_CMD_ERASE2       0x0000000B /* sector erase second command */
#define STATE_CMD_RESET        0x0000000C /* reset */
#define STATE_CMD_RNDOUT       0x0000000D /* random output command */
#define STATE_CMD_RNDOUTSTART  0x0000000E /* random output start command */
#define STATE_CMD_MASK         0x0000000F /* command states mask */


#define PK(fmt, args...)  printk(KERN_INFO "nfc: " fmt, ##args )
#define PKL(fmt, args...) printk(KERN_INFO "nfc: " fmt "\n", ##args )

/* biggest size needed I can think off */
#define NFC_BUF_SIZE  ( 4096 + 1024 )


static uint log            = 0;
module_param(log,            uint, 0400);
MODULE_PARM_DESC(log,            "Perform logging if not zero");


union nfc_mem {
	u_char *byte;    /* for byte access */
	uint16_t *word;  /* for 16-bit word access */
};

struct nfcs_info{
	struct mtd_partition partition;
	struct mtd_info mtd;
	struct nand_chip chip;
	int cmd_state;

	u_char ids[4];

	struct {
		unsigned command;
		u_char status;
		uint row;
		uint column;
		uint count; /* internal counter */
		uint num;   /* number of bytes which must be processed */
		uint off;
	} regs;


	struct {
		uint64_t totsz;
		uint pgsz;
		uint oobsz;
		uint pgszoob;
		uint pgnum;
	} geom;

	struct kmem_cache *slab;  /* Slab allocator for nand pages */
	union nfc_mem *pages;
	union nfc_mem buf;
};

struct nfcs_info nfc;


static int check_command(int cmd)
{
	switch (cmd) {
	case NAND_CMD_READ0:
	case NAND_CMD_READ1:
	case NAND_CMD_READSTART:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_READOOB:
	case NAND_CMD_ERASE1:
	case NAND_CMD_STATUS:
	case NAND_CMD_SEQIN:
	case NAND_CMD_READID:
	case NAND_CMD_ERASE2:
	case NAND_CMD_RESET:
	case NAND_CMD_RNDOUT:
	case NAND_CMD_RNDOUTSTART:
		return 0;

	default:
		return 1;
	}
}


static union nfc_mem * nfc_get_page(uint pg_index)
{
	union nfc_mem *page = &(nfc.pages[pg_index]);

	PKL("get page 0x%x", pg_index);
	if(page->byte)
		return page;

	PKL("allocating memory for page 0x%x", pg_index);
	page->byte = kmem_cache_alloc(nfc.slab, GFP_NOFS);
	if (page->byte == NULL) {
		PKL("prog_page: error allocating memory for page %d", pg_index);
		return NULL;
	}


	memset(page->byte, 0xff, nfc.geom.pgszoob);

	return page;
}


static int nfc_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int oob_required, int page)
{
	return -ENOMEM;
}

static int nfc_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf, int oob_required)
{
	return -ENOMEM;
}

static int nfc_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
	PKL("nfc wait");
	return 0;
}

static void nfc_select_chip(struct mtd_info *mtd, int chip)
{
	return;
}

static void nfc_cmdfunc(struct mtd_info *mtd, unsigned command,	int column, int page_addr)
{
	if (check_command(command)) {
		PKL("unknow command %x", command);
		return;
	}

	nfc.regs.count = 0;
	nfc.regs.command = command;
	nfc.regs.num = 0;
	nfc.regs.off = 0;
	nfc.regs.column = column;
	nfc.regs.row = page_addr;

	PKL("processing command 0x%x", command);

	if(command == NAND_CMD_READID) {
		nfc.cmd_state = STATE_CMD_READID;
		/* 4 id bytes */
		nfc.regs.num = 4;
		/* copy id to the chip buffer */
		memcpy(nfc.buf.byte, nfc.ids, nfc.regs.num);
	}
	else if (command == NAND_CMD_RESET){
		PKL("Reset");
	}
	else if (command == NAND_CMD_READOOB) {
		/* copy oob data from page to buf */
		union nfc_mem * pg = nfc_get_page(nfc.regs.row);

		nfc.cmd_state = STATE_CMD_READOOB;
		nfc.regs.off = nfc.geom.pgsz;
		nfc.regs.num = nfc.geom.pgszoob - nfc.regs.off - nfc.regs.column;

		memcpy(nfc.buf.byte, pg->byte + nfc.regs.off, nfc.regs.num);

	}
	else {
		PKL("cmdfunc: not implemented command %x", command);
		dump_stack();
	}


	return;
}

static u16 nfc_read_word(struct mtd_info *mtd)
{
	struct nand_chip *chip = &nfc.chip;

	return chip->read_byte(mtd) | (chip->read_byte(mtd) << 8);
}

static uint8_t nfc_read_byte(struct mtd_info *mtd)
{
	uint8_t outb = 0;
	if (nfc.regs.count >= nfc.regs.num) {
		PKL("read byte at index %d, out of range %d", nfc.regs.count, nfc.regs.num);
		outb = 0xff;
		return outb;
	}

	outb = nfc.buf.byte[nfc.regs.count];
	nfc.regs.count ++;

	if(nfc.cmd_state == STATE_CMD_READID){
		PKL("read ID byte %d, total %d", nfc.regs.count, nfc.regs.num);
	}
	else {
		PKL("Not supported ");
	}
	return outb;
}

static void nfc_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	PKL("read buf, len 0x%x", len);

	if(nfc.regs.count + len > nfc.regs.num) {
		PKL("read out of the boundary: %d, %d", nfc.regs.count, nfc.regs.num);
		return;
	}

	memcpy(buf, nfc.buf.byte + nfc.regs.count, len);
	nfc.regs.count += len;

	return;
}

static void nfc_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	PKL("write buf not implemented");
	dump_stack();
	return;
}


static int __init nfc_init_module(void)
{
	int retval = -ENOMEM;
	int i;
	struct nand_chip *chip = &nfc.chip;
	struct mtd_info *mtd = &nfc.mtd;

	PKL("module init");

	nfc.partition.name = "nfc simulation partition 1";
	nfc.partition.offset = 0;
	nfc.partition.size = MTDPART_SIZ_FULL;

	nfc.ids[0] = NFC_FIRST_ID_BYTE;
	nfc.ids[1] = NFC_SECOND_ID_BYTE;
	nfc.ids[2] = NFC_THIRD_ID_BYTE;
	nfc.ids[3] = NFC_FOURTH_ID_BYTE;

	mtd->priv = chip;
	mtd->owner = THIS_MODULE;

	chip->ecc.mode = NAND_ECC_HW;
	/* What is the proper value? Need to be set? */
	chip->ecc.size = 512;
	chip->ecc.strength = 1;

	chip->ecc.read_page = nfc_read_page_hwecc;
	chip->ecc.write_page = nfc_write_page_hwecc;
	chip->waitfunc = nfc_waitfunc;
	chip->select_chip = nfc_select_chip;
	chip->cmdfunc = nfc_cmdfunc;
	chip->read_word = nfc_read_word;
	chip->read_byte = nfc_read_byte;
	chip->read_buf = nfc_read_buf;
	chip->write_buf = nfc_write_buf;
	/* needed for nand scan  */
	nfc.buf.byte = kmalloc(NFC_BUF_SIZE, GFP_KERNEL);
	if (!nfc.buf.byte) {
		PKL("unable to allocate chip buf\n");
		retval = -ENOMEM;
		goto exit;
	}

	/* scan nand identity, fill the meta data like page/oob size information */
	retval = nand_scan_ident(mtd, 1, NULL);
	if (retval) {
		PKL("cannot scan NFC simulator device identity");
		if (retval > 0)
			retval = -ENXIO;
		goto exit;

	}

	nfc.geom.totsz = mtd->size;
	nfc.geom.pgsz = mtd->writesize;
	nfc.geom.oobsz = mtd->oobsize;
	nfc.geom.pgszoob = nfc.geom.pgsz + nfc.geom.oobsz;
	nfc.geom.pgnum = div_u64(nfc.geom.totsz, nfc.geom.pgsz);

	nfc.pages = vmalloc(nfc.geom.pgnum * sizeof(union nfc_mem));
	if (!nfc.pages) {
		PKL("unable to allocate page array\n");
		retval = -ENOMEM;
		goto exit;
	}
	for (i = 0; i < nfc.geom.pgnum; i++) {
		nfc.pages[i].byte = NULL;
	}

	nfc.slab = kmem_cache_create("nfcsim", nfc.geom.pgszoob, 0, 0, NULL);
	if (!nfc.slab) {
		PKL("unable to allocate page kmem_cache\n");
		retval = -ENOMEM;
		goto exit;
	}



	retval = nand_scan_tail(mtd);
	if (retval) {
		PKL("cannot scan NFC simulator device tail");
		if (retval > 0)
			retval = -ENXIO;
		goto exit;
	}
exit:
	return retval;
}
module_init(nfc_init_module);


static void __exit nfc_cleanup_module(void)
{
	int i;

	if (nfc.pages) {
		for (i = 0; i < nfc.geom.pgnum; i++) {
			if (nfc.pages[i].byte)
				kmem_cache_free(nfc.slab,
						nfc.pages[i].byte);
		}
		kmem_cache_destroy(nfc.slab);
		vfree(nfc.pages);
	}
	if (nfc.buf.byte) {
		kfree(nfc.buf.byte);
	}
	PKL("module exit");
}
module_exit(nfc_cleanup_module);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Xun Gu");
MODULE_DESCRIPTION ("The NAND flash controller simulator");
