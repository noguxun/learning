#include <linux/kernel.h>
#include <linux/ata.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#include "lax_common.h"
#include "lax_ahci.h"

/*
 * only the port is configurable
 * the first the controller will be used
 */
static int ata_port_index = 0;

void ata_ahci_set_para(int port_i)
{
	ata_port_index = port_i;
}

long ata_ahci_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	if (cmd == 0xFF) {
		/* only support VU command */
		//ata_cmd_uv_issue(arg);
	}
	else  if(cmd == ATA_CMD_PIO_READ_EXT){
		//ata_cmd_rext_pio(arg);
	}
	else {
		printk(KERN_WARNING "not supported command 0x%x", cmd);
		retval = -EPERM;
	}

	return 0;
}

int ata_ahci_file_open(struct inode *inode, struct file *filp)
{
	struct pci_dev *pdev = NULL;
	bool found = false;

	for_each_pci_dev(pdev) {
		u16 class = pdev->class >> 8;

		if (class == PCI_CLASS_STORAGE_RAID || class == PCI_CLASS_STORAGE_SATA) {
			found = true;
			printk(KERN_INFO "found AHCI PCI controller, class type %x", class);
		 	break;
		}
	}

	if(found == false) {
		VPRINTK(KERN_ERR "AHCI device NOT found \n");
		return -EBADSLT;
	}

	return 0;
}


int ata_ahci_file_release(struct inode *inode, struct file *filp)
{
	return 0;
}

