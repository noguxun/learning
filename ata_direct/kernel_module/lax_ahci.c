#include <linux/kernel.h>
#include <linux/ata.h>
#include <linux/libata.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include <asm-generic/dma-coherent.h>

#include "lax_common.h"
#include "lax_ahci.h"




/*
 * only the port is configurable
 * the first the controller will be used
 *
 * TODO: currently, lba is a 32 bit variable, needs to make it 48 bit
 */

static struct lax_ahci lax = { NULL, NULL };

static struct lax_port *get_port(void)
{
	return &(lax.ports[lax.port_index]);
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

static u32 ahci_port_set_irq(u32 irq_mask)
{
	u32 org_irq_mask;

	org_irq_mask = ioread32(P(PORT_IRQ_MASK));
	iowrite32(irq_mask, P(PORT_IRQ_MASK));
	VPK("set port irq max to %x\n", irq_mask);

	return org_irq_mask;
}

static void ahci_port_clear_sata_err(void)
{
	u32 tmp;

	/* clear SError */
	tmp = ioread32(P(PORT_SCR_ERR));
	VPK("PORT_SCR_ERR 0x%x\n", tmp);
	iowrite32(tmp, P(PORT_SCR_ERR));
}

static void ahci_port_clear_irq(void)
{
	u32 tmp;

	/* clear port IRQ */
	tmp = ioread32(P(PORT_IRQ_STAT));
	VPK("PORT_IRQ_STAT 0x%x\n", tmp);
	if (tmp) {
		iowrite32(tmp, P(PORT_IRQ_STAT));
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
	struct lax_port *port = get_port();

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
	//TODO: remove this
	//if (tf->flags & ATA_TFLAG_WRITE) {
	//	opts |= AHCI_CMD_WRITE;
	//}

	ahci_fill_cmd_slot(port, tag, opts);
}

static unsigned int ahci_fill_sg(unsigned int tag, unsigned long size)
{
	unsigned int n_elem = 0;
	struct lax_port *port = get_port();
	void * cmd_tbl;
	u32 *sg_base;

	cmd_tbl = port->cmd_tbl + tag * AHCI_CMD_TBL_SZ;

	sg_base = cmd_tbl + AHCI_CMD_TBL_HDR_SZ;
	while(true) {
		unsigned long len;

		len = LAX_SG_ENTRY_SIZE;
		if(size < len) {
			len = size;
		}

		*(sg_base) = cpu_to_le32(port->sg[n_elem].mem_dma & 0xffffffff);
		*(sg_base + 1) = cpu_to_le32(port->sg[n_elem].mem_dma >> 16) >> 16;
		*(sg_base + 2) = 0;
		*(sg_base + 3) = cpu_to_le32(len -1);

		VPK("sg set sg%d, len 0x%x, size 0x%lx \n", n_elem, *(sg_base + 3), size);

		size -= len;
		n_elem ++;
		if(size == 0 ) {
			break;
		}
		sg_base += 4;
	}

	return n_elem;
}

static void ahci_cmd_prep_pio_datain(const struct ata_taskfile *tf, unsigned int tag,
			u32 cmd_opts, unsigned long size)
{
	void * cmd_tbl;
	unsigned int n_elem;
	const u32 cmd_fis_len = 5; /* five dwords */
	u32 opts;
	struct lax_port *port = get_port();

	cmd_tbl = port->cmd_tbl + tag * AHCI_CMD_TBL_SZ;

	ahci_tf_to_fis(tf, 0, 1, cmd_tbl);

	VPK("prepare data of size 0x%lx\n", size);

	/*
	 * scatter list, n_elem = ahci_fill_sg() 4.2.3, offset 0x80
	 */
	n_elem = ahci_fill_sg(tag , size);

	/*
	 * Fill in command slot information.
	 */
	opts = cmd_fis_len | n_elem << 16 | (0 << 12) | cmd_opts;
	//TODO: remove this
	//if (tf->flags & ATA_TFLAG_WRITE) {
	//	opts |= AHCI_CMD_WRITE;
	//}

	ahci_fill_cmd_slot(port, tag, opts);
}

static bool ahci_busy_wait_irq(u32 wait_msec)
{
	u32 i = wait_msec;
	u32 tmp;

	do {
		tmp = ioread32(P(PORT_IRQ_STAT));
		if(tmp) {
			return true;
		}
		msleep(1);
	}while(i);

	return false;
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
		PK( "wait failed on %d msec wait, reg_val %x mask %x, val %x i %d wait %d\n",
			wait_msec, reg_val, mask, val, i, wait_msec );
	  	return false;
	}
	else {
		return true;
	}
}


static void ahci_fill_taskfile_ncq(u8 cmd, u16 feature, u32 lba, u16 block, struct ata_taskfile *tf)
{
	unsigned int tag = 0;
	union u32_b u32_data;
	union u16_b u16_data;

	tf->command = cmd;

	u32_data.data = lba;
	tf->hob_lbah = 0;
	tf->hob_lbam = 0;
	tf->hob_lbal = u32_data.b[3];
	tf->lbah = u32_data.b[2];
	tf->lbam = u32_data.b[1];
	tf->lbal = u32_data.b[0];

	tf->nsect = tag << 3;

	u16_data.data = block;
	tf->hob_feature = u16_data.b[1];
	tf->feature = u16_data.b[0];

	tf->device = ATA_LBA;
}

static void ahci_fill_taskfile(u8 cmd, u16 feature, u32 lba, u16 block, struct ata_taskfile *tf)
{
	union u32_b u32_data;
	union u16_b u16_data;

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
}

static bool ahci_port_disable_fis_recv(void)
{
	u32   tmp;

	VPK("disable the fis\n");

	/* stop the fis running */
	tmp = ioread32(P(PORT_CMD));
	tmp &= ~PORT_CMD_FIS_RX;
	iowrite32(tmp, P(PORT_CMD));

	tmp = ioread32(P(PORT_CMD));
	if(tmp & PORT_CMD_FIS_RX) {
		bool stat;
		stat = ahci_busy_wait_not(500, P(PORT_CMD), PORT_CMD_FIS_ON, PORT_CMD_FIS_ON);
		if(stat == false) {
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
	struct lax_port *port = get_port();

	cap = ioread32(lax.iohba_base + HOST_CAP);

	/* enable fis reception */
	/* set FIS registers */
	if (cap & HOST_CAP_64) {
		iowrite32((port->cmd_slot_dma >> 16) >> 16, P(PORT_LST_ADDR_HI));
	}
	iowrite32(port->cmd_slot_dma & 0xffffffff, P(PORT_LST_ADDR));

	if (cap & HOST_CAP_64) {
		iowrite32((port->rx_fis_dma >> 16) >> 16, P(PORT_FIS_ADDR_HI));
	}
	iowrite32(port->rx_fis_dma & 0xffffffff, P(PORT_FIS_ADDR));

	/* enable FIS reception */
	tmp = ioread32(P(PORT_CMD));
	tmp |= PORT_CMD_FIS_RX;
	iowrite32(tmp, P(PORT_CMD));
	/* flush */
	ioread32(P(PORT_CMD));
}



static void ahci_tf_init(struct ata_taskfile *tf)
{
	memset(tf, 0, sizeof(*tf));

	tf->device = ATA_DEVICE_OBS;
	tf->ctl = ATA_DEVCTL_OBS;     // obsolete bit in device control
}

static void ahci_exec_pio_datain(u8 command, u16 feature, unsigned long block)
{
	unsigned int tag = 0;
	struct ata_taskfile tf;
	u32 bit_pos = 1 << tag;

	VPK("exec cmd, issue reg 0x%x, tag 0x%x, command 0x%x feature 0x%x, block 0x%lx\n",
		bit_pos, tag, command, feature, block);

	ahci_tf_init(&tf);
	ahci_fill_taskfile(command, feature, 0, block, &tf);

	ahci_cmd_prep_pio_datain(&tf, tag, 0, block * LAX_SECTOR_SIZE);

	/* issue command */
	iowrite32(bit_pos, P(PORT_CMD_ISSUE));

	/* wait command complete */
	if(ahci_busy_wait_irq(1000) == false) {
		printk(KERN_ERR "timeout exec command 0x%x\n", command);
		return;
	}

	ahci_port_clear_irq();
}

static u32 ahci_get_ata_command(unsigned long flags)
{
	u32 cmd = 0;

	if((flags & LAX_RW_FLAG_NCQ) == 0) {
		if((flags & LAX_RW_FLAG_DIRECTION) == 0) {
			if((flags & LAX_RW_FLAG_XFER_MODE) == 0) {
				cmd = ATA_CMD_PIO_READ_EXT;
			}
			else{
				cmd = ATA_CMD_READ_EXT;
			}
			goto END;
		}
		else {
			if((flags & LAX_RW_FLAG_XFER_MODE) == 0) {
				//cmd = ATA_CMD_PIO_WRITE_EXT; not impl yet
			}
			else{
				//cmd = ATA_CMD_WRITE_EXT; not impl yet
			}

			goto END;
		}
	}
	else {
		if((flags & LAX_RW_FLAG_DIRECTION) == 0) {
			cmd = ATA_CMD_FPDMA_READ;
		}
		else {
			// cmd = ATA_CMD_FPDMA_WRITE; not impl yet
		}
		goto END;
	}

END:
	return cmd;

}

static void ahci_exec_rw(unsigned long uarg)
{
	unsigned int tag = 0;
	struct lax_rw arg;
	struct ata_taskfile tf;
	u32 bit_pos = 1 << tag;
	u32 cmdopts = 0;
	u8 command;

	copy_from_user(&arg, (void *)uarg, sizeof(struct lax_rw));

	/* prepare data */
	ahci_tf_init(&tf);

	command = ahci_get_ata_command(arg.flags);
	if(command == 0 ) {
		PK("not supportted command \n");
		goto END;
	}

	VPK("exec rw command 0x%x", command);

	if(arg.block == 0) {
		arg.block = 0x10000;
	}

	if((arg.flags & LAX_RW_FLAG_NCQ) == 0) {
		ahci_fill_taskfile(command, 0x00, arg.lba, arg.block, &tf);
	}
	else{
		ahci_fill_taskfile_ncq(command, 0x00, arg.lba, arg.block, &tf);
	}
	ahci_cmd_prep_pio_datain(&tf, tag, cmdopts, arg.block * LAX_SECTOR_SIZE);

	/* issue command */
	if((arg.flags & LAX_RW_FLAG_NCQ) != 0) {
		iowrite32(bit_pos, P(PORT_SCR_ACT));
	}
	iowrite32(bit_pos, P(PORT_CMD_ISSUE));

	/* wait completion*/
	if(ahci_busy_wait_not(1000, P(PORT_CMD_ISSUE), bit_pos, bit_pos) == false) {
		printk(KERN_ERR "timeout exec command 0x%x\n", command);
	}

	if((arg.flags & LAX_RW_FLAG_NCQ) != 0 &&
		ahci_busy_wait_not(1000, P(PORT_SCR_ACT), bit_pos, bit_pos) == false) {
		printk(KERN_ERR "timeout exec command 0x%x\n", command);

	}
	ahci_port_clear_irq();

END:
	return;
}

static void ahci_exec_nodata(unsigned int tag, u8 command, u16 feature)
{
	struct ata_taskfile tf;
	u32 bit_pos = 1 << tag;

	VPK("exec cmd, issue reg 0x%x, tag %d, command %x feature %x\n",
		bit_pos, tag, command, feature);

	ahci_tf_init(&tf);
	ahci_fill_taskfile(command, feature, 0, 0, &tf);

	ahci_cmd_prep_nodata(&tf, tag, AHCI_CMD_CLR_BUSY);

	iowrite32(bit_pos, P(PORT_CMD_ISSUE));

	if(ahci_busy_wait_not(100, P(PORT_CMD_ISSUE), bit_pos, bit_pos) == false) {
		printk(KERN_ERR "timeout exec command 0x%x\n", command);
	}

	p_regs();
}

static bool ahci_port_engine_stop(void)
{
	u32 tmp;

	VPK("stopping engine\n");

	tmp = ioread32(P(PORT_CMD));
	/* check if the HBA is idle */
	if ((tmp & (PORT_CMD_START | PORT_CMD_LIST_ON | PORT_CMD_FIS_RX | PORT_CMD_FIS_ON)) == 0)
		return true;

	/* setting HBA to idle */
	tmp &= ~PORT_CMD_START;
	iowrite32(tmp, P(PORT_CMD));

	/* wait for engine to stop. This could be as long as 500 msec */
	if(ahci_busy_wait_not(500, P(PORT_CMD), PORT_CMD_LIST_ON, PORT_CMD_LIST_ON) == false) {
		VPK("timeout wait engine stop\n");
		p_regs();
	 	return false;
	}

	return true;
}

static void ahci_port_engine_start(void)
{
	u32 tmp;

	VPK("engine start\n");
	/* start engine */
	tmp = ioread32(P(PORT_CMD));
	tmp |= PORT_CMD_START;
	iowrite32(tmp, P(PORT_CMD));
	ioread32(P(PORT_CMD)); /* flush */
}

static void ahci_port_spin_up(void)
{
	u32 cmd, cap;

	cap = ioread32(lax.iohba_base + HOST_CAP);

	/* spin up if supports staggered spin up, this is normally not supported
	 * on the pc
	 */
	cmd = ioread32(P(PORT_CMD)) & ~ PORT_CMD_ICC_MASK;
	if(cap & HOST_CAP_SSS) {
		cmd |= PORT_CMD_SPIN_UP;
		iowrite32(cmd, P(PORT_CMD));
	}

	/* set interface control to active state */
	iowrite32(cmd | PORT_CMD_ICC_ACTIVE, P(PORT_CMD));
}

static bool ahci_port_verify_dev_idle(void)
{
	u32 tmp;

	/* Check the task file */
	tmp = ioread32(P(PORT_TFDATA));
	if(tmp & (ATA_BUSY | ATA_DRQ)) {
		return false;
	}

	/* Check Device Detection DET */
	tmp = ioread32(P(PORT_SCR_STAT));
	if((tmp & 0xF) != 0x3 ) {
		return false;

	}

	return true;
}

static int ahci_port_sg_alloc(void)
{
	dma_addr_t mem_dma;
	void *mem;
	struct lax_port *port = get_port();
	int i;

	for(i = 0; i < LAX_SG_COUNT; i++) {
		/* __GFP_COMP is needed for parts to be mapped */
		mem = dma_zalloc_coherent(&(lax.pdev->dev), LAX_SG_ENTRY_SIZE,
						&mem_dma, GFP_KERNEL | __GFP_COMP );
		if(!mem) {
			PK("memory allocation failed\n");
			return -ENOMEM;
		}

		port->sg[i].mem = mem;
		port->sg[i].mem_dma = mem_dma;
		port->sg[i].length = LAX_SG_ENTRY_SIZE;
		VPK("allocated dma memory for sg\n");
	}

	return 0;
}

static void ahci_port_sg_free(void)
{
	int i;
	struct lax_port *port = get_port();

	VPK("free port sg mem \n");
	for(i = 0; i < LAX_SG_COUNT; i++) {
		dma_free_coherent(&(lax.pdev->dev), LAX_SG_ENTRY_SIZE,
				port->sg[i].mem, port->sg[i].mem_dma);
	}
}

static int ahci_port_mem_alloc(void)
{
	dma_addr_t mem_dma;
	void * mem;
	size_t dma_sz;
	struct lax_port *port = get_port();

	VPK("mem alloc for port 0x%x, port base addr %p\n", lax.port_index, port->ioport_base);

	/* no FBS(fis based switching) support*/
	dma_sz = AHCI_PORT_PRIV_DMA_SZ;

	mem = dma_alloc_coherent(&(lax.pdev->dev), dma_sz, &mem_dma, GFP_KERNEL);
	if(!mem) {
		PK("memory allocation failed\n");
		return -ENOMEM;
	}
	VPK("allocated dma memory 0x%p for command slot\n", mem);
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

	ahci_port_sg_alloc();

	return 0;
}

static void ahci_port_mem_free(void)
{
	struct lax_port *port = get_port();

	if(port->cmd_slot){
		ahci_port_sg_free();

		VPK("free port mem \n");
		dma_free_coherent(&(lax.pdev->dev), AHCI_PORT_PRIV_DMA_SZ,
				port->cmd_slot, port->cmd_slot_dma);
	}
}

static void ahci_port_restore_irq_mask(void)
{
	struct lax_port *port = get_port();

	ahci_port_set_irq(port->irq_mask);
}

static void ahci_port_reset_hard(void)
{
	u32 tmp;
	int tries = ATA_LINK_RESUME_TRIES;

	/* setting HBA to idle */
	tmp = ioread32(P(PORT_CMD));
	tmp &= ~PORT_CMD_START;
	iowrite32(tmp, P(PORT_CMD));

	/* wait for engine to stop. This could be as long as 500 msec */
	if(ahci_busy_wait_not(500, P(PORT_CMD), PORT_CMD_LIST_ON,
				PORT_CMD_LIST_ON) == false) {
		VPK("timeout wait engine stop\n");
	}

	/* DET, device detection init, sending the COMRESET */
	tmp = ioread32(P(PORT_SCR_CTL));
	tmp = (tmp & 0x0f0) | 0x301;
	iowrite32(tmp, P(PORT_SCR_CTL));

	/* sleep 1 millisecond, 1 for buffer */
	msleep(1 + 1);

	tmp = ioread32(P(PORT_SCR_CTL));
	do {
		tmp = (tmp & 0x0f0) | 0x300;
		iowrite32(tmp, P(PORT_SCR_CTL));

		msleep(200);

		tmp = ioread32(P(PORT_SCR_CTL));
	} while((tmp & 0xf0f) != 0x300 && --tries);

	if((tmp & 0xf0f) != 0x300) {
		VPK("SCR_CTL is not returning to 0");
	}

	tries = 1000;
	do {
		tmp = ioread32(P(PORT_SCR_STAT));
		tmp = tmp & 0x01;
		msleep(1);
	} while(tmp != 1 && --tries);

	if(tries <= 0) {
		VPK("reset does not looks good\n");
	}

	/* clears the sata error */
	tmp = ioread32(P(PORT_SCR_ERR));
	iowrite32(tmp, P(PORT_SCR_ERR));

	ahci_port_clear_irq();
	ahci_port_clear_sata_err();

	if(ahci_port_verify_dev_idle() == false) {
		VPK("reset idle verify failed\n");
	}
}


static void ahci_port_init(void)
{
	u32 tmp;
	struct lax_port *port = get_port();

	if(lax.port_initialized == true) {
		return;
	}

	lax.port_initialized = true;

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
	tmp = ioread32(P(PORT_CMD));
	if ((tmp & (PORT_CMD_START | PORT_CMD_LIST_ON | PORT_CMD_FIS_RX | PORT_CMD_FIS_ON)) != 0) {
		VPK("failed state\n");
		return;
	}

	/* now start real init process*/
	ahci_port_mem_alloc();
	ahci_port_spin_up(); /* may be this is not needed */
	ahci_port_enable_fis_recv();

	/* disable port interrupt */
	port->irq_mask = ahci_port_set_irq(0x0);

	ahci_port_clear_irq();

	if(ahci_port_verify_dev_idle() == false) {
		VPK("device not preset, shall we try hard port reset?");
		return;
	}

	ahci_port_engine_start();
}

static void ahci_port_deinit(void)
{
	if(lax.port_initialized == true) {
		lax.port_initialized = false;
		ahci_port_mem_free();
	}
}


long ahci_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	printk(KERN_INFO "ioctl cmd 0x%x arg 0x%lx \n", cmd, arg);

	if (cmd == LAX_CMD_TST_VU_2B) {
		/* only support VU command */
		ahci_exec_nodata(0, 0xFF, 0x2B);
	}
	else if(cmd == LAX_CMD_TST_PORT_INIT) {
		ahci_port_init();
	}
	else if(cmd == LAX_CMD_TST_PRINT_REGS) {
		p_regs();
	}
	else if(cmd == LAX_CMD_TST_PORT_RESET) {
		ahci_port_reset_hard();
	}
	else if(cmd == LAX_CMD_TST_ID) {
		ahci_exec_pio_datain(ATA_CMD_ID_ATA, 0xFF00, 1);
	}
	else if(cmd == LAX_CMD_TST_RESTORE_IRQ) {
		ahci_port_restore_irq_mask();
	}
	else if(cmd == LAX_CMD_TST_RW) {
		ahci_exec_rw(arg);
	}
	else {
		printk(KERN_WARNING "not supported command 0x%x ", cmd);
		retval = -EPERM;
	}

	return 0;
}


int ahci_file_open(struct inode *inode, struct file *filp)
{
	ahci_port_init();
	return 0;
}


int ahci_file_release(struct inode *inode, struct file *filp)
{
	ahci_port_deinit();
	return 0;
}

void ahci_module_init(int port_i)
{
	struct pci_dev *pdev = NULL;
	bool found = false;
	void __iomem * const * iomap;
	struct lax_port *port;

	lax.port_index = port_i;
	port = get_port();

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
		return;
	}

	lax.pdev = pdev;

	iomap = pcim_iomap_table(lax.pdev);
	lax.iohba_base = iomap[AHCI_PCI_BAR_STANDARD];
	lax.port_initialized = false;

	port->ioport_base = lax.iohba_base + 0x100 + (lax.port_index * 0x80);
	port->cmd_slot = NULL;

	VPK("iohba base address %p", lax.iohba_base);
}

void ahci_module_deinit(void)
{

}

static void ahci_vma_open(struct vm_area_struct *vma)
{

}

static void ahci_vma_close(struct vm_area_struct *vma)
{

}


static int ahci_vma_nopage(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	unsigned long offset_in_file;
	int retval = VM_FAULT_NOPAGE;
	int sg_index;
	struct lax_port *port = get_port();
	int sg_entry_offset;
	unsigned char *p;
	struct page *page;

	offset_in_file = (unsigned long)(vmf->virtual_address - vma->vm_start) +
				(vma->vm_pgoff << PAGE_SHIFT);
	if(offset_in_file >= LAX_SG_ALL_SIZE) {
		goto out;
	}

	sg_index = offset_in_file / LAX_SG_ENTRY_SIZE;
	sg_entry_offset = offset_in_file % LAX_SG_ENTRY_SIZE;

	p = (((unsigned char *)port->sg[sg_index].mem) + sg_entry_offset);
	if(!p) {
		goto out;
	}

	page = virt_to_page(p);

	VPK("nopage sg%d: offset %d page %p mapcount %d\n", sg_index, sg_entry_offset, page, page->_mapcount.counter);

	get_page(page);
	vmf->page = page;
	retval = 0;

out:
	return retval;
}


struct vm_operations_struct ahci_vm_ops = {
	.open  = ahci_vma_open,
	.close = ahci_vma_close,
	.fault = ahci_vma_nopage,
};



int ahci_file_mmap(struct file *filp, struct vm_area_struct *vma)
{
	vma->vm_ops = &ahci_vm_ops;
	ahci_vma_open(vma);
	return 0;
}

