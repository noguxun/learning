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

#include "../kernel/lax_share.h"

#define MAX_READ_WRITE_LEN     (32l * 1024l * 1024l)

static uint8_t *lax_wbuf = NULL;
static uint8_t *lax_rbuf = NULL;

static uint32_t lax_buf_len = MAX_READ_WRITE_LEN;

static uint8_t lax_wbuf_pat = 0xFF;

static void lax_rwbuf_map(void);

void lax_wbuf_set_pat_id(uint64_t lba, uint32_t block, uint8_t pat);


static char *lax_file = "/dev/lax";
static int  lax_fd = 0;


int lax_open(void)
{
	printf("lax_open\n");

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
	printf("lax_close\n");

	if(lax_rbuf) {
		size_t len = lax_buf_len * 2;
		munmap(lax_rbuf, len);
		lax_rbuf = NULL;
	}

	if(lax_fd) {
		close(lax_fd);
	}
}

void lax_command_port_reset(void)
{
	ioctl(lax_fd, LAX_CMD_PORT_RESET, 0);
}

void lax_command_port_reg_print(void)
{
	ioctl(lax_fd, LAX_CMD_PRINT_REGS, 0);
}

void lax_command_ata_direct(uint16_t feature, uint16_t block, uint64_t lba,
                            uint8_t ata_cmd, uint64_t buf_len, uint8_t ata_cmd_type)
{
	struct lax_ata_direct arg;

	arg.ata_cmd = ata_cmd;
	arg.feature = feature;
	arg.lba = lba;
	arg.block = block;
	arg.buf_len = buf_len;
	arg.ata_cmd_type = ata_cmd_type;

	ioctl(lax_fd, LAX_CMD_ATA_DIRECT, &arg);
}

static uint32_t lax_cmd_rw(uint64_t lba, uint32_t block, uint32_t flag)
{
	struct lax_rw arg;
	int retval;
	uint8_t * buf;

	arg.lba = lba;
	arg.block = block;
	arg.flags = flag;

	retval = ioctl(lax_fd, LAX_CMD_RW, &arg);

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

	lax_wbuf_set_pat_id(lba, block, lax_wbuf_pat);
	return lax_cmd_rw(lba, block, flag);
}

uint32_t lax_cmd_r_ncq(uint64_t lba, uint32_t block)
{
	uint32_t flag = 0;

	flag &= (~LAX_RW_FLAG_RW); /* READ */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */
	flag |= (LAX_RW_FLAG_NCQ);  /* NCQ */

	lax_wbuf_set_pat_id(lba, block, lax_wbuf_pat);
	return lax_cmd_rw(lba, block, flag);
}

uint32_t lax_cmd_w_ncq(uint64_t lba, uint32_t block)
{
	uint32_t flag = 0;

	flag |= (LAX_RW_FLAG_RW); /* WRITE */
	flag |= (LAX_RW_FLAG_XFER_MODE);  /* DMA */
	flag |= (LAX_RW_FLAG_NCQ);  /* NCQ */

	lax_wbuf_set_pat_id(lba, block, lax_wbuf_pat);

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

	lax_wbuf_set_pat_id(lba, block, lax_wbuf_pat);

	return lax_cmd_rw(lba, block, flag);
}

void lax_rbuf_clear(void)
{
	memset(lax_rbuf, 0, lax_buf_len);
}

void lax_wbuf_set_pat_id(uint64_t lba, uint32_t block, uint8_t pat)
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
	size_t length = lax_buf_len * 2;

	if(lax_rbuf != NULL) {
		return;
	}

	lax_rbuf = mmap(0, length, PROT_WRITE | PROT_READ, MAP_SHARED, lax_fd, 0);
	if (lax_rbuf == MAP_FAILED) {
		printf("mmap failed\n");
		exit(1);
	}

	lax_wbuf = (((uint8_t *)lax_rbuf) + lax_buf_len);
}

int lax_dump_buf(char *path, uint32_t size, uint8_t *buf)
{
	FILE *stream;
	long byte_write;

	if(path == NULL) {
		printf("Need to set the path\n");
		return -1;
	}

	printf("dumping to file %s\n",path);
	stream = fopen(path, "wb");
	if(stream == NULL) {
		printf("Failed to load file %s\n", path);
		return -1;
	}

	// Read microcode to write buffer
	byte_write = fwrite(buf, 1, size, stream);
	if(byte_write != size){
		printf("Failed to write size %d, only write %ld\n", size, byte_write);
		return -1;
	}

	fclose(stream);

	return 0;
}

void lax_dump_rbuf(char *path, uint32_t size)
{
	lax_dump_buf(path, size, lax_rbuf);
}

void lax_dump_wbuf(char *path, uint32_t size)
{
	lax_dump_buf(path, size, lax_wbuf);
}

void lax_wbuf_set_pat(uint8_t pat)
{
	lax_wbuf_pat = pat;
}

unsigned char * lax_get_rbuf(void)
{
	return lax_rbuf;
}

unsigned char * lax_get_wbuf(void)
{
	return lax_wbuf;
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


int lax_load_data_to_buf(char *path, uint8_t * buf)
{
	FILE *stream;
	long file_size;
	long byte_read;
	long read_size;

	if(path == NULL) {
		printf("Need to set the path\n");
		return -1;
	}

	stream = fopen(path, "rb");
	if(stream == NULL) {
		printf("Failed to load file %s\n", path);
		return -1;
	}

	fseek(stream, 0L, SEEK_END);
	file_size = ftell(stream);
	fseek(stream, 0L, SEEK_SET);

	read_size = file_size < lax_buf_len ? file_size : lax_buf_len;

	// Read microcode to write buffer
	byte_read = fread(buf, 1, read_size, stream);
	if(byte_read != read_size){
		printf("Failed to read size %ld, only read %ld\n", read_size, byte_read);
		return -1;
	}

	fclose(stream);

	return read_size;
}

int lax_load_data_to_wbuf(char *path)
{
	return lax_load_data_to_buf(path, lax_wbuf);
}

int lax_load_data_to_rbuf(char *path)
{
	return lax_load_data_to_buf(path, lax_rbuf);
}

int lax_download_microcode(char *path)
{
	int result = 0;
	result = lax_load_data_to_wbuf(path);
	if( result <= 0 ) {
		return result;
	}

	printf("Downloading microcode to device!\n");
	// now we can issue the command ...
        ioctl(lax_fd, LAX_CMD_MICRO_CODE, (uint16_t)(result/512));

	return 0;
}


