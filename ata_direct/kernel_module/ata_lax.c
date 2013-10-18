/*
 * LAX = Linux MAX
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/ata.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/cdev.h>

MODULE_LICENSE("Dual BSD/GPL");

struct ata_io {
	void __iomem    *base_addr;
	void __iomem    *data_addr;
	void __iomem    *error_addr;
	void __iomem    *feature_addr;
	void __iomem    *nsect_addr;
	void __iomem    *lbal_addr;
	void __iomem    *lbam_addr;
	void __iomem    *lbah_addr;
	void __iomem    *device_addr;
	void __iomem    *status_addr;
	void __iomem    *command_addr;
	void __iomem    *altstatus_addr;
	void __iomem    *ctl_addr;
};

struct ata_lax_dev {
	struct cdev cdev;
};

static struct ata_io mod_ioaddr;
static struct ata_lax_dev mod_dev;

/* Default is likely to be /dev/sdb */
static int ata_ctl_index = 0;
static int ata_port_index = 1;

static int ata_lax_major = 0;
static int ata_lax_minor =   0;

static char *ata_lax_name = "ata_lax";

module_param(ata_ctl_index, int, 0);
module_param(ata_port_index, int, 0);
module_param(ata_lax_major, int, 0);
module_param(ata_lax_minor, int, 0);

bool ata_busy_wait(u32 wait_msec)
{
	u32 i = 0;
	u32 altstatus;

	do {
		altstatus = ioread8(mod_ioaddr.altstatus_addr);

		if(altstatus & ATA_BUSY) {
			msleep(1);
		}
		else {
			break;
		}
		i ++;
	} while (i < wait_msec);

	if(altstatus & ATA_BUSY){
		printk(KERN_ERR "ata device busy after %d msec wait\n", wait_msec);
	  	return false;
	}
	else {
		return true;
	}
}


void ata_cmd_uv_issue( u8 feature )
{
	bool ready;

	ready = ata_busy_wait(100);
	if(ready != true){
		goto end;
	}

	/* write feature */
	iowrite8(feature, mod_ioaddr.feature_addr);

	/* write command */
	iowrite8(0xFF, mod_ioaddr.command_addr);


	printk(KERN_INFO "Issue UV command with feature 0x%x\n", feature);

end:
	return;
}




static void ata_init_ioaddr(struct ata_io *ioaddr, void __iomem * const * iomap, int ata_port)
{
	int mem_off = ata_port * 2;

	ioaddr->base_addr = iomap[mem_off];
	ioaddr->altstatus_addr =
	ioaddr->ctl_addr = (void __iomem *)
		((unsigned long)iomap[mem_off + 1] | ATA_PCI_CTL_OFS );

	ioaddr->data_addr = ioaddr->base_addr + ATA_REG_DATA;
	ioaddr->error_addr = ioaddr->base_addr + ATA_REG_ERR;
	ioaddr->feature_addr = ioaddr->base_addr + ATA_REG_FEATURE;
	ioaddr->nsect_addr = ioaddr->base_addr + ATA_REG_NSECT;
	ioaddr->lbal_addr = ioaddr->base_addr + ATA_REG_LBAL;
	ioaddr->lbam_addr = ioaddr->base_addr + ATA_REG_LBAM;
	ioaddr->lbah_addr = ioaddr->base_addr + ATA_REG_LBAH;
	ioaddr->device_addr = ioaddr->base_addr + ATA_REG_DEVICE;
	ioaddr->status_addr = ioaddr->base_addr + ATA_REG_STATUS;
	ioaddr->command_addr = ioaddr->base_addr + ATA_REG_CMD;
}

/*
 * libata-sff.c
 */
static int ata_resources_present(struct pci_dev *pdev, int port)
{
	int i;

	/* Check the PCI resources for this channel are enabled */
	port = port * 2;
	for (i = 0; i < 2; i++) {
		if (pci_resource_start(pdev, port + i) == 0 ||
		    pci_resource_len(pdev, port + i) == 0)
			return 0;
	}
	return 1;
}

static int ata_scan_pci_info(void)
{
	struct pci_dev *pdev = NULL;
	u16 class, class2;
	void __iomem * const * iomap;
	int ata_ctl_i = -1;

	/*
	 * Reference: kernel/drivers/ata/libata-sff.c
	 */
	for_each_pci_dev(pdev) {
		class = pdev->class >> 8;

		pci_read_config_word(pdev, PCI_CLASS_DEVICE, &class2);
		printk(KERN_INFO "pci %x %x %x class:(%x == %x)\n",
			pdev->vendor, pdev->device,  pdev->devfn, class, class2 );
		printk(KERN_INFO "  name %s\n", pdev->dev.kobj.name);

		switch (class) {
		case PCI_CLASS_STORAGE_IDE:
		case PCI_CLASS_STORAGE_RAID:
		case PCI_CLASS_STORAGE_SATA:
		case 0x105: /* ATA controller, not defined in pci_ids.h */
			ata_ctl_i ++;
			break;
		default:
			break;
		}

		if(ata_ctl_i == ata_ctl_index){
			break;
		}
	}


	printk(KERN_INFO "%d %d %d\n",ata_ctl_i, ata_ctl_index, ata_port_index);
	if (ata_ctl_i == ata_ctl_index) {
		iomap = pcim_iomap_table(pdev);
		if(ata_resources_present(pdev, ata_port_index)){
			ata_init_ioaddr(&mod_ioaddr, iomap, ata_port_index);
			printk(KERN_INFO "  ATA port%d  initialized\n", ata_port_index);
			return 0;
		}
	}

	return -1;
}

long ata_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	if ( cmd == 0xFF ) {
		/* only support VU command */
		ata_cmd_uv_issue(arg);
	}
	else {
		printk(KERN_WARNING "not supported command 0x%x", cmd);
		retval = -EPERM;
	}

	return 0;
}

int ata_file_open(struct inode *inode, struct file *filp)
{
	if ( ata_scan_pci_info() == 0 ) {
		return 0;
	}
	else {
		return -EBADSLT; /* invalid slot */
	}
}

int ata_file_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ata_file_ioctl,
	.open = ata_file_open,
	.release = ata_file_release,
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

	cdev_init(&mod_dev.cdev, &fops);
	mod_dev.cdev.owner = THIS_MODULE;
	if(cdev_add(&mod_dev.cdev, devno, 1)) {
		printk(KERN_WARNING "Cannot add ata lax major %d\n", ata_lax_major);
	}

	printk(KERN_ALERT "Hello, ATA LAX Module loaded\n");

	return 0;
}

static void ata_lax_exit_module(void)
{
	printk(KERN_ALERT "Goodbye, ATA LAX Module unloaded\n");
}

module_init(ata_lax_init_module);
module_exit(ata_lax_exit_module);
