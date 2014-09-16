#define UART1_BASE		0x5a00000
#define UART2_BASE		0x5b00000
#define UART3_BASE		0x5c00000


#define	UART_CTL 		0x000
#define	UART_CFG 		0x004
#define	UART_INT_DIV 	0x008
#define	UART_FRAC_DIV 	0x00c
#define	TX_FIFO_WM 		0x010
#define	FIFO_INTSTAT 	0x018
#define	FIFO_INTEN 		0x01c
#define	FIFO_INT_CLR 	0x020
#define	TX_FIFO_LVL 	0x024
#define	RX_FIFO_LVL		0x028
#define	UART_TX_DAT 	0x02c
#define	UART_RX_DAT 	0x030
#define	UART_STAT 		0x034