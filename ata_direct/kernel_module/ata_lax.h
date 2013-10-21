#ifndef _ATA_LAX_H
#define _ATA_LAX_h

#include <linux/compiler.h>

struct lax_io_rext_pio {
	unsigned long lba;
	unsigned long block;
	void __user *buf;
};

#endif
