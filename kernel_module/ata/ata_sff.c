/*
 * SFF = Small Form Factor
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/ata.h>

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



static void ata_init_ioports(struct ata_io *ports, void __iomem * const * iomap, int ata_port)
{
	int mem_off = ata_port * 2;

	ports->base_addr = iomap[mem_off];
	ports->altstatus_addr =
	ports->ctl_addr = (void __iomem *)
		((unsigned long)iomap[mem_off + 1] | ATA_PCI_CTL_OFS );

	ports->data_addr = ports->base_addr + ATA_REG_DATA;
	ports->error_addr = ports->base_addr + ATA_REG_ERR;
	ports->feature_addr = ports->base_addr + ATA_REG_FEATURE;
	ports->nsect_addr = ports->base_addr + ATA_REG_NSECT;
	ports->lbal_addr = ports->base_addr + ATA_REG_LBAL;
	ports->lbam_addr = ports->base_addr + ATA_REG_LBAM;
	ports->lbah_addr = ports->base_addr + ATA_REG_LBAH;
	ports->device_addr = ports->base_addr + ATA_REG_DEVICE;
	ports->status_addr = ports->base_addr + ATA_REG_STATUS;
	ports->command_addr = ports->base_addr + ATA_REG_CMD;
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

static void scan_pci_info(void)
{
	struct pci_dev *pdev = NULL;
	u16 class, class2;
	void __iomem * const * iomap;
	struct ata_io io;

	/*
	 * Reference: kernel/drivers/ata/libata-sff.c
	 */
	for_each_pci_dev(pdev){
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
			iomap = pcim_iomap_table(pdev);
			if(ata_resources_present(pdev, 0)){
				ata_init_ioports(&io, iomap, 0);
				printk(KERN_INFO "  STORAGE port 0 initialized\n");
			}
			else {
				printk(KERN_INFO "  STORAGE port 0 not exists\n");
			}
			if(ata_resources_present(pdev, 1)){
				ata_init_ioports(&io, iomap, 1);
				printk(KERN_INFO "  STORAGE port 1 initialized\n");
			}
			else {
				printk(KERN_INFO "  STORAGE port 1 not exists\n");
			}

			break;
		default:
			break;
		}
	}
}

static int sff_init(void)
{
	printk(KERN_ALERT "Hello, world\n");
	scan_pci_info();
	return 0;
}

static void sff_exit(void)
{
	printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(sff_init);
module_exit(sff_exit);
