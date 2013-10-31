#include <linux/kernel.h>
#include <linux/ata.h>
#include <linux/libata.h>
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
	bool initialized;
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


#define port_reg_print(reg) \
{\
	u32 value;\
	value = ioread32(get_base() + (reg));\
	VPK("%18s: offset 0x%.2x, value %.8X\n",#reg, (reg), value);\
}

#define hba_reg_print(reg) \
{\
	u32 value;\
	value = ioread32(lax.iohba_base + reg);\
	VPK("%18s: offset 0x%.2x, value %.8X\n",#reg, (reg), value);\
}

static void p_regs(void)
{
	printk(KERN_INFO "HBA REGISTERS: \n");
	hba_reg_print(HOST_CAP);
	hba_reg_print(HOST_CTL);
	hba_reg_print(HOST_IRQ_STAT);
	hba_reg_print(HOST_PORTS_IMPL);
	hba_reg_print(HOST_VERSION);
	hba_reg_print(HOST_EM_LOC);
	hba_reg_print(HOST_EM_CTL);
	hba_reg_print(HOST_CAP2);

	printk(KERN_INFO "PORT REGISTERS: \n");
	port_reg_print(PORT_LST_ADDR);
	port_reg_print(PORT_LST_ADDR_HI);
	port_reg_print(PORT_FIS_ADDR);
	port_reg_print(PORT_FIS_ADDR_HI);
	port_reg_print(PORT_IRQ_STAT);
	port_reg_print(PORT_IRQ_MASK);
	port_reg_print(PORT_CMD);
	port_reg_print(PORT_TFDATA);
	port_reg_print(PORT_SIG);
	port_reg_print(PORT_CMD_ISSUE);
	port_reg_print(PORT_SCR_STAT);
	port_reg_print(PORT_SCR_CTL);
	port_reg_print(PORT_SCR_ERR);
	port_reg_print(PORT_SCR_ACT);
	port_reg_print(PORT_SCR_NTF);
	port_reg_print(PORT_FBS);
	port_reg_print(PORT_DEVSLP);
}

static void ahci_port_set_irq(u32 irq_mask)
{
	iowrite32(irq_mask, get_base() + PORT_IRQ_MASK);
	VPK("set port irq max to %x\n", irq_mask);
}

static void ahci_port_clear_irq_err(void)
{
	u32 tmp;
	void __iomem * port_mmio = get_base();

	/* clear SError */
	tmp = readl(port_mmio + PORT_SCR_ERR);
	VPK("PORT_SCR_ERR 0x%x\n", tmp);
	writel(tmp, port_mmio + PORT_SCR_ERR);

	/* clear port IRQ */
	tmp = readl(port_mmio + PORT_IRQ_STAT);
	VPK("PORT_IRQ_STAT 0x%x\n", tmp);
	if (tmp) {
		writel(tmp, port_mmio + PORT_IRQ_STAT);
	}
}

static void ahci_tf_to_fis(const struct ata_taskfile *tf, u8 pmp, int is_cmd, u8 *fis)
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

	VPK("fill cmd slot tag %d, opts 0x%x\n", tag, opts );
}

static void ahci_cmd_prep_nodata(const struct ata_taskfile *tf, unsigned int tag, u32 cmd_opts)
{
	void * cmd_tbl;
	unsigned int n_elem;
	const u32 cmd_fis_len = 5; /* five dwords */
	u32 opts;
	struct lax_port *port = &(lax.ports[lax.port_index]);

	VPK("nodata cmd prepare\n");
	cmd_tbl = port->cmd_tbl + tag * AHCI_CMD_TBL_SZ;

	ahci_tf_to_fis(tf, 0, 1, cmd_tbl);

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

static bool ahci_busy_wait_not(u32 wait_msec, void __iomem *reg, u32 mask, u32 val)
{
	u32 i = 0;
	u32 reg_val;
	bool condition = false;

	do {
		reg_val = ioread32(reg);

		if((reg_val & mask) == val) {
			msleep(1);
		}
		else {
			condition = true;
			break;
		}
		i ++;
	} while (i < wait_msec);

	if(!condition){
		printk(KERN_ERR "wait failed on %d msec wait, reg_val %x mask %x, val %x i %d wait %d\n",
			wait_msec, reg_val, mask, val, i, wait_msec );
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

	tf->flags = ATA_TFLAG_ISADDR | ATA_TFLAG_LBA48 | ATA_TFLAG_DEVICE;
}

static bool ahci_port_disable_fis_recv(void)
{
	u32   tmp;
	void __iomem *port_mmio = get_base();

	VPK("disable the fis\n");

	/* stop the fis running */
	tmp = ioread32(port_mmio + PORT_CMD);
	tmp &= ~PORT_CMD_FIS_RX;
	iowrite32(tmp, port_mmio + PORT_CMD);


	tmp = ioread32(port_mmio + PORT_CMD);
	if(tmp & PORT_CMD_FIS_RX) {
		if(ahci_busy_wait_not(500, port_mmio + PORT_CMD, PORT_CMD_FIS_ON, PORT_CMD_FIS_ON) == false) {
			VPK("timeout wait fis stop\n");
			p_regs();
		 	return false;
		}
	}

	return true;
}

static void ahci_port_enable_fis_recv(void)
{
	u32  cap, tmp;
	void __iomem *port_mmio = get_base();
	struct lax_port *port = &lax.ports[lax.port_index];

	cap = ioread32(lax.iohba_base + HOST_CAP);

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

	VPK("issue cmd, issue reg 0x%x, tag %d, command %x feature %x\n", bit_pos, tag, command, feature);
	iowrite32(bit_pos, reg_issue);

	if(ahci_busy_wait_not(100, reg_issue, bit_pos, bit_pos) == false) {
		printk(KERN_ERR "timeout exec command 0x%x\n", command);
	}

	p_regs();
}

static bool ahci_port_engine_stop(void)
{
	u32 tmp;
	void __iomem *port_mmio = get_base();


	VPK("stopping engine\n");

	tmp = ioread32(port_mmio + PORT_CMD);
	/* check if the HBA is idle */
	if ((tmp & (PORT_CMD_START | PORT_CMD_LIST_ON | PORT_CMD_FIS_RX | PORT_CMD_FIS_ON)) == 0)
		return true;

	/* setting HBA to idle */
	tmp &= ~PORT_CMD_START;
	iowrite32(tmp, port_mmio + PORT_CMD);

	/* wait for engine to stop. This could be as long as 500 msec */
	if(ahci_busy_wait_not(500, port_mmio + PORT_CMD, PORT_CMD_LIST_ON, PORT_CMD_LIST_ON) == false) {
		VPK("timeout wait engine stop\n");
		p_regs();
	 	return false;
	}

	return true;
}

static void ahci_port_engine_start(void)
{
	u32 tmp;
	void __iomem *port_mmio = get_base();

	VPK("engine start\n");
	/* start engine */
	tmp = ioread32(port_mmio + PORT_CMD);
	tmp |= PORT_CMD_START;
	iowrite32(tmp, port_mmio + PORT_CMD);
	ioread32(port_mmio + PORT_CMD); /* flush */
}

static void ahci_port_spin_up(void)
{
	u32 cmd, cap;
	void __iomem *port_mmio = get_base();

	cap = ioread32(lax.iohba_base + HOST_CAP);

	/* spin up if supports staggered spin up, this is normally not supported
	 * on the pc
	 */
	cmd = ioread32(port_mmio + PORT_CMD) & ~ PORT_CMD_ICC_MASK;
	if(cap & HOST_CAP_SSS) {
		cmd |= PORT_CMD_SPIN_UP;
		iowrite32(cmd, port_mmio + PORT_CMD);
	}

	/* set interface control to active state */
	iowrite32(cmd | PORT_CMD_ICC_ACTIVE, port_mmio + PORT_CMD);
}

static bool ahci_port_verify_dev_idle(void)
{
	u32 tmp;
	void __iomem *port_mmio = get_base();


	/* Check the task file */
	tmp = ioread32(port_mmio + PORT_TFDATA);
	if(tmp & (ATA_BUSY | ATA_DRQ)) {
		return false;
	}

	/* Check Device Detection DET */
	tmp = ioread32(port_mmio + PORT_SCR_STAT);
	if((tmp & 0xF) != 0x3 ) {
		return false;

	}

	return true;
}


static int ahci_port_mem_alloc(void)
{
	dma_addr_t mem_dma;
	void * mem;
	size_t dma_sz;
	struct lax_port *port = &lax.ports[lax.port_index];

	VPK("mem alloc for port 0x%x, port base addr %p\n", lax.port_index, port->ioport_base);

	/* no FBS(fis based switching) support*/
	dma_sz = AHCI_PORT_PRIV_DMA_SZ;

	mem = dma_alloc_coherent(&(lax.pdev->dev), dma_sz, &mem_dma, GFP_KERNEL);
	if(!mem) {
		VPK("memory allocation failed\n");
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

static void ahci_port_mem_free(void)
{
	struct lax_port *port = &lax.ports[lax.port_index];

	if(port->cmd_slot){
		dma_free_coherent(&(lax.pdev->dev), AHCI_PORT_PRIV_DMA_SZ,
				port->cmd_slot, port->cmd_slot_dma);
	}
}

static void ahci_port_reset_hard(void)
{
	u32 tmp;
	void __iomem *port_mmio = get_base();
	int tries = ATA_LINK_RESUME_TRIES;

	/* setting HBA to idle */
	tmp = ioread32(port_mmio + PORT_CMD);
	tmp &= ~PORT_CMD_START;
	iowrite32(tmp, port_mmio + PORT_CMD);

	/* wait for engine to stop. This could be as long as 500 msec */
	if(ahci_busy_wait_not(500, port_mmio + PORT_CMD, PORT_CMD_LIST_ON, PORT_CMD_LIST_ON) == false) {
		VPK("timeout wait engine stop\n");
	}

	/* DET, device detection init, sending the COMRESET */
	tmp = ioread32(port_mmio + PORT_SCR_CTL);
	tmp = (tmp & 0x0f0) | 0x301;
	iowrite32(tmp, port_mmio + PORT_SCR_CTL);

	/* sleep 1 millisecond, 1 for buffer */
	msleep(1 + 1);

	tmp = ioread32(port_mmio + PORT_SCR_CTL);
	do {
		tmp = (tmp & 0x0f0) | 0x300;
		iowrite32(tmp, port_mmio + PORT_SCR_CTL);

		msleep(200);

		tmp = ioread32(port_mmio + PORT_SCR_CTL);
	} while((tmp & 0xf0f) != 0x300 && --tries);

	if((tmp & 0xf0f) != 0x300) {
		VPK("SCR_CTL is not returning to 0");
	}

	tries = 1000;
	do {
		tmp = ioread32(port_mmio + PORT_SCR_STAT);
		tmp = tmp & 0x01;
		msleep(1);
	} while(tmp != 1 && --tries);

	if(tries <= 0) {
		VPK("reset does not looks good\n");
	}

	/* clears the sata error */
	tmp = ioread32(port_mmio + PORT_SCR_ERR);
	iowrite32(tmp, port_mmio + PORT_SCR_ERR);

	ahci_port_clear_irq_err();

	if(ahci_port_verify_dev_idle() == false) {
		VPK("reset idle verify failed\n");
	}
}


static void ahci_port_init(void)
{
	u32 tmp;
	void __iomem *port_mmio = get_base();

	lax.initialized = true;

	/* following the ahci spec 10.1.2  */
	VPK("ahci port init\n");
	if(ahci_port_engine_stop() != true) {
		printk(KERN_INFO "engine stop failed, try hard reset? \n");
		return;
	}
	if(ahci_port_disable_fis_recv() != true ) {
		printk(KERN_INFO "fis recv failed, try hard reset?\n");
		return;
	}

	/* Verify everything is clean */
	tmp = ioread32(port_mmio + PORT_CMD);
	if ((tmp & (PORT_CMD_START | PORT_CMD_LIST_ON | PORT_CMD_FIS_RX | PORT_CMD_FIS_ON)) != 0) {
		VPK("failed state\n");
		return;
	}


	/* now start real init process*/
	ahci_port_mem_alloc();
	ahci_port_spin_up(); /* may be this is not needed */
	ahci_port_enable_fis_recv();

	/* disable port interrupt */
	ahci_port_set_irq(0x0);
	ahci_port_clear_irq_err();
	if(ahci_port_verify_dev_idle() == false) {
		VPK("device not preset, shall we try hard port reset?");
		return;
	}

	ahci_port_engine_start();
}

static void ahci_port_deinit(void)
{
	//struct lax_port *port = &lax.ports[lax.port_index];
	ahci_port_mem_free();
}


void ata_ahci_set_para(int port_i)
{
	lax.port_index = port_i;
}

long ata_ahci_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	if (cmd == LAX_CMD_VU_2B) {
		/* only support VU command */
		ahci_port_init();
		ahci_exec_nodata(0, 0xFF, 0x2B);
	}
	else if(cmd == LAX_CMD_PORT_INIT) {
		ahci_port_init();
	}
	else if(cmd == LAX_CMD_PRINT_REGS) {
		p_regs();
	}
	else if(cmd == LAX_CMD_PORT_RESET) {
		ahci_port_reset_hard();
	}
	else {
		printk(KERN_WARNING "not supported command 0x%x", cmd);
		retval = -EPERM;
	}

	return 0;
}

int ahci_open(void)
{
	struct pci_dev *pdev = NULL;
	bool found = false;
	void __iomem * const * iomap;
	struct lax_port *port = &lax.ports[lax.port_index];


	for_each_pci_dev(pdev) {
		u16 class = pdev->class >> 8;

		if (class == PCI_CLASS_STORAGE_RAID || class == PCI_CLASS_STORAGE_SATA) {
			found = true;
			VPK("found AHCI PCI controller, class type 0x%x\n", class);
		 	break;
		}
	}

	if(found == false) {
		VPK("AHCI device NOT found \n");
		return -EBADSLT;
	}

	lax.pdev = pdev;

	iomap = pcim_iomap_table(lax.pdev);
	lax.iohba_base = iomap[AHCI_PCI_BAR_STANDARD];
	lax.initialized = false;

	port->ioport_base = lax.iohba_base + 0x100 + (lax.port_index * 0x80);
	port->cmd_slot = NULL;


	VPK("iohba base address %p", lax.iohba_base);

	return 0;
}

static void ahci_close(void)
{
	if(lax.initialized) {
		ahci_port_deinit();
	}
}

int ata_ahci_file_open(struct inode *inode, struct file *filp)
{
	ahci_open();
	return 0;
}


int ata_ahci_file_release(struct inode *inode, struct file *filp)
{
	ahci_close();
	return 0;
}

