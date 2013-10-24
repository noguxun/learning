#ifndef _LAX_MODULE_H
#define _LAX_MODULE_H

#include <linux/compiler.h>

struct lax_io_rext_pio {
	unsigned long lba;
	unsigned long block;
	void __user *buf;
};

#endif
