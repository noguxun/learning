/* this file is not maintained anymore */
#include <linux/kernel.h>
#include <linux/ata.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#include "lax_common.h"
#include "lax_sff.h"

struct lax_io_rext_pio {
	unsigned long lba;
	unsigned long block;
	void __user *buf;
};

struct lax_io_rext_dma {
	unsigned long lba;
	unsigned long block;
}


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

static int ata_ctl_index = 0;
static int ata_port_index = 1;

static struct ata_io mod_ioaddr;

static bool ata_busy_wait(u32 wait_msec);

void ata_tf_fill_lba_block(u32 lba, u16 block, struct ata_taskfile *tf)
{
	union u32_b {
		u32  data;
		u8   b[4];
	}u32_data;

	union u16_b {
		u16 data;
		u8   b[4];
	}u16_data;

	u32_data.data = lba;
	tf->hob_lbah = 0;
	tf->hob_lbam = 0;
	tf->hob_lbal = u32_data.b[3];
	tf->lbah = u32_data.b[2];
	tf->lbam = u32_data.b[1];
	tf->lbal = u32_data.b[0];

	u16_data.data = block;
	tf->hob_nsect = u16_data.b[1];
	tf->nsect = u16_data.b[0];

	tf->device = 0xa0 | 0x40; /* using lba, copied code from hparm*/
	tf->feature = 0x0;

	tf->flags = ATA_TFLAG_ISADDR | ATA_TFLAG_LBA48 | ATA_TFLAG_DEVICE;
}

void ata_tf_write(const struct ata_taskfile *tf)
{
	struct ata_io *ioaddr = &mod_ioaddr;
	unsigned int is_addr = tf->flags & ATA_TFLAG_ISADDR;

	if (is_addr && (tf->flags & ATA_TFLAG_LBA48)) {
		WARN_ON_ONCE(!ioaddr->ctl_addr);
		iowrite8(tf->hob_feature, ioaddr->feature_addr);
		iowrite8(tf->hob_nsect, ioaddr->nsect_addr);
		iowrite8(tf->hob_lbal, ioaddr->lbal_addr);
		iowrite8(tf->hob_lbam, ioaddr->lbam_addr);
		iowrite8(tf->hob_lbah, ioaddr->lbah_addr);
		VPK("hob: feat 0x%X nsect 0x%X, lba 0x%X 0x%X 0x%X\n",
			tf->hob_feature,
			tf->hob_nsect,
			tf->hob_lbal,
			tf->hob_lbam,
			tf->hob_lbah);
	}

	if (is_addr) {
		iowrite8(tf->feature, ioaddr->feature_addr);
		iowrite8(tf->nsect, ioaddr->nsect_addr);
		iowrite8(tf->lbal, ioaddr->lbal_addr);
		iowrite8(tf->lbam, ioaddr->lbam_addr);
		iowrite8(tf->lbah, ioaddr->lbah_addr);
		VPK("feat 0x%X nsect 0x%X lba 0x%X 0x%X 0x%X\n",
			tf->feature,
			tf->nsect,
			tf->lbal,
			tf->lbam,
			tf->lbah);
	}

	if (tf->flags & ATA_TFLAG_DEVICE) {
		iowrite8(tf->device, ioaddr->device_addr);
		VPK("device 0x%X\n", tf->device);
	}

	ata_busy_wait(100);
}

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

bool ata_drq_wait(u32 wait_msec)
{
	u32 i = 0;
	u32 altstatus;

	do {
		altstatus = ioread8(mod_ioaddr.altstatus_addr);

		if((ATA_DRDY | ATA_DRQ) == ((ATA_BUSY | ATA_DRDY | ATA_DRQ) & altstatus)) {
			ioread8(mod_ioaddr.status_addr);
			return true;
		}
		else {
			msleep(1);
		}
		i ++;
	} while (i < wait_msec);

	return false;
}


void ata_cmd_rext_pio( unsigned long user_arg )
{
	int i,j;
	struct lax_io_rext_pio arg;
	unsigned long byte_num;
	unsigned char *buf, *p;
	struct ata_taskfile tf;

	copy_from_user(&arg, (void *)user_arg, sizeof(struct lax_io_rext_pio));

	printk(KERN_INFO "block %ld lba %ld\n", arg.block, arg.lba);

	byte_num = 512 * arg.block;
	buf = kmalloc(byte_num, GFP_KERNEL);
	if(!buf){
		return;
	}


	for(i = 0; i < byte_num; i++) {
		/* buf[i] = (i % 0xFF) + 1; */
		buf[i] = 0;
	}


	ata_busy_wait(100);
	ata_tf_fill_lba_block(arg.lba, arg.block, &tf);
	ata_tf_write(&tf);

	msleep(1);

	iowrite8(ATA_CMD_PIO_READ_EXT, mod_ioaddr.command_addr);

	msleep(1);

	p = buf;
	for(j = 0; j < arg.block; j++) {
		u16 *p2 = (u16 *)p;
		if(ata_drq_wait(100) == false){
			printk(KERN_INFO "waitng DRQ error\n");
			break;
		}
		for(i = 0; i < 256; i++) {
			printk(KERN_INFO "reading from %p\n", mod_ioaddr.data_addr);
			*p2 = (u16)ioread16(mod_ioaddr.data_addr);
			p2++;
		}
		p += 512;
	}


	copy_to_user(arg.buf, buf, byte_num);

	kfree(buf);

	/*

	ata_busy_wait(100);

	ata_tf_write(tf);

	iowrite8(ATA_CMD_PIO_READ_EXT, mod_ioaddr.command_addr);
	wait for some time
	DRQ check, read data
	*/
}


void ata_cmd_uv_issue(u8 feature)
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

	printk(KERN_INFO "Issue UV command with feature 0x %x to ports: command %p, feature %p\n",
		feature, mod_ioaddr.command_addr, mod_ioaddr.feature_addr);

end:
	return;
}

static void ata_init_ioaddr(struct ata_io *ioaddr, void __iomem * const * iomap, int ata_port)
{
	int mem_off = ata_port * 2;

	printk(KERN_INFO "IO base address is %p", iomap[mem_off]);
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
			printk(KERN_INFO "found storage PCI controller, class type %x", class);
			ata_ctl_i ++;
			break;
		default:
			break;
		}

		if(ata_ctl_i == ata_ctl_index){
			break;
		}
	}

	//printk(KERN_INFO "%d %d %d\n",ata_ctl_i, ata_ctl_index, ata_port_index);
	if (ata_ctl_i == ata_ctl_index) {
		iomap = pcim_iomap_table(pdev);
		printk(KERN_INFO "pci iomap %p %p %p %p", iomap, iomap[0], iomap[1], iomap[5]);
		if(ata_resources_present(pdev, ata_port_index)){
			ata_init_ioaddr(&mod_ioaddr, iomap, ata_port_index);
			printk(KERN_INFO "  ATA port%d  initialized\n", ata_port_index);
			return 0;
		}
	}

	return -1;
}

void ata_sff_set_para(int ctl_i, int port_i)
{
	ata_ctl_index = ctl_i;
	ata_port_index = port_i;
}

long ata_sff_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	if (cmd == 0xFF) {
		/* only support VU command */
		ata_cmd_uv_issue(arg);
	}
	else  if(cmd == ATA_CMD_PIO_READ_EXT){
		ata_cmd_rext_pio(arg);
	}
	else {
		printk(KERN_WARNING "not supported command 0x%x", cmd);
		retval = -EPERM;
	}

	return 0;
}

int ata_sff_file_open(struct inode *inode, struct file *filp)
{
	if ( ata_scan_pci_info() == 0 ) {
		return 0;
	}
	else {
		return -EBADSLT; /* invalid slot */
	}
}

int ata_sff_file_release(struct inode *inode, struct file *filp)
{
	return 0;
}



