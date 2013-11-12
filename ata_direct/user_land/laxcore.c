#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

static unsigned char * rw_buf = NULL;
static unsigned long  rw_buf_len = ( 32l * 1024l * 1024l );

static void lax_map_rw_buf(void);

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
	LAX_RW_FLAG_DIRECTION       = (unsigned long)(1 << 0), /* READ 0, WRITE 1*/
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
	lax_fd = open(lax_file, O_RDONLY);

	lax_map_rw_buf();

	return lax_fd;
}


void lax_close(void)
{
	if(rw_buf) {
		munmap(rw_buf, rw_buf_len);
		rw_buf = NULL;
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

unsigned char * lax_cmd_rw(unsigned long lba, unsigned long block, unsigned long flag)
{
	struct lax_rw arg;
	int retval;

	arg.lba = lba;
	arg.block = block;
	arg.flags = flag;

	printf("rw lba 0x%lx 0x%lx \n",lba, block);
	retval = ioctl(lax_fd, LAX_CMD_TST_RW, &arg);
	printf("rw command retval %d, buffer %p \n", retval, rw_buf);

	return rw_buf;
}

unsigned char* lax_cmd_rext_dma(unsigned long lba, unsigned long block)
{
	unsigned long flag = 0;

	flag &= (~LAX_RW_FLAG_DIRECTION); /* READ */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */

	return lax_cmd_rw(lba, block, flag);
}

unsigned char* lax_cmd_r_ncq(unsigned long lba, unsigned long block)
{
	unsigned long flag = 0;

	flag &= (~LAX_RW_FLAG_DIRECTION); /* READ */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */
	flag |= (LAX_RW_FLAG_NCQ);  /* DMA */

	return lax_cmd_rw(lba, block, flag);
}

unsigned char* lax_cmd_rext_pio(unsigned long lba, unsigned long block)
{
	unsigned long flag = 0;

	flag &= (~LAX_RW_FLAG_DIRECTION); /* READ */
	flag &= (~LAX_RW_FLAG_XFER_MODE); /* PIO */

	return lax_cmd_rw(lba, block, flag);
}

static void lax_map_rw_buf(void)
{
	unsigned long offset = 0;

	if(rw_buf == NULL) {
		rw_buf = mmap(0, rw_buf_len, PROT_READ | PROT_EXEC, MAP_FILE | MAP_PRIVATE,
				lax_fd, offset);
		if (rw_buf == (void *)-1) {
			printf("mmap failed\n");
			exit(1);
		}
	}
}

void lax_dump_rw_buf(unsigned long size)
{
	unsigned char *p = rw_buf;
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



