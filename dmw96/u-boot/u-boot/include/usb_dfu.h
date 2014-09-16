#ifndef _DFU_H
#define _DFU_H

/* USB Device Firmware Update Implementation for u-boot
 * (C) 2007 by OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * based on: USB Device Firmware Update Implementation for OpenPCD
 * (C) 2006 by Harald Welte <hwelte@hmw-consulting.de>
 *
 * This ought to be compliant to the USB DFU Spec 1.0 as available from
 * http://www.usb.org/developers/devclass_docs/usbdfu10.pdf
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <asm/types.h>
#include <usbdescriptors.h>
#include <usb_dfu_descriptors.h>
#include <config.h>
#include <usbdevice.h>


#define EP_OUT		UDC_OUT_ENDPOINT
#define EP_IN		UDC_IN_ENDPOINT
#define EP_OUT_ADDR	(EP_OUT | USB_DIR_OUT)
#define EP_IN_ADDR	(EP_IN | USB_DIR_IN)
#define EP_OUT_MAXPACKET	UDC_BULK_PACKET_SIZE
#define EP_IN_MAXPACKET		UDC_BULK_PACKET_SIZE


#define DFU_NUM_ENDPOINTS 	2

/* USB DFU functional descriptor */
#define DFU_FUNC_DESC  {						\
	.bLength		= USB_DT_DFU_SIZE,			\
	.bDescriptorType	= USB_DT_DFU,				\
	.bmAttributes		= USB_DFU_CAN_UPLOAD | USB_DFU_CAN_DOWNLOAD | USB_DFU_MANIFEST_TOL, \
	.wDetachTimeOut		= 0xff00,				\
	.wTransferSize		= CONFIG_USBD_DFU_XFER_SIZE,		\
	.bcdDFUVersion		= 0x0100,				\
}

/* USB Interface descriptor in Runtime mode */
#define DFU_RT_IF_DESC	{						\
	.bLength		= USB_DT_INTERFACE_SIZE,		\
	.bDescriptorType	= USB_DT_INTERFACE,			\
	.bInterfaceNumber	= CONFIG_USBD_DFU_INTERFACE,		\
	.bAlternateSetting	= 0x00,					\
	.bNumEndpoints		= 0x00,					\
	.bInterfaceClass	= 0xfe,					\
	.bInterfaceSubClass	= 0x01,					\
	.bInterfaceProtocol	= 0x01,					\
	.iInterface		= DFU_STR_CONFIG,			\
}

#define DFU_RT_IF_ASSOC_BULK_DESC	{				\
	.bLength		= USB_DT_INTERFACE_ASSOC_SIZE,		\
	.bDescriptorType	= USB_DT_INTERFACE_ASSOCIATION,		\
	.bFirstInterface	= 0x00,					\
	.bInterfaceCount	= 0x02,					\
	.bFunctionClass		= 0xfe,					\
	.bFunctionSubClass	= 0x01,					\
	.bFunctionProtocol	= 0x02,					\
	.iFunction		= 0x00,					\
}

#define DFU_RT_IF_BULK_DESC	{					\
	.bLength		= USB_DT_INTERFACE_SIZE,		\
	.bDescriptorType	= USB_DT_INTERFACE,			\
	.bInterfaceNumber	= 0x01,					\
	.bAlternateSetting	= 0x00,					\
	.bNumEndpoints		= 0x02,					\
	.bInterfaceClass	= 0xfe,					\
	.bInterfaceSubClass	= 0x01,					\
	.bInterfaceProtocol	= 0xff,					\
	.iInterface		= DFU_STR_CONFIG,			\
}

#define DFU_EP_IN_DESC 	{						\
	.bLength 		= USB_DT_ENDPOINT_SIZE,			\
	.bDescriptorType 	= USB_DT_ENDPOINT,			\
	.bEndpointAddress 	= EP_IN_ADDR,				\
	.bmAttributes 		= USB_ENDPOINT_XFER_BULK,		\
	.wMaxPacketSize 	= EP_IN_MAXPACKET,			\
	.bInterval		= 0x0,					\
}

#define DFU_EP_OUT_DESC 	{						\
	.bLength 		= USB_DT_ENDPOINT_SIZE,			\
	.bDescriptorType 	= USB_DT_ENDPOINT,			\
	.bEndpointAddress 	= EP_OUT_ADDR,				\
	.bmAttributes 		= USB_ENDPOINT_XFER_BULK,		\
	.wMaxPacketSize 	= EP_OUT_MAXPACKET,			\
	.bInterval		= 0x0,					\
}

#define ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))

#define STR_LANG		0x00
#define STR_MANUFACTURER	0x01
#define STR_PRODUCT		0x02
#define STR_SERIAL		0x03
#define STR_CONFIG		0x04
#define STR_DATA_INTERFACE	0x05
#define STR_CTRL_INTERFACE	0x06
#define STR_COUNT		0x07

#ifndef DFU_NUM_ALTERNATES
#define DFU_NUM_ALTERNATES	12
#endif

#define DFU_STR_MANUFACTURER	STR_MANUFACTURER
#define DFU_STR_PRODUCT		STR_PRODUCT
#define DFU_STR_SERIAL		STR_SERIAL
#define DFU_STR_CONFIG		(STR_COUNT)
#define DFU_STR_ALT(n)		(STR_COUNT+(n)+1)
#define DFU_STR_COUNT		DFU_STR_ALT(DFU_NUM_ALTERNATES)

#define NUM_STRINGS		DFU_STR_COUNT

#define CONFIG_DFU_CFG_STR	"USB Device Firmware Upgrade"
#define CONFIG_DFU_ALT0_STR	"RAM 0x63000000"

#define CONFIG_USBD_CONFIGURATION_STR "TTY via USB"

#define CONFIG_USBD_SERIAL_OUT_ENDPOINT UDC_OUT_ENDPOINT
#define CONFIG_USBD_SERIAL_OUT_PKTSIZE	UDC_OUT_PACKET_SIZE
#define CONFIG_USBD_SERIAL_IN_ENDPOINT	UDC_IN_ENDPOINT
#define CONFIG_USBD_SERIAL_IN_PKTSIZE	UDC_IN_PACKET_SIZE
#define CONFIG_USBD_SERIAL_INT_ENDPOINT UDC_INT_ENDPOINT
#define CONFIG_USBD_SERIAL_INT_PKTSIZE	UDC_INT_PACKET_SIZE
#define CONFIG_USBD_SERIAL_BULK_PKTSIZE	UDC_BULK_PACKET_SIZE

#define USBTTY_DEVICE_CLASS	COMMUNICATIONS_DEVICE_CLASS

#define USBTTY_BCD_DEVICE	0x00
#define USBTTY_MAXPOWER		0x00

/*
struct usb_interface_instance_dfu {
	struct usb_interface_descriptor uif_alt ;
	struct usb_endpoint_descriptor data_endpoints[DFU_NUM_ENDPOINTS] ;
}__attribute__((packed));
*/

#if defined(CONFIG_USBD_DFUF) && defined(CONFIG_USBD_DFUF_SI)
struct _dfu_alt_desc {
	struct usb_interface_descriptor uif;
	struct usb_endpoint_descriptor ep_out_dfu;
	struct usb_endpoint_descriptor ep_in_dfu;
}__attribute__((packed));
#endif

//IN and OUT
struct _dfu_desc {
	struct usb_configuration_descriptor ucfg;
#if defined(CONFIG_USBD_DFUF) && defined(CONFIG_USBD_DFUF_SI)
	struct _dfu_alt_desc alternates[DFU_NUM_ALTERNATES];
#else
	struct usb_interface_assoc_descriptor uif_assoc_bulk_dfu;
	struct usb_interface_descriptor uif[DFU_NUM_ALTERNATES];
	struct usb_interface_descriptor uif_bulk_dfu;
	struct usb_endpoint_descriptor ep_out_dfu;
	struct usb_endpoint_descriptor ep_in_dfu;
#endif
	struct usb_dfu_func_descriptor func_dfu;
}__attribute__((packed));

int dfu_init_instance(struct usb_device_instance *dev);

#define DFU_EP0_NONE		0
#define DFU_EP0_UNHANDLED	1
#define DFU_EP0_STALL		2
#define DFU_EP0_ZLP		3
#define DFU_EP0_DATA		4

extern volatile enum dfu_state *system_dfu_state; /* for 3rd parties */

int dfu_ep0_handler(struct urb *urb);
void dfu_event(struct usb_device_instance *device,
	       usb_device_event_t event, int data);

#endif /* _DFU_H */
