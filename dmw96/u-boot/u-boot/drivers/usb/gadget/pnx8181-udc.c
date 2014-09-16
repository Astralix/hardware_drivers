/*
 *  drivers/usb/pnx8181_udc.c - driver for pnx8181 USB device controller
 *
 *  Copyright (C) 2009, DSPG Technologies GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#if defined(CONFIG_FIRETUX) && defined(CONFIG_USB_DEVICE)
#include <common.h>

#define PNX8181_USB_INTSTAT	0xC2009000
#define PNX8181_USB_INTEN	0xC2009004
#define PNX8181_USB_INTCLR	0xC2009008
#define PNX8181_USB_INTSET	0xC200900C
#define PNX8181_USB_CMDCODE	0xC2009010
#define PNX8181_USB_CMDDATA	0xC2009014
#define PNX8181_USB_RXDATA	0xC2009018
#define PNX8181_USB_TXDATA	0xC200901C
#define PNX8181_USB_RXPCKL	0xC2009020
#define PNX8181_USB_TXPCKL	0xC2009024
#define PNX8181_USB_CON		0xC2009028

#define PNX8181_INTC_REQUEST	0xC1100400
#define PNX8181_INTC_VECTOR_IRQ	0xC1100100

#define PNX8181_CGU_GATESC	0xC2200008

#define PNX8181_EXTINT_SIGNAL	0xC2105070

#define REQ_DISABLE		(1<<26)
#define REQ_ENABLE		((1<<26)|(1<<16))
#define REQ_DEFAULT		((1<<28)|0x1)
#define REQ_ACTIVE_HIGH		(1<<25)
#define REQ_ACTIVE_LOW		((1<<25)|(1<<17))

#define UDC_USB_IRQ		13
#define UDC_VBUS_IRQ		1

#ifndef UDC_DEBUG_LEVEL
#define UDC_DEBUG_LEVEL 1
#endif

#undef UDC_DEBUG
//#define UDC_DEBUG
#ifdef UDC_DEBUG
#define udc_debug(level,fmt,args...) do { \
	if (UDC_DEBUG_LEVEL >= level) \
		printf(fmt,##args); \
	} while(0)
#else
#define udc_debug(level,fmt,args...) do { } while(0)
#endif

DECLARE_GLOBAL_DATA_PTR;

#include <asm/io.h>

#include <usb_dfu.h>
#include <usbdevice.h>
#include <usbdcore_dspg.h>
#include "ep0.h"
#include <usb_cdc_acm.h>
#include "pnx8181-udc.h"

static const char pnx8181udc_name[] = "pnx8181_udc";
static const char ep0name[] = "ep0";

static struct urb *ep0_urb = NULL;

struct usb_device_instance *udc_device; /* Used in interrupt handler */

#define DRIVER_VERSION "Juli 2009"
#define NUM_ENDPOINTS 7

//#define DEBUG 1
//#ifndef DEBUG
//#define printf(args...) do {} while(0)
//#endif

struct pnx8181udc_ep {
	struct usb_endpoint_instance *ep;
	unsigned index;
	unsigned int maxpacket;
	int is_in;
	int double_ack;
};

struct pnx8181udc {
	struct pnx8181udc_ep udc_ep[NUM_ENDPOINTS];
	int irq;
	int irqvbus;
	u32 pending;
	unsigned suspended;
	int req_pending;
	int has_vbus;
	int connected;
};

static struct pnx8181udc controller = {
	.udc_ep[0] = {
		.index = 0,
		.maxpacket = 8,
	},
	.udc_ep[1] = { // out
		.index = 1,
		.maxpacket = 16,
	},
	.udc_ep[2] = { // in
		.index = 1,
		.maxpacket = 16,
	},
	.udc_ep[3] = { // out
		.index = 2,
		.maxpacket = 16,
	},
	.udc_ep[4] = { // in
		.index = 2,
		.maxpacket = 16,
	},
	.udc_ep[5] = { // out
		.index = 3,
		.maxpacket = 64,
	},
	.udc_ep[6] = { // in
		.index = 3,
		.maxpacket = 64,
	},
};

#define UDC_CMD(cmd)          ((cmd) << 16)
#define CMD_SETADDR           UDC_CMD(0xD0)
#define CMD_CONFIG            UDC_CMD(0xD8)
#define CMD_SETMODE           UDC_CMD(0xF3)
#define CMD_CURFRAME          UDC_CMD(0xF5)
#define CMD_READID            UDC_CMD(0xFD)
#define CMD_GETSTATUS         UDC_CMD(0xFE)
#define CMD_SETSTATUS         UDC_CMD(0xFE)
#define CMD_GETERR            UDC_CMD(0xFF)
#define CMD_ERRSTATUS         UDC_CMD(0xFB)
#define CMD_GETINT            UDC_CMD(0xF4)
#define CMD_EPSELECT(ep)      UDC_CMD(0x00 + (ep))
#define CMD_EPSELECTCLR(ep)   UDC_CMD(0x40 + (ep))
#define CMD_EPSETSTATUS(ep)   UDC_CMD(0x40 + (ep))
#define CMD_EPCLRBUF          UDC_CMD(0xF2)
#define CMD_EPVALBUF          UDC_CMD(0xFA)

#define CMD_PHASE_WRITE       0x0100
#define CMD_PHASE_READ        0x0200
#define CMD_PHASE_CMD         0x0500

#define INT_FRAME             (1 <<  0)
#define INT_ENDPOINT(ep)      (1 << (1 + (ep))) /* NOTE! physical ep */
#define INT_DEVSTAT           (1 <<  9)
#define INT_CMDCODE_EMPTY     (1 << 10)
#define INT_CMDDATA_FULL      (1 << 11)
#define INT_ENDPCK_OUT        (1 << 12)
#define INT_ENDPCK_IN         (1 << 13)

#define STATUS_CONNECT        0x01
#define STATUS_CONNECT_C      0x02
#define STATUS_SUSPEND        0x04
#define STATUS_SUSPEND_C      0x08
#define STATUS_BUS_RESET      0x10

#define EPSTATUS_FULLEMPTY    0x01
#define EPSTATUS_STALL        0x02
#define EPSTATUS_SETUP        0x04
#define EPSTATUS_OVERWRITTEN  0x08
#define EPSTATUS_NAKED        0x10
#define EPSTATUS_B1FULL       0x20
#define EPSTATUS_B2FULL       0x40

#define SETEPSTATUS_STALL     0x01
#define SETEPSTATUS_ENABLE    0x00
#define SETEPSTATUS_DISABLE   0x20
#define SETEPSTATUS_RATE      0x40
#define SETEPSTATUS_CONDSTALL 0x80

#define CON_READ_ENABLE       0x01
#define CON_WRITE_ENABLE      0x02
#define CON_LOGEP(ep)         (((ep) & 0xf) << 2)

#define RXPCKL_VALID          0x0400
#define RXPCKL_LEN            0x03FF

#define MODE_INTON_NAK_CI     0x02

/*
 * hardware
 */

/*
 * FIXME: ugly, use some proper way to detect if extint signal is
 *        high or low (gpio_get_value() works with internal signal?)
 */
static int
pnx8181udc_has_vbus(void)
{
#define EXTINT_13 0x00002000
	u32 signal = readl(PNX8181_EXTINT_SIGNAL);
	return !!(signal & EXTINT_13);
}

static inline void
wait_cmdcode_written(void)
{
	while ((readl(PNX8181_USB_INTSTAT) & INT_CMDCODE_EMPTY) == 0);
}

static inline void
wait_cmddata_ready(void)
{
	while ((readl(PNX8181_USB_INTSTAT) & INT_CMDDATA_FULL) == 0);
}

static void
pnx8181udc_enable_int(unsigned short interrupt)
{
	u32 ier = readl(PNX8181_USB_INTEN);

	//printf("%s()\n", __func__);

	ier |= interrupt;
	writel(ier, PNX8181_USB_INTEN);
}

static void
pnx8181udc_disable_int(unsigned short interrupt)
{
	u32 ier = readl(PNX8181_USB_INTEN);

	ier &= ~interrupt;
	writel(ier, PNX8181_USB_INTEN);
}

static void
pnx8181udc_cmd_read(unsigned cmd, unsigned char *buf, int count)
{
	//printf("%s(): cmd = 0x%04X\n", __func__, cmd);

	writel(cmd | CMD_PHASE_CMD, PNX8181_USB_CMDCODE);
	wait_cmdcode_written();

	while (count) {
		writel(cmd | CMD_PHASE_READ, PNX8181_USB_CMDCODE);
		wait_cmddata_ready();

		*buf++ = readb(PNX8181_USB_CMDDATA);
		count--;
	}
}

static void
pnx8181udc_cmd_write(unsigned cmd, unsigned char *buf, int count)
{
	//printf("%s(): cmd = 0x%04X\n", __func__, cmd);

	writel(cmd | CMD_PHASE_CMD, PNX8181_USB_CMDCODE);
	wait_cmdcode_written();

	while (count) {
		writel((*buf) << 16 | CMD_PHASE_WRITE, PNX8181_USB_CMDCODE);
		wait_cmdcode_written();

		buf++;
		count--;
	}
}

static void
pnx8181udc_set_addr(u8 addr)
{
	/* this command also enables the usb handler */
	addr |= 0x80;

	pnx8181udc_cmd_write(CMD_SETADDR, &addr, sizeof(addr));
	//printf("%s(): addr = %d\n", __func__, addr & 0x7f);
}

static void
pnx8181udc_connect(struct pnx8181udc *udc)
{
	u8 status;

	pnx8181udc_cmd_read(CMD_GETSTATUS, &status, sizeof(status));
	status |= STATUS_CONNECT;
	pnx8181udc_cmd_write(CMD_SETSTATUS, &status, sizeof(status));
}

static void
pnx8181udc_disconnect(struct pnx8181udc *udc)
{
	unsigned char status;
	//printf("%s()\n", __func__);

	pnx8181udc_cmd_read(CMD_GETSTATUS, &status, sizeof(status));
	status &= ~STATUS_CONNECT;
	pnx8181udc_cmd_write(CMD_SETSTATUS, &status, sizeof(status));
}

static void
pnx8181udc_stall_ep0(void)
{
	u8 status;

	status = SETEPSTATUS_CONDSTALL;
	pnx8181udc_cmd_write(CMD_EPSETSTATUS(0), &status, 1);

	status = SETEPSTATUS_STALL;
	pnx8181udc_cmd_write(CMD_EPSETSTATUS(1), &status, 1);
}

/*
 * request implementation
 */

static int
pnx8181udc_read_fifo(struct pnx8181udc_ep *udc_ep)
{
	struct usb_endpoint_instance *endpoint = udc_ep->ep;
	struct urb *urb;
	unsigned phys_ep = udc_ep->index * 2;
	u8 *data;
	unsigned space;
	int to_read, i, done;
	u8 status;
	u16 rxlen;

	urb = endpoint->rcv_urb;
	space = urb->buffer_length - urb->actual_length;
	data = urb->buffer + urb->actual_length;

	/*
	 * we might have been called by ep_queue() and haven't received any
	 * data yet
	 */
#if 0
	if ((readl(PNX8181_USB_INTSTAT) & INT_ENDPOINT(phys_ep)) == 0)
		return 0;
#endif

	pnx8181udc_cmd_read(CMD_EPSELECT(phys_ep), &status, 1);
	if ((status & EPSTATUS_FULLEMPTY) == 0) {
		udc_debug(1, "READ: FULLEMPTY\n");
		return 0;
	}

	pnx8181udc_cmd_read(CMD_EPSELECTCLR(phys_ep), &status, 1);

	/* FIXME: handle stall here? */
	if (status & 0x02) {
		//printf("%s(): pep%d stalled\n", __func__, phys_ep);
		// FIXME: stall in pep
		udc_debug(1, "STALL in READ\n");
		return 0;
	}

	writel(CON_LOGEP(udc_ep->index) | CON_READ_ENABLE, PNX8181_USB_CON);
	udelay(10);

	rxlen = readw(PNX8181_USB_RXPCKL);
	if ((rxlen & RXPCKL_VALID) == 0) {
		pnx8181udc_cmd_write(CMD_EPCLRBUF, NULL, 0);
		udc_debug(1, "%s(): received invalid bytes\n", __func__);
		return 0; /* FIXME: return 0; */
	}

	to_read = rxlen & RXPCKL_LEN;
	if (to_read > udc_ep->maxpacket)
		to_read = udc_ep->maxpacket;

	if (to_read > space) {
		/* printf("%s(endpoint %d): buffer overflow\n", __func__,
		   udc_ep->index); */
		//req->req.status = -EOVERFLOW;
		to_read = space;
	}

	udc_debug(1, "READ FIFO: len=%d space=%d max=%d actual=%d\n", to_read,
	          space, udc_ep->maxpacket, urb->actual_length);

	for (i = 0; i < to_read; i += 4) {
		u32 tmp = readl(PNX8181_USB_RXDATA);

		data[i+0] = (tmp & 0x000000ff);
		data[i+1] = (tmp & 0x0000ff00) >> 8;
		data[i+2] = (tmp & 0x00ff0000) >> 16;
		data[i+3] = (tmp & 0xff000000) >> 24;
	}

	udelay(1);
	pnx8181udc_cmd_write(CMD_EPCLRBUF, NULL, 0);
	writel(CON_LOGEP(udc_ep->index), PNX8181_USB_CON);

	urb->actual_length += to_read;

	done = (to_read < udc_ep->maxpacket) || (to_read == space);

	/* if (to_read < space) space = to_read;
	   if (done) usbd_rcv_complete(endpoint, urb_avail, 0);*/

	return done;
}

static int
pnx8181udc_write_fifo(struct pnx8181udc_ep *udc_ep)
{
	unsigned int last, i, to_write;
	struct urb *urb;
	struct usb_endpoint_instance *endpoint = udc_ep->ep;
	unsigned phys_ep = (udc_ep->index * 2) + 1;
	u8 *data;
	u8 status;
	//int done;

	urb = endpoint->tx_urb;
	last = urb->buffer_length + urb->actual_length;

	if (last > udc_ep->maxpacket)
		last = udc_ep->maxpacket;
	if (last > (urb->actual_length - endpoint->sent))
		last = urb->actual_length - endpoint->sent;
	to_write = (last + 3)/4;

	endpoint->last = last;

	data = urb->buffer + endpoint->sent;

	udc_debug(1, "WRITE FIFO (off=%d, len=%d): ", endpoint->sent, last);
	for (i = 0; i < last; i++)
		udc_debug(1, "0x%x ", urb->buffer[endpoint->sent+i]);
	udc_debug(1, "\n");

	udc_debug(2, "%s(): len = %d, act = %d, sending = %d\n",
	          __func__, urb->buffer_length, urb->actual_length, to_write);

	/*
	 * if called from ep_queue() make sure the last packet is gone before
	 * proceeding with this request
	 */
	pnx8181udc_cmd_read(CMD_EPSELECTCLR(phys_ep), &status, 1);
	if (status & EPSTATUS_FULLEMPTY) {
		udc_debug(1, "FULLEMPTY\n");
		return 0;
	}

	/* select endpoint and set length of packet */
	writel(CON_LOGEP(udc_ep->index) | CON_WRITE_ENABLE, PNX8181_USB_CON);
	writel(last, PNX8181_USB_TXPCKL);

	/*
	 * write data
	 *
	 * XXX: we may go over buf[]'s boundaries here if len is not
	 * aligned to a multiple of 4. probably not an issue since we need the
	 * dummy bytes anyway and linux always allocs multiple of 4 (so we
	 * do not get aborts).
	 */
	for (i = 0; i < last; i+= 4) {
		u32 tmp;

		tmp = (data[i+0]) |
		      (data[i+1] << 8) |
		      (data[i+2] << 16) |
		      (data[i+3] << 24);

		writel(tmp, PNX8181_USB_TXDATA);
	}

	/* if we send a ZLP, no data has been written yet, so write dummy */
	if (last == 0)
		writel(0, PNX8181_USB_TXDATA);

	/* toggle sending to host */
	pnx8181udc_cmd_write(CMD_EPVALBUF, NULL, 0);

	if (endpoint->sent + last < urb->actual_length) {
		/* not all data has been transmitted so far */
		udc_debug(1, "WRITE: NOT YET! sent=%d, last=%d, "
		          "actual_len=%d\n", endpoint->sent, last,
		          urb->actual_length);
		return 0;
	}

	if (last == endpoint->tx_packetSize) {
		/* we need to send one more packet (ZLP) */
		udc_debug(1, "WRITE: ZLP! len=%d\n", last);
		return 0;
	}

	// urb->actual_length += last; /* handled in usbdfu.c */

	udc_debug(1, "WRITE: DONE! len=%d\n", last);

	return 1;

}

/*
 * endpoint implementation
 */

static void
pnx8181udc_send_ack(void)
{
	u8 status;

	do {
		pnx8181udc_cmd_read(CMD_EPSELECTCLR(1), &status, 1);
	} while (status & EPSTATUS_FULLEMPTY);

	writel(CON_LOGEP(0) | CON_WRITE_ENABLE, PNX8181_USB_CON);
	writel(0, PNX8181_USB_TXPCKL);
	writel(0, PNX8181_USB_TXDATA);
	writel(CON_LOGEP(0), PNX8181_USB_CON);

	pnx8181udc_cmd_read(CMD_EPSELECTCLR(1), &status, 1);
	pnx8181udc_cmd_write(CMD_EPVALBUF, NULL, 0);

	//printf("%s()\n", __func__);
}

/*
 * udc_setup_ep - setup endpoint
 *
 * Associate a physical endpoint with endpoint_instance
 */
int udc_setup_ep(struct usb_device_instance *device,
                 unsigned int ep, struct usb_endpoint_instance *endpoint)
{
	struct pnx8181udc_ep *udc_ep;
	int ep_addr = endpoint->endpoint_address;
	int maxpacket;

	unsigned long flags;
	unsigned pep_out, pep_in;
	u8 status;

	if (ep >= NUM_ENDPOINTS)
		return -1;

	udc_ep = &controller.udc_ep[ep];

	if ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) {
		maxpacket = endpoint->tx_packetSize;
	} else {
		maxpacket = endpoint->rcv_packetSize;
	}

	switch (maxpacket) {
	case 8:
	case 16:
	case 32:
	case 64:
		break;
	default:
		printf("%s(): maxpacket not supported\n", __func__);
		return -1;
	}

	pep_out = (udc_ep->index * 2);
	pep_in  = (udc_ep->index * 2) + 1;

	local_irq_save(flags);

	udc_ep->is_in = ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN);
	udc_ep->ep = endpoint;
	//udc_ep->ep.maxpacket = maxpacket;
	/* TODO: tx_packetSize / rcv_packetSize */
	udc_debug(1, "Setting up EP %d (in=%d)\n", ep, udc_ep->is_in);

	/* enable physical endpoints and their interrupts */
	if (udc_ep->is_in)
		pnx8181udc_enable_int(INT_ENDPOINT(pep_in));
	else
		pnx8181udc_enable_int(INT_ENDPOINT(pep_out));

	status = SETEPSTATUS_ENABLE;
	if (udc_ep->is_in)
		pnx8181udc_cmd_write(CMD_EPSETSTATUS(pep_in), &status, 1);
	else
		pnx8181udc_cmd_write(CMD_EPSETSTATUS(pep_out), &status, 1);

	local_irq_restore(flags);

	return 0;
}

struct usb_ctrlrequest {
	__u8 bRequestType;
	__u8 bRequest;
	__le16 wValue;
	__le16 wIndex;
	__le16 wLength;
} __attribute__ ((packed));

union setup {
	u32 raw[2];
	struct usb_ctrlrequest r;
};

static void pnx8181udc_handle_setup(struct pnx8181udc *udc,
                                    struct pnx8181udc_ep *udc_ep0)
{
	struct usb_endpoint_instance *ep0 = udc_ep0->ep;
	union setup *pkt = (union setup *)&ep0_urb->device_request;
	unsigned short reg;

	udc_debug(1, "HANDLE SETUP\n");

	writel(CON_LOGEP(0) | CON_READ_ENABLE, PNX8181_USB_CON);
	udelay(1);

	reg = readl(PNX8181_USB_RXPCKL);
	if ((reg & RXPCKL_VALID) == 0) {
		/* received bytes invalid; clear fifo & return */
		pnx8181udc_cmd_write(CMD_EPCLRBUF, NULL, 0);
		printf("%s(): received invalid bytes\n", __func__);
		return;
	}

	if ((reg & RXPCKL_LEN) != 8) {
		/* no setup packet, but expected one; clear fifo & return */
		pnx8181udc_cmd_write(CMD_EPCLRBUF, NULL, 0);
		printf("%s(): setup packet expected\n", __func__);
		return;
	}

	pkt->raw[0] = readl(PNX8181_USB_RXDATA);
	udelay(100); /* FIXME */
	pkt->raw[1] = readl(PNX8181_USB_RXDATA);

	udelay(1);
	pnx8181udc_cmd_write(CMD_EPCLRBUF, NULL, 0);

	/* clear read enable, FIXME: needed? */
	writel(CON_LOGEP(0), PNX8181_USB_CON);

	/*
	 * process setup requests that touch hardware
	 */
#define w_index  le16_to_cpu(pkt->r.wIndex)
#define w_value  le16_to_cpu(pkt->r.wValue)
#define w_length le16_to_cpu(pkt->r.wLength)

	udc_debug(1, "setup: 0x%x 0x%x - %02x.%02x v%04x i%04x l%04x\n",
	//printf("%s(): setup: %02x.%02x v%04x i%04x l%04x\n", __func__,
	       pkt->raw[0], pkt->raw[1], pkt->r.bRequestType, pkt->r.bRequest,
	       w_value, w_index, w_length);

	if (pkt->r.bRequestType & USB_DIR_IN) {
		udc_debug(1, "IN PHASE\n");
		udc_ep0->is_in = 1;
	} else {
		udc_debug(1, "OUT PHASE\n");
		udc_ep0->is_in = 0;
	}

	udc->req_pending = 1;

	if (w_length == 0) {
		debug("LENGTH == 0\n");
		if (ep0_recv_setup(ep0_urb)) {
			/* Not a setup packet, stall next EP0 transaction */
			debug("can't parse setup packet1\n");
			udc_debug(1, "%s(): req %02x.%02x protocol STALL\n",
			       __func__, pkt->r.bRequestType, pkt->r.bRequest);
			/* force stall */
			pnx8181udc_stall_ep0();
			udc->req_pending = 0;
			return;
		}

		switch ((pkt->r.bRequestType << 8) | pkt->r.bRequest) {

		/* requests that reset data toggle to DATA0 cause problems
		 * with this controller, so treat them in a special way */
		case ((USB_TYPE_STANDARD|USB_RECIP_DEVICE) << 8)
				| USB_REQ_SET_CONFIGURATION:
			/* SET_INTERFACE request are handled by DFU, not here! */

			if (! w_value)
				usbd_device_event_irq(udc_device,
				                      DEVICE_DE_CONFIGURED, 0);
			else
				usbd_device_event_irq(udc_device,
				                      DEVICE_CONFIGURED, 0);

			/* we will get a ZLP queued, make sure it is sent */
			//udc_ep0->is_in = 1;
			/* also, send the ack twice, because first one will
			 * have the wrong data toggle due to controller
			 * stupidity */
			udc_ep0->double_ack = 1;
			udc_debug(1, "SET CONFIGURATION\n");

			ep0_urb->buffer = ep0_urb->buffer_data;
			ep0_urb->buffer_length = 0;
			ep0_urb->actual_length = 0;

			ep0->tx_urb = ep0_urb;
			ep0->rcv_urb = ep0_urb;

			/* Send ZLP */
			pnx8181udc_write_fifo(udc_ep0);

			break;

		/* device */
		case ((USB_TYPE_STANDARD|USB_RECIP_DEVICE) << 8)
				| USB_REQ_SET_ADDRESS:
			/* we send the ack directly to make sure we don't
			 * timeout */
			pnx8181udc_send_ack();
			pnx8181udc_set_addr(w_value);
			usbd_device_event_irq(udc_device,
			                      DEVICE_ADDRESS_ASSIGNED, 0);
			udc_debug(1, "SET ADDRESS\n");
			return;

		default:
			udc_debug(1, "DEFAULT !!!\n");
			ep0_urb->buffer = ep0_urb->buffer_data;
			ep0_urb->buffer_length = sizeof(ep0_urb->buffer_data);
			ep0_urb->actual_length = 0;

			ep0->tx_urb = ep0_urb;
			ep0->rcv_urb = ep0_urb;

			ep0->sent = ep0->last = 0;

			pnx8181udc_write_fifo(udc_ep0);

			if (pkt->r.bRequest == USB_REQ_SET_INTERFACE) {
				udc_debug(1, "Send ACK!\n");
				if (udc_ep0->double_ack) {
					udc_ep0->double_ack = 0;
					pnx8181udc_send_ack();
				}
			}

			break;
		}

	} else {
		ep0_urb->buffer = ep0_urb->buffer_data;
		ep0_urb->buffer_length = sizeof(ep0_urb->buffer_data);
		ep0_urb->actual_length = 0;

		ep0->tx_urb = ep0_urb;
		ep0->rcv_urb = ep0_urb;

		if ((pkt->r.bRequestType & USB_REQ_DIRECTION_MASK) ==
		                                        USB_REQ_DEVICE2HOST) {
			udc_debug(1, "DEVICE TO HOST: WRITING DATA\n");

			if (ep0_recv_setup(ep0_urb)) {
				/* Not a setup packet, stall next EP0
				 * transaction */
				debug("can't parse setup packet2\n");
				udc_debug(1, "%s(): req %02x.%02x protocol "
				          "STALL\n", __func__,
				          pkt->r.bRequestType, pkt->r.bRequest);
				/* force stall */
				pnx8181udc_stall_ep0();
				udc->req_pending = 0;
				return;
			}
			ep0->sent = ep0->last = 0;

			pnx8181udc_write_fifo(udc_ep0);
		}
	}

#undef w_length
#undef w_value
#undef w_index

	if (pkt->r.bRequest == USB_REQ_SET_CONFIGURATION) {
		u8 reg = 0x01;
		pnx8181udc_cmd_write(CMD_CONFIG, &reg, 1);
		udc_debug(1, "SET CONFIGURATION - CMD_CONFIG\n");
	}
}

static void
pnx8181udc_handle_ep(int i, struct pnx8181udc_ep *udc_ep)
{
	struct usb_endpoint_instance *ep = udc_ep->ep;

	if (ep->tx_urb && udc_ep->is_in)
		pnx8181udc_write_fifo(udc_ep);
	else if (ep->rcv_urb && !udc_ep->is_in)
		pnx8181udc_read_fifo(udc_ep);
}

static void
pnx8181udc_handle_ep0(void)
{
	struct pnx8181udc *udc = &controller;
	struct pnx8181udc_ep *udc_ep0 = &udc->udc_ep[0];
	struct usb_endpoint_instance *ep0 = udc_ep0->ep;
	unsigned char stat;

	/* udc_debug(1, "HANDLE EP0 0x%x 0x%x 0x%x\n", udc->pending,
	 *           INT_ENDPOINT(0), INT_ENDPOINT(1)); */

	/* check for OUT physical endpoint */
	if (udc->pending & INT_ENDPOINT(0)) {
		pnx8181udc_cmd_read(CMD_EPSELECTCLR(0), &stat, sizeof(stat));

		if (stat & EPSTATUS_STALL) {
			//printf("%s(): pep0 stalled\n", __func__);
			/* TODO: stall pep1 */
			udc_debug(1, "STALL\n");
			return;
		}

		if (stat & EPSTATUS_SETUP) {
			udc_debug(1, "IDLE MODE - CALLING SETUP\n");
			pnx8181udc_handle_setup(udc, udc_ep0);
			return;
		}

		if (!udc_ep0->is_in)
			pnx8181udc_read_fifo(udc_ep0);

		//if (fifo_count < ep0->rcv_packetSize ||
		if (ep0_urb->actual_length >= ep0_urb->device_request.wLength) {
			udc_debug(1, "NEW REQUEST?\n");
			if (ep0_recv_setup(ep0_urb)) {
				/* Not a setup packet, stall next EP0
				 * transaction */
				debug("can't parse setup packet3\n");
				return;
			}
			ep0->sent = ep0->last = 0;

			pnx8181udc_write_fifo(udc_ep0);

		}

		/* handle next request if there is one */
		/*if ((ep0_urb->device_request.wLength != 0) && !udc_ep0->is_in)
			pnx8181udc_handle_ep(0, ep0);*/
	}

	/* check for IN physical endpoint */
	if (udc->pending & INT_ENDPOINT(1)) {
		pnx8181udc_cmd_read(CMD_EPSELECTCLR(1), &stat, sizeof(stat));

		udc_debug(1, "EP0 - IN(1): stat=0x%x state=%d\n", stat, ep0->state);

		if (stat & EPSTATUS_STALL) {
			//printf("%s(): pep1 stalled, clr = %02x\n", __func__,
			//       stat);
			udc_debug(1, "STALL\n");
			return;
		}

		/*if ((ep0_urb->device_request.wLength != 0) && udc_ep0->is_in)
			pnx8181udc_handle_ep(0, ep0);*/

		if (udc_ep0->is_in) {
			ep0->sent += ep0->last;
			if (ep0_urb->actual_length > 0) {
				udc_debug(1, "WRITE: DATA! sent=%d, last=%d, "
				          "actual_len=%d\n", ep0->sent,
				          ep0->last, ep0_urb->actual_length);
				pnx8181udc_write_fifo(udc_ep0);
			} else udc_debug(1, "NO DATA???\n");
		} else udc_debug(1, "NOT IN???\n");

		if (udc_ep0->double_ack) {
			udc_ep0->double_ack = 0;
			pnx8181udc_send_ack();
		}
	}
}

/*
 * gadget implementation
 */

static void
pnx8181udc_stop(struct pnx8181udc *udc)
{
	int i;

	udc->suspended = 0;

	for (i = 0; i < NUM_ENDPOINTS; i++) {
		struct pnx8181udc_ep *ep = &udc->udc_ep[i];
		unsigned pep_out = (ep->index * 2);
		unsigned pep_in  = (ep->index * 2) + 1;

		if (ep->is_in)
			pnx8181udc_disable_int(INT_ENDPOINT(pep_in));
		else
			pnx8181udc_disable_int(INT_ENDPOINT(pep_out));
	}

	pnx8181udc_disconnect(udc);
}

#define to_pnx8181udc(g) \
	container_of((g), struct pnx8181udc, gadget)

static int
pnx8181udc_vbus_session(int is_active)
{
	struct pnx8181udc *udc = &controller;
	unsigned long flags;

	local_irq_save(flags);

	if (is_active) {
		/* make sure both interrupt sources are enabled */
		pnx8181udc_enable_int(INT_ENDPOINT(0));
		pnx8181udc_enable_int(INT_ENDPOINT(1));

		pnx8181udc_connect(udc);
		udc->connected = 1;
	} else {
		udc->connected = 0;
		pnx8181udc_disconnect(udc);
		pnx8181udc_stop(udc);
	}

	local_irq_restore(flags);

	return 0;
}

/*
 * driver interface
 */

#define PNX8181_SYSMUX1			0xC2204010
#define PNX8181_SYSMUX4			0xC220401C

#define PNX8181_INTC_PRIOMASK_IRQ	0xC1100000
#define PNX8181_INTC_PRIOMASK_FIQ	0xC1100004

#define PNX8181_EXTINT_13		0xC2105034
#define PNX8181_EXTINT_ENABLE1		0xC2105060
#define PNX8181_EXTINT_STATUS		0xC210506C

static void pnx8181udc_irqvbus(void)
{
	struct pnx8181udc *udc = &controller;

	/* clear EXTINT interrupt */
	writel(~(1<<13), (void *)PNX8181_EXTINT_STATUS);

	udc->has_vbus = pnx8181udc_has_vbus();
	udc_debug(1, "USB: has_vbus=%d\n", udc->has_vbus);
	pnx8181udc_vbus_session(udc->has_vbus);
}

static void pnx8181udc_irq(void)
{
	struct pnx8181udc *udc = &controller;
	u32 pending = readl(PNX8181_USB_INTSTAT);

	/* clear active interrupts */
	writel(pending, PNX8181_USB_INTCLR);

	/* only process enabled interrupts */
	pending &= readl(PNX8181_USB_INTEN);

	if (!pending)
		return;

	/* handle status interrupts */
	if (pending & INT_DEVSTAT) {
		u8 status;

		pnx8181udc_cmd_read(CMD_GETSTATUS, &status, sizeof(status));

		/* handle bus reset */
		if (status & STATUS_BUS_RESET) {
			//printf("%s(): handling bus reset\n", __func__);

			pnx8181udc_stop(udc);

			/* set transmit length of ep0 to zero */
			writel(CON_LOGEP(0), PNX8181_USB_CON);
			udelay(2);
			writel(0, PNX8181_USB_TXPCKL);

			/* clear all interrupts */
			/* FIXME: necessary? */
			writel(0xffff, PNX8181_USB_INTCLR);

			pnx8181udc_enable_int(INT_ENDPOINT(0));
			pnx8181udc_enable_int(INT_ENDPOINT(1));

			udc->suspended = 0;
			pnx8181udc_connect(udc);

			usbd_device_event_irq(udc_device, DEVICE_RESET, 0);
		}

		/* handle changed suspend bit */
		if (status & STATUS_SUSPEND_C) {
			/* host initiated suspend */
			if (status & STATUS_SUSPEND) {
				if (!udc->suspended) {
					udc->suspended = 1;
					/* only status indication ->
					 * usbd_device_event_irq() has to be
					 * called before ? */
					usbd_device_event_irq(udc_device,
					              DEVICE_BUS_INACTIVE, 0);

				}

			/* host initiated resume */
			} else {
				if (udc->suspended) {
					udc->suspended = 0;
					usbd_device_event_irq(udc_device,
					              DEVICE_BUS_ACTIVITY, 0);
				}
			}
		}

	/* handle endpoint interrupts */
	} else {
		int i;

		udc->pending = pending;

		if (pending & (INT_ENDPOINT(0) | INT_ENDPOINT(1)))
			pnx8181udc_handle_ep0();

		for (i = 1; i < NUM_ENDPOINTS; i++) {
			if (pending & INT_ENDPOINT(i+1))
				pnx8181udc_handle_ep(i, &udc->udc_ep[i]);
		}
	}
}

void udc_endpoint_write (struct usb_endpoint_instance *endpoint)
{
	pnx8181udc_write_fifo((struct pnx8181udc_ep *)&endpoint);
}

void do_irq(struct pt_regs *pt_regs)
{
	u32 irq;

	irq = (readl((void *)PNX8181_INTC_VECTOR_IRQ) >> 3) & 0xff;

	/* acknowledge / mask interrupt */
	writel(REQ_DISABLE, (void *)(PNX8181_INTC_REQUEST + (irq * 4)));

	if (irq == UDC_USB_IRQ)
		pnx8181udc_irq();
	if (irq == UDC_VBUS_IRQ)
		pnx8181udc_irqvbus();

	/* unmask interrupt */
	writel(REQ_ENABLE, (void *)(PNX8181_INTC_REQUEST + (irq * 4)));
}

void udc_irq(void)
{
	do_irq(0);
}

void undefined_instruction(void);
void software_interrupt(void);
void prefetch_abort(void);
void data_abort(void);
void not_used(void);
void irq(void);
void fiq(void);

int udc_init(void)
{
	struct pnx8181udc *udc = &controller;

	debug("%s: USB device controller\n", pnx8181udc_name);

	udc_debug(1, "udc_init()\n");

	/* Install exception vectors as they are not automatically relocated.
	 * We want to make use of interrupts */
	memcpy(0x0, (void *)VECTOR_TABLE, 0x1000);
	*(unsigned long *)0x20 = (unsigned long)&undefined_instruction;
	*(unsigned long *)0x24 = (unsigned long)&software_interrupt;
	*(unsigned long *)0x28 = (unsigned long)&prefetch_abort;
	*(unsigned long *)0x2C = (unsigned long)&data_abort;
	*(unsigned long *)0x30 = (unsigned long)&not_used;
	*(unsigned long *)0x34 = (unsigned long)&irq;
	*(unsigned long *)0x38 = (unsigned long)&fiq;

	/* config gpio a18 for USBCN */
	writel((readl((void *)PNX8181_SYSMUX1) & ~(0x3 << 4)) | (0x2 << 4),
	       (void *)PNX8181_SYSMUX1);

	/* config gpio c14 for USBVBUS */
	writel((readl((void *)PNX8181_SYSMUX4) & ~(0x3 << 28)),
	       (void *)PNX8181_SYSMUX4);

	/* enable extint clock */
	writel(readl((void *)PNX8181_CGU_GATESC) | (1<<1),
	       (void *)PNX8181_CGU_GATESC);

	/* disable extints */
	writel(0x00, (void *)PNX8181_EXTINT_ENABLE1);
	writel(0x00, (void *)PNX8181_EXTINT_STATUS);

	/* set input of extint 13 (gpio a19) to usbvbus */
	writel(((3<<6) | (7<<3) | (1<<1)), (void *)PNX8181_EXTINT_13);

	/* Disable the priority masks */
	writel(0x0000, (void *)PNX8181_INTC_PRIOMASK_IRQ);
	writel(0x0000, (void *)PNX8181_INTC_PRIOMASK_FIQ);

	/* configure priority & disable interrupt */
	writel(REQ_DISABLE | REQ_DEFAULT | REQ_ACTIVE_LOW,
	       (void *)(PNX8181_INTC_REQUEST + (UDC_VBUS_IRQ * 4)));
	writel(REQ_DISABLE | REQ_DEFAULT,
	       (void *)(PNX8181_INTC_REQUEST + (UDC_USB_IRQ * 4)));

	/* enable int clock */
	writel(readl((void *)PNX8181_CGU_GATESC) | 1,
	       (void *)PNX8181_CGU_GATESC);

	/* enable the clock of the udc block */
	writel(readl((void *)PNX8181_CGU_GATESC) | (1<<17),
	       (void *)PNX8181_CGU_GATESC);

	udc_debug(1, "udc_init -> disconnect\n");
	pnx8181udc_disconnect(udc);

	/* disable & clear all pending interrupts from prior use */
	writel(0x0000, PNX8181_USB_INTEN);
	writel(0xffff, PNX8181_USB_INTCLR);

	pnx8181udc_set_addr(0);
	pnx8181udc_enable_int(INT_DEVSTAT);

	/* sample vbus once incase we start with a plugged-in cable */
	udc->has_vbus = pnx8181udc_has_vbus();
	udc_debug(1, "USB VBUS: %d\n", udc->has_vbus);

	/* enable interrupts */
	writel(REQ_ENABLE, (void *)(PNX8181_INTC_REQUEST+(UDC_VBUS_IRQ * 4)));
	writel(REQ_ENABLE, (void *)(PNX8181_INTC_REQUEST+(UDC_USB_IRQ * 4)));

	writel((1<<13), (void *)PNX8181_EXTINT_ENABLE1);

	printf("USB:   ready, ");

	if (udc->has_vbus)
		pnx8181udc_vbus_session(1);
	else
		printf("not ");
	printf("connected\n");

	//local_irq_enable();

	return 0;
}

void udc_disconnect(void)
{
	struct pnx8181udc *udc = &controller;

	debug("disconnect, disable Pullup\n");

	local_irq_disable();

	/* Turn off the USB connection by disabling the pullup resistor */
	if (udc->connected)
		pnx8181udc_vbus_session(0);

	/* disable extints */
	writel(0x00, (void *)PNX8181_EXTINT_ENABLE1);
	writel(0x00, (void *)PNX8181_EXTINT_STATUS);

	/* set input of extint 13 (gpio a19) to usbvbus */
	writel(0x0, (void *)PNX8181_EXTINT_13);

	/* disable extint clock */
	writel(readl((void *)PNX8181_CGU_GATESC) & ~(1<<1),
	       (void *)PNX8181_CGU_GATESC);

	/* configure priority & disable interrupt */
	writel(REQ_DISABLE | REQ_DEFAULT | REQ_ACTIVE_LOW,
	       (void *)(PNX8181_INTC_REQUEST + (UDC_VBUS_IRQ * 4)));
	writel(REQ_DISABLE | REQ_DEFAULT,
	       (void *)(PNX8181_INTC_REQUEST + (UDC_USB_IRQ * 4)));

	/* disable & clear all pending interrupts from prior use */
	writel(0x0000, PNX8181_USB_INTEN);
	writel(0xffff, PNX8181_USB_INTCLR);

	/* disable the clock of the udc block */
	writel(readl((void *)PNX8181_CGU_GATESC) & ~(1<<17),
	       (void *)PNX8181_CGU_GATESC);
}

void udc_ctrl(enum usbd_event event, int param)
{
	/* not used? */
}

/* Turn on the USB connection by enabling the pullup resistor */
void udc_connect (void)
{
	debug("connect, enable Pullup\n");
	pnx8181udc_connect(&controller);
}


/* Switch on the UDC */
void udc_enable(struct usb_device_instance *device)
{
	debug("enable device %p, status %d\n", device, device->status);

	/* Save the device structure pointer */
	udc_device = device;

	/* Setup ep0 urb */
	if (!ep0_urb) {
		ep0_urb = usbd_alloc_urb(udc_device,
					 udc_device->bus->endpoint_array);
	} else
		serial_printf("udc_enable: ep0_urb already allocated %p\n",
			       ep0_urb);

	pnx8181udc_enable_int(INT_DEVSTAT);
}

/* Switch off the UDC */
void udc_disable(void)
{
	/* disable all interrupts */
	pnx8181udc_disable_int(0x3f);
}

/**
 * udc_startup - allow udc code to do any additional startup
 */
void udc_startup_events(struct usb_device_instance *device)
{
	/* The DEVICE_INIT event puts the USB device in the state STATE_INIT.
	 */
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

	udc_enable(device);
}

void udc_set_nak(int epid)
{
	/* FIXME: implement this */
}

void udc_unset_nak(int epid)
{
	/* FIXME: implement this */
}

#endif
