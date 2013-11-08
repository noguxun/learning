#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <dlfcn.h>


#define ATA_CMD_PIO_READ_EXT  (0x24)
#define ATA_CMD_VU            (0xFF)

#define ATA_FEATURE_2B  (0x2B)

struct lax_io_rext_pio {
	unsigned long lba;
	unsigned long block;
	void *buf;
};


static char *lax_file = "/dev/lax";
static int  lax_fd = 0;


int lax_open(void)
{
	if(lax_fd) {
		close(lax_fd);
	}
	lax_fd = open(lax_file, O_RDONLY);

	return lax_fd;
}


void lax_close(void)
{
	if(lax_fd) {
		close(lax_fd);
	}
}

void lax_command_vu_2b(long fea)
{
	int cmd = ATA_CMD_VU;

	printf("PLEASE DO NOT CALL THIS\n");
	printf("ioctl simple vu 0x%x 0x%lx \n", cmd, fea);
	ioctl(lax_fd, cmd, fea);
}

void lax_command_simple(int cmd, long feature)
{
	printf("ioctl simple cmd  0x%x 0x%lx \n", cmd, feature);
	ioctl(lax_fd, cmd, feature);
}

void lax_command_rext_pio(unsigned long lba, unsigned short block, unsigned char data[])
{
	int i = 0;
	unsigned long byte_num = 512 * block;
	int cmd = ATA_CMD_PIO_READ_EXT;
        struct lax_io_rext_pio arg;

	arg.buf = data;
	arg.block = block;
	arg.lba = lba;

	ioctl(lax_fd, cmd, &arg);

	/*
	for(i = 0; i < byte_num; i++) {
		unsigned char *p = (unsigned char*)arg.buf;
		printf("%2x ", p[i]);
		if(i%16 == 15){
			printf("\n");
		}
	}
	*/
}

void lax_tst_mmap(void)
{
	unsigned long offset, len;
	void *addr;
	unsigned char * p;

	len = ( 512 );
	offset = 0;

	printf("\n about to map\n");
	addr = mmap(0, len, PROT_READ, MAP_FILE | MAP_PRIVATE, lax_fd, offset);
	printf("\n mmap called");
	if (addr == (void *)-1) {
		printf("mmap failed\n");
		exit(1);
	}

	p = (unsigned char *)addr;
	printf("\n mapped data: %d %d %d %d %d \n", p[0], p[1], p[2], p[3], p[4]);

	munmap(addr, len);
}


void main1(void)
{
	lax_open();
	lax_tst_mmap();
	lax_close();
}


int (*lax_open1)(void);
void (*lax_tst_mmap1)(void);
void (*lax_close1)(void);

void main(void)
{
	void *handle;

	handle = dlopen("liblaxcore.so", RTLD_NOW);
	printf("hanle %p \n", handle);
	lax_open1 = dlsym(handle, "lax_open");
	lax_tst_mmap1 = dlsym(handle, "lax_tst_mmap");
	lax_close1 = dlsym(handle, "lax_close");

	lax_open1();
	lax_tst_mmap1();
	lax_close1();

}