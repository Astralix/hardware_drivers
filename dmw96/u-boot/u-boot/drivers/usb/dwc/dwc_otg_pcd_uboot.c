/* ==========================================================================
 * $File: $
 * $Revision: $
 * $Date: $
 * $Change: $
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * Ported from dwc_otg_pcd_linux.c.
 * Portions Copyright (c) 2011 Intelligraphics, Inc.
 * stephen.smith@intelligraphics.com
 * ========================================================================== */
#ifndef DWC_HOST_ONLY

/** @file
 * This file implements the Peripheral Controller Driver.
 *
 * The Peripheral Controller Driver (PCD) is responsible for
 * translating requests from the Function Driver into the appropriate
 * actions on the DWC_otg controller. It isolates the Function Driver
 * from the specifics of the controller by providing an API to the
 * Function Driver.
 *
 * The Peripheral Controller Driver for Linux will implement the
 * Gadget API, so that the existing Gadget drivers can be used.
 * (Gadget Driver is the Linux terminology for a Function Driver.)
 *
 * The Linux Gadget API is defined in the header file
 * <code><linux/usb_gadget.h></code>.  The USB EP operations API is
 * defined in the structure <code>usb_ep_ops</code> and the USB
 * Controller API is defined in the structure
 * <code>usb_gadget_ops</code>.
 *
 */

#include <common.h>

#include <usbdevice.h>

#include "dwc_otg_os_dep.h"
#include "dwc_otg_dbg.h"
#include "dwc_otg_pcd_if.h"
#include "dwc_otg_pcd.h"

extern dwc_otg_device_t _otg_dev, *otg_dev;
extern struct usb_device_instance *udc_device;

int dwc_otg_pcd_uboot_ep_queue(unsigned int ep, void *buf, int length)
{
	int retval;

	DWC_DEBUGPL(DBG_PCDV, "%s(%d,%p,%d)\n", __func__, ep, buf, length);

	retval = dwc_otg_pcd_ep_queue(otg_dev->pcd, (void *) ep, buf, (dwc_dma_t) NULL, length, 0, NULL, 1);
	if (retval != 0) {
		DWC_ERROR("%s:dwc_otg_pcd_ep_queue failed, error %d.\n", __func__, retval);
	}

	return retval;
}

int dwc_otg_pcd_uboot_ep_set_stall(unsigned int ep)
{
	int retval;

	DWC_DEBUGPL(DBG_PCDV, "%s(%d)\n", __func__, ep);

	if (ep != 0) {
		retval = dwc_otg_pcd_ep_set_stall(otg_dev->pcd, (void *) ep);
	}
	else {
		DWC_ERROR("%s: attempt to stall EP0!\n", __func__);
		retval = -EINVAL;
	}

	return retval;
}

int dwc_otg_pcd_uboot_ep_clear_stall(unsigned int ep)
{
	int retval;

	if (ep != 0) {
		retval = dwc_otg_pcd_ep_clear_stall(otg_dev->pcd, (void *) ep);
	}
	else {
		DWC_ERROR("%s: attempt to clear stall EP0!\n", __func__);
		retval = -EINVAL;
	}
	return retval;
}

int dwc_otg_pcd_uboot_ep_disable(unsigned int ep)
{
        int retval;

        DWC_DEBUGPL(DBG_PCDV, "%s(%d)\n", __func__, ep);

        retval = dwc_otg_pcd_ep_disable(otg_dev->pcd, (void *) ep);
        if (retval) {
                retval = -EINVAL;
        }

        return retval;
}

int dwc_otg_pcd_uboot_ep_enable(struct usb_endpoint_descriptor *ep_desc)
{
	int ep;
	int retval;

	ep = ep_desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;

	DWC_DEBUGPL(DBG_PCDV, "%s(%d,%p)\n", __func__, ep, ep_desc);

	if (ep == 0) {
		/* DWC sets up EP0 automatically */
		return 0;
	}

	dwc_otg_pcd_uboot_ep_disable((void *) ep);
	retval = dwc_otg_pcd_ep_enable(otg_dev->pcd, (const uint8_t *) ep_desc, (void *) ep);
	if (retval) {
		DWC_ERROR("%s:dwc_otg_pcd_ep_enable failed, error %d.\n", __func__, retval);
		return -EINVAL;
	}

	return retval;
}

static int _setup(dwc_otg_pcd_t * pcd, uint8_t * bytes)
{
	extern int udc_setup(usb_device_request_t *pctrl);
	extern void udc_set_ep0_urb(usb_device_request_t *pctrl);
	extern int udc_prepare_ep0_urb_for_data(int wLength);
	extern int udc_queue_ep0_urb_for_data(int wLength);
	extern int udc_queue_ep0_urb(void);

	int retval;

	DWC_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, pcd);

	DWC_DEBUGPL(DBG_PCD,
			"SETUP PKT: %02x.%02x v%04x i%04x l%04x\n",
			pcd->setup_pkt->req.bmRequestType,
			pcd->setup_pkt->req.bRequest,
			UGETW(pcd->setup_pkt->req.wValue),
			UGETW(pcd->setup_pkt->req.wIndex),
			UGETW(pcd->setup_pkt->req.wLength));

	if (pcd->ep0state == EP0_OUT_DATA_PHASE) {
		DWC_DEBUGPL(DBG_PCD, "%s:EP0_OUT_DATA_PHASE\n", __func__);
		retval = udc_queue_ep0_urb_for_data(UGETW(pcd->setup_pkt->req.wLength));
	} else {
		retval = udc_setup(&pcd->setup_pkt->req);
		if (retval == 0) {
			retval = udc_queue_ep0_urb();
		}
	}

	return retval;
}

static int _complete(dwc_otg_pcd_t *pcd, void *ep_handle,
		void *req_handle, int32_t status, uint32_t actual)
{
	extern int udc_complete(int ep_num, int32_t status, uint32_t actual);
	int retval;

	DWC_DEBUGPL(DBG_PCDV, "%s(%p,%d,%p,%d,%d)\n", __func__, pcd, (int) ep_handle, req_handle, status, actual);

	retval = udc_complete((int) ep_handle, status, actual);

	return retval;
}

static int _connect(dwc_otg_pcd_t * pcd, int speed)
{
	DWC_DEBUGPL(DBG_PCDV, "%s(%p, %d)\n", __func__, pcd, speed);

	return 0;
}

static int _disconnect(dwc_otg_pcd_t * pcd)
{
	DWC_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, pcd);

	usbd_device_event_irq(udc_device, DEVICE_RESET, 0);

	return 0;
}

static int _resume(dwc_otg_pcd_t * pcd)
{
	DWC_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, pcd);

	usbd_device_event_irq(udc_device, DEVICE_BUS_ACTIVITY, 0);

	return 0;
}

static int _suspend(dwc_otg_pcd_t * pcd)
{
	DWC_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, pcd);

	usbd_device_event_irq(udc_device, DEVICE_BUS_INACTIVE, 0);

	return 0;
}

static int _reset(dwc_otg_pcd_t * pcd)
{
	DWC_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, pcd);

	usbd_device_event_irq(udc_device, DEVICE_RESET, 0);

	return 0;
}

static const struct dwc_otg_pcd_function_ops fops = {
	.connect = _connect
		, .disconnect = _disconnect
		, .setup = _setup
		, .complete = _complete
		, .isoc_complete = (dwc_isoc_completion_cb_t) 0
		, .suspend = _suspend
		, .sleep = (dwc_sleep_cb_t) 0
		, .resume = _resume
		, .reset = _reset
		, .hnp_changed = (dwc_hnp_params_changed_cb_t) 0
		, .cfi_setup = (cfi_setup_cb_t) 0
#ifdef DWC_UTE_PER_IO
		, .xisoc_complete = (xiso_completion_cb_t) 0
#endif
};

/**
 * This function is the top level PCD interrupt handler.
 */
void dwc_otg_pcd_irq(void)
{
	int32_t retval;

	retval = dwc_otg_pcd_handle_intr(otg_dev->pcd);
	if (retval != 0) {
		S3C2410X_CLEAR_EINTPEND();
	}
}

/**
 * This function initialized the PCD portion of the driver.
 *
 */
int pcd_init(dwc_otg_device_t *otg_dev)
{
	int retval = 0;

	DWC_DEBUGPL(DBG_PCDV, "%s otg_dev:%p\n", __func__, otg_dev);

	otg_dev->pcd = dwc_otg_pcd_init(otg_dev, otg_dev->core_if);

	if (!otg_dev->pcd) {
		DWC_ERROR("dwc_otg_pcd_init failed\n");
		return -ENOMEM;
	}

	dwc_otg_pcd_start(otg_dev->pcd, &fops);

	return retval;
}

/**
 * Cleanup the PCD.
 */
void pcd_remove(dwc_otg_device_t *otg_dev)
{
	DWC_DEBUGPL(DBG_PCDV, "%s otg_dev:0x%08x\n", __func__, (uint32_t) otg_dev);

	dwc_otg_pcd_remove(otg_dev->pcd);
	otg_dev->pcd = 0;
}

#endif /* DWC_HOST_ONLY */
