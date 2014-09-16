/*
 * (C) 2007 by OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * based on existing SAM7DFU code from OpenPCD:
 * (C) Copyright 2006 by Harald Welte <hwelte@hmw-consulting.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * TODO:
 * - make NAND support reasonably self-contained and put in apropriate
 *   ifdefs
 * - add some means of synchronization, i.e. block commandline access
 *   while DFU transfer is in progress, and return to commandline once
 *   we're finished
 * - add VERIFY support after writing to flash
 * - sanely free() resources allocated during first upload/download
 *   request when aborting
 * - sanely free resources when another alternate interface is selected
 *
 * Maybe:
 * - add something like uImage or some other header that provides CRC
 *   checking?
 * - make 'dnstate' attached to 'struct usb_device_instance'
 */

#include <config.h>
#if defined(CONFIG_USBD_DFU)

//#define DEBUG
#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <linux/types.h>
#include <linux/list.h>
#include <asm/errno.h>
#include <usb_defs.h>
#include "usbdescriptors.h"
#include <usbdevice.h>
#ifdef CONFIG_MUSB
#include <usb/musb_udc.h>
#elif defined(CONFIG_DWC_OTG_UDC)
#include <usb/dwc_otg_udc.h>
#else
#include "usbdcore_dspg.h"
#endif
#include <usb_dfu.h>
#include <usb_dfu_descriptors.h>
#include <usb_dfu_trailer.h>
#include <stdio_dev.h>
#include <part.h>
#include <mmc.h>

#ifdef CONFIG_SYS_HUSH_PARSER
#include <hush.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#include <nand.h>
#include "jffs2/load_kernel.h"
int mtdparts_init(void);
extern struct list_head devices;
char *partnames[DFU_NUM_ALTERNATES];
struct part_info *dfu_parts[DFU_NUM_ALTERNATES];
int devtype[DFU_NUM_ALTERNATES];
static struct block_dev_desc *bdev;

#define RET_NOTHING	0
#define RET_ZLP		1
#define RET_STALL	2

#define DFU_RX_EP	2

#define FAST_XFER_SIZE (128*1024)

#if 0
#undef debug
#define debug printf
#endif

volatile enum dfu_state *system_dfu_state; /* for 3rd parties */

struct dnload_state {
	unsigned char *ptr;	/* pointer to next empty byte in buffer */
	unsigned int off;	/* offset of current erase page in flash chip */
	unsigned char *buf;	/* pointer to allocated erase page buffer */
	unsigned int remain;	/* expected remaining data on bulk endpoint */

	/* unless doing an atomic transfer, we use the static buffer below.
	 * This saves us from having to clean up dynamic allications in the
	 * various error paths of the code.  Also, it will always work, no
	 * matter what the memory situation is. */
	/* needed only for flash download: */
	/* unsigned char _buf[0x20000 + 4096]; */ /* FIXME: depends flash page size, large pages */
};

static struct dnload_state _dnstate;
#if 0
static int dfu_trailer_matching(const struct uboot_dfu_trailer *trailer)
{
	if (trailer->magic != UBOOT_DFU_TRAILER_MAGIC ||
	    trailer->version != UBOOT_DFU_TRAILER_V1 ||
	    trailer->vendor != CONFIG_USBD_VENDORID ||
	    (trailer->product != CONFIG_USBD_PRODUCTID_CDCACM &&
	     trailer->product != CONFIG_USBD_PRODUCTID_GSERIAL))
		return 0;
#ifdef CONFIG_REVISION_TAG
	if (trailer->revision != get_board_rev())
		return 0;
#endif

	return 1;
}
#endif

extern int dfu_active;
extern unsigned long dfu_start_time;

unsigned char *dfu_loadaddr;

static struct usb_device_instance *dfu_usb_dev = 0;

int handle_dnload(struct urb *urb, u_int16_t val, u_int16_t len, int first)
{
	struct usb_device_instance *dev = urb->device;
	struct dnload_state *ds = &_dnstate;

	debug("download(len=%u, first=%u) ", len, first);

	dfu_active = 1;

	if (len > CONFIG_USBD_DFU_XFER_SIZE) {
		/* Too big. Not that we'd really care, but it's a
		 * DFU protocol violation */
		debug("length exceeds flash page size ");
		dev->dfu_state = DFU_STATE_dfuERROR;
		dev->dfu_status = DFU_STATUS_errADDRESS;
		return RET_STALL;
	}

	/* really end of image transfer */
	if (len == 0) {
		/* will be handled in dfu_poll() */
		debug("zero-size write -> MANIFEST_SYNC ");
		dev->dfu_state = DFU_STATE_dfuMANIFEST_SYNC;
	} else {
		if (first) {
			printf("Starting DFU download to SDRAM (0x%08x)",
			       (unsigned int)dfu_loadaddr);
			ds->buf = dfu_loadaddr;
			ds->ptr = ds->buf;
			ds->remain = 0;
			if ((dev->alternate > 0) && (partnames[dev->alternate-1]))
				printf(" for partition '%s'", partnames[dev->alternate-1]);
			printf("\n");
		}

		memcpy(ds->ptr, urb->buffer, len);

		urb->actual_length -= len;
		ds->ptr += len;
	}

	return RET_ZLP;
}

int handle_dnload_fast(struct urb *urb, u_int16_t val, u_int16_t len, int first)
{
	struct usb_device_instance *dev = urb->device;
	struct dnload_state *ds = &_dnstate;
	struct dfuf_dnload *dnload = (struct dfuf_dnload *)urb->buffer;

	debug("download_fast(dwTransferSize=%u, dwCRC32=0x%08x, first=%u) ",
		dnload->dwTransferSize, dnload->dwCRC32, first);

	if (len < sizeof(*dnload)) {
		debug("DFUF_DNLOAD invalid request ");
		dev->dfu_state = DFU_STATE_dfuERROR;
		dev->dfu_status = DFU_STATUS_errSTALLEDPKT;
		return RET_STALL;
	}

	if (dnload->dwTransferSize > FAST_XFER_SIZE) {
		/* Too big. Not that we'd really care, but it's a
		 * DFU protocol violation */
		debug("length exceeds flash page size ");
		dev->dfu_state = DFU_STATE_dfuERROR;
		dev->dfu_status = DFU_STATUS_errADDRESS;
		return RET_STALL;
	}

	/* really end of image transfer */
	if (dnload->dwTransferSize == 0) {
		if (first) {
			dev->dfu_state = DFU_STATE_dfuERROR;
			dev->dfu_status = DFU_STATUS_errNOTDONE;
			return RET_STALL;
		}

		/* will be handled in dfu_poll() */
		debug("zero-size write -> MANIFEST_SYNC ");
		dev->dfu_state = DFU_STATE_dfuMANIFEST_SYNC;
	} else {
		if (first) {
			printf("Starting DFU FAST download to SDRAM (0x%08x)",
			       (unsigned int)dfu_loadaddr);
			ds->buf = dfu_loadaddr;
			ds->ptr = ds->buf;
			if ((dev->alternate > 0) && (partnames[dev->alternate-1]))
				printf(" for partition '%s'", partnames[dev->alternate-1]);
			printf("\n");
		}

		ds->remain = dnload->dwTransferSize;
	}

	return RET_ZLP;
}

static int handle_upload(struct urb *urb, u_int16_t val, u_int16_t len, int first)
{
	struct usb_device_instance *dev = urb->device;
	struct dnload_state *ds = &_dnstate;
	unsigned int remain;
	int rc;

	debug("upload(val=0x%02x, len=%u, first=%u) ", val, len, first);

	dfu_active = 1;

	if (len > CONFIG_USBD_DFU_XFER_SIZE) {
		/* Too big */
		dev->dfu_state = DFU_STATE_dfuERROR;
		dev->dfu_status = DFU_STATUS_errADDRESS;
		//udc_ep0_send_stall();
		debug("Error: Transfer size > CONFIG_USBD_DFU_XFER_SIZE ");
		return -EINVAL;
	}

	switch (dev->alternate) {
	case 0:
		if (first) {
			serial_printf("Starting DFU Upload of RAM (0x%08x)\n",
				(uint32_t) dfu_loadaddr);
			ds->ptr = dfu_loadaddr;
		}

		/* FIXME: end at some more dynamic point */
		if (ds->ptr + len > dfu_loadaddr + 0x200000)
			len = (dfu_loadaddr + 0x20000) - ds->ptr;

		urb->buffer = ds->ptr;
		urb->actual_length = len;
		ds->ptr += len;
		break;
	default:
	{
#ifdef CONFIG_MTD_DEVICE
		if (first && (devtype[dev->alternate-1] == MTD_DEV_TYPE_NAND)) {
			size_t size = dfu_parts[dev->alternate-1]->size;
			loff_t offset = dfu_parts[dev->alternate-1]->offset;
			nand_info_t *nand = &nand_info[nand_curr_device];
			int ret;

			printf("Copy content of partition %s on NAND to RAM (0x%08x)\n",
			       dfu_parts[dev->alternate-1]->name, (uint32_t) dfu_loadaddr);
			ret = nand_read_skip_bad(nand, offset, &size,
			                         (u_char *)dfu_loadaddr);
			if (ret < 0) {
				len = 0;
				break;
			}
			ds->ptr = dfu_loadaddr;
		} else if (first && (devtype[dev->alternate-1] == MTD_DEV_TYPE_NOR)) {
			size_t size = dfu_parts[dev->alternate-1]->size;
			loff_t offset = dfu_parts[dev->alternate-1]->offset;

			printf("Copy content of partition %s on NOR to RAM (0x%08x)\n",
			       dfu_parts[dev->alternate-1]->name, (uint32_t) dfu_loadaddr);
			memcpy(dfu_loadaddr, 0x3c000000 + offset, size);
			ds->ptr = dfu_loadaddr;
		}
#elif defined(CONFIG_MMC)
		if (first) {
			size_t size = dfu_parts[dev->alternate-1]->size / bdev->blksz;
			loff_t offset = dfu_parts[dev->alternate-1]->offset;
			int ret;

			printf("Copy content of partition %s on MMC to RAM (0x%08x)\n",
			       dfu_parts[dev->alternate-1]->name, (uint32_t) dfu_loadaddr);
			ret = bdev->block_read(bdev->dev, offset, size,
				(void *)dfu_loadaddr);
			if (ret != size) {
				len = 0;
				break;
			}
			ds->ptr = dfu_loadaddr;
		}
#else
#error "Neither MTD nor MMC support."
#endif

		if (ds->ptr + len > dfu_loadaddr + dfu_parts[dev->alternate-1]->size)
			len = (dfu_loadaddr + dfu_parts[dev->alternate-1]->size) - ds->ptr;

		urb->buffer = ds->ptr;
		urb->actual_length = len;
		ds->ptr += len;
		break;
	}
	}

	debug("returning len=%u\n", len);
	return len;
}

static void handle_getstatus(struct urb *urb, int max)
{
	struct usb_device_instance *dev = urb->device;
	struct dfu_status *dstat = (struct dfu_status *) urb->buffer;
	struct dnload_state *ds = &_dnstate;
	u_int8_t state = dev->dfu_state;

	debug("getstatus ");

	if (!urb->buffer || urb->buffer_length < sizeof(*dstat)) {
		debug("invalid urb! ");
		return;
	}

	switch (dev->dfu_state) {
	case DFU_STATE_dfuDNLOAD_SYNC:
	case DFU_STATE_dfuDNBUSY:
		if (ds->remain)
			dev->dfu_state = DFU_STATE_dfuDNBUSY;
		else {
			debug("DNLOAD_IDLE ");
			dev->dfu_state = DFU_STATE_dfuDNLOAD_IDLE;
		}
		state = dev->dfu_state;
		break;
	case DFU_STATE_dfuMANIFEST_SYNC:
		state = DFU_STATE_dfuMANIFEST;
		break;
	default:
		//return;
		break;
	}

	/* send status response */
	dstat->bStatus = dev->dfu_status;
	dstat->bState = state;
	dstat->iString = 0;
	dstat->bwPollTimeout[0] = 1;
	dstat->bwPollTimeout[1] = 0;
	dstat->bwPollTimeout[2] = 0;
	urb->actual_length = MIN(sizeof(*dstat), max);

	/* we don't need to explicitly send data here, will
	 * be done by the original caller! */
}

static void handle_getstate(struct urb *urb, int max)
{
	debug("getstate ");

	if (!urb->buffer || urb->buffer_length < sizeof(u_int8_t)) {
		debug("invalid urb! ");
		return;
	}

	urb->buffer[0] = urb->device->dfu_state & 0xff;
	urb->actual_length = sizeof(u_int8_t);
}

static void handle_setup(struct urb *urb, int max)
{
	struct dfuf_setup *setup = (struct dfuf_setup *)urb->buffer;

	debug("setup ");

	if (!urb->buffer || urb->buffer_length < 8) {
		debug("invalid urb! ");
		return;
	}

	setup->dwMaxDNLOAD = FAST_XFER_SIZE;
	setup->dwMaxUPLOAD = FAST_XFER_SIZE;
	urb->actual_length = MIN(sizeof(*setup), max);
}

#ifndef CONFIG_USBD_PRODUCTID_DFU
#define CONFIG_USBD_PRODUCTID_DFU CONFIG_USBD_PRODUCTID_CDCACM
#endif

static const struct usb_device_descriptor dfu_dev_descriptor = {
	.bLength		= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
#if defined(CONFIG_MUSB) || defined(CONFIG_DWC_OTG_UDC)
	.bcdUSB = 		cpu_to_le16(USB_BCD_VERSION2),
#else
	.bcdUSB = 		cpu_to_le16(USB_BCD_VERSION),
#endif
#if defined(CONFIG_USBD_DFUF) && !defined(CONFIG_USBD_DFUF_SI)
	.bDeviceClass		= 0xef,
	.bDeviceSubClass	= 0x02,
	.bDeviceProtocol	= 0x01,
#else
	.bDeviceClass		= 0x00,
	.bDeviceSubClass	= 0x00,
	.bDeviceProtocol	= 0x00,
#endif
	.bMaxPacketSize0	= EP0_MAX_PACKET_SIZE,
	.idVendor		= CONFIG_USBD_VENDORID,
	.idProduct		= CONFIG_USBD_PRODUCTID_DFU,
	.bcdDevice		= 0x0000,
	.iManufacturer		= DFU_STR_MANUFACTURER,
	.iProduct		= DFU_STR_PRODUCT,
	.iSerialNumber		= DFU_STR_SERIAL,
	.bNumConfigurations	= 0x01,
};

static struct _dfu_desc dfu_cfg_descriptor = {
	.ucfg = {
		.bLength		= USB_DT_CONFIG_SIZE,
		.bDescriptorType	= USB_DT_CONFIG,
		.wTotalLength		= USB_DT_CONFIG_SIZE +	(DFU_NUM_ALTERNATES * USB_DT_INTERFACE_SIZE) + USB_DT_DFU_SIZE,
					//cpu_to_le16(sizeof(struct _dfu_desc)),
#if defined(CONFIG_USBD_DFUF) && !defined(CONFIG_USBD_DFUF_SI)
		.bNumInterfaces		= 2,
#else
		.bNumInterfaces		= 1,
#endif		
		.bConfigurationValue	= 1,
		.iConfiguration		= DFU_STR_CONFIG,
		.bmAttributes		= BMATTRIBUTE_RESERVED,
		.bMaxPower		= 50,
	},
	.func_dfu = DFU_FUNC_DESC,
#if defined(CONFIG_USBD_DFUF) && !defined(CONFIG_USBD_DFUF_SI)
	.uif_assoc_bulk_dfu = DFU_RT_IF_ASSOC_BULK_DESC,
	.uif_bulk_dfu = DFU_RT_IF_BULK_DESC,
	.ep_in_dfu = DFU_EP_IN_DESC,
	.ep_out_dfu = DFU_EP_OUT_DESC,
#endif
};

int dfu_ep0_handler(struct urb *urb)
{
	int rc, ret = RET_NOTHING;
	u_int8_t req = urb->device_request.bRequest;
	u_int16_t val = urb->device_request.wValue;
	u_int16_t len = urb->device_request.wLength;
	struct usb_device_instance *dev = urb->device;

#if defined(CONFIG_DSPG_DW) && !defined(CONFIG_MUSB) && !defined(CONFIG_DWC_OTG_UDC)
	struct urb* ep_urb = dev->bus->endpoint_array[EP_OUT].rcv_urb;
#endif

	debug("dfu_ep0(req=0x%x, val=0x%x, len=%u) old_state = %u ",
		req, val, len, dev->dfu_state);

	switch (dev->dfu_state) {
	case DFU_STATE_appIDLE:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus(urb, len);
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate(urb, len);
			break;
		case USB_REQ_DFU_DETACH:
			dev->dfu_state = DFU_STATE_appDETACH;
			ret = RET_ZLP;
			goto out;
			break;
		case USB_REQ_DFUF_SETUP:
			handle_setup(urb, len);
			break;
		default:
			ret = RET_STALL;
		}
		break;
	case DFU_STATE_appDETACH:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus(urb, len);
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate(urb, len);
			break;
		case USB_REQ_DFUF_SETUP:
			handle_setup(urb, len);
			break;
		default:
			dev->dfu_state = DFU_STATE_appIDLE;
			ret = RET_STALL;
			goto out;
			break;
		}
		/* FIXME: implement timer to return to appIDLE */
		break;
	case DFU_STATE_dfuIDLE:
		switch (req) {
		case USB_REQ_DFU_DNLOAD:
			if (len == 0) {
				dev->dfu_state = DFU_STATE_dfuERROR;
				ret = RET_STALL;
				goto out;
			}
			dev->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
			ret = handle_dnload(urb, val, len, 1);
			break;
		case USB_REQ_DFUF_DNLOAD:
			dev->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
			ret = handle_dnload_fast(urb, val, len, 1);
			break;
		case USB_REQ_DFU_UPLOAD:
			dev->dfu_state = DFU_STATE_dfuUPLOAD_IDLE;
			handle_upload(urb, val, len, 1);
			break;
		case USB_REQ_DFU_ABORT:
			/* no zlp? */
			ret = RET_ZLP;
			break;
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus(urb, len);
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate(urb, len);
			break;
		case USB_REQ_DFU_DETACH:
			/* Proprietary extension: 'detach' from idle mode and
			 * get back to runtime mode in case of USB Reset.  As
			 * much as I dislike this, we just can't use every USB
			 * bus reset to switch back to runtime mode, since at
			 * least the Linux USB stack likes to send a number of resets
			 * in a row :( */
			dev->dfu_state = DFU_STATE_dfuMANIFEST_WAIT_RST;
			break;
		case USB_REQ_DFUF_SETUP:
			handle_setup(urb, len);
			break;
		default:
			dev->dfu_state = DFU_STATE_dfuERROR;
			ret = RET_STALL;
			goto out;
			break;
		}
		break;
	case DFU_STATE_dfuDNLOAD_SYNC:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus(urb, len);
			/* FIXME: state transition depending on block completeness */
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate(urb, len);
			break;
		default:
			dev->dfu_state = DFU_STATE_dfuERROR;
			ret = RET_STALL;
			goto out;
		}
		break;
	case DFU_STATE_dfuDNBUSY:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			/* FIXME: only accept getstatus if bwPollTimeout
			 * has elapsed */
			handle_getstatus(urb, len);
			break;
		default:
			dev->dfu_state = DFU_STATE_dfuERROR;
			ret = RET_STALL;
			goto out;
		}
		break;
	case DFU_STATE_dfuDNLOAD_IDLE:
		switch (req) {
		case USB_REQ_DFU_DNLOAD:
			dev->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
			ret = handle_dnload(urb, val, len, 0);
			break;
		case USB_REQ_DFUF_DNLOAD:
			dev->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
			ret = handle_dnload_fast(urb, val, len, 0);
			break;
		case USB_REQ_DFU_ABORT:
			dev->dfu_state = DFU_STATE_dfuIDLE;
			ret = RET_ZLP;
			break;
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus(urb, len);
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate(urb, len);
			break;
		default:
			dev->dfu_state = DFU_STATE_dfuERROR;
			ret = RET_STALL;
			break;
		}
		break;
	case DFU_STATE_dfuMANIFEST_SYNC:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			/* We're MainfestationTolerant */
			//dev->dfu_state = DFU_STATE_dfuIDLE;
			handle_getstatus(urb, len);
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate(urb, len);
			break;
		default:
			dev->dfu_state = DFU_STATE_dfuERROR;
			ret = RET_STALL;
			break;
		}
		break;
	case DFU_STATE_dfuMANIFEST:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			/* FIXME: only accept getstatus if bwPollTimeout
			 * has elapsed */
			handle_getstatus(urb, len);
			break;
		default:
			/* we should never go here */
			dev->dfu_state = DFU_STATE_dfuERROR;
			ret = RET_STALL;
			break;
		}
		break;
	case DFU_STATE_dfuMANIFEST_WAIT_RST:
		/* we should never go here */
		break;
	case DFU_STATE_dfuUPLOAD_IDLE:
		switch (req) {
		case USB_REQ_DFU_UPLOAD:
			/* state transition if less data then requested */
			rc = handle_upload(urb, val, len, 0);
			if (rc >= 0 && rc < len) {
				dfu_active = 0;
				dfu_start_time = get_timer(0);
				dev->dfu_state = DFU_STATE_dfuIDLE;
			}
			break;
		case USB_REQ_DFU_ABORT:
			dev->dfu_state = DFU_STATE_dfuIDLE;
			/* no zlp? */
			ret = RET_ZLP;
			break;
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus(urb, len);
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate(urb, len);
			break;
		default:
			dev->dfu_state = DFU_STATE_dfuERROR;
			ret = RET_STALL;
			break;
		}
		break;
	case DFU_STATE_dfuERROR:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus(urb, len);
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate(urb, len);
			break;
		case USB_REQ_DFU_CLRSTATUS:
			dev->dfu_state = DFU_STATE_dfuIDLE;
			dev->dfu_status = DFU_STATUS_OK;
#if defined(CONFIG_DSPG_DW) && !defined(CONFIG_MUSB) && !defined(CONFIG_DWC_OTG_UDC)
			DSPG_UDC_SETIX(EP_OUT);
			REG8_WRITE(musb_regs->OutCSR1,UDC_OUTCSR_FlushFIFO); /* (1<<4) */

			ep_urb->buffer = ep_urb->buffer_data;
			ep_urb->actual_length = 0;
			ep_urb->buffer_length = sizeof(ep_urb->buffer_data); /* URB_BUF_SIZE = 4224 */
#endif
			/* ds->ptr = ds->buf = ds->_buf; */
			/* no zlp? */
			ret = RET_ZLP;
			break;
		default:
			dev->dfu_state = DFU_STATE_dfuERROR;
			ret = RET_STALL;
			break;
		}
		break;
	default:
		return DFU_EP0_UNHANDLED;
		break;
	}

out:
	debug("new_state = %u, ret = %u\n", dev->dfu_state, ret);


	if (dev->dfu_state == DFU_STATE_dfuERROR) {
		udc_endpoint_feature(dev, DFU_RX_EP /* FIXME: rx_endpoint */, 1);
		//udc_endpoint_feature(dev, DFU_TX_EP /* FIXME: tx_endpoint */, 1);
	}

	switch (ret) {
	case RET_ZLP:
		//udc_ep0_send_zlp();
		urb->actual_length = 0;
		return DFU_EP0_ZLP;
		break;
	case RET_STALL:
		//udc_ep0_send_stall();
		return DFU_EP0_STALL;
		break;
	case RET_NOTHING:
		break;
	}

	return DFU_EP0_DATA;
}

/* utility function for converting char* to wide string used by USB */
void str2wide (char *str, u16 * wide)
{
	int i;
	for (i = 0; i < strlen (str) && str[i]; i++){
		#if defined(__LITTLE_ENDIAN)
			wide[i] = (u16) str[i];
		#elif defined(__BIG_ENDIAN)
			wide[i] = ((u16)(str[i])<<8);
		#else
			#error "__LITTLE_ENDIAN or __BIG_ENDIAN undefined"
		#endif
	}
}

static struct usb_string_descriptor *create_usbstring(char *string)
{
	struct usb_string_descriptor *strdesc;
	int size = sizeof(*strdesc) + strlen(string)*2;

	if (size > 255)
		return NULL;

	strdesc = malloc(size);
	if (!strdesc)
		return NULL;

	strdesc->bLength = size;
	strdesc->bDescriptorType = USB_DT_STRING;
	str2wide(string, strdesc->wData);

	return strdesc;
}

extern struct usb_string_descriptor **usb_strings;

static void dfu_init_strings(struct usb_device_instance *dev)
{
	int i;
	struct usb_string_descriptor *strdesc;
	int num_alternates = 0;
	struct usb_configuration_descriptor *conf;
	unsigned char tmp[4];

#ifdef CONFIG_MTD_DEVICE
	struct mtd_device *mtd_dev;
	struct list_head *dentry;
	struct list_head *pentry;
	struct part_info *part;

	if (mtdparts_init())
		printf("mtdparts failed\n");

	list_for_each(dentry, &devices) {
		mtd_dev = list_entry(dentry, struct mtd_device, link);
		if ((mtd_dev->id->type == MTD_DEV_TYPE_NAND) ||
		    (mtd_dev->id->type == MTD_DEV_TYPE_NOR)) {
			list_for_each(pentry, &mtd_dev->parts) {
				part = list_entry(pentry,
				    struct part_info, link);
				if (num_alternates < DFU_NUM_ALTERNATES) {
					partnames[num_alternates] = part->name;
					dfu_parts[num_alternates] = part;
					devtype[num_alternates] = mtd_dev->id->type;
					num_alternates++;
				}
			}
		}
	}
#elif defined(CONFIG_MMC)
	struct disk_partition partinfo;
	int ret;

	i = 1;
	do {
		ret = get_partition_info(bdev, i, &partinfo);
		if (ret)
			continue;

		partnames[num_alternates] = malloc(16);
		sprintf(partnames[num_alternates], "mmcblk0p%d", i);
		dfu_parts[num_alternates] = malloc(sizeof(struct part_info));
		dfu_parts[num_alternates]->size = partinfo.size * partinfo.blksz;
		dfu_parts[num_alternates]->offset = partinfo.start;
		dfu_parts[num_alternates]->name = partnames[num_alternates];
		num_alternates++;
	} while (++i <= 5 || !ret);
#else
#error "Neither MTD nor MMC support."
#endif
	//printf("found %d alternates (max=%d)\n", num_alternates, DFU_NUM_ALTERNATES);
	num_alternates++; /* for default interface */

	strdesc = create_usbstring(CONFIG_DFU_CFG_STR);
	usb_strings[DFU_STR_CONFIG] = strdesc;

	for (i = 0; i < num_alternates; i++) {
		if (i == 0)
			strdesc = create_usbstring(CONFIG_DFU_ALT0_STR);
		else
			strdesc = create_usbstring(partnames[i-1]);
		if (!strdesc)
			continue;
		usb_strings[STR_COUNT+i+1] = strdesc;
	}
	for (i = num_alternates; i < DFU_NUM_ALTERNATES; i++) {
		sprintf(tmp, "%d", i);
		strdesc = create_usbstring(tmp);
		if (!strdesc)
			continue;
		usb_strings[STR_COUNT+i+1] = strdesc;
	}

#ifdef CONFIG_USBD_DFUF_SI
	dfu_cfg_descriptor.ucfg.wTotalLength = USB_DT_CONFIG_SIZE +	(DFU_NUM_ALTERNATES * (USB_DT_INTERFACE_SIZE + USB_DT_ENDPOINT_SIZE + USB_DT_ENDPOINT_SIZE)) + USB_DT_DFU_SIZE;
#else	
	dfu_cfg_descriptor.ucfg.wTotalLength = USB_DT_CONFIG_SIZE +	(DFU_NUM_ALTERNATES * USB_DT_INTERFACE_SIZE) + 			USB_DT_DFU_SIZE + USB_DT_INTERFACE_ASSOC_SIZE + USB_DT_INTERFACE_SIZE + (2 * USB_DT_ENDPOINT_SIZE);
#endif

	dev->dfu_num_alternates = DFU_NUM_ALTERNATES; //num_alternates;
}

/******************************************************************************/

int drv_usbdfu_init(void)
{
	char *s;

	dfu_loadaddr = (unsigned char *)CONFIG_SYS_LOAD_ADDR;
	s = getenv("dfu_loadaddr");
	if (s) {
		dfu_loadaddr = (unsigned char *)
		               simple_strtoul(s, (char **)&dfu_loadaddr, 16);
	}
	//local_irq_enable();
	enable_interrupts();

	return 1;
}

int dfu_init_instance(struct usb_device_instance *dev)
{
	int i;

	for (i = 0; i != DFU_NUM_ALTERNATES; i++) {
#if defined(CONFIG_USBD_DFUF) && defined(CONFIG_USBD_DFUF_SI)
		struct _dfu_alt_desc *alt = dfu_cfg_descriptor.alternates + i;
		struct usb_interface_descriptor *uif = &alt->uif;
		uif->bLength		= USB_DT_INTERFACE_SIZE;
		uif->bDescriptorType	= USB_DT_INTERFACE;
		uif->bAlternateSetting	= i;
		uif->bNumEndpoints	= 2;
		uif->bInterfaceClass	= 0xfe;
		uif->bInterfaceSubClass	= 1;
		uif->bInterfaceProtocol	= 2;
		uif->iInterface		= DFU_STR_ALT(i);

		alt->ep_in_dfu.bLength		= USB_DT_ENDPOINT_SIZE;
		alt->ep_in_dfu.bDescriptorType	= USB_DT_ENDPOINT;
		alt->ep_in_dfu.bEndpointAddress	= EP_IN_ADDR;
		alt->ep_in_dfu.bmAttributes	= USB_ENDPOINT_XFER_BULK;
		alt->ep_in_dfu.wMaxPacketSize	= EP_IN_MAXPACKET;
		alt->ep_in_dfu.bInterval	= 0x0;

		alt->ep_out_dfu.bLength		= USB_DT_ENDPOINT_SIZE;
		alt->ep_out_dfu.bDescriptorType	= USB_DT_ENDPOINT;
		alt->ep_out_dfu.bEndpointAddress= EP_OUT_ADDR;
		alt->ep_out_dfu.bmAttributes	= USB_ENDPOINT_XFER_BULK;
		alt->ep_out_dfu.wMaxPacketSize	= EP_OUT_MAXPACKET;
		alt->ep_out_dfu.bInterval	= 0x0;
#else
		struct usb_interface_descriptor *uif =
		    dfu_cfg_descriptor.uif+i;

		uif->bLength		= USB_DT_INTERFACE_SIZE;
		uif->bDescriptorType	= USB_DT_INTERFACE;
		uif->bAlternateSetting	= i;
		uif->bNumEndpoints	= 0;
		uif->bInterfaceClass	= 0xfe;
		uif->bInterfaceSubClass	= 1;
		uif->bInterfaceProtocol	= 2;
		uif->iInterface		= DFU_STR_ALT(i);
#endif
	}

	dev->dfu_dev_desc = &dfu_dev_descriptor;
	dev->dfu_cfg_desc = &dfu_cfg_descriptor;
	dev->dfu_state = DFU_STATE_appIDLE;
	dev->dfu_status = DFU_STATUS_OK;

	if (system_dfu_state)
		printf("SURPRISE: system_dfu_state is already set\n");
	system_dfu_state = &dev->dfu_state;

#if !defined(CONFIG_MTD_DEVICE) && defined(CONFIG_MMC)
	{
		struct mmc *mmc = find_mmc_device(0);

		if (!mmc) {
			printf("dfu: no mmc 0 dev!\n");
			return 1;
		}

		mmc_init(mmc);
		bdev = &mmc->block_dev;
	}
#endif

	dfu_init_strings(dev);

	dfu_usb_dev = dev;

	return 0;
}

/* event handler for usb device state events */
void dfu_event(struct usb_device_instance *device, usb_device_event_t event,
               int data)
{
	switch (event) {
	case DEVICE_RESET:
		switch (device->dfu_state) {
		case DFU_STATE_appDETACH:
			device->dfu_state = DFU_STATE_dfuIDLE;
			debug("DFU: Switching to DFU Mode\n");
			break;
		case DFU_STATE_dfuMANIFEST_WAIT_RST:
			device->dfu_state = DFU_STATE_appIDLE;
			debug("DFU: Switching back to Runtime mode\n");
			break;
		default:
			break;
		}
		break;
	case DEVICE_CONFIGURED:
	case DEVICE_DE_CONFIGURED:
		debug("SET_CONFIGURATION(%u) ", device->configuration);
		/* fallthrough */
	case DEVICE_SET_INTERFACE:
		debug("SET_INTERFACE(%u,%u) old_state = %u ",
			device->interface, device->alternate,
			device->dfu_state);
		switch (device->dfu_state) {
		case DFU_STATE_appIDLE:
		case DFU_STATE_appDETACH:
		case DFU_STATE_dfuIDLE:
		case DFU_STATE_dfuMANIFEST_WAIT_RST:
			/* do nothing, we're fine */
			break;
		case DFU_STATE_dfuDNLOAD_SYNC:
		case DFU_STATE_dfuDNBUSY:
		case DFU_STATE_dfuDNLOAD_IDLE:
		case DFU_STATE_dfuMANIFEST:
			device->dfu_state = DFU_STATE_dfuERROR;
			device->dfu_status = DFU_STATUS_errNOTDONE;
			/* FIXME: free malloc()ed buffer! */
			break;
		case DFU_STATE_dfuMANIFEST_SYNC:
		case DFU_STATE_dfuUPLOAD_IDLE:
		case DFU_STATE_dfuERROR:
			device->dfu_state = DFU_STATE_dfuERROR;
			device->dfu_status = DFU_STATUS_errUNKNOWN;
			break;
		}
		/* reset data toggle on bulk endpoints */
		udc_endpoint_feature(device, DFU_RX_EP /* FIXME: rx_endpoint */, 0);
		//udc_endpoint_feature(device, DFU_TX_EP /* FIXME: tx_endpoint */, 0);
		debug("new_state = %u\n", device->dfu_state);
		break;
	default:
		break;
	}
}

void dfu_poll(void)
{
	struct usb_device_instance *dev = dfu_usb_dev;
	struct dnload_state *ds = &_dnstate;
	char buf[32];
	char *s;

	if (!dev)
		return;

	switch (dev->dfu_state) {
	case DFU_STATE_dfuDNLOAD_SYNC:
	case DFU_STATE_dfuDNBUSY:
		if (ds->remain) {
			struct usb_endpoint_instance *endpoint =
				dev->bus->endpoint_array + DFU_RX_EP; /* FIXME: rx_endpoint */
			unsigned long flags;

			/* look in the receive bulk endpoint for data */
			local_irq_save(flags);
			if (endpoint->rcv_urb && endpoint->rcv_urb->actual_length) {
				unsigned int len;

				len = MIN(endpoint->rcv_urb->actual_length, ds->remain);
				memcpy(ds->ptr, endpoint->rcv_urb->buffer, len);
				endpoint->rcv_urb->actual_length -= len;
				ds->ptr += len;
				ds->remain -= len;
			}
			local_irq_restore(flags);
		}
		break;

	case DFU_STATE_dfuMANIFEST_SYNC:
		dev->dfu_state = DFU_STATE_dfuMANIFEST;
		sprintf(buf, "%x", (ds->ptr - dfu_loadaddr));
		setenv("filesize", buf);

		/* cleanup */
		switch (dev->alternate) {
		case 0:
			sprintf(buf, "dfu_manifest");
			ds->ptr = ds->buf;
			break;
		default:
			if (partnames[dev->alternate-1])
				sprintf(buf, "dfu_manifest_%s", partnames[dev->alternate-1]);
			else
				sprintf(buf, "dfu_manifest_%d", dev->alternate);
			break;
		}

		/* execute dfu_manifest command, e.g.
		 * "erase <start> <end> cp.b <dfu_loadaddr> <start> ${filesize}" */
		s = getenv(buf);
		if (s) {
			printf("Executing '%s' command\n", buf);
			int prev = disable_ctrlc(1);
#ifndef CONFIG_SYS_HUSH_PARSER
			run_command(s, 0);
#else
			parse_string_outer(s, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
#endif
			disable_ctrlc(prev);
		} else {
			printf("No '%s' command set!\n", buf);
		}

		dfu_active = 0;
		dfu_start_time = get_timer(0);

		dev->dfu_state = DFU_STATE_dfuIDLE;
		break;
	default:
		break;
	}
}

#endif /* CONFIG_USBD_DFU */
