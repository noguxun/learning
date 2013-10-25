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


void ata_ahci_set_para(int port_i)
{
	lax.port_index = port_i;
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

void ata_tf_to_fis(const struct ata_taskfile *tf, u8 pmp, int is_cmd, u8 *fis)
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

static void ahci_cmd_prep_nodata(const struct ata_taskfile *tf, unsigned int tag)
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
	opts = cmd_fis_len | n_elem << 16 | (0 << 12);
	if (tf->flags & ATA_TFLAG_WRITE) {
		opts |= AHCI_CMD_WRITE;
	}

	ahci_fill_cmd_slot(port, tag, opts);
}

static void ahci_cmd_issue_nodata(unsigned int tag)
{
	struct lax_port *port = &(lax.ports[lax.port_index]);

	writel(1 << tag, port->ioport_base + PORT_CMD_ISSUE);
}

static int ahci_init_port(int portno)
{
	/* no FBS(fis based switching) support*/
	dma_addr_t mem_dma;
	void * mem;
	size_t dma_sz;
	struct lax_port *port = &lax.ports[portno];

	port->ioport_base = lax.iohba_base + 0x100 + (portno * 0x80);

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

	return 0;
}

int ahci_init(void)
{
	struct pci_dev *pdev = NULL;
	bool found = false;
	void __iomem * const * iomap;
	int i;

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

	for(i = 0; i < AHCI_MAX_PORTS; i++) {
		ahci_init_port(i);
	}

	return 0;
}

int ata_ahci_file_open(struct inode *inode, struct file *filp)
{

	return 0;
}


int ata_ahci_file_release(struct inode *inode, struct file *filp)
{
	return 0;
}

