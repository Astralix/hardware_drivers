/*
 * (C) Copyright 2009 Texas Instruments Incorporated.
 *
 * Based on
 * u-boot OMAP1510 USB drivers (drivers/usbdcore_omap1510.c)
 * twl4030 init based on linux (drivers/i2c/chips/twl4030_usb.c)
 *
 * Author:	Diego Dompe (diego.dompe@ridgerun.com)
 *		Atin Malaviya (atin.malaviya@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * (C) Copyright 2009 Intelligraphics Inc.
 * Release History: 
 * Atanas (Tony) Zlatinski - port to DSPG DW74 chipset
 * 						   - re-implementation of EP 0 and USB state machine
 *
*/

#include <common.h>

#include <asm/io.h>

#ifdef CONFIG_USB_OMAP3
#include <asm/arch/clocks.h>
#include <asm/arch/clocks_omap3.h>
#include <asm/arch/sys_proto.h>
#endif

#ifdef CONFIG_DSPG_DW
#include <dspg_otg_udc.h>
#endif

#include <usbdevice.h>
#include <usb/musb_udc.h>
#include "ep0.h"

#if 0
 #define MUSB_DEBUG_EP0(fmt,args...) printf("[%s] %s:%d: "fmt"\n",__FILE__,__FUNCTION__,__LINE__,##args)
#else
 #define MUSB_DEBUG_EP0(fmt,args...) 
#endif

#define MUSB_WARN_EP0(fmt,args...) printf("[%s] %s:%d: "fmt"\n",__FILE__,__FUNCTION__,__LINE__,##args)


#if 0
 #define MUSB_DEBUG printf
#else
 #define MUSB_DEBUG(fmt, args...) do{ ; }while(0)
#endif

#if CONFIG_MUSB_FULL_SPEED
 #define MUSB_POWER_DEFAULT_VAL 0
#else
 #define MUSB_POWER_DEFAULT_VAL MUSB_POWER_HSENAB
#endif

#define MUSB_INTR_MASK (MUSB_INTR_SUSPEND 		| \
						MUSB_INTR_RESUME  		| \
						MUSB_INTR_RESET	  		| \
						/* MUSB_INTR_SOF  */	  \
						MUSB_INTR_CONNECT		| \
						MUSB_INTR_DISCONNECT 	| \
						MUSB_INTR_SESSREQ	 	| \
						MUSB_INTR_VBUSERROR	 	)


/* Private definitions */
//enum ep0_status { IDLE, DATA_STAGE, DATA_COMPLETE };

/* peripheral side ep0 states */
enum musb_ep0_stage {
	MUSB_EP0_STAGE_IDLE,		/* idle, waiting for SETUP */
	MUSB_EP0_STAGE_SETUP,		/* received SETUP */
	MUSB_EP0_STAGE_TX,		/* IN data */
	MUSB_EP0_STAGE_RX,		/* OUT data */
	MUSB_EP0_STAGE_STATUSIN,	/* (after OUT data) */
	MUSB_EP0_STAGE_STATUSOUT,	/* (after IN data) */
	MUSB_EP0_STAGE_ACKWAIT		/* after zlp, before statusin */
};

typedef enum musb_buff_xfer_status {
	MUSB_BUFF_XFER_ERR = -1,		/* ERROR when transfering buffer */
	MUSB_BUFF_XFER_FULL = 0,			/* The entire buffer has been transfered */
	MUSB_BUFF_XFER_PARTIAL = 1,			/* Part of the buffer has been transfered */
	MUSB_BUFF_XFER_INVALID = ~((unsigned)0)		/* END of enum */
}musb_buff_xfer_status_t;

/* Private variables */
static struct usb_device_instance *udc_device;
static enum musb_ep0_stage ep0stage = MUSB_EP0_STAGE_IDLE;
static unsigned char do_set_address = 0;
static struct urb *ep0_urb = NULL;
static unsigned int hw_fifos_configured = 0;

static const char *decode_ep0stage(musb_buff_xfer_status_t stage)
{
	switch (stage) {
	case MUSB_EP0_STAGE_IDLE:	return "IDLE";
	case MUSB_EP0_STAGE_SETUP:	return "SETUP";
	case MUSB_EP0_STAGE_TX:		return "IN - TX";
	case MUSB_EP0_STAGE_RX:		return "OUT - RX ";
	case MUSB_EP0_STAGE_ACKWAIT:	return "ACK";
	case MUSB_EP0_STAGE_STATUSIN:	return "IN  STATUS";
	case MUSB_EP0_STAGE_STATUSOUT:	return "OUT STATUS";
	default:			return "UNKNOWN";
	}
}


/* Helper functions */
static void insl(u32 reg, u32 *data, u32 size)
{
	u32 t;

	for (t = 0; t < size; t++, data++)
		*data = inl(reg);
}

static void outsl(u32 reg, u32 *data, u32 size)
{
	u32 t;

	for (t = 0; t < size; t++, data++)
		outl(*data, reg);
}

static void outsb(u32 reg, u8 *data, u32 size)
{
	u32 t;

	for (t = 0; t < size; t++, data++)
		outb(*data, reg);
}

static void musb_fifo_read(int epnumber, u8 *data, u32 size)
{
	if ((u32)data & 0x3) {		/* Not aligned data */
		insb((UDC_FIFO0 + (epnumber << 2)), data, size);
	} else {			/* 32 bits aligned data */
		int i;

		insl(UDC_FIFO0 + (epnumber << 2), (u32 *)data, size >> 2);
		data += size & ~0x3;
		i = size & 0x3;
		while (i--) {
			*data = inb(UDC_FIFO0 + (epnumber << 2));
			data++;
		}
	}
}

static void musb_fifo_write(int epnumber, u8 *data, u32 size)
{
	if ((u32)data & 0x3) {		/* Not aligned data */
		outsb(UDC_FIFO0 + (epnumber << 2), data, size);
	} else {			/* 32 bits aligned data */
		int i;

		outsl(UDC_FIFO0 + (epnumber << 2), (u32 *)data, size >> 2);
		data += size & ~0x3;
		i = size & 0x3;
		while (i--) {
			outb(*data, UDC_FIFO0 + (epnumber << 2));
			data++;
		}
	}
}

static void musb_fifos_configure(struct usb_device_instance *device)
{
	int ep;
	struct usb_bus_instance *bus;
	struct usb_endpoint_instance *endpoint;
	unsigned short ep_ptr, ep_size, ep_doublebuffer;
	int ep_addr, packet_size, buffer_size, attributes;

	bus = device->bus;

	ep_ptr = 0;

	for (ep = 0; ep < bus->max_endpoints; ep++) {
		endpoint = bus->endpoint_array + ep;
		ep_addr = endpoint->endpoint_address;
		if ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) {
			/* IN endpoint */
			packet_size = endpoint->tx_packetSize;
			attributes = endpoint->tx_attributes;
		} else {
			/* OUT endpoint */
			packet_size = endpoint->rcv_packetSize;
			attributes = endpoint->rcv_attributes;
		}
		
		switch (packet_size) {
		case 0:
			ep_size = 0;
			break;
		case 8:
			ep_size = 0;
			break;
		case 16:
			ep_size = 1;
			break;
		case 32:
			ep_size = 2;
			break;
		case 64:
			ep_size = 3;
			break;
		case 128:
			ep_size = 4;
			break;
		case 256:
			ep_size = 5;
			break;
		case 512:
			ep_size = 6;
			break;
		case 1024:
			ep_size = 7;
			break;

		default:
			printf("ep 0x%02x has bad packet size %d",
				ep_addr, packet_size);
			packet_size = 0;
			ep_size = 0;
			break;
		}

		switch (attributes & USB_ENDPOINT_XFERTYPE_MASK) {
		case USB_ENDPOINT_XFER_CONTROL:
		case USB_ENDPOINT_XFER_BULK:
		case USB_ENDPOINT_XFER_INT:
		default:
			/* A non-isochronous endpoint may optionally be
			 * double-buffered. For now we disable
			 * double-buffering.
			 */
			ep_doublebuffer = 0;
			if (packet_size > 1024)
				packet_size = 0;
			if (!ep || !ep_doublebuffer)
				buffer_size = packet_size;
			else
				buffer_size = packet_size * 2;
			break;
		case USB_ENDPOINT_XFER_ISOC:
			/* Isochronous endpoints are always double-
			 * buffered
			 */
			ep_doublebuffer = 1;
			buffer_size = packet_size * 2;
			break;
		}

		/* check to see if our packet buffer RAM is exhausted */
		if ((ep_ptr + buffer_size) > UDC_MAX_FIFO_SIZE) {
			printf("out of packet RAM for ep 0x%02x buf size %d",
				ep_addr, buffer_size);
			buffer_size = packet_size = 0;
		}
		MUSB_DEBUG("Configure EP %d - ep_ptr 0x%04x, packet size %d, buffer_size %d, attr %d\n", 
					ep, ep_ptr, packet_size, buffer_size, (attributes & USB_ENDPOINT_XFERTYPE_MASK));

		/* force a default configuration for endpoint 0 since it is
		 * always enabled
		 */
		if (!ep && ((packet_size < 8) || (packet_size > 64))) {
			buffer_size = packet_size = 64;
			ep_size = 3;
		}

		outb(ep & 0xF, UDC_INDEX);
		if ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) {
			/* IN endpoint */
			outb((ep_doublebuffer << 4) | (ep_size & 0xf),	UDC_TXFIFOSZ);
			outw(ep_ptr >> 3, UDC_TXFIFOADDR);
			if (!ep) {	/* This only apply for ep != 0 */
				outw(packet_size & 0x3FF, UDC_TXMAXP);
			}
		} else {
			/* OUT endpoint */
			outb((ep_doublebuffer << 4) | (ep_size & 0xf),
				UDC_RXFIFOSZ);
			outw(ep_ptr >> 3, UDC_RXFIFOADDR);
			if (!ep) {	/* This only apply for ep != 0 */
				outw(packet_size & 0x3FF, UDC_RXMAXP);
			}
		}
		ep_ptr += buffer_size;
	}
}

static musb_buff_xfer_status_t musb_ep0_tx(struct usb_endpoint_instance *endpoint)
{
	unsigned int size = 0;
	struct urb *urb = endpoint->tx_urb;
	musb_buff_xfer_status_t result = MUSB_BUFF_XFER_INVALID; 
	
	outb(0, UDC_INDEX);

	if (urb) {

		if ((size =
			MIN(urb->actual_length - endpoint->sent,
				endpoint->tx_packetSize))) {
			MUSB_DEBUG_EP0("EP0 TX buff %p, size %d, packet size: %d\n",(urb->buffer + endpoint->sent), size, endpoint->tx_packetSize);
			musb_fifo_write(0, urb->buffer + endpoint->sent, size);
		}
		endpoint->last = size;

		if (((endpoint->sent + size) == ep0_urb->device_request.wLength)
			|| (size < endpoint->tx_packetSize)) {
			/* Transmit packet and set data end */
			MUSB_DEBUG_EP0("%s: END EP0 data\n", __FUNCTION__);
			result = MUSB_BUFF_XFER_FULL;
		} else {
			MUSB_DEBUG_EP0("%s: More EP0 data\n", __FUNCTION__);
			result = MUSB_BUFF_XFER_PARTIAL;
		}
	}
	else
	{
		MUSB_DEBUG_EP0("No EP0 URB csr0: 0x%04x!!!\n", inw(UDC_CSR0));
		result = MUSB_BUFF_XFER_ERR;
	}

	return result;
}

static void musb_ep0_handler(struct usb_endpoint_instance *endpoint)
{
	u16 csr;
	u16 len;
	u16 ackpend = 0; 
	
	outb(0, UDC_INDEX);

	/* Check errors */
	csr = inw(UDC_CSR0);
	len =  inb(UDC_COUNT0);

	MUSB_DEBUG_EP0("Enter %s\n", decode_ep0stage(ep0stage));

#if 1
	if(ep0_urb->actual_length > ep0_urb->buffer_length)
		printf("ERROR EP0 len > BUFF LEN %s\n", decode_ep0stage(ep0stage));
#endif

	if (csr & MUSB_CSR0_P_SENTSTALL) {			/* Sent stall */
		outw(csr & ~MUSB_CSR0_P_SENTSTALL, UDC_CSR0);	/* Clear stall */
		ep0stage = MUSB_EP0_STAGE_IDLE;
		csr = inw(UDC_CSR0);
		
		MUSB_DEBUG_EP0("Stall received on EP0! %s\n", decode_ep0stage(ep0stage));
	}

	/* request ended "early" */
	if (csr & MUSB_CSR0_P_SETUPEND) {
		outw(MUSB_CSR0_P_SVDSETUPEND, UDC_CSR0);
		MUSB_DEBUG_EP0("Request ended early %s\n",decode_ep0stage(ep0stage));
		
		/* Transition into the early status phase */
		switch (ep0stage) {
		case MUSB_EP0_STAGE_TX:
			ep0stage = MUSB_EP0_STAGE_STATUSOUT;
			break;
		case MUSB_EP0_STAGE_RX:
			ep0stage = MUSB_EP0_STAGE_STATUSIN;
			break;
		default:
			ep0stage = MUSB_EP0_STAGE_IDLE;
			MUSB_DEBUG_EP0("SetupEnd came in a wrong ep0stage %s\n", decode_ep0stage(ep0stage));
		}
		csr = inw(UDC_CSR0);
		/* NOTE:  request may need completion */
	}

	/* docs from Mentor only describe tx, rx, and idle/setup states.
	 * we need to handle nuances around status stages, and also the
	 * case where status and setup stages come back-to-back ...
	 */
	switch (ep0stage) {

	case MUSB_EP0_STAGE_TX:
		MUSB_DEBUG_EP0("Enter STAGE_TX %s\n", decode_ep0stage(ep0stage));

		/* irq on clearing txpktrdy */
		if ((csr & MUSB_CSR0_TXPKTRDY) == 0) {
			musb_buff_xfer_status_t result;
			endpoint->sent += endpoint->last;
			/*
			 * If we finished sending data we would not
			 * be on the DATA_STAGE
			 */
			result = musb_ep0_tx(endpoint);
			if ( MUSB_BUFF_XFER_FULL == result)
			{
				outw(MUSB_CSR0_TXPKTRDY | MUSB_CSR0_P_DATAEND, UDC_CSR0);
				ep0stage = MUSB_EP0_STAGE_STATUSOUT;
			}
			else if ( MUSB_BUFF_XFER_PARTIAL == result)
			{
				outw(MUSB_CSR0_TXPKTRDY, UDC_CSR0);
			}
			else
			{
				outw(MUSB_CSR0_P_SENDSTALL, UDC_CSR0);
				ep0stage = MUSB_EP0_STAGE_IDLE;
			}
		}
		else
			MUSB_WARN_EP0("MUSB_CSR0_TXPKTRDY 0x%04x\n", csr);
		break;

	case MUSB_EP0_STAGE_RX:
		MUSB_DEBUG_EP0("Enter STAGE_RX %s, len %d\n", decode_ep0stage(ep0stage), len);

		/* irq on set rxpktrdy */
		if (csr & MUSB_CSR0_RXPKTRDY) {
			if (len) {
				if (ep0_urb->actual_length + len >	ep0_urb->device_request.wLength)
					len = ep0_urb->device_request.wLength - ep0_urb->actual_length;

				endpoint->last = len;

				musb_fifo_read(0, &ep0_urb->buffer[ep0_urb->actual_length], len);
				MUSB_DEBUG_EP0("Read RX FIFO total %d, len %d\n", ep0_urb->device_request.wLength, len);
				ep0_urb->actual_length += len;
			}

			/*
			 * We finish if we received the amount of data expected,
			 * or less of the packet size
			 */
			if ((ep0_urb->actual_length == ep0_urb->device_request.wLength) ||
									(endpoint->last < endpoint->tx_packetSize)) 
			{
				ep0stage = MUSB_EP0_STAGE_STATUSIN;
				
				MUSB_DEBUG_EP0("RX Req done actual = %d, len %d\n", ep0_urb->actual_length, len);

				/* This will process the incoming data */
				if (ep0_recv_setup(ep0_urb)) {
					/*
					 * Not a setup packet, stall next EP0
					 * transaction
					 */
					MUSB_DEBUG_EP0("RX STALL req !!!\n");
					ackpend = MUSB_CSR0_P_SENDSTALL;
					outw(MUSB_CSR0_P_SVDRXPKTRDY | MUSB_CSR0_P_SENDSTALL, UDC_CSR0);
				}
				else
				{
					ep0stage = MUSB_EP0_STAGE_STATUSIN;
					/* Clear RXPKTRDY and DATAEND */
					outw(MUSB_CSR0_P_SVDRXPKTRDY | MUSB_CSR0_P_DATAEND, UDC_CSR0);
				}
			} 
			else
			{
				MUSB_DEBUG_EP0("RX Wait Next Packet xfer left: %d\n", 
								ep0_urb->device_request.wLength - ep0_urb->actual_length);
				outw(MUSB_CSR0_P_SVDRXPKTRDY, UDC_CSR0);	/* Clear RXPKTRDY */
			}
		}
		else
			MUSB_WARN_EP0("MUSB_CSR0_RXPKTRDY 0x%04x\n", csr);
		break;

	case MUSB_EP0_STAGE_STATUSIN:
		MUSB_DEBUG_EP0("Enter STATUSIN %s\n", decode_ep0stage(ep0stage));

		/* end of sequence #2 (OUT/RX state) or #3 (no data) */

		/* update address (if needed) only @ the end of the
		 * status phase per usb spec, which also guarantees
		 * we get 10 msec to receive this irq... until this
		 * is done we won't see the next packet.
		 */
		if (do_set_address) {
			MUSB_DEBUG_EP0("Finish Device Set Address to %d\n", udc_device->address);
			do_set_address = 0;					   
			outb(udc_device->address, UDC_FADDR);
		}
  
		/* FALLTHROUGH */
	case MUSB_EP0_STAGE_STATUSOUT:
		MUSB_DEBUG_EP0("Enter STATUSOUT %s\n", decode_ep0stage(ep0stage));

		/* end of sequence #1: write to host (TX state) */
		/*
		 * In case when several interrupts can get coalesced,
		 * check to see if we've already received a SETUP packet...
		 */

		ep0stage = MUSB_EP0_STAGE_IDLE;
		
		if (!(csr & MUSB_CSR0_RXPKTRDY))
			break;

	case MUSB_EP0_STAGE_IDLE:
		MUSB_DEBUG_EP0("Enter IDLE %s\n", decode_ep0stage(ep0stage));

		/*
		 * This state is typically (but not always) indiscernible
		 * from the status states since the corresponding interrupts
		 * tend to happen within too little period of time (with only
		 * a zero-length packet in between) and so get coalesced...
		 */
		ep0stage = MUSB_EP0_STAGE_SETUP;
		/* FALLTHROUGH */

	case MUSB_EP0_STAGE_SETUP:
		MUSB_DEBUG_EP0("Enter SETUP %s\n", decode_ep0stage(ep0stage));

		if (csr & MUSB_CSR0_RXPKTRDY) {

			if (len != 8) 
			{
				MUSB_WARN_EP0("SETUP packet len %d != 8 ?\n", len);
				break;
			}
			
			insl(UDC_FIFO0,	(unsigned int *) &ep0_urb->device_request, 2);

			do_set_address = 0;
			ackpend = MUSB_CSR0_P_SVDRXPKTRDY;

			MUSB_DEBUG_EP0("EP0 Request -> bmRequestType: %d, bRequest: %d, wValue: %d, wIndex: %d, wLength: %d,\n", 
				ep0_urb->device_request.bmRequestType,
				ep0_urb->device_request.bRequest,
				ep0_urb->device_request.wValue,
				ep0_urb->device_request.wIndex,
				ep0_urb->device_request.wLength);

			if (ep0_urb->device_request.wLength == 0) 
			{
				MUSB_DEBUG_EP0("Zero request req %x\n", ep0_urb->device_request.bRequest);
				/* Try to process setup packet */
				if (ep0_recv_setup(ep0_urb)) 
				{
					/*
					 * Not a setup packet, stall
					 * next EP0 transaction
					 */
					MUSB_WARN_EP0("SETUP packet STALL !!!\n");
					ackpend |= MUSB_CSR0_P_SENDSTALL;
					ep0stage = MUSB_EP0_STAGE_IDLE;
				}
				else
				{
					ackpend |= MUSB_CSR0_P_DATAEND;
					ep0stage = MUSB_EP0_STAGE_STATUSIN;
					
					switch (ep0_urb->device_request.bRequest) 
					{
						case USB_REQ_SET_ADDRESS:
							MUSB_DEBUG_EP0("Device Set Address\n");
							usbd_device_event_irq(udc_device, DEVICE_ADDRESS_ASSIGNED, 0);
							do_set_address = 1;
						break;
						case USB_REQ_SET_CONFIGURATION:
							MUSB_DEBUG_EP0("Device Set Configuration\n");
							usbd_device_event_irq(udc_device, DEVICE_CONFIGURED, 0);
						break;
					}
				}
			}
			else if (ep0_urb->device_request.bmRequestType & USB_DIR_IN) 
			{
				MUSB_DEBUG_EP0("Data WRITE (TX) request size %d\n", ep0_urb->device_request.wLength);
				/* Try to process setup packet */
				if (ep0_recv_setup(ep0_urb)) 
				{
					/*
					 * Not a setup packet, stall
					 * next EP0 transaction
					 */
					MUSB_WARN_EP0("SETUP packet STALL !!!\n");
					ackpend |= MUSB_CSR0_P_SENDSTALL;
					ep0stage = MUSB_EP0_STAGE_IDLE;
				}
				else
				{
					//if ( (ep0_urb->actual_length + ep0_urb->device_request.wLength) > ep0_urb->buffer_length)
					if (ep0_urb->buffer_length < ep0_urb->device_request.wLength)
					{
						MUSB_WARN_EP0("ERROR READ request size too big - size %d\n",ep0_urb->device_request.wLength);
						ackpend = MUSB_CSR0_P_SENDSTALL;
						ep0stage = MUSB_EP0_STAGE_IDLE;
					}
					else
					{
						musb_buff_xfer_status_t result; 
						ep0stage = MUSB_EP0_STAGE_TX;
						outw(MUSB_CSR0_P_SVDRXPKTRDY, UDC_CSR0);
						#if 1
						while ((inw(UDC_CSR0) & MUSB_CSR0_RXPKTRDY) != 0)
							;
						#endif
						/*
						 * If we are sending data, do it now, as
						 * ep0_recv_setup should have prepare
						 * them
						 */
						endpoint->tx_urb = ep0_urb;
						endpoint->sent = 0;
						endpoint->last = 0;

						result = musb_ep0_tx(endpoint);
						if ( MUSB_BUFF_XFER_FULL == result)
						{
							ackpend = (MUSB_CSR0_TXPKTRDY | MUSB_CSR0_P_DATAEND);
							ep0stage = MUSB_EP0_STAGE_STATUSOUT;
						}
						else if ( MUSB_BUFF_XFER_PARTIAL == result)
						{
							ackpend = MUSB_CSR0_TXPKTRDY;
						}
						else
						{
							ackpend = MUSB_CSR0_P_SENDSTALL;
							ep0stage = MUSB_EP0_STAGE_IDLE;
						}
					}
				}
			} 
			else
			{
				MUSB_DEBUG_EP0("Req WRITE (RX ) requested = %d, current %d, max buffer size %d\n", 
					ep0_urb->device_request.wLength, ep0_urb->actual_length, ep0_urb->buffer_length);
				if (ep0_urb->buffer_length < ep0_urb->device_request.wLength)
				{
					MUSB_WARN_EP0("ERROR WRITE request size too big - size %d\n",ep0_urb->device_request.wLength);
					ackpend = MUSB_CSR0_P_SENDSTALL;
					ep0stage = MUSB_EP0_STAGE_IDLE;
				}
				else
				{
					ep0stage = MUSB_EP0_STAGE_RX;
					endpoint->rcv_urb = ep0_urb;
					ep0_urb->actual_length = 0;
					endpoint->last = 0;
				}
			}
			MUSB_DEBUG_EP0("ackpend %04x, ep0stage %s\n",	ackpend, decode_ep0stage(ep0stage));

			outw(ackpend, UDC_CSR0);
			ackpend = 0;
		}
		else
		{

			/* This must be our acknowlagment of the setup - no data to transfer */
		}

		break;

	case MUSB_EP0_STAGE_ACKWAIT:
		/* This should not happen. But happens with tusb6010 with
		 * g_file_storage and high speed. Do nothing.
		 */
		break;

	default:
		/* "can't happen" */
		MUSB_WARN_EP0("Incorect EP0 state handling !!!\n");
		outw(MUSB_CSR0_P_SENDSTALL, UDC_CSR0);
		ep0stage = MUSB_EP0_STAGE_IDLE;
		break;
	}

}

static void musb_ep_tx(struct usb_endpoint_instance *endpoint)
{
	unsigned int size = 0, epnumber =
		endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK;
	struct urb *urb = endpoint->tx_urb;

	outb(epnumber, UDC_INDEX);
	if (urb) {
		if ((size =
			MIN(urb->actual_length - endpoint->sent,
			endpoint->tx_packetSize))) {
			musb_fifo_write(epnumber, urb->buffer + endpoint->sent,
					size);
		}
		endpoint->last = size;
		endpoint->state = 1; /* Transmit hardware is busy */

		outw(inw(UDC_TXCSR) | 0x1, UDC_TXCSR); /* Transmit packet */
	}
}


static void musb_tx_handler(struct usb_endpoint_instance *endpoint)
{
	unsigned int epnumber =
		endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK;
	u16 txcsr;

	outb(epnumber, UDC_INDEX);

	/* Check errors */
	txcsr = inw(UDC_TXCSR);

	if (txcsr & 0x4) {		/* Clear underrun */
		txcsr &= ~0x4;
	}
	if (txcsr & 0x20) {		/* SENTSTALL */
		outw(txcsr & ~0x20, UDC_TXCSR);	/* Clear stall */
		return;
	}

	if (endpoint->tx_urb && !(txcsr & 0x1)) { /* The packet was send? */
		if ((endpoint->sent + endpoint->last == endpoint->tx_urb->
			actual_length)	/* Send a zero length packet? */
			&& (endpoint->last == endpoint->tx_packetSize)) {
			/* Prepare to transmit a zero-length packet. */
			endpoint->sent += endpoint->last;
			musb_ep_tx(endpoint);
		} else if (endpoint->tx_urb->actual_length) {
			/* retire the data that was just sent */
			usbd_tx_complete(endpoint);
			endpoint->state = 0; /* Transmit hardware is free */

			/*
			 * Check to see if we have more data ready to transmit
			 * now.
			 */
			if (endpoint->tx_urb && endpoint->tx_urb->
				actual_length) {
				musb_ep_tx(endpoint);
			}
		}
	}
}

static void musb_rx_handler(struct usb_endpoint_instance *endpoint)
{
	unsigned int epnumber =
		endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK;
	u16 rxcsr;
	u16 length;

	outb(epnumber, UDC_INDEX);

	/* Check errors */
	rxcsr = inw(UDC_RXCSR);

	if (!(rxcsr & 0x1))		/* There is a package received? */
		return;

	if (rxcsr & 0x40)		/* SENTSTALL */
		outw(rxcsr & ~0x40, UDC_RXCSR);	/* Clear stall */

	length = inw(UDC_RXCOUNT);

	if (endpoint->rcv_urb) {
		/* Receiving data */
		if (length) {
			musb_fifo_read(epnumber, &endpoint->rcv_urb->
				buffer[endpoint->rcv_urb->actual_length],
				length);
			outw(rxcsr & ~0x1, UDC_RXCSR);	/* Clear RXPKTRDY */
			usbd_rcv_complete(endpoint, length, 0);
		}
	} else {
		printf("%s: no receive URB!\n", __FUNCTION__);
	}
}

static void musb_reset(void)
{
	MUSB_DEBUG("MUSB: musb_reset()\n");
	
	usbd_device_event_irq(udc_device, DEVICE_HUB_CONFIGURED, 0);
	usbd_device_event_irq(udc_device, DEVICE_RESET, 0);
	ep0stage = MUSB_EP0_STAGE_IDLE;
	do_set_address = 0;
}

/* Public functions - called by usbdcore, usbtty, etc. */
void udc_irq(void)
{
	unsigned char int_usb = inb(UDC_INTRUSB);
	unsigned short int_tx = inw(UDC_INTRTX);
	unsigned short int_rx = inw(UDC_INTRRX);
//	unsigned char devctl = inb(UDC_DEVCTL);
//	unsigned char power = inb(UDC_POWER);
	int ep;

	MUSB_DEBUG("udc_irq: int_usb 0x%02x, int_tx 0x%04x, int_rx 0x%04x, devctrl 0x%02x, power 0x%02x\n", 
	int_usb, int_tx, int_rx, inb(UDC_DEVCTL), inb(UDC_POWER));
     
	if (int_usb) {
		if (int_usb & MUSB_INTR_RESET) {	/* Reset */
			/* printf("\nUSB IF CONNECTED/RESET\n"); */
			/* The controller clears FADDR, INDEX, and FIFOs */
			musb_reset();
		}
		if (int_usb & MUSB_INTR_DISCONNECT) {	/* Disconnected */
			/* printf("\nUSB IF DISCONNECTED\n"); */

			usbd_device_event_irq(udc_device, DEVICE_HUB_RESET, 0);
			//outb(MUSB_POWER_DEFAULT_VAL, UDC_POWER);
			//outb(0x00, UDC_DEVCTL);
		}
		if (int_usb & MUSB_INTR_SUSPEND)
		{
			/* printf("\nUSB IF SUSPEND\n"); */
			usbd_device_event_irq(udc_device,
					DEVICE_BUS_INACTIVE, 0);
		}
		if (int_usb & MUSB_INTR_RESUME)
		{
			/* printf("\nUSB IF RESUME\n"); */
			usbd_device_event_irq(udc_device,
					DEVICE_BUS_ACTIVITY, 0);
		}
	}

	/* Note: IRQ values auto clear so read just before processing */
	if (int_rx) {		/* OUT endpoints */
		ep = 1;
		int_rx >>= 1;
		while (int_rx) {
			if (int_rx & 1)
				musb_rx_handler(udc_device->bus->endpoint_array
						+ ep);
			int_rx >>= 1;
			ep++;
		}
	}
	if (int_tx) {		/* IN endpoints */
		if (int_tx & 1)
			musb_ep0_handler(udc_device->bus->endpoint_array);

		ep = 1;
		int_tx >>= 1;
		while (int_tx) {
			if (int_tx & 1)
				musb_tx_handler(udc_device->bus->endpoint_array
						+ ep);
			int_tx >>= 1;
			ep++;
		}
	}
}

/* Turn on the USB connection */
void udc_connect(void)
{
	MUSB_DEBUG("MUSB: udc_connect()\n");
	outb( MUSB_DEVCTL_SESSION, UDC_DEVCTL);

	if (!(inb(UDC_DEVCTL) & MUSB_DEVCTL_BDEVICE)) {
		printf("Error, the USB hardware is not on B mode\n");
		outb(0x00, UDC_DEVCTL);
		return;
	}
}

/* Turn off the USB connection */
void udc_disconnect(void)
{
	MUSB_DEBUG("MUSB: udc_disconnect()\n");
	if (!(inb(UDC_DEVCTL) & MUSB_DEVCTL_BDEVICE)) {
		printf("Error, the USB hardware is not on B mode");
		return;
	}

	outb(0x00, UDC_DEVCTL);
	outb(MUSB_POWER_DEFAULT_VAL, UDC_POWER);
}

int udc_endpoint_write(struct usb_endpoint_instance *endpoint)
{
	/* Transmit only if the hardware is available */
	if (endpoint->tx_urb && endpoint->state == 0)
		musb_ep_tx(endpoint);

	return 0;
}

/*
 * udc_setup_ep - setup endpoint
 *
 * Associate a physical endpoint with endpoint_instance
 */
void udc_setup_ep(struct usb_device_instance *device, unsigned int ep,
			struct usb_endpoint_instance *endpoint)
{
	int ep_addr;
	int attributes;

	/*
	 * We dont' have a way to identify if the endpoint definitions changed,
	 * so we have to always reconfigure the FIFOs to avoid problems
	 */
	if (1) //(hw_fifos_configured == 0)
	{
		musb_fifos_configure(device);
		hw_fifos_configured = 1;
	}

	ep_addr = endpoint->endpoint_address;
	if ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) {
		/* IN endpoint */
		attributes = endpoint->tx_attributes;
	} else {
		/* OUT endpoint */
		attributes = endpoint->rcv_attributes;
	}

	outb(ep & 0xF, UDC_INDEX);
	if ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) {
		/* IN endpoint */
		if (!ep) {		/* This only apply for ep != 0 */
			/* Empty fifo twice on case of previous double buffer */
			outw(1<<3, UDC_TXCSR);
			outw(1<<3, UDC_TXCSR);

			if (attributes & USB_ENDPOINT_XFER_ISOC)
				outw(inw(UDC_TXCSR) | (1 << 13) | (1 << 14) |
					0x4, UDC_TXCSR);
			else
				outw((inw(UDC_TXCSR) | (1 << 13) | 0x4) &
					~(1 << 14), UDC_TXCSR);
		}
		/* Enable interrupt */
		outw(inw(UDC_INTRTXE) | (1 << ep), UDC_INTRTXE);
	} else {
		/* OUT endpoint */
		if (!ep) {		/* This only apply for ep != 0 */
			if (attributes & USB_ENDPOINT_XFER_ISOC)
				outw((inw(UDC_RXCSR) | (1 << 14)) & ~(1 << 13),
					UDC_RXCSR);
			else
				outw(inw(UDC_RXCSR) & ~(1 << 14) & ~(1 << 13),
					UDC_RXCSR);
		}
		/* Enable interrupt */
		outw(inw(UDC_INTRRXE) | (1 << ep), UDC_INTRRXE);
	}
}

/*
 * udc_startup_events - allow udc code to do any additional startup
 */
void udc_startup_events(struct usb_device_instance *device)
{
	/* The DEVICE_INIT event puts the USB device in the state STATE_INIT. */
	usbd_device_event_irq(device, DEVICE_INIT, 0);

	/*
	 * The DEVICE_CREATE event puts the USB device in the state
	 * STATE_ATTACHED.
	 */
	usbd_device_event_irq(device, DEVICE_CREATE, 0);

	/*
	 * Some USB controller driver implementations signal
	 * DEVICE_HUB_CONFIGURED and DEVICE_RESET events here.
	 * DEVICE_HUB_CONFIGURED causes a transition to the state STATE_POWERED,
	 * and DEVICE_RESET causes a transition to the state STATE_DEFAULT.
	 * The MUSB client controller has the capability to detect when the
	 * USB cable is connected to a powered USB bus, so we will defer the
	 * DEVICE_HUB_CONFIGURED and DEVICE_RESET events until later.
	 */

	/* Save the device structure pointer */
	udc_device = device;

	/* Setup ep0 urb */
	if (!ep0_urb) {
		ep0_urb =
			usbd_alloc_urb(udc_device, udc_device->bus->
					endpoint_array);
	} else {
		printf("udc_enable: ep0_urb already allocated %p\n", ep0_urb);
	}

	/* Enable control interrupts */
	outb(MUSB_INTR_MASK, UDC_INTRUSBE);


}

void udc_set_nak(int epid)
{
/*
 * On MUSB the NAKing is controlled by the USB controller buffers,
 * so as long as we don't read data from the FIFO, the controller will NAK.
 * Nothing to see here, move along...
 */
}

void udc_unset_nak(int epid)
{
/*
 * On MUSB the NAKing is controlled by the USB controller buffers,
 * so as long as we don't read data from the FIFO, the controller will NAK.
 * Nothing to see here, move along...
 */
}

int udc_endpoint_feature(struct usb_device_instance *dev, int ep, int enable)
{
	struct usb_endpoint_instance *endpoint;
	struct urb *urb;
	unsigned int oldep;

	endpoint = &dev->bus->endpoint_array[ep];
	oldep = inb(UDC_INDEX);
	outb(ep, UDC_INDEX);

	if (endpoint->endpoint_address & USB_DIR_IN) {
		u32 txcsr = inw(UDC_TXCSR);
		if (enable && !(txcsr & MUSB_TXCSR_P_SENDSTALL)) {
			txcsr |= MUSB_TXCSR_P_SENDSTALL;
		} else if (!enable) {
			txcsr &= ~(MUSB_TXCSR_P_SENDSTALL | MUSB_TXCSR_P_SENTSTALL);
			txcsr |= MUSB_TXCSR_CLRDATATOG;
			urb = endpoint->tx_urb;
			if (urb)
				urb->actual_length = 0;
		} else
			goto out;

		/* double-buffer! */
		txcsr |= MUSB_TXCSR_FLUSHFIFO;
		outw(txcsr, UDC_TXCSR);
		outw(txcsr, UDC_TXCSR);
	} else {
		u32 rxcsr = inw(UDC_RXCSR);
		if (enable && !(rxcsr & MUSB_RXCSR_P_SENDSTALL)) {
			rxcsr |= MUSB_RXCSR_P_SENDSTALL;
		} else if (!enable) {
			rxcsr &= ~(MUSB_RXCSR_P_SENDSTALL | MUSB_RXCSR_P_SENTSTALL);
			rxcsr |= MUSB_RXCSR_CLRDATATOG;
			urb = endpoint->rcv_urb;
			if (urb)
				urb->actual_length = 0;
		} else
			goto out;

		/* double-buffer! */
		rxcsr |= MUSB_RXCSR_FLUSHFIFO;
		outw(rxcsr, UDC_RXCSR);
		outw(rxcsr, UDC_RXCSR);
	}

out:
	outb(oldep, UDC_INDEX);
	return 0;
}

/* Start to initialize h/w stuff */
int udc_init(void)
{
	/* Clock is initialized on the board code */

#ifdef CONFIG_USB_OMAP3
	/* MUSB soft-reset */
	outl(2, UDC_SYSCONFIG);
#endif

	/* MUSB end any previous session */
	outb(0x00, UDC_DEVCTL);

	if (udc_musb_platform_init()) {
		printf("udc_init: platform init failed\n");
		return -1;
	}

	/* MUSB end any previous session */
	outb(0x00, UDC_DEVCTL);
	outb(MUSB_POWER_DEFAULT_VAL, UDC_POWER);
	/* No VBUS PULSING */
	outb(0x00, MUSB_VPLEN);

#ifdef CONFIG_USB_OMAP3
	outl(inl(UDC_FORCESTDBY) & ~1, UDC_FORCESTDBY); /* disable MSTANDBY */
	outl(inl(UDC_SYSCONFIG) | (2<<12), UDC_SYSCONFIG); /* ena SMARTSTDBY */
	outl(inl(UDC_SYSCONFIG) & ~1, UDC_SYSCONFIG); /* disable AUTOIDLE */
	outl(inl(UDC_SYSCONFIG) | (2<<3), UDC_SYSCONFIG); /* enable SMARTIDLE */
	outl(inl(UDC_SYSCONFIG) | 1, UDC_SYSCONFIG); /* enable AUTOIDLE */

	/* Configure the PHY as PHY interface is 12-pin, 8-bit SDR ULPI */
	sr32((void *)UDC_INTERFSEL, 0, 1, 1);
#endif

	/* Turn off interrupts */
	outw(0x00, UDC_INTRTXE);
	outw(0x00, UDC_INTRRXE);

#if CONFIG_MUSB_FULL_SPEED
	/*
	 * Use Full speed for debugging proposes, useful so most USB
	 * analyzers can catch the transactions
	 */
	outb(MUSB_POWER_DEFAULT_VAL, UDC_POWER);
	printf("MUSB: using full speed\n");
#else
	printf("MUSB: using high speed\n");
#endif
	
	hw_fifos_configured = 0;
	
	return 0;
}
