#include <linux/kernel.h>
#include <linux/ata.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include <asm-generic/dma-coherent.h>

#include "lax_common.h"
#include "lax_ahci.h"


struct lax_port {
	void __iomem * ioport_base;

        struct ahci_cmd_hdr *cmd_slot;
	dma_addr_t cmd_slot_dma;

	void * rx_fis;
	dma_addr_t rx_fis_dma;

	void *cmd_tbl;
	dma_addr_t cmd_tbl_dma;
};

struct lax_ahci {
	struct pci_dev * pdev;
	void __iomem * iohba_base;
	struct lax_port ports[AHCI_MAX_PORTS];
	int port_index;
};


/*
 * only the port is configurable
 * the first the controller will be used
 */

static struct lax_ahci lax = { NULL, NULL };

static void __iomem * get_base(void)
{
	struct lax_port *port = &(lax.ports[lax.port_index]);

	return port->ioport_base;
}

static void ahci_set_irq(u32 irq_max)
{
	iowrite32(irq_max, get_base() + PORT_IRQ_MASK);
}

static void ata_tf_to_fis(const struct ata_taskfile *tf, u8 pmp, int is_cmd, u8 *fis)
{
	fis[0] = 0x27;			/* Register - Host to Device FIS */
	fis[1] = pmp & 0xf;		/* Port multiplier number*/
	if (is_cmd)
		fis[1] |= (1 << 7);	/* bit 7 indicates Command FIS */

	fis[2] = tf->command;
	fis[3] = tf->feature;

	fis[4] = tf->lbal;
	fis[5] = tf->lbam;
	fis[6] = tf->lbah;
	fis[7] = tf->device;

	fis[8] = tf->hob_lbal;
	fis[9] = tf->hob_lbam;
	fis[10] = tf->hob_lbah;
	fis[11] = tf->hob_feature;

	fis[12] = tf->nsect;
	fis[13] = tf->hob_nsect;
	fis[14] = 0;
	fis[15] = tf->ctl;

	fis[16] = 0;
	fis[17] = 0;
	fis[18] = 0;
	fis[19] = 0;
}

static void ahci_fill_cmd_slot(struct lax_port *port,unsigned int tag, u32 opts)
{
	dma_addr_t cmd_tbl_dma;

	cmd_tbl_dma = port->cmd_tbl_dma + tag * AHCI_CMD_TBL_SZ;

	port->cmd_slot[tag].opts = cpu_to_le32(opts);
	port->cmd_slot[tag].status = 0;
	port->cmd_slot[tag].tbl_addr = cpu_to_le32(cmd_tbl_dma & 0xffffffff);
	port->cmd_slot[tag].tbl_addr_hi = cpu_to_le32((cmd_tbl_dma >> 16) >> 16);
}

static void ahci_cmd_prep_nodata(const struct ata_taskfile *tf, unsigned int tag, u32 cmd_opts)
{
	void * cmd_tbl;
	unsigned int n_elem;
	const u32 cmd_fis_len = 5; /* five dwords */
	u32 opts;
	struct lax_port *port = &(lax.ports[lax.port_index]);

	cmd_tbl = port->cmd_tbl + tag * AHCI_CMD_TBL_SZ;

	ata_tf_to_fis(tf, 0, 1, cmd_tbl);

	n_elem = 0;
	/*
	 * scatter list, n_elem = ahci_fill_sg() 4.2.3, offset 0x80
	 */

	/*
	 * Fill in command slot information.
	 */
	opts = cmd_fis_len | n_elem << 16 | (0 << 12) | cmd_opts;
	if (tf->flags & ATA_TFLAG_WRITE) {
		opts |= AHCI_CMD_WRITE;
	}

	ahci_fill_cmd_slot(port, tag, opts);
}

static bool ahci_busy_wait(u32 wait_msec, void __iomem *reg, u32 mask, u32 val)
{
	u32 i = 0;
	u32 reg_val;
	bool condition = false;

	do {
		reg_val = ioread32(reg);

		if((reg_val & mask) != val) {
			msleep(1);
		}
		else {
			break;
			condition = true;
		}
		i ++;
	} while (i < wait_msec);

	if(!condition){
		printk(KERN_ERR "wait failed on %d msec wait\n", wait_msec );
	  	return false;
	}
	else {
		return true;
	}
}

static void ahci_fill_info(u8 cmd, u16 feature, u32 lba, u16 block, struct ata_taskfile *tf)
{
	union u32_b {
		u32  data;
		u8   b[4];
	}u32_data;

	union u16_b {
		u16 data;
		u8   b[4];
	}u16_data;

	tf->command = cmd;

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

	u16_data.data = feature;
	tf->hob_feature = u16_data.b[1];
	tf->feature = u16_data.b[0];

	tf->device = 0xa0 | 0x40; /* using lba, copied code from hparm*/
	tf->feature = 0x0;

	tf->flags = ATA_TFLAG_ISADDR | ATA_TFLAG_LBA48 | ATA_TFLAG_DEVICE;
}

static void ahci_tf_init(struct ata_taskfile *tf)
{
	memset(tf, 0, sizeof(*tf));

	tf->device = ATA_DEVICE_OBS;
	tf->ctl = ATA_DEVCTL_OBS;     // obsolete bit in device control
}

static void ahci_exec_nodata(unsigned int tag, u8 command, u16 feature)
{
	struct ata_taskfile tf;
	void __iomem * reg_issue = get_base() + PORT_CMD_ISSUE;
	u32 bit_pos = 1 << tag;


	ahci_tf_init(&tf);
	ahci_fill_info(command, feature, 0, 0, &tf);

	ahci_cmd_prep_nodata(&tf, 0, AHCI_CMD_CLR_BUSY);

	iowrite32(bit_pos, reg_issue);

	if(ahci_busy_wait(100, reg_issue, bit_pos, bit_pos) == false) {
		printk(KERN_ERR "timeout exec command 0x%x\n", command);
	}
}

static void ahci_port_start(void)
{
	/* power up */
	u32 cmd, cap, tmp;
	void __iomem *port_mmio = get_base();
	struct lax_port *port = &lax.ports[lax.port_index];

	cap = ioread32(lax.iohba_base + HOST_CAP);

	cmd = ioread32(port_mmio + PORT_CMD) & ~ PORT_CMD_ICC_MASK;
	if(cap & HOST_CAP_SSS) {
		cmd |= PORT_CMD_SPIN_UP;
		iowrite32(cmd, port_mmio + PORT_CMD);
	}
	iowrite32(cmd | PORT_CMD_ICC_ACTIVE, port_mmio + PORT_CMD);


	/* enable fis reception */
	/* set FIS registers */
	if (cap & HOST_CAP_64) {
		iowrite32((port->cmd_slot_dma >> 16) >> 16, port_mmio + PORT_LST_ADDR_HI);
	}
	iowrite32(port->cmd_slot_dma & 0xffffffff, port_mmio + PORT_LST_ADDR);

	if (cap & HOST_CAP_64) {
		iowrite32((port->rx_fis_dma >> 16) >> 16, port_mmio + PORT_FIS_ADDR_HI);
	}
	iowrite32(port->rx_fis_dma & 0xffffffff, port_mmio + PORT_FIS_ADDR);

	/* enable FIS reception */
	tmp = ioread32(port_mmio + PORT_CMD);
	tmp |= PORT_CMD_FIS_RX;
	iowrite32(tmp, port_mmio + PORT_CMD);

	/* flush */
	ioread32(port_mmio + PORT_CMD);


	/* start engine */
	tmp = ioread32(port_mmio + PORT_CMD);
	tmp |= PORT_CMD_START;
	iowrite32(tmp, port_mmio + PORT_CMD);
	ioread32(port_mmio + PORT_CMD); /* flush */

}


static int ahci_init_port(void)
{
	/* no FBS(fis based switching) support*/
	dma_addr_t mem_dma;
	void * mem;
	size_t dma_sz;
	struct lax_port *port = &lax.ports[lax.port_index];

	printk(KERN_INFO "Init port 0x%x", lax.port_index);

	port->ioport_base = lax.iohba_base + 0x100 + (lax.port_index * 0x80);

	dma_sz = AHCI_PORT_PRIV_DMA_SZ;
	mem = dma_alloc_coherent(&(lax.pdev->dev), dma_sz, &mem_dma, GFP_KERNEL);
	if(!mem) {
		VPRINTK("memory allocation failed\n");
		return -ENOMEM;
	}
	memset(mem, 0 , dma_sz);

	port->cmd_slot = mem;
	port->cmd_slot_dma = mem_dma;

	mem += AHCI_CMD_SLOT_SZ;
	mem_dma += AHCI_CMD_SLOT_SZ;

	port->rx_fis = mem;
	port->rx_fis_dma = mem_dma;

	mem += AHCI_RX_FIS_SZ;
	mem_dma += AHCI_RX_FIS_SZ;

	port->cmd_tbl = mem;
	port->cmd_tbl_dma = mem_dma;

	// TODO: investigate if reset and init controller is needed or not
	ahci_port_start();

	ahci_set_irq(0x0);

	return 0;
}


void ata_ahci_set_para(int port_i)
{
	lax.port_index = port_i;
}

long ata_ahci_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	if (cmd == 0xFF) {
		/* only support VU command */
		ahci_exec_nodata(0, cmd, arg);

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

int ahci_init(void)
{
	struct pci_dev *pdev = NULL;
	bool found = false;
	void __iomem * const * iomap;

	if(lax.iohba_base) {
		return 0;
	}

	for_each_pci_dev(pdev) {
		u16 class = pdev->class >> 8;

		VPRINTK("dev class 0x%x\n", class);

		if (class == PCI_CLASS_STORAGE_RAID || class == PCI_CLASS_STORAGE_SATA) {
			found = true;
			VPRINTK("found AHCI PCI controller, class type 0x%x\n", class);
		 	break;
		}
	}

	if(found == false) {
		VPRINTK("AHCI device NOT found \n");
		return -EBADSLT;
	}

	lax.pdev = pdev;

	iomap = pcim_iomap_table(lax.pdev);
	lax.iohba_base = iomap[AHCI_PCI_BAR_STANDARD];

	ahci_init_port();

	return 0;
}

int ata_ahci_file_open(struct inode *inode, struct file *filp)
{
	ahci_init();
	return 0;
}


int ata_ahci_file_release(struct inode *inode, struct file *filp)
{
	return 0;
}

