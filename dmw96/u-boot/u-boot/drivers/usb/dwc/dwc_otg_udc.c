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
 * (C) Copyright 2009-2011 Intelligraphics Inc.
 * Release History: 

 * Atanas (Tony) Zlatinski - port to DSPG DW74 chipset
 * 	- re-implementation of EP 0 and USB state machine
 * stephen.smith@intelligraphics.com
 * 	- rewritten for DWC for DSPG DMW96 chipset
 */

#include <common.h>

#include <usbdevice.h>
#include <usb/dwc_otg_udc.h>
#include <dspg_otg_udc.h>

#include "../gadget/ep0.h"

#include "dwc_os.h"
#include "dwc_otg_dbg.h"
#include "dwc_otg_pcd_uboot.h"
#include "dwc_otg_pcd.h"
#include "dwc_otg_platform.h"
#include "dwc_otg_driver.h"

/* Private variables */
struct usb_device_instance *udc_device;
static struct urb *ep0_urb = NULL;
static struct usb_endpoint_instance *local_ep_requeue_urb;

extern void dump_msg(const u8 * buf, unsigned int length);

void udc_usb_endpoint_instance_to_usb_endpoint_descriptor(
		struct usb_endpoint_instance *endpoint,
		struct usb_endpoint_descriptor *ep_desc
		)
{
	if (endpoint->rcv_attributes !=  endpoint->tx_attributes)
		DWC_ERROR("%s: endpoint rcv_attributes != tx_attributes!\n", __func__);
	if (endpoint->rcv_packetSize !=  endpoint->tx_packetSize)
		DWC_ERROR("%s: endpoint rcv_packetSize != tx_packetSize!\n", __func__);

	memset(ep_desc, 0, sizeof(*ep_desc));

	ep_desc->bLength = USB_DT_ENDPOINT_SIZE;
	ep_desc->bDescriptorType = USB_DT_ENDPOINT;
	ep_desc->bEndpointAddress = endpoint->endpoint_address;
	ep_desc->bmAttributes = endpoint->rcv_attributes;
	ep_desc->wMaxPacketSize = endpoint->rcv_packetSize;
}

int udc_queue_ep0_urb_for_data(int wLength)
{
	int retval;

	DWC_DEBUGPL(DBG_UDC, "%s:setting up buffer to get data\n", __func__);

#ifdef DEBUG
	memset(ep0_urb->buffer_data, 0, sizeof(ep0_urb->buffer_data));
#endif

	ep0_urb->buffer = ep0_urb->buffer_data;
	ep0_urb->buffer_length = wLength;
	ep0_urb->actual_length = ep0_urb->buffer_length;

	if (ep0_urb->buffer_length > sizeof(ep0_urb->buffer_data)) {
		DWC_ERROR("%s: wLength %d exceeds maximum buffer size %d!\n", __func__,
				ep0_urb->buffer_length, sizeof(ep0_urb->buffer_data));
		return -EINVAL;
	}

	retval = dwc_otg_pcd_uboot_ep_queue(0, ep0_urb->buffer, ep0_urb->buffer_length);

	return retval;
}

int udc_queue_ep0_urb(void)
{
	int retval;

	DWC_DEBUGPL(DBG_UDC, "%s: buffer:%p buffer_length:%d actual_length:%d\n", __func__,
			ep0_urb->buffer, ep0_urb->buffer_length, ep0_urb->actual_length);
	dump_msg(ep0_urb->buffer, ep0_urb->actual_length);

	retval = dwc_otg_pcd_uboot_ep_queue(0, ep0_urb->buffer, ep0_urb->actual_length);
	if (retval) {
		DWC_ERROR("dwc_otg_pcd_uboot_ep_queue() returned %d\n", retval);
	}
	ep0_urb->buffer = ep0_urb->buffer_data;
	ep0_urb->buffer_length = sizeof(ep0_urb->buffer_data);
	ep0_urb->actual_length = 0;

	return retval;
}

int udc_find_endpoint_from_epnum(struct usb_device_instance *dev, int epid)
{
	int i;

	for (i = 0; i < udc_device->bus->max_endpoints; i++) {
		if ((dev->bus->endpoint_array[i].endpoint_address & USB_ENDPOINT_NUMBER_MASK) == epid)
			return i;
	}

	return -1;
}

int udc_endpoint_write(struct usb_endpoint_instance *endpoint)
{
	struct urb *urb;
	unsigned int ep;
	int size;
	int retval = 0;

	urb = endpoint->tx_urb;
	if (! urb) {
		return -EINVAL;
	}

	ep = (endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK);

	DWC_DEBUGPL(DBG_UDC, "%s(%p) ep:%d urb:%p actual_length:%d sent:%d tx_packetSize:%d\n", __func__,
			endpoint, ep, urb, urb->actual_length, endpoint->sent, endpoint->tx_packetSize);

	/* Transmit only if the hardware is available */
	if (endpoint->state == 0 && urb->actual_length != 0) {
		endpoint->state = 1; /* Transmit hardware is busy */

		size = MIN(urb->actual_length - endpoint->sent, endpoint->tx_packetSize);
		dump_msg(urb->buffer + endpoint->sent, size);
		retval = dwc_otg_pcd_uboot_ep_queue(ep, urb->buffer + endpoint->sent, size);
	} else {
		DWC_ERROR("%s: unable to send, tx_urb:%p  state:%d\n", __func__, urb, endpoint->state);
		usbd_tx_complete(endpoint);

		retval = -EINVAL;
	}

	return retval;
}

int udc_complete(int ep_num, int32_t status, uint32_t actual)
{
	struct usb_endpoint_instance *endpoint;
	struct urb *urb;
	int ep;
	int retval = 0;

	ep = udc_find_endpoint_from_epnum(udc_device, ep_num);
	if (ep == -1) {
		DWC_ERROR("Invalid EP (EP%d)", ep_num);
		return -EINVAL;
	}
	endpoint = &udc_device->bus->endpoint_array[ep];

	DWC_DEBUGPL(DBG_PCDV, "%s(%d,%d,%d) endpoint:%p [%d]\n", __func__,
			ep_num, status, actual, endpoint, ep);

	if (endpoint->endpoint_address & USB_DIR_IN) {
		DWC_DEBUGPL(DBG_PCD, "for IN EP%d, urb:%p actual:%d sent:%d last:%d\n",
				ep_num, endpoint->tx_urb, actual, endpoint->sent, endpoint->last);

		endpoint->state = 0; /* Transmit hardware is free */
		endpoint->last = actual;
		if (status == 0) {
			usbd_tx_complete(endpoint);
			urb = endpoint->tx_urb;
			if (urb && urb->actual_length) {
				DWC_DEBUGPL(DBG_PCD, "actual_length:%d\n", urb->actual_length);
				retval = udc_endpoint_write(endpoint);
			}
		}
	} else {
		DWC_DEBUGPL(DBG_PCD, "OUT EP%d, urb:%p\n", ep, endpoint->rcv_urb);

		urb = endpoint->rcv_urb;
		if (urb && status == 0) {
			DWC_DEBUGPL(DBG_PCD, "buf_len=%d, act_len=%d actual:%d\n",
					urb->buffer_length, urb->actual_length, actual);

			endpoint->state = 0;

			dump_msg(urb->buffer + urb->actual_length, actual);

			/* we're given a complete transfer, i.e., 1 or more packets.
			    usbd_rcv_complete() expects to be called every packet, which
			    all it does is bump up the actual length.  it will error if
			    the actual exceeds a packet size.
			   so we'll just maintain actual_length ourselves.
			*/
#if 0
			usbd_rcv_complete(endpoint, actual, status);
#else
			urb->actual_length += actual;
#endif

			DWC_DEBUGPL(DBG_PCD, "buf_len=%d, act_len=%d actual:%d\n",
					urb->buffer_length, urb->actual_length, actual);


			/* kludge until I can figure out how to chain URBs for BULK OUT EP */
#if 1
			if (!local_ep_requeue_urb) {
				local_ep_requeue_urb = endpoint;
			} else {
				DWC_ERROR("local_ep_requeue_urb in use!");
				retval = -EINVAL;
			}
#else
			if (urb->actual_length == 0) {
				/* urb data has been processed, queue up another */
				endpoint->state = 1;
				retval = dwc_otg_pcd_uboot_ep_queue(ep, urb->buffer, urb->buffer_length);
			}
#endif

		}
	}

	return retval;
}

int udc_endpoint_feature(struct usb_device_instance *dev, int ep, int enable)
{
	struct usb_endpoint_instance *endpoint;

	endpoint = &udc_device->bus->endpoint_array[ep];

	if (enable) {
		dwc_otg_pcd_uboot_ep_set_stall(endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK);
	} else {
		dwc_otg_pcd_uboot_ep_clear_stall(endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK);
	}

	return 0;
}

int udc_setup(usb_device_request_t *pctrl)
{
	extern int ep0_recv_setup(struct urb *urb);
	int retval = 0;

	DWC_DEBUGPL(DBG_UDC, "%s: %p\n", __func__, pctrl);

	memcpy(&ep0_urb->device_request, pctrl, sizeof(ep0_urb->device_request));

	if ((pctrl->bmRequestType & USB_REQ_DIRECTION_MASK) == USB_REQ_HOST2DEVICE) {
		DWC_DEBUGPL(DBG_UDC, "%s: DIRECTION_MASK: USB_REQ_HOST2DEVICE\n", __func__);

		/* There are some requests with which we need to deal manually here */
		switch (pctrl->bRequest)
		{
			case USB_REQ_SET_CONFIGURATION:
				DWC_DEBUGPL(DBG_UDC, "%s: USB_REQ_SET_CONFIGURATION (%d)\n", __func__, UGETW(pctrl->wValue));
				if (UGETW(pctrl->wValue) == 0)
					usbd_device_event_irq(udc_device, DEVICE_DE_CONFIGURED, 0);
				else
					usbd_device_event_irq(udc_device, DEVICE_CONFIGURED, 0);
				break;
			case USB_REQ_SET_ADDRESS:
				DWC_DEBUGPL(DBG_UDC, "%s: USB_REQ_SET_ADDRESS\n", __func__);
				usbd_device_event_irq(udc_device, DEVICE_ADDRESS_ASSIGNED, 0);
				break;
			default:
				DWC_DEBUGPL(DBG_UDC, "%s: unhandled bRequest 0x%x\n", __func__, pctrl->bRequest);
				break;
		}

	} else {
		DWC_DEBUGPL(DBG_UDC, "%s: DIRECTION_MASK: USB_REQ_DEVICE2HOST\n", __func__);
	}

	if (ep0_recv_setup(ep0_urb)) {
		/* Not a setup packet, stall next EP0 transaction */
		DWC_ERROR("can't parse setup packet\n");
		retval = -EINVAL;
	}

	return retval;
}

/* Public functions - called by usbdcore, usbtty, etc. */
void udc_irq(void)
{
	int retval;

	dwc_otg_pcd_irq();
	dwc_otg_common_irq();

	/* kludge until I can figure out how to chain URBs for BULK OUT EP */

	/* we have to wait until the main poll loop processes the URB until we requeue it */
	if (local_ep_requeue_urb && local_ep_requeue_urb->state == 0) {
		if (local_ep_requeue_urb->rcv_urb->actual_length == 0) {
			/* urb data has been processed, queue up another */
			DWC_DEBUGPL(DBG_UDC, "%s: requeueing URB\n", __func__);

			local_ep_requeue_urb->state = 1;
			retval = dwc_otg_pcd_uboot_ep_queue(
					udc_find_endpoint_from_epnum(
						udc_device,
						local_ep_requeue_urb->endpoint_address & USB_ENDPOINT_NUMBER_MASK
						),
					local_ep_requeue_urb->rcv_urb->buffer,
					local_ep_requeue_urb->rcv_urb->buffer_length
					);
			if (retval != 0) {
				DWC_ERROR("dwc_otg_pcd_uboot_ep_queue(0 returned error 0x%08x, stalling EP%d",
					retval, local_ep_requeue_urb->endpoint_address & USB_ENDPOINT_NUMBER_MASK);
				dwc_otg_pcd_uboot_ep_set_stall(
						local_ep_requeue_urb->endpoint_address & USB_ENDPOINT_NUMBER_MASK);
			}
			local_ep_requeue_urb = (struct usb_endpoint_instance *) NULL;
		}
	}
}

/* Turn on the USB connection */
void udc_connect(void)
{
	DWC_DEBUGPL(DBG_UDC, "%s\n", __func__);

	usbd_device_event_irq(udc_device, DEVICE_CREATE, 0);
}

void udc_disable_ep(unsigned int ep_num)
{
	struct usb_endpoint_instance *endpoint;
	int ep;

	ep = udc_find_endpoint_from_epnum(udc_device, ep_num);
	if (ep == -1) {
		DWC_ERROR("Invalid EP (EP%d)", ep_num);
		return;
	}
	endpoint = &udc_device->bus->endpoint_array[ep];

	DWC_DEBUGPL(DBG_UDC, "%s(%d, %p) 0x%02x %d %s %d\n", __func__,
			ep, endpoint, endpoint->endpoint_address, endpoint->state,
			(endpoint->endpoint_address & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT ? "OUT" : "IN",
			endpoint->state);

	if (endpoint->state != 0) {
		endpoint->state = 0;
	}
}

/* Turn off the USB connection */
void udc_disconnect(void)
{
	struct usb_endpoint_instance *endpoint;
	int i;

	DWC_DEBUGPL(DBG_UDC, "%s\n", __func__);

	/* since any pending URBs have been cancelled, reset endpoint state */
	for (i = 0; i < sizeof(udc_device->bus->endpoint_array) / sizeof(udc_device->bus->endpoint_array[0]); i++) {
		endpoint = &udc_device->bus->endpoint_array[i];
		if (endpoint->state != 0) {
			endpoint->state = 0;
		}
	}

	usbd_device_event_irq(udc_device, DEVICE_RESET, 0);
}

/*
 * udc_setup_ep - setup endpoint
 *
 * Associate a physical endpoint with endpoint_instance
 */
void udc_setup_ep(struct usb_device_instance *device, unsigned int ep,
		struct usb_endpoint_instance *endpoint)
{
	struct usb_endpoint_descriptor ep_desc;
	int retval;

	DWC_DEBUGPL(DBG_UDC, "%s(%p, %d, %p) 0x%02x %d %s\n", __func__,
			device, ep, endpoint, endpoint->endpoint_address, endpoint->state,
			(endpoint->endpoint_address & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT ? "OUT" : "IN");

	udc_usb_endpoint_instance_to_usb_endpoint_descriptor(endpoint, &ep_desc);
	dwc_otg_pcd_uboot_ep_enable(&ep_desc);

	endpoint->state = 0;
	if ((endpoint->endpoint_address & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT) {
		DWC_DEBUGPL(DBG_UDC, "%s: buffer:%p actual_length:%d\n", __func__,
				endpoint->rcv_urb->buffer, endpoint->rcv_urb->actual_length);
		retval = dwc_otg_pcd_uboot_ep_queue(ep, endpoint->rcv_urb->buffer, endpoint->rcv_urb->buffer_length);
		if (retval == 0) {
			endpoint->state = 1;
		}
	}
}

/*
 * udc_startup_events - allow udc code to do any additional startup
 */
void udc_startup_events(struct usb_device_instance *device)
{
	DWC_DEBUGPL(DBG_UDC, "%s\n", __func__);

	/* Save the device structure pointer */
	udc_device = device;

	/* The DEVICE_INIT event puts the USB device in the state STATE_INIT. */
	usbd_device_event_irq(device, DEVICE_INIT, 0);

	/*
	 * Some USB controller driver implementations signal
	 * DEVICE_HUB_CONFIGURED and DEVICE_RESET events here.
	 * DEVICE_HUB_CONFIGURED causes a transition to the state STATE_POWERED,
	 * and DEVICE_RESET causes a transition to the state STATE_DEFAULT.
	 * The DWC client controller has the capability to detect when the
	 * USB cable is connected to a powered USB bus, so we will defer the
	 * DEVICE_HUB_CONFIGURED and DEVICE_RESET events until later.
	 */

	/* Setup ep0 urb */
	if (!ep0_urb) {
		ep0_urb = usbd_alloc_urb(udc_device, udc_device->bus->endpoint_array);

		DWC_DEBUGPL(DBG_UDC, "%s: buffer:%p buffer_data:%p buffer_length:%d actual_length:%d\n", __func__,
				ep0_urb->buffer, &ep0_urb->buffer_data, ep0_urb->buffer_length, ep0_urb->actual_length);

		ep0_urb->buffer = ep0_urb->buffer_data;
		ep0_urb->buffer_length = sizeof(ep0_urb->buffer_data);
		ep0_urb->actual_length = 0;

	} else {
		DWC_DEBUGPL(DBG_UDC, "%s: ep0_urb already allocated %p\n", __func__, ep0_urb);
	}
}

void udc_set_nak(int epid)
{
	DWC_DEBUGPL(DBG_UDC, "%s\n", __func__);
}

void udc_unset_nak(int epid)
{
	struct usb_endpoint_instance *endpoint;
	int ep;
	int retval;

	ep = udc_find_endpoint_from_epnum(udc_device, epid);
	if (ep != -1) {
		endpoint = &udc_device->bus->endpoint_array[ep];
		if (endpoint->state == 0 && (endpoint->endpoint_address & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT) {
			DWC_DEBUGPL(DBG_UDC, "%s: buffer:%p actual_length:%d\n", __func__,
					endpoint->rcv_urb->buffer, endpoint->rcv_urb->actual_length);
			retval = dwc_otg_pcd_uboot_ep_queue(ep, endpoint->rcv_urb->buffer, endpoint->rcv_urb->buffer_length);
			if (retval == 0) {
				endpoint->state = 1;
			}
		}
	}
}

int udc_init(void)
{
	int retval;

	DWC_DEBUGPL(DBG_UDC, "%s\n", __func__);

	retval = dwc_otg_platform_init();
	if (retval != 0) {
		DWC_ERROR("dwc_otg_platform_init returned %d!\n", retval);
	}
	return retval;
}
