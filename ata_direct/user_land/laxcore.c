#include "stdio.h"

#define ATA_CMD_VU   (0xFF)

#define ATA_FEATURE_2B  (0x2B)

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

void lax_command(int cmd, long fea)
{
	printf("ioctl 0x%x 0x%lx \n", cmd, fea);
	ioctl(lax_fd, cmd, fea);
}
