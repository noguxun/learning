#ifndef _LAX_AHCI_H
#define _LAX_AHCI_H

void ahci_module_init(int port_i);
void ahci_module_deinit(void);
long ahci_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int ahci_file_open(struct inode *inode, struct file *filp);
int ahci_file_release(struct inode *inode, struct file *filp);
int ahci_file_mmap(struct file *filp, struct vm_area_struct *vma);

enum {
	LAX_CMD_TST_START = 0x101,
	LAX_CMD_TST_PORT_INIT,
	LAX_CMD_TST_VU_2B,
	LAX_CMD_TST_PRINT_REGS,
	LAX_CMD_TST_PORT_RESET,
	LAX_CMD_TST_ID,
	LAX_CMD_TST_RW,
	LAX_CMD_TST_RESTORE_IRQ,


	RW_FLAG_XFER_MODE       = (1 << 1), /* PIO 0,  DMA 1 */
	RW_FLAG_DIRECTION       = (1 << 0), /* READ 0, WRITE 1*/

};

enum {
	AHCI_PCI_BAR_STANDARD = 5,

	AHCI_MAX_PORTS		= 32,
	AHCI_MAX_SG		= 168, /* hardware max is 64K */
	AHCI_DMA_BOUNDARY	= 0xffffffff,
	AHCI_MAX_CMDS		= 32,
	AHCI_CMD_SZ		= 32,
	AHCI_CMD_SLOT_SZ	= AHCI_MAX_CMDS * AHCI_CMD_SZ,
	AHCI_RX_FIS_SZ		= 256,
	AHCI_CMD_TBL_CDB	= 0x40,
	AHCI_CMD_TBL_HDR_SZ	= 0x80,
	AHCI_CMD_TBL_SZ		= AHCI_CMD_TBL_HDR_SZ + (AHCI_MAX_SG * 16),
	AHCI_CMD_TBL_AR_SZ	= AHCI_CMD_TBL_SZ * AHCI_MAX_CMDS,
	AHCI_PORT_PRIV_DMA_SZ	= AHCI_CMD_SLOT_SZ + AHCI_CMD_TBL_AR_SZ +
				  AHCI_RX_FIS_SZ,

	AHCI_IRQ_ON_SG		= (1 << 31),
	AHCI_CMD_ATAPI		= (1 << 5),
	AHCI_CMD_WRITE		= (1 << 6),
	AHCI_CMD_PREFETCH	= (1 << 7),
	AHCI_CMD_RESET		= (1 << 8),
	AHCI_CMD_CLR_BUSY	= (1 << 10),

	/* global controller registers */
	HOST_CAP		= 0x00, /* host capabilities */
	HOST_CTL		= 0x04, /* global host control */
	HOST_IRQ_STAT		= 0x08, /* interrupt status */
	HOST_PORTS_IMPL		= 0x0c, /* bitmap of implemented ports */
	HOST_VERSION		= 0x10, /* AHCI spec. version compliancy */
	HOST_EM_LOC		= 0x1c, /* Enclosure Management location */
	HOST_EM_CTL		= 0x20, /* Enclosure Management Control */
	HOST_CAP2		= 0x24, /* host capabilities, extended */

	/* HOST_CTL bits */
	HOST_RESET		= (1 << 0),  /* reset controller; self-clear */
	HOST_IRQ_EN		= (1 << 1),  /* global IRQ enable */
	HOST_AHCI_EN		= (1 << 31), /* AHCI enabled */

	/* HOST_CAP bits */
	HOST_CAP_SXS		= (1 << 5),  /* Supports External SATA */
	HOST_CAP_EMS		= (1 << 6),  /* Enclosure Management support */
	HOST_CAP_CCC		= (1 << 7),  /* Command Completion Coalescing */
	HOST_CAP_PART		= (1 << 13), /* Partial state capable */
	HOST_CAP_SSC		= (1 << 14), /* Slumber state capable */
	HOST_CAP_PIO_MULTI	= (1 << 15), /* PIO multiple DRQ support */
	HOST_CAP_FBS		= (1 << 16), /* FIS-based switching support */
	HOST_CAP_PMP		= (1 << 17), /* Port Multiplier support */
	HOST_CAP_ONLY		= (1 << 18), /* Supports AHCI mode only */
	HOST_CAP_CLO		= (1 << 24), /* Command List Override support */
	HOST_CAP_LED		= (1 << 25), /* Supports activity LED */
	HOST_CAP_ALPM		= (1 << 26), /* Aggressive Link PM support */
	HOST_CAP_SSS		= (1 << 27), /* Staggered Spin-up */
	HOST_CAP_MPS		= (1 << 28), /* Mechanical presence switch */
	HOST_CAP_SNTF		= (1 << 29), /* SNotification register */
	HOST_CAP_NCQ		= (1 << 30), /* Native Command Queueing */
	HOST_CAP_64		= (1 << 31), /* PCI DAC (64-bit DMA) support */

	/* registers for each SATA port */
	PORT_LST_ADDR		= 0x00, /* command list DMA addr */
	PORT_LST_ADDR_HI	= 0x04, /* command list DMA addr hi */
	PORT_FIS_ADDR		= 0x08, /* FIS rx buf addr */
	PORT_FIS_ADDR_HI	= 0x0c, /* FIS rx buf addr hi */
	PORT_IRQ_STAT		= 0x10, /* interrupt status */
	PORT_IRQ_MASK		= 0x14, /* interrupt enable/disable mask */
	PORT_CMD		= 0x18, /* port command */
	PORT_TFDATA		= 0x20,	/* taskfile data */
	PORT_SIG		= 0x24,	/* device TF signature */
	PORT_CMD_ISSUE		= 0x38, /* command issue */
	PORT_SCR_STAT		= 0x28, /* SATA phy register: SStatus */
	PORT_SCR_CTL		= 0x2c, /* SATA phy register: SControl */
	PORT_SCR_ERR		= 0x30, /* SATA phy register: SError */
	PORT_SCR_ACT		= 0x34, /* SATA phy register: SActive */
	PORT_SCR_NTF		= 0x3c, /* SATA phy register: SNotification */
	PORT_FBS		= 0x40, /* FIS-based Switching */
	PORT_DEVSLP		= 0x44, /* device sleep */

	/* PORT_IRQ_{STAT,MASK} bits */
	PORT_IRQ_COLD_PRES	= (1 << 31), /* cold presence detect */
	PORT_IRQ_TF_ERR		= (1 << 30), /* task file error */
	PORT_IRQ_HBUS_ERR	= (1 << 29), /* host bus fatal error */
	PORT_IRQ_HBUS_DATA_ERR	= (1 << 28), /* host bus data error */
	PORT_IRQ_IF_ERR		= (1 << 27), /* interface fatal error */
	PORT_IRQ_IF_NONFATAL	= (1 << 26), /* interface non-fatal error */
	PORT_IRQ_OVERFLOW	= (1 << 24), /* xfer exhausted available S/G */
	PORT_IRQ_BAD_PMP	= (1 << 23), /* incorrect port multiplier */

	PORT_IRQ_PHYRDY		= (1 << 22), /* PhyRdy changed */
	PORT_IRQ_DEV_ILCK	= (1 << 7), /* device interlock */
	PORT_IRQ_CONNECT	= (1 << 6), /* port connect change status */
	PORT_IRQ_SG_DONE	= (1 << 5), /* descriptor processed */
	PORT_IRQ_UNK_FIS	= (1 << 4), /* unknown FIS rx'd */
	PORT_IRQ_SDB_FIS	= (1 << 3), /* Set Device Bits FIS rx'd */
	PORT_IRQ_DMAS_FIS	= (1 << 2), /* DMA Setup FIS rx'd */
	PORT_IRQ_PIOS_FIS	= (1 << 1), /* PIO Setup FIS rx'd */
	PORT_IRQ_D2H_REG_FIS	= (1 << 0), /* D2H Register FIS rx'd */

	/* PORT_CMD bits */
	PORT_CMD_ASP		= (1 << 27), /* Aggressive Slumber/Partial */
	PORT_CMD_ALPE		= (1 << 26), /* Aggressive Link PM enable */
	PORT_CMD_ATAPI		= (1 << 24), /* Device is ATAPI */
	PORT_CMD_FBSCP		= (1 << 22), /* FBS Capable Port */
	PORT_CMD_PMP		= (1 << 17), /* PMP attached */
	PORT_CMD_LIST_ON	= (1 << 15), /* cmd list DMA engine running */
	PORT_CMD_FIS_ON		= (1 << 14), /* FIS DMA engine running */
	PORT_CMD_FIS_RX		= (1 << 4), /* Enable FIS receive DMA engine */
	PORT_CMD_CLO		= (1 << 3), /* Command list override */
	PORT_CMD_POWER_ON	= (1 << 2), /* Power up device */
	PORT_CMD_SPIN_UP	= (1 << 1), /* Spin up device */
	PORT_CMD_START		= (1 << 0), /* Enable port DMA engine */

	PORT_CMD_ICC_MASK	= (0xf << 28), /* i/f ICC state mask */
	PORT_CMD_ICC_ACTIVE	= (0x1 << 28), /* Put i/f in active state */
	PORT_CMD_ICC_PARTIAL	= (0x2 << 28), /* Put i/f in partial state */
	PORT_CMD_ICC_SLUMBER	= (0x6 << 28), /* Put i/f in slumber state */

};

#define LAX_SG_ENTRY_SIZE     (4l*1024l*1024l)
#define LAX_SG_COUNT          (8l)    /* 32M/4M */
#define LAX_SG_ALL_SIZE       ((LAX_SG_COUNT) * (LAX_SG_ENTRY_SIZE))
#define LAX_SECTOR_SIZE       (512l)

struct ahci_cmd_hdr {
	__le32			opts;
	__le32			status;
	__le32			tbl_addr;
	__le32			tbl_addr_hi;
	__le32			reserved[4];
};

struct lax_rw {
	unsigned long lba;
	unsigned long block;
	unsigned long flags;
};


struct lax_sg {
	void *mem;
	dma_addr_t mem_dma;
	u32 length;
};

struct lax_port {
	u32 irq_mask;
	void __iomem * ioport_base;

        struct ahci_cmd_hdr *cmd_slot;
	dma_addr_t cmd_slot_dma;

	void * rx_fis;
	dma_addr_t rx_fis_dma;

	void *cmd_tbl;
	dma_addr_t cmd_tbl_dma;

	struct lax_sg sg[LAX_SG_COUNT];
};

struct lax_ahci {
	struct pci_dev * pdev;
	void __iomem * iohba_base;
	struct lax_port ports[AHCI_MAX_PORTS];
	int port_index;
	bool port_initialized;
};

#endif
