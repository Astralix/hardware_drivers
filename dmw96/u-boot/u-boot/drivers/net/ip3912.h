/*
 * ip3912 ethernet driver interface (PNX8181 / firetux)
 *
 * (C) Copyright 2007-2009, emlix GmbH, Germany
 * Juergen Schoew <js@emlix.com>
 *
 * (C) Copyright 2008, DSPG Technologies GmbH, Germany
 * (C) Copyright 2007, NXP Semiconductors Germany GmbH
 * Matthias Wenzel, <nxp@mazzoo.de>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* exported ethernet functions */
int mii_discover_phy(void);

#define PHYADDR_TO_REG(x)			(x << 8)

/* data types */

/* rx */
typedef struct{
	volatile void *packet;
	volatile unsigned long control;
} rx_descriptor_t;

typedef struct{
	volatile unsigned long info;		/* RO */
	volatile unsigned long hashCRC;		/* RO */
} rx_status_t;

/* tx */
typedef struct{
	volatile void *packet;
	volatile unsigned long control;
} tx_descriptor_t;

typedef struct{
	volatile unsigned long info;		/* RO */
} tx_status_t;

/* NXP's OUI registered @ IEEE  */
#define NXP_ETN_OUI			0x006037

/* ip3912 ETN registers */
#define ETN1_BASE			CONFIG_IP3912_ETN1_BASE
#define ETN2_BASE			CONFIG_IP3912_ETN2_BASE

/* offsets to base address */
#define ETN_MAC1			0x0000
#define ETN_MAC2			0x0004
#define ETN_IPGT			0x0008
#define ETN_IPGR			0x000c
#define ETN_CLRT			0x0010
#define ETN_MAXF			0x0014
#define ETN_SUPP			0x0018
#define ETN_TEST			0x001c
#define ETN_MCFG			0x0020
#define ETN_MCMD			0x0024
#define ETN_MADR			0x0028
#define ETN_MWTD			0x002c
#define ETN_MRDD			0x0030
#define ETN_MIND			0x0034
#define ETN_SA0				0x0040
#define ETN_SA1				0x0044
#define ETN_SA2				0x0048
#define ETN_COMMAND			0x0100
#define ETN_STATUS			0x0104
#define ETN_RXDESCRIPTOR		0x0108
#define ETN_RXSTATUS			0x010c
#define ETN_RXDESCRIPTORNUMBER		0x0110
#define ETN_RXPRODUCEINDEX		0x0114
#define ETN_RXCONSUMEINDEX		0x0118
#define ETN_TXDESCRIPTOR		0x011c
#define ETN_TXSTATUS			0x0120
#define ETN_TXDESCRIPTORNUMBER		0x0124
#define ETN_TXPRODUCEINDEX		0x0128
#define ETN_TXCONSUMEINDEX		0x012c
#define ETN_TXRTDESCRIPTOR		0x0130
#define ETN_TXRTSTATUS			0x0134
#define ETN_TXRTDESCRIPTORNUMBER	0x0138
#define ETN_TXRTPRODUCEINDEX		0x013c
#define ETN_TXRTCONSUMEINDEX		0x0140
#define ETN_QOSTIMEOUT			0x0148
#define ETN_TSV0			0x0158
#define ETN_TSV1			0x015c
#define ETN_RSV				0x0160
#define ETN_FLOWCONTROLCOUNTER		0x0170
#define ETN_FLOWCONTROLSTATUS		0x0174
#define ETN_RXFILTERCTRL		0x0200
#define ETN_RXFILTERWOLSTATUS		0x0204
#define ETN_RXFILTERWOLCLEAR		0x0208
#define ETN_HASHFILTERL			0x0210
#define ETN_HASHFILTERH			0x0214
#define ETN_INTSTATUS			0x0fe0
#define ETN_INTENABLE			0x0fe4
#define ETN_INTCLEAR			0x0fe8
#define ETN_INTSET			0x0fec
#define ETN_POWERDOWN			0x0ff4
#define ETN_MODULEID			0x0ffc

/* values for control */
#define ETN_CONTROL_INTERRUPT	0x80000000
#define ETN_CONTROL_LAST	0x40000000
#define ETN_CONTROL_CRC		0x20000000
#define ETN_CONTROL_PAD		0x10000000
#define ETN_CONTROL_HUGE	0x08000000
#define ETN_CONTROL_OVERRIDE	0x04000000

/* registers in the SMSC LAN8700 PHY */
/*
00 Basic Control Register				Basic
01 Basic Status Register				Basic
02 PHY Identifier 1					Extended
03 PHY Identifier 2					Extended
04 Auto-Negotiation Advertisement Register		Extended
05 Auto-Negotiation Link Partner Ability Register	Extended
06 Auto-Negotiation Expansion Register			Extended
16 Silicon Revision Register				Vendor-specific
17 Mode Control/Status Register				Vendor-specific
18 Special Modes					Vendor-specific
20 Reserved						Vendor-specific
21 Reserved						Vendor-specific
22 Reserved						Vendor-specific
23 Reserved						Vendor-specific
27 Control / Status Indication Register			Vendor-specific
28 Special internal testability controls		Vendor-specific
29 Interrupt Source Register				Vendor-specific
30 Interrupt Mask Register				Vendor-specific
31 PHY Special Control/Status Register			Vendor-specific
*/
#define ETN_PHY_BASIC_CONTROL			0x00
#define ETN_PHY_BASIC_STATUS			0x01
#define ETN_PHY_ID1				0x02
#define ETN_PHY_ID2				0x03
#define ETN_PHY_AUTONEG_ADV			0x04
#define ETN_PHY_AUTONEG_LINK			0x05
#define ETN_PHY_AUTONEG_EXP			0x06
#define ETN_PHY_SILICON				0x10
#define ETN_PHY_MODE_CONTROL_STATUS		0x11
#define ETN_PHY_SPECIAL_MODES			0x12
#define ETN_PHY_CONTROL_STATUS_INDICATION	0x1b
#define ETN_PHY_INTERNAL_TESTABILITY		0x1c
#define ETN_PHY_INTERRUPT_SOURCE		0x1d
#define ETN_PHY_INTERRUPT_MASK			0x1e
#define ETN_PHY_SPECIAL_MODE_CONTROL_STATUS	0x1f

/* PHY identification */
#define PHY_ID_LXT970		0x78100000	/* LXT970 */
#define PHY_ID_LXT971		0x001378e0	/* LXT971 and 972 */
#define PHY_ID_82555		0x02a80150	/* Intel 82555 */
#define PHY_ID_QS6612		0x01814400	/* QS6612 */
#define PHY_ID_AMD79C784	0x00225610	/* AMD 79C784 */
#define PHY_ID_LSI80225		0x0016f870	/* LSI 80225 */
#define PHY_ID_LSI80225B	0x0016f880	/* LSI 80225/B */
#define PHY_ID_DM9161		0x0181B880	/* Davicom DM9161 */
#define PHY_ID_KSM8995M		0x00221450	/* MICREL KS8995MA */
#define PHY_ID_SMSC8700		0x0007C0C0	/* SMSC LAN 8700 */
