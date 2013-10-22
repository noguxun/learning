#include <stdio.h>
#include <stdlib.h>


#define ATA_CMD_PIO_READ_EXT  (0x24)
#define ATA_CMD_VU            (0xFF)

#define ATA_FEATURE_2B  (0x2B)

struct lax_io_rext_pio {
	unsigned long lba;
	unsigned long block;
	void *buf;
};


static char *lax_file = "/dev/ata_lax";
static int  lax_fd = 0;


void lax_open(void)
{
	if(lax_fd) {
		close(lax_fd);
	}
	lax_fd = open(lax_file, 0);
}


void lax_close(int input)
{
	if(lax_fd) {
		close(lax_file);
	}
}

void lax_command_vu_2b(long fea)
{
	int cmd = ATA_CMD_VU;

	printf("ioctl simple vu 0x%x 0x%lx \n", cmd, fea);
	ioctl(lax_fd, cmd, fea);
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



