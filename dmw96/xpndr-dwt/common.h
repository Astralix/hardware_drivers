

/* PLATFORM INDEPENDENT TYPE DEFINITIONS */
 
typedef unsigned char UINT8;   
typedef signed char INT8;
typedef unsigned int UINT16;
typedef signed int INT16;
typedef unsigned long UINT32;
typedef signed long INT32;
typedef float FLOAT32;       

typedef enum {
	false = 0,
	true = 1
} BOOL;

typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned long	uint32;
typedef double long 	uint64;

typedef volatile uint8*		Reg8Ptr;
typedef volatile uint16*	Reg16Ptr;
typedef volatile uint32*	Reg32Ptr;
typedef volatile uint64*	Reg64Ptr;

#ifndef DWT_PRINT
#ifdef UART_OUTPUT
#define DWT_PRINT(...) do { char message_buf_int[1024]; sprintf(message_buf_int,__VA_ARGS__); \
    UART_WRITE(message_buf_int); } while(0)
#else
#define DWT_PRINT printf
#endif
#endif

#ifndef DWT_ERROR
#define DWT_ERROR(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#endif

#ifndef VERIFY_ARGS
//#define VERIFY_ARGS(...)
#define VERIFY_ARGS(FUNCTION_NAME,NUM_OF_PARAMS) if(verify_arguments(FUNCTION_NAME,args_num,NUM_OF_PARAMS)) return -1; 
#endif

#ifndef NULL
#define NULL 0
#endif

#define REGISTER8(x)  ( *(Reg8Ptr)(x) )
#define REGISTER16(x) ( *(Reg16Ptr)(x) )
#define REGISTER32(x) ( *(Reg32Ptr)(x) )
#define REGISTER64(x) ( *(Reg64Ptr)(x) )

#define writel(v,p)	(*(unsigned long *)(p) = (v))
#define	readl(a)	(*(volatile unsigned int *)(a))


#define MAX_PS_TICKS  128

#define HCLK_OUT_PS_CLK 12
#define HCLK_OUT_PS_CS 14


#define DW_PS_CLK 6
#define DW_PS_CS 7

#define GCR0	0x5200000

#define IO_LOC 0x5200020

#define CMR	0x5300000
#define CSR0	0x5300004
#define CSR1	0x5300008
#define LPCR	0x5300018
#define DMR		0x5300054
#define CDR0	0x530001c
#define CDR1	0x5300020

#define PLL1CR	0x5300030
#define Pll2CR0	0x5300034
#define PLL2CR1	0x5300038
#define CDR2	0x5300024


#define WDREFR	 0x5800008
#define WDCTRL	 0x5800000

#define FC_EN 0x4000

#define COMR	 0x530002c

#define AGPDATA		 0x05000000
#define AGPDIR	 	 0x05000004
#define AGPEN		 0x05000008
#define AGPPULLEN	 0x0500000C
#define AGPPULLTYPE  0x05000010
#define AGPOD		 0x05000014
#define BGPDATA		 0x05000018
#define BGPDIR		 0x0500001C
#define BGPEN		 0x05000020
#define BGPPULLEN	 0x05000024
#define BGPPULLTYPE  0x05000028
#define BGPOD		 0x0500002C
#define CGPDATA		 0x05000030
#define CGPDIR		 0x05000034
#define CGPEN		 0x05000038
#define CGPPULLEN	 0x0500003C
#define CGPPULLTYPE  0x05000040
#define CGPOD		 0x05000044

#define AGPIN		 0x050000A0
#define BGPIN		 0x050000A4
#define CGPIN		 0x050000A8


#define HIUCFG1		 0x06200000        
#define HIUCFG2		 0x06200004

#define CPU_STAT	 0x06200008
#define CPU_CMD		 0x0620000C
#define HOST_FLG	 0x06200010
#define CPU_FLG		 0x06200014
#define HOST_INT	 0x06200018

#define SPI_RX_BUF	 0x05e00014
#define SPI_TX_BUF	 0x05e00010

#define SDIO_CFG	 0x52000018

#define OCR		0x6300100
#define IO_OCR	0x6300134
#define CID_MSU 0x6300114
#define CID_MSL 0x6300118
#define CID_LSU 0x630011c
#define CID_LSL 0x6300120
#define CSD_MSU 0xbabe1234 
#define CSD_MSL 0x5678babe 
#define CSD_LSU 0x9a9aefef 
#define CSD_LSL 0x0055a1a0 
#define CARD_SIZE 0x00100000 


#define UART1_BASE			0x5b00000
#define UART2_BASE			0x5c00000
#define UART_CONTROL		0x0
#define UART_CONFIG			0x4
#define UART_DIV			0x8
#define UART_FIFO_AE_LEVEL	0xC
#define UART_FIFO_AF_LEVEL	0x10
#define UART_FIFO_ISR		0x14
#define UART_FIFO_IER		0x18
#define UART_FIFO_ICR		0x1C
#define TX_FIFO_LEVEL		0x20
#define RX_FIFO_LEVEL		0x24
#define TX_DATA				0x28
#define RX_DATA				0x2C
#define UART_STATUS			0x30

#define TMR_LOAD	0x0
#define TMR_VALUE	0x4
#define TMR_CONTROL	0x8
#define TMR_RIS		0x10
#define TMR_INTCLR  0xc

#define TIMER0_BASE 0x5900000
#define TIMER1_BASE 0x5900020
#define TIMER2_BASE 0x5a00000
#define TIMER3_BASE 0x5a00020

#define ICUFIQMSK	0x5100000
#define ICUIRQMSK	0x5100004
#define ICUFIQCAUSE	0x5100008
#define ICUIRQCAUSE	0x510000c
#define ICUINTSTAT	0x5100018
#define ICUINTCLR	0x5100014

#define SBUS_BASE	0x5d00000

#define SBUSTX		0x0
#define SBUSRX		0x4
#define	SBUSCFG		0x8
#define SBUSCTL		0xC
#define SBUSSTAT	0x10
#define SBUSFREQ	0x14

#define SPI_BASE	0x5e00000
#define SPI_CFG		0x0
#define SPI_INTEN	0x4
#define SPI_INTSTAT 0x8
#define SPI_INTCLR	0xc
#define SPI_TXBUF	0x10
#define SPI_RXBUF	0x14

#define TDMBASE		0x5F00000

#define TDMEN		0x0
#define TDMCFG1		0x4
#define TDMCFG2		0x8
#define TDMCYCLE	0xC
#define TDMFSYNC	0x10
#define TDMFSYNCDUR	0x14
#define TDMFIFOWM	0x18
#define TDMFIFOBNKCTRL	0x1C
#define TDMINTEN	0x20
#define TDMTAFINTEN	0x24
#define TDMINTCLR	0x28
#define TDMINTSTAT	0x2C
#define TDMTXFLAGS	0x34
#define TDMTXFIFO	0x38
#define TDMRXFIFO	0x3c

#define TDM_TX_SLOTS	0xA00
#define TDM_RX_SLOTS	0x1200
#define TDM_FDA         0x1800

#define RTC_BASE		0x5700000
#define RTC_CFG			0x0
#define RTC_CTRL		0x4
#define	RTC_ALARM_VALUE	0x8
#define	RTC_STATUS		0xc
#define RTC_INT_CLR		0x10
#define RTC_TIME		0x14

#define DSPRST			0x530005c
#define DSPCR			0x5300058

#define MAILBOX_BASE	0x18000000

#define EXTINT_LOC		0x520001C

#define	NFL_OFFSET 0x5404000

#define FC_CTL			0x00
#define FC_STATUS		0x04
#define FC_STS_CLR		0x08
#define FC_INT_EN		0x0C
#define FC_SEQUENCE		0x10
#define FC_ADDR_COL		0x14
#define FC_ADDR_ROW		0x18
#define FC_CMD			0x1C
#define FC_WAIT			0x20
#define FC_PULSETIME	0x24
#define FC_DCOUNT		0x28
#define FC_TIMEOUT		0x2C
#define FC_ECC_STS		0x34
#define FC_ECC_S_12		0x38
#define FC_ECC_S_34		0x3C
#define FC_ECC_S_57		0x40
#define FC_HAMMING		0x44
#define FC_BCH_READ_L	0x48
#define FC_BCH_READ_H	0x4C
#define FC_GF_MUL_OUT	0x50
#define FC_GF_MUL_IN	0x54
#define FC_FBYP_CRL		0x58
#define FC_FBYP_DAT		0x5C


#define DMA_OFFSET 0x5500000

#define DMA_CNTL 0x0000
#define DMA_ADDR1 0x0004
#define DMA_ADDR2 0x0008
#define DMA_BURST 0x000C
#define DMA_BLK_LEN 0x0010
#define DMA_BLK_CNT 0x0014
#define DMA_ISR 0x0018
#define DMA_IER 0x001C
#define DMA_ICR 0x0020
#define DMA_START 0x0024



#define FIFO_OFFSET 0x5600000
#define FIFO_CNTL 0x0000
#define FIFO_AE_LEVEL 0x0004
#define FIFO_AF_LEVEL 0x0008
#define FIFO_ISR 0x000C
#define FIFO_IER 0x0010
#define FIFO_ICR 0x0014
#define FIFO_RST 0x0018
#define FIFO_VAL 0x001C
#define DMA_TST_EN 0x0020

#define SWRST 0x530003c


	//;***********************************************************
	//;***** SELECT CHANNEL (This is HEX, so 28 is channel 40,This is HEX, so F7 is channel 247)
	#define ChanNum	0x28
	//;***********************************************************
	//;***** SELECT MAC ADDRESS (should match probe request)
	#define	MAC_ADDR_B1		0x00000000
	#define	MAC_ADDR_B2		0x0000000E
	#define	MAC_ADDR_B3		0x0000004C
	#define	MAC_ADDR_B4		0x00000000
	#define	MAC_ADDR_B5		0x0000000B
	#define	MAC_ADDR_B6		0x000000AD
	//;***********************************************************
	//;***** SELECT BSSID (any for now...)
	#define	BSS_ID_B1		0x00
	#define	BSS_ID_B2		0x0E
	#define	BSS_ID_B3		0x4C
	#define	BSS_ID_B4		0x00
	#define	BSS_ID_B5		0xBA
	#define	BSS_ID_B6		0xBE
	//;
	//**********************************************************
	//**** These MAC base addr's should not have to change:
	#define	MacAPB_Base		0x06509000
	#define	MSC_Base		0x06502000
	#define	TRPC_Base		0x06504000
	#define	MBQ_Base		0x09700000
	#define	TMR_Base		0x06508000
	#define	PHYBUS_Base		0x06507000
	#define Keystore_Base	0x06506000
	#define Helion_Base		0x06500000
	//;***********************************************************
	//;***** These ARM_SYS base addr's should not have to change:
	#define DescRam_Base	0x14000000
	#define MPMC_Base		0x09100000
	#define CMU_Base		0x05300000
	#define ICU_Base		0x05100000
	#define GPIO_Base		0x05000000

	#define MAX_SCREEN_LINES	0x10



