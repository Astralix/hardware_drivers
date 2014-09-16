/*
 * (C) Copyright 2011 DSP Group
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
 */

#ifndef __DWC_OTG_UDC_H
#define __DWC_OTG_UDC_H

#include <usbdevice.h>

/* usbtty */
#ifdef CONFIG_USB_TTY
#define EP0_MAX_PACKET_SIZE     64 /* DWC_EP0_FIFOSIZE */
#define UDC_INT_ENDPOINT        3
#define UDC_INT_PACKET_SIZE     16
#define UDC_OUT_ENDPOINT        2
#define UDC_OUT_PACKET_SIZE     64
#define UDC_IN_ENDPOINT         1
#define UDC_IN_PACKET_SIZE      64
#define UDC_BULK_PACKET_SIZE    512
#endif /* CONFIG_USB_TTY */

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

void dwc_otg_init(unsigned long reg_addr);

#endif /* __DWC_OTG_UDC_H */
