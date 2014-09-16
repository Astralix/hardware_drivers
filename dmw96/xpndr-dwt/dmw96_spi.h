#define	BASE_SPI1_ADDR	 0x5d00000
#define BASE_SPI2_ADDR	 0x5e00000


#define	SPI_CFG_REG  		0x0000
#define	SPI_RATE_CNTL  		0x0004
#define	SPI_INT_EN  		0x0008
#define	SPI_INT_ST  		0x000c
#define	SPI_INT_CLR  		0x0010
#define	SPI_CAUSE_REG  		0x0014
#define	SPI_TX_DAT  		0x0018
#define	SPI_RX_DAT  		0x001c
#define	SPI_DEL_VAL  		0x0020
#define	SPI_RX_BUFF  		0x003c

#define ConstantSPIcint           0x00002000 //;; IcuCause SPI
