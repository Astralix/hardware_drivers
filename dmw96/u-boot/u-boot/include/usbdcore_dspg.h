/* linux/include/asm/arch-s3c2410/regs-udc.h
 *
 * Copyright (C) 2004 Herbert Poetzl <herbert@13thfloor.at>
 *
 * This include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 *  Changelog:
 *    01-08-2004	Initial creation
 *    12-09-2004	Cleanup for submission
 *    24-10-2004	Fixed S3C2410_UDC_MAXP_REG definition
 *    10-03-2005	Changed S3C2410_VA to S3C24XX_VA
 *    10-01-2007	Modify for u-boot
 */

#ifndef __ASM_ARCH_REGS_UDC_H
#define __ASM_ARCH_REGS_UDC_H
#include <dspg_udc.h>
#define DSPG_UDC_NUM_ENDPOINTS	16

/* The various control and status bits */
#define UDC_EP0IRQ			0x1

#define UDC_IRQSUSPEND			0x1
#define UDC_IRQRESUME			0x2
#define UDC_IRQRESET			0x4
#define UDC_IRQSOF			0x8

#define UDC_CSR0_ServicedSetupEnd	0x80
#define UDC_CSR0_ServicedOutPktRdy	0x40
#define	UDC_CSR0_SendStall		0x20
#define UDC_CSR0_SetupEnd		0x10
#define	UDC_CSR0_DataEnd		0x08
#define UDC_CSR0_SentStall		0x04
#define UDC_CSR0_InPktRdy		0x02
#define	UDC_CSR0_OutPktRdy		0x01

#define UDC_INCSR_ClrDataTog		0x40
#define UDC_INCSR_SentStall		0x20
#define UDC_INCSR_SendStall		0x10
#define UDC_INCSR_FlushFIFO		0x08
#define UDC_INCSR_UnderRun		0x04
#define UDC_INCSR_FIFONotEmpty		0x02
#define UDC_INCSR_InPktRdy		0x01
//Reg2
#define UDC_INCSR_AutoSet		0x80
#define UDC_INCSR_ISO			0x40
#define UDC_INCSR_Mode			0x20
#define UDC_INCSR_DMAEnab		0x10
#define UDC_INCSR_FrcDataTog		0x08

#define UDC_OUTCSR_ClrDataTog		0x80
#define UDC_OUTCSR_SentStall		0x40
#define UDC_OUTCSR_SendStall		0x20
#define UDC_OUTCSR_FlushFIFO		0x10
#define UDC_OUTCSR_DataError		0x08
#define UDC_OUTCSR_OverRun		0x04
#define UDC_OUTCSR_FIFOFull		0x02
#define UDC_OUTCSR_OutPktRdy		0x01
//Reg2
#define UDC_OUTCSR_AutoClear		0x80
#define UDC_OUTCSR_ISO			0x40
#define UDC_OUTCSR_DMAEnab		0x20
#define UDC_OUTCSR_DMAMode		0x10

#define UDC_ISOUpdate		0x80
#define UDC_Reset		0x08
#define UDC_Resume		0x04
#define UDC_SuspendMode		0x02
#define UDC_EnableSuspend	0x01

#if CONFIG_DSPG_DW 
#define EP0_MAX_PACKET_SIZE	16
#define UDC_OUT_ENDPOINT 	2
#define UDC_OUT_PACKET_SIZE	16
#define UDC_IN_ENDPOINT		1
#define UDC_IN_PACKET_SIZE	16
#define UDC_INT_ENDPOINT	5
#define UDC_INT_PACKET_SIZE	16
#define UDC_BULK_PACKET_SIZE	64
#endif

#ifdef CONFIG_FIRETUX
#define EP0_MAX_PACKET_SIZE	8
#define UDC_OUT_ENDPOINT	2
#define UDC_OUT_PACKET_SIZE	16
#define UDC_IN_ENDPOINT		1
#define UDC_IN_PACKET_SIZE	16
#define UDC_INT_ENDPOINT	3
#define UDC_INT_PACKET_SIZE	16
#define UDC_BULK_PACKET_SIZE	64
#endif

void udc_irq (void);
/* Flow control */
void udc_set_nak(int epid);
void udc_unset_nak (int epid);

/* Higher level functions for abstracting away from specific device */
int  udc_endpoint_write(struct usb_endpoint_instance *endpoint);

int  udc_init (void);

void udc_enable(struct usb_device_instance *device);
void udc_disable(void);

void udc_connect(void);
void udc_disconnect(void);

void udc_startup_events(struct usb_device_instance *device);
int udc_setup_ep(struct usb_device_instance *device, unsigned int ep, struct usb_endpoint_instance *endpoint);
int udc_endpoint_feature(struct usb_device_instance *dev, int ep, int enable);


extern struct musbfcfs_regs* musb_regs;
extern struct dw52mb_ic_regs* foo_regs;

/****************** MACROS ******************/
#define DSPG_BIT_MASK	0xFF

#if 1
#define maskl(v,m,a)      \
		REG8_WRITE(a,v)
#else
#define maskl(v,m,a)	do {					\
	unsigned long foo = readl(a);				\
	unsigned long bar = (foo & ~(m)) | ((v)&(m));		\
	serial_printf("0x%08x:0x%x->0x%x\n", (a), foo, bar);	\
	writel(bar, (a));					\
} while(0)
#endif

#define DSPG_UDC_SETIX(X)			REG8_WRITE(musb_regs->Index,X)
#define clear_ep0_sst()			do{	\
						DSPG_UDC_SETIX(0);	\
						ep0csr &=~UDC_CSR0_SendStall;		\
						REG8_WRITE(musb_regs->CSR0,ep0csr);		\
					}while(0)

#define clear_ep0_opr() do {				\
	DSPG_UDC_SETIX(0);				\
	maskl(UDC_CSR0_ServicedOutPktRdy,		\
		DSPG_BIT_MASK, musb_regs->CSR0); 	\
} while(0)

#define clear_ep0_se() 	do {				\
	DSPG_UDC_SETIX(0); 				\
	ep0csr |= UDC_CSR0_ServicedSetupEnd;		\
	REG8_WRITE(musb_regs->CSR0,ep0csr);		\
} while(0)

#define set_ep0_ss() do {				\
	DSPG_UDC_SETIX(0);				\
	maskl(UDC_CSR0_SendStall,		\
		DSPG_BIT_MASK, musb_regs->CSR0);	\
} while(0)

#define set_ep0_de() do {				\
	DSPG_UDC_SETIX(0);				\
	maskl(UDC_CSR0_DataEnd,			\
		DSPG_BIT_MASK, musb_regs->CSR0);	\
} while(0)

#define set_ep0_de_out() do {				\
	DSPG_UDC_SETIX(0);				\
	maskl((UDC_CSR0_ServicedOutPktRdy	 	\
		| UDC_CSR0_DataEnd),		\
		DSPG_BIT_MASK,  musb_regs->CSR0);	\
} while(0)

#define set_ep0_ipr() do {				\
	DSPG_UDC_SETIX(0);				\
	maskl(UDC_CSR0_InPktRdy,		\
		DSPG_BIT_MASK, musb_regs->CSR0); 	\
} while(0)
#define set_ep0_de_in() do {				\
	DSPG_UDC_SETIX(0);				\
	maskl((UDC_CSR0_InPktRdy		\
		| UDC_CSR0_DataEnd),		\
		DSPG_BIT_MASK, musb_regs->CSR0);	\
} while(0)

#endif
