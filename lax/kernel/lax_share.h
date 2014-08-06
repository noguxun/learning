#ifndef _LAX_DATA_H
#define _LAX_DATA_H

enum {
	LAX_CMD_START = 0x101,
	LAX_CMD_PORT_INIT,
	LAX_CMD_RSVD1,
	LAX_CMD_PRINT_REGS,
	LAX_CMD_PORT_RESET,
	LAX_CMD_ID,
	LAX_CMD_RW,
	LAX_CMD_RESTORE_IRQ,
	LAX_CMD_MICRO_CODE,
	LAX_CMD_ATA_DIRECT,

	LAX_RW_FLAG_NCQ             = (unsigned long)(1 << 2), /* None-NCQ 0, NCQ 1*/
	LAX_RW_FLAG_XFER_MODE       = (unsigned long)(1 << 1), /* PIO 0,  DMA 1 */
	LAX_RW_FLAG_RW              = (unsigned long)(1 << 0), /* READ 0, WRITE 1*/

	LAX_READ                    = 0,
	LAX_WRITE                   = 1,

	LAX_SECTOR_SIZE             = 512,
};

enum{
	LAX_CMD_TYPE_NO_DATA        = 0,
	LAX_CMD_TYPE_PIO_IN,
	LAX_CMD_TYPE_PIO_OUT,
	LAX_CMD_TYPE_DMA_IN,
	LAX_CMD_TYPE_DMA_OUT,
};


struct lax_rw {
	uint64_t lba;
	uint16_t block;
	uint32_t flags;
	uint32_t tfd;
};

struct lax_ata_direct {
	uint16_t feature;
	uint16_t block;
	uint64_t lba;
        uint8_t  ata_cmd;

	uint64_t buf_len;
	uint8_t ata_cmd_type;
};




#endif
