#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#define MAX_READ_WRITE_LEN     (32l * 1024l * 1024l)

static unsigned char *lax_wbuf = NULL;
static unsigned char *lax_rbuf = NULL;

static unsigned long lax_rbuf_len = MAX_READ_WRITE_LEN;
static unsigned long lax_wbuf_len = MAX_READ_WRITE_LEN;

static unsigned char lax_wbuf_pat = 0xFF;

static void lax_rwbuf_map(void);
static void lax_rwbuf_set_patid(unsigned long lba, unsigned long block, unsigned char pat);

enum {
	LAX_CMD_TST_START = 0x101,
	LAX_CMD_TST_PORT_INIT,
	LAX_CMD_TST_VU_2B,
	LAX_CMD_TST_PRINT_REGS,
	LAX_CMD_TST_PORT_RESET,
	LAX_CMD_TST_ID,
	LAX_CMD_TST_RW,
	LAX_CMD_TST_RESTORE_IRQ,

	LAX_RW_FLAG_NCQ             = (unsigned long)(1 << 2), /* None-NCQ 0, NCQ 1*/
	LAX_RW_FLAG_XFER_MODE       = (unsigned long)(1 << 1), /* PIO 0,  DMA 1 */
	LAX_RW_FLAG_RW              = (unsigned long)(1 << 0), /* READ 0, WRITE 1*/

	LAX_SECTOR_SIZE             = 512,
};

struct lax_io_rext_pio {
	unsigned long lba;
	unsigned long block;
	void *buf;
};

struct lax_rw {
	unsigned long lba;
	unsigned long block;
	unsigned long flags;
};


static char *lax_file = "/dev/lax";
static int  lax_fd = 0;


int lax_open(void)
{
	printf("lax_open \n");
	if(lax_fd) {
		close(lax_fd);
	}
	lax_fd = open(lax_file, O_RDWR);
	if(lax_fd == 0) {
		printf("open failed \n");
		exit(1);
	}

	lax_rwbuf_map();

	return lax_fd;
}


void lax_close(void)
{
	if(lax_rbuf) {
		size_t len = lax_rbuf_len + lax_wbuf_len;
		munmap(lax_rbuf, len);
		lax_rbuf = NULL;
	}

	if(lax_fd) {
		close(lax_fd);
	}
}

void lax_command_simple(int cmd, long feature)
{
	printf("ioctl simple cmd  0x%x 0x%lx \n", cmd, feature);
	ioctl(lax_fd, cmd, feature);
}

static void lax_cmd_rw(unsigned long lba, unsigned long block, unsigned long flag)
{
	struct lax_rw arg;
	int retval;
	unsigned char * buf;

	arg.lba = lba;
	arg.block = block;
	arg.flags = flag;

	printf("rw lba 0x%lx 0x%lx \n",lba, block);
	retval = ioctl(lax_fd, LAX_CMD_TST_RW, &arg);
	printf("rw command retval %d \n", retval);
}

unsigned char* lax_cmd_rext_dma(unsigned long lba, unsigned long block)
{
	unsigned long flag = 0;

	flag &= (~LAX_RW_FLAG_RW); /* READ */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */

	lax_cmd_rw(lba, block, flag);

	return lax_rbuf;
}

unsigned char* lax_cmd_wext_dma(unsigned long lba, unsigned long block)
{
	unsigned long flag = 0;

	flag |= (LAX_RW_FLAG_RW); /* WRITE */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */

	lax_rwbuf_set_patid(lba, block, lax_wbuf_pat);
	lax_cmd_rw(lba, block, flag);

	return lax_wbuf;
}

unsigned char* lax_cmd_r_ncq(unsigned long lba, unsigned long block)
{
	unsigned long flag = 0;

	flag &= (~LAX_RW_FLAG_RW); /* READ */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */
	flag |= (LAX_RW_FLAG_NCQ);  /* NCQ */

	lax_rwbuf_set_patid(lba, block, lax_wbuf_pat);
	lax_cmd_rw(lba, block, flag);

	return lax_rbuf;
}


unsigned char* lax_cmd_w_ncq(unsigned long lba, unsigned long block)
{
	unsigned long flag = 0;

	flag |= (LAX_RW_FLAG_RW); /* WRITE */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */
	flag |= (LAX_RW_FLAG_NCQ);  /* NCQ */

	lax_rwbuf_set_patid(lba, block, lax_wbuf_pat);

	lax_cmd_rw(lba, block, flag);

	return lax_wbuf;
}

unsigned char* lax_cmd_rext_pio(unsigned long lba, unsigned long block)
{
	unsigned long flag = 0;

	flag &= (~LAX_RW_FLAG_RW); /* READ */
	flag &= (~LAX_RW_FLAG_XFER_MODE); /* PIO */

	lax_cmd_rw(lba, block, flag);

	return lax_rbuf;
}

unsigned char* lax_cmd_wext_pio(unsigned long lba, unsigned long block)
{
	unsigned long flag = 0;

	flag |= (LAX_RW_FLAG_RW); /* WRITE */
	flag &= (~LAX_RW_FLAG_XFER_MODE); /* PIO */

	lax_rwbuf_set_patid(lba, block, lax_wbuf_pat);
	lax_cmd_rw(lba, block, flag);

	return lax_wbuf;
}

void lax_rwbuf_set_patid(unsigned long lba, unsigned long block, unsigned char pat)
{
	unsigned long i;

	// set pattern
	memset(lax_wbuf, pat, block * LAX_SECTOR_SIZE);

	// set id
	for(i=0; i < block; i++) {
		unsigned long *p;
		p = (unsigned long *)((unsigned char *)lax_wbuf + (i * LAX_SECTOR_SIZE));
		*p = lba + i;
	}
}

void lax_rwbuf_map(void)
{
	size_t length = lax_rbuf_len + lax_wbuf_len;

	if(lax_rbuf != NULL) {
		return;
	}

	printf("mmap kernel buffer\n");
	lax_rbuf = mmap(0, length, PROT_WRITE | PROT_READ, MAP_SHARED, lax_fd, 0);
	if (lax_rbuf == MAP_FAILED) {
		printf("mmap failed\n");
		exit(1);
	}

	lax_wbuf = (((unsigned char *)lax_rbuf) + lax_rbuf_len);
}

void lax_rbuf_dump(unsigned long size)
{
	unsigned char *p = lax_rbuf;
	unsigned long i;

	if(p == NULL) {
		printf("not mapped\n");
		return;
	}

	for(i=0; i < size; i++) {
		printf("%.2x ", p[i]);
		if(i%16 == 15) {
			printf("\n");
		}
		if(i%512 == 511) {
			printf("------------\n");
		}
	}
}

void lax_wbuf_set_pat(unsigned char pat)
{
	lax_wbuf_pat = pat;
}



