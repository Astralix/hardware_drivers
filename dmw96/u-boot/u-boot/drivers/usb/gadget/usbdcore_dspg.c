/* DSPG USB Device Controller Driver for u-boot
 *
 * (C) Copyright 2007 by OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * based on Linux' s3c2410_udc.c, which is
 * Copyright (C) 2004-2006 Herbert Pötzl - Arnaud Patard
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#include <config.h>
#if (defined(CONFIG_DSPG_DW)) && defined(CONFIG_USB_DEVICE) && !defined(CONFIG_MUSB)
#include <common.h>

/* we can't use the regular debug macros since the console might be
 * set to usbtty, which would cause deadlocks! */
//#define DEBUG
#ifdef DEBUG
#undef debug
#undef debugX
#define debug(fmt,args...) serial_printf (fmt ,##args)
#define debugX(level,fmt,args...) if (DEBUG>=level) serial_printf(fmt,##args)
#endif

DECLARE_GLOBAL_DATA_PTR;

#include <asm/io.h>

#include <usb_dfu.h>
#include <usbdevice.h>
#include <usbdcore_dspg.h>
#include "ep0.h"
#include <usb_cdc_acm.h>

struct musbfcfs_regs* musb_regs;
//struct dw52mb_ic_regs* foo_regs;

enum ep0_state{
	EP0_IDLE,
	EP0_IN_DATA_PHASE,
	EP0_OUT_DATA_PHASE,
	EP0_END_XFER,
	EP0_STALL,
};

static struct urb *ep0_urb = NULL;

struct usb_device_instance *udc_device; /* Used in interrupt handler */

static inline int fifo_count_out(int ep)
{
	u_int8_t tmp;
	DSPG_UDC_SETIX(ep);
	if(ep == 0)
		tmp = REG8_READ(musb_regs->Count0);
	else
		tmp = REG8_READ(musb_regs->OutCount1);

	return tmp & 0x7f; /* 7 bit */
}

static const unsigned long ep_fifo_reg[DSPG_UDC_NUM_ENDPOINTS] = {
	/*&(musb_regs->FIFO[0])*/0x0950a020,
	/*&(musb_regs->FIFO[1])*/0x0950a024,
	/*&(musb_regs->FIFO[2])*/0x0950a028,
	/*&(musb_regs->FIFO[3])*/0x0950a02c,
	/*&(musb_regs->FIFO[4])*/0x0950a030,
	/*&(musb_regs->FIFO[5])*/0x0950a034,
	/*&(musb_regs->FIFO[6])*/0x0950a038,
	/*&(musb_regs->FIFO[7])*/0x0950a03c,
	/*&(musb_regs->FIFO[8])*/0x0950a040,
	/*&(musb_regs->FIFO[9])*/0x0950a044,
	/*&(musb_regs->FIFO[10])*/0x0950a048,
	/*&(musb_regs->FIFO[11])*/0x0950a04c,
	/*&(musb_regs->FIFO[12])*/0x0950a050,
	/*&(musb_regs->FIFO[13])*/0x0950a054,
	/*&(musb_regs->FIFO[14])*/0x0950a058,
	/*&(musb_regs->FIFO[15])*/0x0950a05c,
};

extern void dw_usb_pullup(int enable);

void udc_ctrl(enum usbd_event event, int param)
{
	switch (event) {
	case UDC_CTRL_PULLUP_ENABLE:
		dw_usb_pullup(param);
		break;
	default:
		break;
	}
}

static int dspg_write_noniso_tx_fifo(struct usb_endpoint_instance *endpoint)
{
	struct urb *urb = endpoint->tx_urb;
	unsigned int last, i;
	unsigned int ep = endpoint->endpoint_address & 0x7f;
	unsigned long fifo_reg = ep_fifo_reg[ep];

	/* WARNING: don't ever put serial debug printf's in non-error codepaths
	 * here, it is called from the time critical EP0 codepath ! */

	if (!urb || ep >= DSPG_UDC_NUM_ENDPOINTS) {
		serial_printf("no urb or wrong endpoint\n");
		return -1;
	}

	DSPG_UDC_SETIX(ep);
	if ((last = MIN(urb->actual_length - endpoint->sent,
		        endpoint->tx_packetSize))) {
		u8 *cp = urb->buffer + endpoint->sent;

		debug("udc: writing %d bytes (max: %d; ep: %d)\n", last, endpoint->tx_packetSize, ep);

		for (i = 0; i < last; i++)
			outb(*(cp+i), fifo_reg);
	}
	endpoint->last = last;

	if (endpoint->sent + last < urb->actual_length) {
		/* not all data has been transmitted so far */
		return 0;
	}

	if (last == endpoint->tx_packetSize) {
		/* we need to send one more packet (ZLP) */
		return 0;
	}

	return 1;
}


static void dspg_deconfigure_device (void)
{
	/* FIXME: Implement this */
}

static void dspg_configure_device (struct usb_device_instance *device)
{
	/* enable USB interrupts for SUSPEND/RESUME */
	REG8_WRITE(musb_regs->IntrUSBE,0x6);
	/* enable endpoint interrupt */
	REG8_WRITE(musb_regs->IntrIn1E,0xff);
	REG8_WRITE(musb_regs->IntrOut1E,0xfe);
}

static void udc_set_address(unsigned char address)
{
	address |= 0x80; /* ADDR_UPDATE bit */
	REG8_WRITE(musb_regs->FAddr,address);
}

extern struct usb_device_descriptor device_descriptor;

//static
void handle_ep0(void)
{
	u_int8_t ep0csr;
	struct usb_endpoint_instance *ep0 = udc_device->bus->endpoint_array;

	DSPG_UDC_SETIX(0);

	ep0csr = REG8_READ(musb_regs->CSR0);
	/* clear stall status */
	if (ep0csr & UDC_CSR0_SendStall) {
		debug("Clearing SENT_STALL\n");
		clear_ep0_sst();
		if (ep0csr & UDC_CSR0_ServicedOutPktRdy)
			clear_ep0_opr();
		ep0->state = EP0_IDLE;
		return;
	}

	/* clear setup end */
	if (ep0csr & UDC_CSR0_SetupEnd/* && ep0->state != EP0_IDLE */) {
		debug("Clearing SETUP_END\n");
		clear_ep0_se();
		ep0->state = EP0_IDLE;
		//return;
	}

	//debug("HANDLE_EP0 %d 0x%x\n", ep0->state, ep0csr);
	/* Don't ever put [serial] debugging in non-error codepaths here, it
	 * will violate the tight timing constraints of this USB Device
	 * controller (and lead to bus enumeration failures) */

	switch (ep0->state) {
		int i, fifo_count = 0;
		unsigned char *datap;
	case EP0_IDLE:
		if (!(ep0csr & UDC_CSR0_OutPktRdy)) {
			//serial_printf("UDC_CSR0_OutPktRdy\n");
			return;
		}

		datap = (unsigned char *) &ep0_urb->device_request;
		/* host->device packet has been received */
		fifo_count = REG8_READ(musb_regs->Count0);
		/* pull it out of the fifo */
		if (fifo_count != 8) {
			serial_printf("strange fifo count: %u bytes\n", fifo_count);
			set_ep0_ss();
			return;
		}

		for (i = 0; i < fifo_count; i++) {
			*datap = (unsigned char)REG8_READ(musb_regs->FIFO[0]);
			datap++;
		}


		//debug("WLENGTH %d\n", ep0_urb->device_request.wLength);
		if (ep0_urb->device_request.wLength == 0) {
			if (ep0_recv_setup(ep0_urb)) {
				/* Not a setup packet, stall next EP0 transaction */
				debug("can't parse setup packet1\n");
				set_ep0_ss();
				set_ep0_de_out();
				ep0->state = EP0_IDLE;
				return;
			}
			/* There are some requests with which we need to deal
			 * manually here */
			switch (ep0_urb->device_request.bRequest)
			{
			case USB_REQ_SET_CONFIGURATION:
				if (!ep0_urb->device_request.wValue)
					usbd_device_event_irq(udc_device,
							DEVICE_DE_CONFIGURED, 0);
				else
					usbd_device_event_irq(udc_device,
							DEVICE_CONFIGURED, 0);
				break;
			case USB_REQ_SET_ADDRESS:
				udc_set_address(udc_device->address);
				usbd_device_event_irq(udc_device,
						DEVICE_ADDRESS_ASSIGNED, 0);
				break;
			default:
				break;
			}
			set_ep0_de_out();
			ep0->state = EP0_IDLE;
		} else {
			//serial_printf("DIRECTION_MASK: 0x%x\n", ep0_urb->device_request.bmRequestType);
			if ((ep0_urb->device_request.bmRequestType & USB_REQ_DIRECTION_MASK)
			    == USB_REQ_HOST2DEVICE) {
				clear_ep0_opr();
				ep0->state = EP0_OUT_DATA_PHASE;
				ep0_urb->buffer = ep0_urb->buffer_data;
				ep0_urb->buffer_length = sizeof(ep0_urb->buffer_data);
				ep0_urb->actual_length = 0;
			} else {
				ep0->state = EP0_IN_DATA_PHASE;
				ep0_urb->buffer = ep0_urb->buffer_data;
				ep0_urb->buffer_length = sizeof(ep0_urb->buffer_data);
				ep0_urb->actual_length = 0;

				if (ep0_recv_setup(ep0_urb)) {
					/* Not a setup packet, stall next EP0 transaction */
					debug("can't parse setup packet2\n");
					set_ep0_ss();
					//set_ep0_de_out();
					ep0->state = EP0_IDLE;
					return;
				}
				clear_ep0_opr();
				ep0->tx_urb = ep0_urb;
				ep0->sent = ep0->last = 0;

				if (dspg_write_noniso_tx_fifo(ep0)) {
					ep0->state = EP0_IDLE;
					set_ep0_de_in();
				}
				else
					set_ep0_ipr();
			}
		}
		break;
	case EP0_IN_DATA_PHASE:
		if (!(ep0csr & UDC_CSR0_InPktRdy)) {
			ep0->sent += ep0->last;

			if (dspg_write_noniso_tx_fifo(ep0)) {
				ep0->state = EP0_IDLE;
				set_ep0_de_in();
			} else
				set_ep0_ipr();
		}
		break;
	case EP0_OUT_DATA_PHASE:
		if (ep0csr & UDC_CSR0_OutPktRdy) {
			u32 urb_avail = ep0_urb->buffer_length - ep0_urb->actual_length;
			u_int8_t *cp = ep0_urb->buffer + ep0_urb->actual_length;
			int i, fifo_count;
			//serial_printf("FIFO0 = %x,FIFO2= %x\n",&(musb_regs->FIFO[0]),&(musb_regs->FIFO[2]));
			fifo_count = REG8_READ(musb_regs->Count0);
			if (fifo_count < urb_avail)
				urb_avail = fifo_count;

			for (i = 0; i < urb_avail; i++)
				*cp++ = REG8_READ(musb_regs->FIFO[0]);

			ep0_urb->actual_length += urb_avail;

			if (fifo_count < ep0->rcv_packetSize ||
			    ep0_urb->actual_length >= ep0_urb->device_request.wLength) {
				ep0->state = EP0_IDLE;
				if (ep0_recv_setup(ep0_urb)) {
					/* Not a setup packet, stall next EP0 transaction */
					debug("can't parse setup packet3\n");
					set_ep0_ss();
					//set_ep0_de_out();
					return;
				}
				set_ep0_de_out();
			} else
				clear_ep0_opr();
		}
		break;
	case EP0_END_XFER:
		ep0->state = EP0_IDLE;
		break;
	case EP0_STALL:
		//set_ep0_ss;
		ep0->state = EP0_IDLE;
		break;
	}
}


static void handle_epin(int ep)
{
	//serial_printf("handle ep in\n");
}


static void handle_epout(int ep)
{
	struct usb_endpoint_instance *endpoint;
	struct urb *urb;
	u32 ep_csr1;
	struct usb_device_request *request;
	struct usb_device_instance *device;

	int first = 0;

	if (ep >= DSPG_UDC_NUM_ENDPOINTS)
		return;

	endpoint = &udc_device->bus->endpoint_array[ep];

	DSPG_UDC_SETIX(ep);

	if (endpoint->endpoint_address & USB_DIR_IN) {
		/* IN transfer (device to host) */
		ep_csr1 = REG8_READ(musb_regs->InCSR1);

		debug("for ep=%u, CSR1=0x%x ", ep, ep_csr1);

		urb = endpoint->tx_urb;
		request = &urb->device_request;
		device = urb->device;
		if (ep_csr1 & UDC_INCSR_SentStall) {
			/* Stall handshake */
			debug("stall\n");
			ep_csr1 &= UDC_INCSR_SentStall;
			REG8_WRITE(musb_regs->InCSR1,ep_csr1);
			return;
		}
		if (!(ep_csr1 & UDC_INCSR_InPktRdy) && urb &&
		      urb->actual_length) {
			debug("completing previously send data ");
			usbd_tx_complete(endpoint);

			/* push pending data into FIFO */
			if ((endpoint->last == endpoint->tx_packetSize) &&
			    (urb->actual_length - endpoint->sent - endpoint->last == 0)) {
				endpoint->sent += endpoint->last;
				/* Write 0 bytes of data (ZLP) */
				debug("ZLP ");
				REG8_WRITE(musb_regs->InCSR1,ep_csr1|UDC_INCSR_InPktRdy);
			} else {
				/* write actual data to fifo */
				debug("TX_DATA ");
				dspg_write_noniso_tx_fifo(endpoint);
				REG8_WRITE(musb_regs->InCSR1,ep_csr1|UDC_INCSR_InPktRdy);
			}
		}
		debug("\n");
	} else {
		/* OUT transfer (host to device) */
		ep_csr1 = REG8_READ(musb_regs->OutCSR1);
		debug("for ep=%u, CSR1=0x%x ", ep, ep_csr1);

		urb = endpoint->rcv_urb;
		request = &urb->device_request;
		device = urb->device;
		//u_int16_t val = request->wValue;

		if (ep_csr1 & UDC_OUTCSR_SentStall) {
			/* Stall handshake */
			ep_csr1 &= ~UDC_OUTCSR_SentStall;
			REG8_WRITE(musb_regs->OutCSR1, ep_csr1);
			return;
		}
		if ((ep_csr1 & UDC_OUTCSR_OutPktRdy) && urb) {
			/* Read pending data from fifo */
			u32 fifo_count = REG8_READ(musb_regs->OutCount1)|(REG8_READ(musb_regs->OutCount2)<<8);
			int is_last = 0;
			urb->buffer = urb->buffer_data;
			u32 i, urb_avail = urb->buffer_length - urb->actual_length;
			u8 *cp = urb->buffer + urb->actual_length;

			if (fifo_count < endpoint->rcv_packetSize)
				is_last = 1;

			debug("fifo_count=%u is_last=%d, urb_avail=%u, buf_len=%d, act_len=%d\n",
			      fifo_count, is_last, urb_avail, urb->buffer_length, urb->actual_length);

			if (fifo_count < urb_avail)
				urb_avail = fifo_count;
			for (i = 0; i < urb_avail; i++) {
				*cp++ = inb(ep_fifo_reg[ep]);
			}

//			if (is_last)
			ep_csr1 &= ~UDC_OUTCSR_OutPktRdy;
			REG8_WRITE(musb_regs->OutCSR1,ep_csr1);
			usbd_rcv_complete(endpoint, urb_avail, 0);

#ifdef CONFIG_USBD_DFU
			if (is_last || urb->actual_length >= CONFIG_USBD_DFU_XFER_SIZE) {
				switch (device->dfu_state) {
				case DFU_STATE_dfuIDLE:
					first = 1;
					device->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
					break;
				case DFU_STATE_dfuDNLOAD_IDLE:
					first = 0;
					device->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
					break;
				default:
					break;
				}
				//handle_dnload(urb, 0, urb->actual_length, first);
			}
#endif
		}
	}

	urb = endpoint->rcv_urb;
}

/*
-------------------------------------------------------------------------------
*/

/* Handle general USB interrupts and dispatch according to type.
 * This function implements TRM Figure 14-13.
 */
void udc_irq(void)
{
	struct usb_endpoint_instance *ep0 = udc_device->bus->endpoint_array;
	u_int8_t save_idx = REG8_READ(musb_regs->Index);//bit 4

	//serial_printf("UDC/USB INTERRUPT\n");

	/* read interrupt sources */

	u_int8_t usb_status = REG8_READ(musb_regs->IntrUSB);
	u_int16_t usb_epin_status = REG8_READ(musb_regs->IntrIn1) | (REG8_READ(musb_regs->IntrIn2)<<8);
	u_int16_t usb_epout_status = REG8_READ(musb_regs->IntrOut1) | (REG8_READ(musb_regs->IntrOut2)<<8);


	//debug("< IRQ usbs=0x%02x, usbin=0x%02x, usbout=0x%02x start>\n", usb_status, usb_epin_status, usb_epout_status);

#if 1
	if (usb_status & UDC_IRQRESET) {
		//serial_putc('R');
		//debug("ep0 addr = %x\n", (u32)ep0->endpoint_address);
		udc_setup_ep(udc_device, 0, ep0);
		REG8_WRITE(musb_regs->FAddr,0);
		REG8_WRITE(musb_regs->CSR0,UDC_CSR0_ServicedSetupEnd|UDC_CSR0_ServicedOutPktRdy);
		ep0->state = EP0_IDLE;
		usbd_device_event_irq (udc_device, DEVICE_RESET, 0);
	}

	if (usb_status & UDC_IRQRESUME) {
		debug("RESUME\n");
		usbd_device_event_irq(udc_device, DEVICE_BUS_ACTIVITY, 0);
	}

	if (usb_status & UDC_IRQSUSPEND) {
		debug("SUSPEND\n");
		usbd_device_event_irq(udc_device, DEVICE_BUS_INACTIVE, 0);
	}

	/* Endpoint Interrupts */
	if (usb_epin_status | usb_epout_status) {
		int i;

		if (usb_epin_status & UDC_EP0IRQ) {
			handle_ep0();
		}

		for (i = 1; i < DSPG_UDC_NUM_ENDPOINTS; i++) {
			u_int32_t tmp = 1 << i;

			if ((usb_epin_status | usb_epout_status) & tmp) {
				handle_epout(i);
			}
		}
	}
	DSPG_UDC_SETIX(save_idx);
#endif
}

/*
-------------------------------------------------------------------------------
*/


/*
 * Start of public functions.
 */

/* Called to start packet transmission. */
int udc_endpoint_write (struct usb_endpoint_instance *endpoint)
{
	u32 ep_csr1;
	struct urb *urb;
	unsigned short epnum =
		endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK;

	debug("Entering for ep %x ", epnum);

	DSPG_UDC_SETIX(epnum);

	ep_csr1 = REG8_READ(musb_regs->InCSR1);

	urb = endpoint->tx_urb;
	if (ep_csr1 & UDC_INCSR_SendStall) {
		/* Stall handshake enabled, not writing data */
		ep_csr1 &= UDC_INCSR_SentStall;
		REG8_WRITE(musb_regs->InCSR1,ep_csr1);
		return 0;
	}
	if (!(ep_csr1 & UDC_INCSR_InPktRdy) && urb &&
	      urb->actual_length) {
		debug("completing previously send data ");
		usbd_tx_complete(endpoint);

		/* push pending data into FIFO */
		if ((endpoint->last == endpoint->tx_packetSize) &&
		    (urb->actual_length - endpoint->sent - endpoint->last == 0)) {
			endpoint->sent += endpoint->last;
			/* Write 0 bytes of data (ZLP) */
			debug("ZLP ");
			REG8_WRITE(musb_regs->InCSR1,ep_csr1|UDC_INCSR_InPktRdy);
		} else {
			/* write actual data to fifo */
			debug("TX_DATA ");
			dspg_write_noniso_tx_fifo(endpoint);
			REG8_WRITE(musb_regs->InCSR1,ep_csr1|UDC_INCSR_InPktRdy);
		}
	}

	return 0;
}

/* Start to initialize h/w stuff */
int udc_init (void)
{
	debug("udc_init\n");
	//already init before, just map reg.
//	foo_regs = (struct dw52mb_ic_regs *)(DW_SIC_BASE);

	musb_regs = (struct musbfcfs_regs *)(0x0950a000);

	debug("udc_init_end\n");
	return 0;
}

/*
 * udc_setup_ep - setup endpoint
 *
 * Associate a physical endpoint with endpoint_instance
 */
int udc_setup_ep (struct usb_device_instance *device,
		   unsigned int ep, struct usb_endpoint_instance *endpoint)
{
	int ep_addr = endpoint->endpoint_address;
	int packet_size;
	int attributes;
	u_int32_t maxp;


	DSPG_UDC_SETIX(ep);

	if (ep) {
		if ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) {
			REG8_WRITE(musb_regs->InCSR1,UDC_INCSR_ClrDataTog|UDC_INCSR_FlushFIFO);
			REG8_WRITE(musb_regs->InCSR2,UDC_INCSR_Mode);
			packet_size = endpoint->tx_packetSize;
			attributes = endpoint->tx_attributes;
			maxp = (packet_size == 1023)?128:(packet_size/8);
			REG8_WRITE(musb_regs->InMaxP,maxp);
		} else {
			packet_size = endpoint->rcv_packetSize;
			attributes = endpoint->rcv_attributes;
			maxp = (packet_size == 1023)?128:(packet_size/8);
			REG8_WRITE(musb_regs->OutMaxP,maxp);

			//REG8_WRITE(musb_regs->InCSR1,UDC_INCSR_ClrDataTog);
			//REG8_WRITE(musb_regs->InCSR2,0);
			REG8_WRITE(musb_regs->OutCSR2,0);
			REG8_WRITE(musb_regs->OutCSR1,UDC_OUTCSR_ClrDataTog|UDC_OUTCSR_FlushFIFO);
		}
	} else{
		packet_size = endpoint->tx_packetSize;
		maxp = (packet_size == 1023)?128:(packet_size/8);
		REG8_WRITE(musb_regs->InMaxP,maxp);
	}


	debug("setting up endpoint %u addr %x packet_size %u maxp %u\n", ep,
		endpoint->endpoint_address, packet_size, maxp);

	return 0;
}

/* ************************************************************************** */

/**
 * udc_connected - is the USB cable connected
 *
 * Return non-zero if cable is connected.
 */
#if 0
int udc_connected (void)
{
	return ((inw (UDC_DEVSTAT) & UDC_ATT) == UDC_ATT);
}
#endif

/* Turn on the USB connection by enabling the pullup resistor */
void udc_connect (void)
{
	debug("connect, enable Pullup\n");
	udc_ctrl(UDC_CTRL_PULLUP_ENABLE, 0);
	udelay(10000);
	udc_ctrl(UDC_CTRL_PULLUP_ENABLE, 1);

}

/* Turn off the USB connection by disabling the pullup resistor */
void udc_disconnect (void)
{
	debug("disconnect, disable Pullup\n");

	udc_ctrl(UDC_CTRL_PULLUP_ENABLE, 0);

	/* Disable interrupt (we don't want to get interrupts while the kernel*/
}

/* Switch on the UDC */
void udc_enable (struct usb_device_instance *device)
{
	debug("enable device %p, status %d\n", device, device->status);

	/* Save the device structure pointer */
	udc_device = device;

	/* Setup ep0 urb */
	if (!ep0_urb)
	{
		ep0_urb = usbd_alloc_urb(udc_device,
					 udc_device->bus->endpoint_array);
	}
	else
		serial_printf("udc_enable: ep0_urb already allocated %p\n",
			       ep0_urb);

	dspg_configure_device(device);
}

/* Switch off the UDC */
void udc_disable (void)
{
	debug("disable UDC\n");

	dspg_deconfigure_device();

	/* Free ep0 URB */
	if (ep0_urb) {
		/*usbd_dealloc_urb(ep0_urb); */
		ep0_urb = NULL;
	}

	/* Reset device pointer.
	 * We ought to do this here to balance the initialization of udc_device
	 * in udc_enable, but some of our other exported functions get called
	 * by the bus interface driver after udc_disable, so we have to hang on
	 * to the device pointer to avoid a null pointer dereference. */
	/* udc_device = NULL; */
}

/**
 * udc_startup - allow udc code to do any additional startup
 */
void udc_startup_events (struct usb_device_instance *device)
{
	/* The DEVICE_INIT event puts the USB device in the state STATE_INIT. */
	usbd_device_event_irq (device, DEVICE_INIT, 0);

	/* The DEVICE_CREATE event puts the USB device in the state
	 * STATE_ATTACHED.
	 */
	usbd_device_event_irq (device, DEVICE_CREATE, 0);

	/* Some USB controller driver implementations signal
	 * DEVICE_HUB_CONFIGURED and DEVICE_RESET events here.
	 * DEVICE_HUB_CONFIGURED causes a transition to the state STATE_POWERED,
	 * and DEVICE_RESET causes a transition to the state STATE_DEFAULT.
	 * The OMAP USB client controller has the capability to detect when the
	 * USB cable is connected to a powered USB bus via the ATT bit in the
	 * DEVSTAT register, so we will defer the DEVICE_HUB_CONFIGURED and
	 * DEVICE_RESET events until later.
	 */

	/* The GTA01 can detect usb device attachment, but we just assume being
	 * attached for now (go to STATE_POWERED) */
	usbd_device_event_irq (device, DEVICE_HUB_CONFIGURED, 0);

	udc_enable (device);
}

void udc_set_nak(int epid)
{
	/* FIXME: implement this */
}

void udc_unset_nak(int epid)
{
	/* FIXME: implement this */
}

int udc_endpoint_feature(struct usb_device_instance *dev, int ep, int enable)
{
	struct usb_endpoint_instance *endpoint;
	struct urb *urb;
	u32 ep_csr1;

	endpoint = &dev->bus->endpoint_array[ep];
	DSPG_UDC_SETIX(ep);

	if (endpoint->endpoint_address & USB_DIR_IN) {
		ep_csr1 = REG8_READ(musb_regs->InCSR1);
		if (enable && !(ep_csr1 & UDC_INCSR_SendStall)) {
			ep_csr1 |= UDC_INCSR_SendStall;
		} else if (!enable) {
			ep_csr1 &= ~(UDC_INCSR_SendStall | UDC_INCSR_SentStall);
			ep_csr1 |= UDC_INCSR_ClrDataTog;
			urb = endpoint->tx_urb;
			if (urb)
				urb->actual_length = 0;
		} else
			return 0;

		/* double-buffer! */
		ep_csr1 |= UDC_INCSR_FlushFIFO;
		REG8_WRITE(musb_regs->InCSR1, ep_csr1);
		REG8_WRITE(musb_regs->InCSR1, ep_csr1);
	} else {
		ep_csr1 = REG8_READ(musb_regs->OutCSR1);
		if (enable && !(ep_csr1 & UDC_OUTCSR_SendStall)) {
			ep_csr1 |= UDC_OUTCSR_SendStall;
		} else if (!enable) {
			ep_csr1 &= ~(UDC_OUTCSR_SendStall | UDC_OUTCSR_SentStall);
			ep_csr1 |= UDC_OUTCSR_ClrDataTog;
			urb = endpoint->rcv_urb;
			if (urb)
				urb->actual_length = 0;
		} else
			return 0;

		/* double-buffer! */
		ep_csr1 |= UDC_OUTCSR_FlushFIFO;
		REG8_WRITE(musb_regs->OutCSR1, ep_csr1);
		REG8_WRITE(musb_regs->OutCSR1, ep_csr1);
	}

	return 0;
}

#endif /* CONFIG_DSPG_DW && CONFIG_USB_DEVICE */
