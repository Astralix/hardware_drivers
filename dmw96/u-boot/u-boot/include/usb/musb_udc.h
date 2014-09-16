/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * (C) Copyright 2009 Texas Instruments Incorporated.
 *
 * Based on
 * u-boot OMAP1510 USB drivers (include/usbdcore_omap1510.h)
 *
 * Author: Diego Dompe (diego.dompe@ridgerun.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef __MUSB_UDC_H__
#define __MUSB_UDC_H__

#include <usbdevice.h>

/* USB Function Module Registers */

/*
 * UDC_BASE is defined for the specific silicon under soc
 * specific cpu.h or related header.
 */

#define UDC_OFFSET(offset)	(UDC_BASE+(offset))

#define UDC_INTSRCR		UDC_OFFSET(0x0A) /* USB Interrupt src reg */
#define UDC_INTCLRR		UDC_OFFSET(0x0A) /* USB Interrupt src clr reg */

#define UDC_FADDR		UDC_OFFSET(0x00)
#define UDC_POWER		UDC_OFFSET(0x01)
#define UDC_INTRTX		UDC_OFFSET(0x02)
#define UDC_INTRRX		UDC_OFFSET(0x04)
#define UDC_INTRTXE		UDC_OFFSET(0x06) /* Enable reg for INTRTX */
#define UDC_INTRRXE		UDC_OFFSET(0x08) /* Enable reg for INTRRX */
#define UDC_INTRUSB		UDC_OFFSET(0x0A)
#define UDC_INTRUSBE		UDC_OFFSET(0x0B)
#define UDC_INDEX		UDC_OFFSET(0x0E)
#define UDC_TESTMODE		UDC_OFFSET(0x0F)
#define UDC_TXMAXP		UDC_OFFSET(0x10)
#define UDC_CSR0		UDC_OFFSET(0x12)
#define UDC_TXCSR		UDC_OFFSET(0x12)
#define UDC_RXMAXP		UDC_OFFSET(0x14)
#define UDC_RXCSR		UDC_OFFSET(0x16)
#define UDC_COUNT0		UDC_OFFSET(0x18)
#define UDC_RXCOUNT		UDC_OFFSET(0x18)
#define UDC_FIFO0		UDC_OFFSET(0x20)
#define UDC_FIFO1		UDC_OFFSET(0x24)
#define UDC_FIFO2		UDC_OFFSET(0x28)
#define UDC_FIFO3		UDC_OFFSET(0x2C)
#define UDC_FIFO4		UDC_OFFSET(0x30)
#define UDC_FIFO5		UDC_OFFSET(0x34)
#define UDC_FIFO6		UDC_OFFSET(0x38)
#define UDC_FIFO7		UDC_OFFSET(0x3C)
#define UDC_FIFO8		UDC_OFFSET(0x40)
#define UDC_FIFO9		UDC_OFFSET(0x44)
#define UDC_FIFO10		UDC_OFFSET(0x48)
#define UDC_FIFO11		UDC_OFFSET(0x4C)
#define UDC_FIFO12		UDC_OFFSET(0x50)
#define UDC_FIFO13		UDC_OFFSET(0x54)
#define UDC_FIFO14		UDC_OFFSET(0x58)
#define UDC_FIFO15		UDC_OFFSET(0x5C)
#define UDC_DEVCTL		UDC_OFFSET(0x60)
#define UDC_TXFIFOSZ		UDC_OFFSET(0x62)
#define UDC_RXFIFOSZ		UDC_OFFSET(0x63)
#define UDC_TXFIFOADDR		UDC_OFFSET(0x64)
#define UDC_RXFIFOADDR		UDC_OFFSET(0x66)

#define MUSB_VPLEN			UDC_OFFSET(0x7b)

#define UDC_SYSCONFIG		UDC_OFFSET(0x404)
#define UDC_INTERFSEL		UDC_OFFSET(0x40C)
#define UDC_FORCESTDBY		UDC_OFFSET(0x414)

#if !defined(CONFIG_USB_TTY)
/* MUSB Endpoint parameters */
#define EP0_MAX_PACKET_SIZE	64
//#define EP0_MAX_PACKET_SIZE	16	/* DSPG DFU/EP0 has issues with handling sizes bigger than 16 */ 
#define UDC_OUT_ENDPOINT	2	/* Device RX endpoint */
#define UDC_OUT_PACKET_SIZE	512
#define UDC_IN_ENDPOINT		3	/* Device TX endpoint */
#define UDC_IN_PACKET_SIZE	512
#define UDC_INT_ENDPOINT	1	/* Device Interrupt/Status endpoint */
#define UDC_INT_PACKET_SIZE	16
#define UDC_BULK_PACKET_SIZE	512
#endif

#define UDC_MAX_FIFO_SIZE	4096

#define DEV_CONFIG_VALUE	1	/* Only one i.e. CDC */



#define MUSB_EP0_FIFOSIZE	64	/* This is non-configurable */

/*
 * MUSB Register bits
 */

/* Offsets to endpoint registers */
#define MUSB_TXMAXP			0x00
#define MUSB_TXCSR			0x02
#define MUSB_CSR0		MUSB_TXCSR	/* Re-used for EP0 */
#define MUSB_RXMAXP			0x04
#define MUSB_RXCSR			0x06
#define MUSB_RXCOUNT		0x08
#define MUSB_COUNT0		MUSB_RXCOUNT	/* Re-used for EP0 */
#define MUSB_TXTYPE			0x0A
#define MUSB_TYPE0		MUSB_TXTYPE	/* Re-used for EP0 */
#define MUSB_TXINTERVAL		0x0B
#define MUSB_NAKLIMIT0		MUSB_TXINTERVAL	/* Re-used for EP0 */
#define MUSB_RXTYPE			0x0C
#define MUSB_RXINTERVAL		0x0D
#define MUSB_FIFOSIZE		0x0F
#define MUSB_CONFIGDATA		MUSB_FIFOSIZE	/* Re-used for EP0 */

/* POWER */
#define MUSB_POWER_ISOUPDATE	0x80
#define MUSB_POWER_SOFTCONN		0x40
#define MUSB_POWER_HSENAB		0x20
#define MUSB_POWER_HSMODE		0x10
#define MUSB_POWER_RESET		0x08
#define MUSB_POWER_RESUME		0x04
#define MUSB_POWER_SUSPENDM		0x02
#define MUSB_POWER_ENSUSPEND	0x01

/* INTRUSB */
#define MUSB_INTR_SUSPEND		0x01
#define MUSB_INTR_RESUME		0x02
#define MUSB_INTR_RESET			0x04
#define MUSB_INTR_BABBLE		0x04
#define MUSB_INTR_SOF			0x08
#define MUSB_INTR_CONNECT		0x10
#define MUSB_INTR_DISCONNECT	0x20
#define MUSB_INTR_SESSREQ		0x40
#define MUSB_INTR_VBUSERROR		0x80	/* For SESSION end */

/* DEVCTL */
#define MUSB_DEVCTL_BDEVICE		0x80
#define MUSB_DEVCTL_FSDEV		0x40
#define MUSB_DEVCTL_LSDEV		0x20
#define MUSB_DEVCTL_VBUS		0x18
#define MUSB_DEVCTL_VBUS_VALID	0x10
#define MUSB_DEVCTL_VBUS_SHIFT	3
#define MUSB_DEVCTL_HM			0x04
#define MUSB_DEVCTL_HR			0x02
#define MUSB_DEVCTL_SESSION		0x01


#define MUSB_VBUS_BELOW_SESS_END 	(0 << MUSB_DEVCTL_VBUS_SHIFT)
#define MUSB_VBUS_ABOVE_SESS_END 	(1 << MUSB_DEVCTL_VBUS_SHIFT)
#define MUSB_VBUS_ABOVE_AVALID 		(2 << MUSB_DEVCTL_VBUS_SHIFT)
#define MUSB_VBUS_ABOVE_VBUS_VALID 	(3 << MUSB_DEVCTL_VBUS_SHIFT)

  /* TESTMODE */
#define MUSB_TEST_FORCE_HOST	0x80
#define MUSB_TEST_FIFO_ACCESS	0x40
#define MUSB_TEST_FORCE_FS		0x20
#define MUSB_TEST_FORCE_HS		0x10
#define MUSB_TEST_PACKET		0x08
#define MUSB_TEST_K				0x04
#define MUSB_TEST_J				0x02
#define MUSB_TEST_SE0_NAK		0x01

/* Allocate for double-packet buffering (effectively doubles assigned _SIZE) */
#define MUSB_FIFOSZ_DPB	0x10
/* Allocation size (8, 16, 32, ... 4096) */
#define MUSB_FIFOSZ_SIZE	0x0f

/* CSR0 */
#define MUSB_CSR0_FLUSHFIFO		0x0100
#define MUSB_CSR0_TXPKTRDY		0x0002
#define MUSB_CSR0_RXPKTRDY		0x0001

/* CSR0 in Peripheral mode */
#define MUSB_CSR0_P_SVDSETUPEND	0x0080
#define MUSB_CSR0_P_SVDRXPKTRDY	0x0040
#define MUSB_CSR0_P_SENDSTALL	0x0020
#define MUSB_CSR0_P_SETUPEND	0x0010
#define MUSB_CSR0_P_DATAEND		0x0008
#define MUSB_CSR0_P_SENTSTALL	0x0004

/* CSR0 in Host mode */
#define MUSB_CSR0_H_DIS_PING		0x0800
#define MUSB_CSR0_H_WR_DATATOGGLE	0x0400	/* Set to allow setting: */
#define MUSB_CSR0_H_DATATOGGLE		0x0200	/* Data toggle control */
#define MUSB_CSR0_H_NAKTIMEOUT		0x0080
#define MUSB_CSR0_H_STATUSPKT		0x0040
#define MUSB_CSR0_H_REQPKT			0x0020
#define MUSB_CSR0_H_ERROR			0x0010
#define MUSB_CSR0_H_SETUPPKT		0x0008
#define MUSB_CSR0_H_RXSTALL			0x0004

/* CSR0 bits to avoid zeroing (write zero clears, write 1 ignored) */
#define MUSB_CSR0_P_WZC_BITS	\
	(MUSB_CSR0_P_SENTSTALL)
#define MUSB_CSR0_H_WZC_BITS	\
	(MUSB_CSR0_H_NAKTIMEOUT | MUSB_CSR0_H_RXSTALL \
	| MUSB_CSR0_RXPKTRDY)

/* TxType/RxType */
#define MUSB_TYPE_SPEED		0xc0
#define MUSB_TYPE_SPEED_SHIFT	6
#define MUSB_TYPE_PROTO		0x30	/* Implicitly zero for ep0 */
#define MUSB_TYPE_PROTO_SHIFT	4
#define MUSB_TYPE_REMOTE_END	0xf	/* Implicitly zero for ep0 */

/* CONFIGDATA */
#define MUSB_CONFIGDATA_MPRXE		0x80	/* Auto bulk pkt combining */
#define MUSB_CONFIGDATA_MPTXE		0x40	/* Auto bulk pkt splitting */
#define MUSB_CONFIGDATA_BIGENDIAN	0x20
#define MUSB_CONFIGDATA_HBRXE		0x10	/* HB-ISO for RX */
#define MUSB_CONFIGDATA_HBTXE		0x08	/* HB-ISO for TX */
#define MUSB_CONFIGDATA_DYNFIFO		0x04	/* Dynamic FIFO sizing */
#define MUSB_CONFIGDATA_SOFTCONE	0x02	/* SoftConnect */
#define MUSB_CONFIGDATA_UTMIDW		0x01	/* Data width 0/1 => 8/16bits */

/* TXCSR in Peripheral and Host mode */
#define MUSB_TXCSR_AUTOSET			0x8000
#define MUSB_TXCSR_DMAENAB			0x1000
#define MUSB_TXCSR_FRCDATATOG		0x0800
#define MUSB_TXCSR_DMAMODE			0x0400
#define MUSB_TXCSR_CLRDATATOG		0x0040
#define MUSB_TXCSR_FLUSHFIFO		0x0008
#define MUSB_TXCSR_FIFONOTEMPTY		0x0002
#define MUSB_TXCSR_TXPKTRDY			0x0001

/* TXCSR in Peripheral mode */
#define MUSB_TXCSR_P_ISO			0x4000
#define MUSB_TXCSR_P_INCOMPTX		0x0080
#define MUSB_TXCSR_P_SENTSTALL		0x0020
#define MUSB_TXCSR_P_SENDSTALL		0x0010
#define MUSB_TXCSR_P_UNDERRUN		0x0004

/* TXCSR in Host mode */
#define MUSB_TXCSR_H_WR_DATATOGGLE	0x0200
#define MUSB_TXCSR_H_DATATOGGLE		0x0100
#define MUSB_TXCSR_H_NAKTIMEOUT		0x0080
#define MUSB_TXCSR_H_RXSTALL		0x0020
#define MUSB_TXCSR_H_ERROR			0x0004

/* TXCSR bits to avoid zeroing (write zero clears, write 1 ignored) */
#define MUSB_TXCSR_P_WZC_BITS	\
	(MUSB_TXCSR_P_INCOMPTX | MUSB_TXCSR_P_SENTSTALL \
	| MUSB_TXCSR_P_UNDERRUN | MUSB_TXCSR_FIFONOTEMPTY)
#define MUSB_TXCSR_H_WZC_BITS	\
	(MUSB_TXCSR_H_NAKTIMEOUT | MUSB_TXCSR_H_RXSTALL \
	| MUSB_TXCSR_H_ERROR | MUSB_TXCSR_FIFONOTEMPTY)

/* RXCSR in Peripheral and Host mode */
#define MUSB_RXCSR_AUTOCLEAR		0x8000
#define MUSB_RXCSR_DMAENAB			0x2000
#define MUSB_RXCSR_DISNYET			0x1000
#define MUSB_RXCSR_PID_ERR			0x1000
#define MUSB_RXCSR_DMAMODE			0x0800
#define MUSB_RXCSR_INCOMPRX			0x0100
#define MUSB_RXCSR_CLRDATATOG		0x0080
#define MUSB_RXCSR_FLUSHFIFO		0x0010
#define MUSB_RXCSR_DATAERROR		0x0008
#define MUSB_RXCSR_FIFOFULL			0x0002
#define MUSB_RXCSR_RXPKTRDY			0x0001

/* RXCSR in Peripheral mode */
#define MUSB_RXCSR_P_ISO			0x4000
#define MUSB_RXCSR_P_SENTSTALL		0x0040
#define MUSB_RXCSR_P_SENDSTALL		0x0020
#define MUSB_RXCSR_P_OVERRUN		0x0004

/* RXCSR in Host mode */
#define MUSB_RXCSR_H_AUTOREQ		0x4000
#define MUSB_RXCSR_H_WR_DATATOGGLE	0x0400
#define MUSB_RXCSR_H_DATATOGGLE		0x0200
#define MUSB_RXCSR_H_RXSTALL		0x0040
#define MUSB_RXCSR_H_REQPKT			0x0020
#define MUSB_RXCSR_H_ERROR			0x0004

/* RXCSR bits to avoid zeroing (write zero clears, write 1 ignored) */
#define MUSB_RXCSR_P_WZC_BITS	\
	(MUSB_RXCSR_P_SENTSTALL | MUSB_RXCSR_P_OVERRUN \
	| MUSB_RXCSR_RXPKTRDY)
#define MUSB_RXCSR_H_WZC_BITS	\
	(MUSB_RXCSR_H_RXSTALL | MUSB_RXCSR_H_ERROR \
	| MUSB_RXCSR_DATAERROR | MUSB_RXCSR_RXPKTRDY)

/* HUBADDR */
#define MUSB_HUBADDR_MULTI_TT		0x80

/* UDC level routines */
void udc_irq(void);
void udc_set_nak(int ep_num);
void udc_unset_nak(int ep_num);
int udc_endpoint_write(struct usb_endpoint_instance *endpoint);
void udc_setup_ep(struct usb_device_instance *device, unsigned int id,
		  struct usb_endpoint_instance *endpoint);
void udc_connect(void);
void udc_disconnect(void);
void udc_enable(struct usb_device_instance *device);
void udc_disable(void);
void udc_startup_events(struct usb_device_instance *device);
int udc_init(void);

/* usbtty */
#ifdef CONFIG_USB_TTY

#define EP0_MAX_PACKET_SIZE	64 /* MUSB_EP0_FIFOSIZE */
#define UDC_INT_ENDPOINT	1
#define UDC_INT_PACKET_SIZE	64
#define UDC_OUT_ENDPOINT	2
#define UDC_OUT_PACKET_SIZE	64
#define UDC_IN_ENDPOINT		3
#define UDC_IN_PACKET_SIZE	64
#define UDC_BULK_PACKET_SIZE	64

#endif /* CONFIG_USB_TTY */

#endif /* __MUSB_UDC_H__ */
