#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>

#define MAX_READ_WRITE_LEN     (32l * 1024l * 1024l)

static uint8_t *lax_wbuf = NULL;
static uint8_t *lax_rbuf = NULL;

static uint32_t lax_rbuf_len = MAX_READ_WRITE_LEN;
static uint32_t lax_wbuf_len = MAX_READ_WRITE_LEN;

static uint8_t lax_wbuf_pat = 0xFF;

static void lax_rwbuf_map(void);
static void lax_rwbuf_set_patid(uint64_t lba, uint32_t block, uint8_t pat);

enum {
	LAX_CMD_TST_START = 0x101,
	LAX_CMD_TST_PORT_INIT,
	LAX_CMD_TST_VU_2B,
	LAX_CMD_TST_PRINT_REGS,
	LAX_CMD_TST_PORT_RESET,
	LAX_CMD_TST_ID,
	LAX_CMD_TST_RW,
	LAX_CMD_TST_RESTORE_IRQ,

	LAX_RW_FLAG_NCQ             = (uint32_t)(1 << 2), /* None-NCQ 0, NCQ 1*/
	LAX_RW_FLAG_XFER_MODE       = (uint32_t)(1 << 1), /* PIO 0,  DMA 1 */
	LAX_RW_FLAG_RW              = (uint32_t)(1 << 0), /* READ 0, WRITE 1*/

	LAX_SECTOR_SIZE             = 512,
};


struct lax_rw {
	uint64_t lba;
	uint16_t block;
	uint32_t flags;
	uint32_t tfd;
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

static uint32_t lax_cmd_rw(uint64_t lba, uint32_t block, uint32_t flag)
{
	struct lax_rw arg;
	int retval;
	uint8_t * buf;

	arg.lba = lba;
	arg.block = block;
	arg.flags = flag;

	retval = ioctl(lax_fd, LAX_CMD_TST_RW, &arg);

	printf("rw lba 0x%lx block 0x%x retval %d tfd 0x%x \n", lba, block, retval, arg.tfd);

	return arg.tfd;
}

uint32_t lax_cmd_rext_dma(uint64_t lba, uint32_t block)
{
	uint32_t flag = 0;

	flag &= (~LAX_RW_FLAG_RW); /* READ */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */

	return lax_cmd_rw(lba, block, flag);
}

uint32_t lax_cmd_wext_dma(uint64_t lba, uint32_t block)
{
	uint32_t flag = 0;

	flag |= (LAX_RW_FLAG_RW); /* WRITE */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */

	lax_rwbuf_set_patid(lba, block, lax_wbuf_pat);
	return lax_cmd_rw(lba, block, flag);
}

uint32_t lax_cmd_r_ncq(uint64_t lba, uint32_t block)
{
	uint32_t flag = 0;

	flag &= (~LAX_RW_FLAG_RW); /* READ */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */
	flag |= (LAX_RW_FLAG_NCQ);  /* NCQ */

	lax_rwbuf_set_patid(lba, block, lax_wbuf_pat);
	return lax_cmd_rw(lba, block, flag);
}


uint32_t lax_cmd_w_ncq(uint64_t lba, uint32_t block)
{
	uint32_t flag = 0;

	flag |= (LAX_RW_FLAG_RW); /* WRITE */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */
	flag |= (LAX_RW_FLAG_NCQ);  /* NCQ */

	lax_rwbuf_set_patid(lba, block, lax_wbuf_pat);

	return lax_cmd_rw(lba, block, flag);
}

uint32_t lax_cmd_rext_pio(uint64_t lba, uint32_t block)
{
	uint32_t flag = 0;

	flag &= (~LAX_RW_FLAG_RW); /* READ */
	flag &= (~LAX_RW_FLAG_XFER_MODE); /* PIO */

	return lax_cmd_rw(lba, block, flag);

}

uint32_t lax_cmd_wext_pio(uint64_t lba, uint32_t block)
{
	uint32_t flag = 0;

	flag |= (LAX_RW_FLAG_RW); /* WRITE */
	flag &= (~LAX_RW_FLAG_XFER_MODE); /* PIO */

	lax_rwbuf_set_patid(lba, block, lax_wbuf_pat);

	return lax_cmd_rw(lba, block, flag);
}

void lax_rwbuf_set_patid(uint64_t lba, uint32_t block, uint8_t pat)
{
	uint32_t i;

	// set pattern
	memset(lax_wbuf, pat, block * LAX_SECTOR_SIZE);

	// set id
	for(i=0; i < block; i++) {
		uint64_t *p;
		p = (uint64_t *)((uint8_t *)lax_wbuf + (i * LAX_SECTOR_SIZE));
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

	lax_wbuf = (((uint8_t *)lax_rbuf) + lax_rbuf_len);
}

void lax_rbuf_dump(uint32_t size)
{
	uint8_t *p = lax_rbuf;
	uint32_t i;

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

void lax_wbuf_set_pat(uint8_t pat)
{
	lax_wbuf_pat = pat;
}


int lax_cmp_rw_buf(uint32_t sn)
{
	uint32_t buf_size = sn * LAX_SECTOR_SIZE;
	uint32_t i;


	for(i = 0; i < sn; i++) {
		int res;
		res = memcmp(lax_rbuf, lax_wbuf, LAX_SECTOR_SIZE);
		if(res != 0) {
			return i;
		}
	}

	return -1;
}


