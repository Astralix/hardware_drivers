#ifndef __DSPG_UDC_H__
#define __DSPG_UDC_H__

/* HW IO helpers */

static inline void reg32_setbitrange(void* reg, int start, int stop, unsigned int value)
{
	unsigned int temp;
	unsigned int mask = ((1<<(stop-start+1))-1) << start;

	temp = __raw_readl(reg);
	temp &= ~mask;
	temp |= (value << start) & mask;
	__raw_writel(temp,reg);
};

#define REG32_SETBITRNG(reg,start,stop,value)	(reg32_setbitrange((reg),(start),(stop),(value)))
#define REG8_READ(reg)				(__raw_readb(&(reg)))
#define REG8_WRITE(reg,value)			(__raw_writeb((value),&(reg)))
#define REG32_READ(reg)				(__raw_readl(&(reg)))

typedef volatile u8	DW52_REG8;
typedef volatile u16	DW52_REG16;
typedef volatile u32	DW52_REG32;

/* UDC register set */
struct musbfcfs_regs {
	DW52_REG8		FAddr;  //0
	DW52_REG8		Power;	//1
	DW52_REG8		IntrIn1;//2
	DW52_REG8		IntrIn2;//3
	DW52_REG8		IntrOut1;
	DW52_REG8		IntrOut2;
	DW52_REG8		IntrUSB;
	DW52_REG8		IntrIn1E;	/* The IRQ enable Registers seem to be dead */
	DW52_REG8		IntrIn2E;	/* All stuck at "1"s */
	DW52_REG8		IntrOut1E;
	DW52_REG8		IntrOut2E;//a
	DW52_REG8		IntrUSBE;
	DW52_REG8		Frame1;
	DW52_REG8		Frame2;
	DW52_REG8		Index;
	DW52_REG8		_unused1;//f
	DW52_REG8		InMaxP;//0x10
	union {
		DW52_REG8	CSR0;
		DW52_REG8	InCSR1;
	}__attribute__ ((packed));
	DW52_REG8		InCSR2;//0x12
	DW52_REG8		OutMaxP;
	DW52_REG8		OutCSR1;
	DW52_REG8	OutCSR2;
	union {
		DW52_REG8	Count0;//0x16
		DW52_REG8	OutCount1;
	}__attribute__ ((packed));;
	DW52_REG8		OutCount2;
	DW52_REG8	_unused2[8];
	DW52_REG32	FIFO[16]; /* This should have been uint8_t, but it wasn't :( */
} __attribute__ ((packed));

/* Clock control register set */

struct dw52cmu_regs {
	DW52_REG32	CMR;
	DW52_REG32	CSR0;
	DW52_REG32	CSR1;
	DW52_REG32	LPWUR;
	DW52_REG32	LPBDTR0;
	DW52_REG32	LPBDTR1;
	DW52_REG32	LPCR;
	DW52_REG32	CDR0;
	DW52_REG32	CDR1;
	DW52_REG32	CDR2;
	DW52_REG32	CDR3;
	DW52_REG32	COMR;
	DW52_REG32	PLL1CR;
	DW52_REG32	PLL2CR0;
	DW52_REG32	PLL2CR1;
	DW52_REG32	SWRST;
	DW52_REG32	RSTSR;
	DW52_REG32	CUSTATR;
	DW52_REG32	CSFEOVR;
	DW52_REG32	CSFVOVR;
	DW52_REG32	BSWRSTR;
	DW52_REG32	DMR;
	DW52_REG32	DSPCR;
	DW52_REG32	DSPRST;
	DW52_REG32	OSCR;
	DW52_REG32	CSR2;
} __attribute__ ((packed));

struct dw52mb_gpio_regs {
	uint32_t	GPDATA;
	uint32_t	GPDIR;
	uint32_t	GPEN;
	uint32_t	GPPULLEN;
	uint32_t	GPPULLTYPE;
	uint32_t	GPPOD;
} __attribute__ ((packed));

struct dw52mb_ic_regs {
	uint32_t FIQMSK;
	uint32_t IRQMSK;
	uint32_t FIQCAUSE;
	uint32_t IRQCAUSE;
	uint32_t FIQCLR;
	uint32_t IRQCLR;
	uint32_t INTSTAT;
} __attribute__ ((packed));


#endif /* DSPG_UDC_H */
