/*
 *  NAND FLASH/LCD Register Offsets.
 */
#define DW_FC_CTL	  	  		0x5400000 		/* FC control register */
#define DW_FC_STATUS  	  		0x5400004 		/* FC status register */
#define DW_FC_STS_CLR  	  		0x5400008 		/* FC status clear register */
#define DW_FC_INT_EN  	  		0x540000C 		/* FC interrupt enable mask register */
#define DW_FC_SEQUENCE  	  	0x5400010 		/* FC sequence register */
#define DW_FC_ADDR_COL  	  	0x5400014 		/* FC address-column register */
#define DW_FC_ADDR_ROW  	  	0x5400018 		/* FC address-row register */
#define DW_FC_CMD  	  		    0x540001C 		/* FC command code configuration register */
#define DW_FC_WAIT 	  		    0x5400020 		/* FC wait time configuration register */
#define DW_FC_PULSETIME  	  	0x5400024 		/* FC pulse time configuration register */
#define DW_FC_DCOUNT  	  		0x5400028 		/* FC data count register */
#define DW_FC_FBYP_CTL  	  	0x5400058 		/* FC GF FIFO bypass control register */
#define DW_FC_FBYP_DATA  	  	0x540005C 		/* FC GF FIFO bypass data register */

#define DW_FC_STATUS_TRANS_DONE (1 << 0)
#define DW_FC_STATUS_TRANS_BUSY (1 << 6)

#define DW_FC_FBYP_CTL_BP_EN    (1 << 1)
#define DW_FC_FBYP_CTL_BP_WRITE (1 << 2)


/*
 *  DMA Register Offsets.
 */
#define DW_DMA_CTL	  	  		0x0000 		/* DMA control register */
#define DW_DMA_ADDR1  	  		0x0004 		/* DMA address 1 */
#define DW_DMA_ADDR2            0x0008 		/* DMA address 2 */
#define DW_DMA_BURST            0x000C 		/* DMA burst length */
#define DW_DMA_BLK_LEN          0x0010 		/* DMA transfer block length (words) */
#define DW_DMA_BLK_CNT          0x0014 		/* DMA block transfer count */
#define DW_DMA_ISR              0x0018 		/* DMA interrupt status */
#define DW_DMA_IER              0x001C 		/* DMA interrupt enable */
#define DW_DMA_ICR              0x0020 		/* DMA interrupt clear */
#define DW_DMA_START            0x0024 		/* DMA start trigger */

#define DW_DMA_DW_EN            (1 << 0)
#define DW_DMA_DMA_MODE         (1 << 1)
#define DW_DMA_FLASH_MODE       (1 << 2)
#define DW_DMA_FLASH_ECC_WORDS  (1 << 4)
#define DW_DMA_SWAP_MODE        (1 << 12)

#define DW_DMA_ISR_DMA_DONE     (1 << 0)
#define DW_DMA_ISR_DMA_ERROR    (1 << 1)

#define DW_DMA_IER_DMA_DONE     (1 << 0)
#define DW_DMA_IER_DMA_ERROR    (1 << 1)

#define DW_DMA_ICR_DMA_DONE     (1 << 0)
#define DW_DMA_ICR_DMA_ERROR    (1 << 1)


/*
 *  FIFO Register Offsets.
 */
#define DW_FIFO_CTL	  	  		0x0000 		/* FIFO control register */
#define DW_FIFO_AE_LEVEL  	  	0x0004 		/* FIFO almose empty level */
#define DW_FIFO_AF_LEVEL  	  	0x0008 		/* FIFO almost full level */
#define DW_FIFO_ISR             0x000C      /* FIFO interrupt status */
#define DW_FIFO_IER             0x0010      /* FIFO interrupot enable */
#define DW_FIFO_ICR             0x0014      /* FIFO interrupt clear*/
#define DW_FIFO_RST             0x0018      /* FIFO reset */
#define DW_FIFO_VAL             0x001C      /* FIFO value */

#define DW_FIFO_CTL_FIFO_EN     (1 << 0)
#define DW_FIFO_CTL_FIFO_WIDTH  (1 << 1)
#define DW_FIFO_CTL_FIFO_MODE   (1 << 2)

#define DW_FIFO_ISR_OVER        (1 << 0)
#define DW_FIFO_ISR_UNDER       (1 << 1)

#define DW_FIFO_IER_OVER        (1 << 0)
#define DW_FIFO_IER_UNDER       (1 << 1)

#define DW_FIFO_ICR_OVER        (1 << 0)
#define DW_FIFO_ICR_UNDER       (1 << 1)

#define NAND16_EN 0X100000
