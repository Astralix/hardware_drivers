

#define DW_UART_FIFO_SIZE	(128)

/*
 *  UART Register Offsets.
 */
#define DW_UART_CTL		   			  	  		0x0000 		/* UART control register. 			Default = 0x0 */
#define DW_UART_CFG		   			  	  		0x0004 		/* UART configuration register. 	Default = 0x0 */
#define DW_UART_DIV		   			  	  		0x0008 		/* UART divider register. 			Default = 0x0 */
#define DW_UART_FIFO_AE_LEVEL	   	   	  		0x000C 		/* FIFO Tx wateramrk register. 		Default = (FIFO size)/2 */
#define DW_UART_FIFO_AF_LEVEL	   	   			0x0010 		/* FIFO Rx wateramrk register. 		Default = (FIFO size)/2 */
#define DW_UART_FIFO_ISR	   					0x0014 		/* FIFO interrrupt status register. Default = 0x2 */
#define DW_UART_FIFO_IER	   		   			0x0018 		/* FIFO interrrupt enable register. Default = 0x0 */
#define DW_UART_FIFO_ICR	   		   			0x001C 		/* FIFO interrrupt clear register. 	Default = 0x0 */
#define DW_UART_TX_FIFO_LEVEL	   	  			0x0020 		/* Tx FIFO level register. 			Default = 0x0 */
#define DW_UART_RX_FIFO_LEVEL	   	 			0x0024 		/* Rx FIFO level register. 			Default = 0x0 */
#define DW_UART_TX_DATA			   	 			0x0028 		/* Tx data. 						Default = 0x0 */
#define DW_UART_RX_DATA			   				0x002C 		/* Rx data. 						Default = 0x0 */
#define DW_UART_STAT		   	   				0x0030 		/* UART status register. 			Default = 0x101 */
#define DW_UART_TX_FIFO_TST_ADDRx  				0x0400 		/* Tx FIFO test adddress. 			Default = 0x0 */
#define DW_UART_RX_FIFO_TST_ADDRx  	   			0x0800 		/* Rx FIFO test adddress. 			Default = 0x0 */

#define DW_UART_CTL_UART_EN		   				(1 << 0)	/* Enable the UART block operation. */
#define DW_UART_CTL_UART_DIS	   				(0 << 0)	/* Disable the UART block operation. */
#define DW_UART_RESET		   				    (1 << 7)	/* Reset the UART block operation. */

#define DW_UART_CFG_DATA_LENGTH_MASK 			((1 << 2) | (1 << 3))
#define DW_UART_CFG_DATA_LENGTH_8_VAL			(0 << 2)	/* Data is 8 bits length. */
#define DW_UART_CFG_DATA_LENGTH_7_VAL			(1 << 2)	/* Data is 7 bits length. */
#define DW_UART_CFG_DATA_LENGTH_9_VAL			(1 << 3)	/* Data is 9 bits length. */
#define DW_UART_CFG_STOP_CFG	  				(1 << 4)	/* Number of stop bits. 0=1bit, 1=2bits */
#define DW_UART_CFG_PAR_OD_EVEN  				(1 << 5)	/* Parity. 0=even, 1=odd */
#define DW_UART_CFG_PAR_EN		  				(1 << 6)	/* Parity enabled/disabled. */
#define DW_UART_CFG_SW_RST		  				(1 << 7)
#define DW_UART_CFG_OD_EN		  				(1 << 8)
#define DW_UART_CFG_RTSN_EN	  			   		(1 << 9)	/* RTSN flow control enabled/disabled. */
#define DW_UART_CFG_CTSN_EN 	  		   		(1 << 10)   /* CTSN flow control enabled/disabled. */
#define DW_UART_CFG_BREAK_EN	  				(1 << 11)   /* BREAK detection logic enabled/disabled. */
#define DW_UART_CFG_IRDA_DTX_INV 				(1 << 13)
#define DW_UART_CFG_IRDA_DRX_INV 				(1 << 14)
#define DW_UART_CFG_IRDA_UART	  				(1 << 15)

#define DW_UART_DIV_FRAC_RATE_LOC               (12)
#define DW_UART_DIV_INT_RATE_MASK				(0x1FF)

#define DW_UART_TX_FIFO_OVER_ISR  	   	  		(1 << 0)	/* Tx FIFO over-run interrupt status. */
#define DW_UART_TX_FIFO_WTR_ISR   	   	   		(1 << 1)	/* Tx FIFO watermark interrupt status. */
#define DW_UART_RX_FIFO_UNDER_ISR 	   	   		(1 << 8)    /* Rx FIFO under-run interrupt status. */
#define DW_UART_RX_FIFO_OVER_ISR  	   	   		(1 << 9)    /* Rx FIFO over-run interrupt status. */
#define DW_UART_RX_FIFO_WTR_ISR   	   	  		(1 << 10)   /* Rx FIFO watermark interrupt status. */
#define DW_UART_RX_FIFO_TIMEOUT_ISR	   	   		(1 << 11)   /* Rx FIFO timeout interrupt status. */
#define DW_UART_RX_BREAK_ISR	   	   			(1 << 12)   /* Rx FIFO BREAK interrupt status. */

#define DW_UART_TX_FIFO_OVER_IER		 		(1 << 0)	/* Tx FIFO over-run interrupt enable. */
#define DW_UART_TX_FIFO_WTR_IER					(1 << 1)	/* Tx FIFO watermark interrupt enable. */
#define DW_UART_RX_FIFO_UNDER_IER	 			(1 << 8)	/* Rx FIFO under-run interrupt enable. */
#define DW_UART_RX_FIFO_OVER_IER	   			(1 << 9)	/* Rx FIFO over-run interrupt enable. */
#define DW_UART_RX_FIFO_WTR_IER		   			(1 << 10)	/* Rx FIFO watermark interrupt enable. */
#define DW_UART_RX_FIFO_TIMEOUT_IER	   			(1 << 11)	/* Rx FIFO timeout interrupt enable. */
#define DW_UART_RX_BREAK_IER	   		 		(1 << 12)	/* Rx FIFO BREAK interrupt enable. */
#define DW_UART_STARTUP_IER                     (/*DW_UART_RX_FIFO_UNDER_IER|*/DW_UART_RX_FIFO_OVER_IER \
                                                |DW_UART_RX_FIFO_WTR_IER|DW_UART_RX_FIFO_TIMEOUT_IER \
                                                |DW_UART_RX_BREAK_IER)
#define DW_UART_TX_IER                          (DW_UART_TX_FIFO_OVER_IER|DW_UART_TX_FIFO_WTR_IER)
#define DW_UART_RX_OE_BE_IER					(DW_UART_RX_FIFO_OVER_IER|DW_UART_RX_BREAK_IER)

#define DW_UART_TX_FIFO_OVER_ICR	   			(1 << 0)	/* Tx FIFO over-run interrupt clear.*/
#define DW_UART_RX_FIFO_UNDER_ICR	   			(1 << 8)	/* Rx FIFO under-run interrupt clear.*/
#define DW_UART_RX_FIFO_OVER_ICR	   			(1 << 9)	/* Rx FIFO over-run interrupt clear.*/
#define DW_UART_RX_BREAK_ICR		   			(1 << 12)   /* Rx BREAK interrupt clear.*/
#define DW_UART_CLR_ICR					        (DW_UART_TX_FIFO_OVER_ICR|DW_UART_RX_FIFO_UNDER_ICR|DW_UART_RX_FIFO_OVER_ICR|DW_UART_RX_BREAK_ICR)

#define DW_UART_TX_FIFO_LEVEL_MASK				(0x1F)		/* Tx FIFO current level bits. Spreads on bits [0 : Log2(fifo_depth)]. */

#define DW_UART_RX_FIFO_LEVEL_MASK				(0x1F)		/* Rx FIFO current level bits. Spreads on bits [0 : Log2(fifo_depth)]. */

#define DW_UART_RX_DATA_WORD					(0x1FF)		/* UART word from Rx FIFO - received word. */
#define DW_UART_RX_DATA_PE						(0x200)		/* UART word from Rx FIFO - Parity error bit. */
#define DW_UART_RX_DATA_FE						(0x400)		/* UART word from Rx FIFO - frame error bit. */
#define DW_UART_RX_PE_FE    					(DW_UART_RX_DATA_PE | DW_UART_RX_DATA_FE)

#define DW_UART_STAT_TX_EMPTY 					(1 << 0)	/* Tx FIFO completely empty. */
#define DW_UART_STAT_TX_FULL					(1 << 1)	/* Tx FIFO completely full. */
#define DW_UART_STAT_RTS						(1 << 2)	/* The inverted(!) values of the RTS port. */
#define DW_UART_STAT_CTS						(1 << 3)	/* The inverted(!) values of the CTS port. */
#define DW_UART_STAT_RX_EMPTY					(1 << 8)	/* Rx FIFO completely empty. */
#define DW_UART_STAT_RX_FULL					(1 << 9)	/* Rx FIFO completely full. */
#define DW_UART_STAT_BREAK_DETECTED             (1 << 10)   /* Rx BREAK was detected. */

#define DW_UART_STAT_MASK_LOC_0                 (9)
#define DW_UART_STAT_MASK_LOC_1                 (11)
#define DW_UART_STAT_MASK_LOC_2_3               (7)

