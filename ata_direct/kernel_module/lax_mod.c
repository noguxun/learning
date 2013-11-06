/*
 * LAX = Linux MAX
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#include "lax_common.h"
#include "lax_ahci.h"


/* Default is likely to be /dev/sdb */
static int ata_ctl_index = 0;    /* controller index, could have more than one controller */
static int ata_port_index = 1;   /* port index of the selected controller */

static int ata_lax_major = 0;
static int ata_lax_minor = 0;

module_param(ata_ctl_index, int, 0);
module_param(ata_port_index, int, 0);
module_param(ata_lax_major, int, 0);
module_param(ata_lax_minor, int, 0);

MODULE_LICENSE("Dual BSD/GPL");

struct ata_lax_dev {
	struct cdev cdev;
};


static char *ata_lax_name = "lax";
static struct ata_lax_dev mod_dev;


static struct file_operations fops_ahci = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = ahci_file_ioctl,
	.open           = ahci_file_open,
	.release        = ahci_file_release,
	.mmap           = ahci_file_mmap,
};

static int ata_lax_init_module(void)
{
	int result;
	dev_t devno = 0;

	if (ata_lax_major) {
		devno = MKDEV(ata_lax_major, ata_lax_minor);
		result = register_chrdev_region(devno, 1, ata_lax_name);
	} else {
		result = alloc_chrdev_region(&devno, ata_lax_minor, 1, ata_lax_name);
		ata_lax_major = MAJOR(devno);
	}

	if (result < 0) {
		printk(KERN_WARNING "Cannot get ata lax major %d\n", ata_lax_major);
	}

	cdev_init(&mod_dev.cdev, &fops_ahci);
	mod_dev.cdev.owner = THIS_MODULE;
	if(cdev_add(&mod_dev.cdev, devno, 1)) {
		printk(KERN_WARNING "Cannot add ata lax major %d\n", ata_lax_major);
	}

	ahci_module_init(ata_port_index);

	printk(KERN_ALERT "Hello, ATA LAX Module loaded\n");

	return 0;
}

static void ata_lax_exit_module(void)
{
	dev_t devno = MKDEV(ata_lax_major, ata_lax_minor);

	ahci_module_deinit();
	printk(KERN_ALERT "Goodbye, ATA LAX Module unloaded\n");

	cdev_del(&mod_dev.cdev);

	unregister_chrdev_region(devno, 1);
}

module_init(ata_lax_init_module);
module_exit(ata_lax_exit_module);
