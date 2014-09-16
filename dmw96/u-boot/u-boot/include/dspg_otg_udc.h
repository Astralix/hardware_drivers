#ifndef __DSPG_OTG_UDC_H__
#define __DSPG_OTG_UDC_H__

#include <config.h>

#define DW74_OTG_USB_BASE		0x09900000	/* Mentor's Base */
#define DW74_OTG_USB_IO_SIZE		SZ_1K

#define DMW96_OTG_USB1_BASE		0x00300000
#define DMW96_OTG_USB2_BASE		0x00400000
#define DWW96_OTG_USB_IO_SIZE		SZ_1K

#if defined(CONFIG_DSPG_DMW)
#  if CONFIG_USB_DWC_PORT==1
#define UDC_BASE			(DMW96_OTG_USB1_BASE)
#  else
#define UDC_BASE			(DMW96_OTG_USB2_BASE)
#  endif
#else
#define UDC_BASE			(DW74_OTG_USB_BASE)
#endif

#endif /* __DSPG_OTG_UDC_H__ */
