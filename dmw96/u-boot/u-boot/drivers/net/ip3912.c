/*
 * ip3912 ethernet driver (PNX8181 / firetux)
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

#include <common.h>
#include <net.h>
#include <malloc.h>

#include <asm/io.h>

#include "ip3912.h"
#include <miiphy.h>

#define ALIGN8	__attribute__ ((aligned(8)))
#define ALIGN4	__attribute__ ((aligned(4)))

/* globals */
/* ETN rx */
ALIGN8 rx_descriptor_t	etn_rxdescriptor[CONFIG_SYS_ETN_RX_DESCRIPTOR_NUMBER];
ALIGN8 rx_status_t	etn_rxstatus[CONFIG_SYS_ETN_RX_DESCRIPTOR_NUMBER];

/* ETN tx */
ALIGN8 tx_descriptor_t	etn_txdescriptor[CONFIG_SYS_ETN_TX_DESCRIPTOR_NUMBER];
ALIGN4 tx_status_t	etn_txstatus[CONFIG_SYS_ETN_TX_DESCRIPTOR_NUMBER];

struct ip3912_device {
	unsigned int		etn_base;
	unsigned int		phy_base;
	unsigned char		nr;
	unsigned char		phy_addr;
	unsigned char		autonegotiate;
	unsigned char		speed;
	unsigned char		duplex;
	unsigned char		rmii;

	const struct device	*dev;
	struct eth_device	*netdev;
};

int ip3912_miiphy_write(char *devname, unsigned char addr,
				unsigned char reg, unsigned short value)
{
	int status = 1;
	int i = 0;
	struct eth_device *netdev;
	struct ip3912_device *ip3912;

	netdev = eth_get_dev();
	ip3912 = netdev->priv;

	reg &= 0x001f; /* 5 bit PHY register address */

	writel(PHYADDR_TO_REG(addr) | reg, (void *)(ip3912->phy_base
								+ ETN_MADR));
	writel(value, (void *)(ip3912->phy_base + ETN_MWTD));

	/* poll for done, max 100ms */
	while (status && i < 100000) {
		status = readl((void *)(ip3912->phy_base + ETN_MIND)) & 0x7;
		udelay(1);
		i++;
	}

	if (status) {
		printf("ERROR: ip3912_miiphy_write(%d) = "
				"0x%x [phy_addr=%x]\n", reg, status, addr);
		return -1;
	} else {
		debug("### ip3912_miiphy_write(%2.d, 0x%4.4x) success after"
			" %d cycles [phy_addr=%x]###\n", reg, value, i, addr);
	}

	return 0;
}

int ip3912_miiphy_read(char *devname, unsigned char addr,
				unsigned char reg, unsigned short *value)
{
	int i = 0, status = 1;
	struct eth_device *netdev;
	struct ip3912_device *ip3912;

	netdev = eth_get_dev();
	ip3912 = netdev->priv;

	reg &= 0x001f; /* 5 bit PHY register address */
	writel(PHYADDR_TO_REG(addr) | reg, (void *)(ip3912->phy_base
								+ ETN_MADR));
	writel(0x00000001, (void *)(ip3912->phy_base + ETN_MCMD));

	/* poll for done, max 100ms */
	while (status && i < 100000) {
		status = readl((void *)(ip3912->phy_base + ETN_MIND)) & 0x7;
		udelay(1);
		i++;
	}

	*value = (unsigned short)readl((void *)(ip3912->phy_base + ETN_MRDD));

	writel(0, (void *)(ip3912->phy_base + ETN_MCMD));	/* stop MII */

	if (status) {
		printf("ERROR: ip3912_miiphy_read(%d) = 0x%x after %d cycles "
			"[phy_addr=%x]\n", reg, *value, i, ip3912->phy_addr);
		return -1;
	} else {
		debug("### ip3912_phy_read(%2.d)=0x%4.4x success after %d "
					"cycles [phy_addr=%x]###\n",
				reg, *value, i, ip3912->phy_addr);
	}

	return 0;
}

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
int ip3912_mii_negotiate_phy(void)
{
	char *mode;
	int i;
	unsigned short value;
	struct eth_device *netdev;
	struct ip3912_device *ip3912;

	netdev = eth_get_dev();
	ip3912 = netdev->priv;

	/* only set phy if exists */
	ip3912_miiphy_read(netdev->name, ip3912->phy_addr,
							PHY_PHYIDR1, &value);
	if (value  == 0xffff)
		return -1;

	/* get mode from environment */
	mode = getenv("phymode");
	if (mode  != NULL) {
		if (0 == strcmp(mode, "auto")) {
			ip3912->autonegotiate = 1;
			ip3912->speed = 100;
			ip3912->duplex = 1;
		} else {
			if (0 == strcmp(mode, "100FD")) {
				ip3912->speed = 100;
				ip3912->duplex = 1;
			}
			if (0 == strcmp(mode, "100HD")) {
				ip3912->speed = 100;
				ip3912->duplex = 0;
			}
			if (0 == strcmp(mode, "10FD")) {
				ip3912->speed = 10;
				ip3912->duplex = 1;
			}
			if (0 == strcmp(mode, "10HD")) {
				ip3912->speed = 10;
				ip3912->duplex = 0;
			}
			ip3912->autonegotiate = 0;
		}
	} else {
		/* we use 10Mbit FD as fallback */
		ip3912->autonegotiate = 0;
		ip3912->speed = 10;
		ip3912->duplex = 1;
	}

	/* do autonegotiation */
	if (ip3912->autonegotiate) {
		/* 10/100 and FD/HD mode supported, ieee802.3 */
		ip3912_miiphy_write(netdev->name, ip3912->phy_addr,
				ETN_PHY_AUTONEG_ADV, ((0xf << 5) | 1));
		/* force autorenegotiation */
		ip3912_miiphy_write(netdev->name, ip3912->phy_addr,
				ETN_PHY_BASIC_CONTROL, ((1 << 13) | (1 << 12) |
							 (1 << 9) | (1 << 8)));
	} else {
		/* only advertise the selected mode */
		i = 0x1e0;
		if (ip3912->speed == 100)
			i &= 0x180;
		else
			i &= 0x060;
		if (ip3912->duplex)
			i &= 0x140;
		else
			i &= 0x0a0;
		/* set advertise mode */
		ip3912_miiphy_write(netdev->name, ip3912->phy_addr,
						ETN_PHY_AUTONEG_ADV, (i|1));
		/* we set the phy parameter */
		ip3912_miiphy_write(netdev->name, ip3912->phy_addr,
			ETN_PHY_BASIC_CONTROL, ((ip3912->duplex ? (1<<8) : 0)
			| (1 << 9) | ((ip3912->speed == 100) ? (1 << 13) : 0)));
	}

	/* wait for negotiation finished (max 3.5s) */
	i = 0;
	ip3912_miiphy_read(netdev->name, ip3912->phy_addr, ETN_PHY_BASIC_STATUS,
									&value);
	while (((value & (1 << 5)) == 0) && (i < 350)) {
		udelay(10000);
		i++;
		ip3912_miiphy_read(netdev->name, ip3912->phy_addr,
						 ETN_PHY_BASIC_STATUS, &value);
	}
	if (i == 350)
		puts("link negotiation timed out\n");


	/* check for link */
	if (value & (1 << 2)) {
		/* OK link present */
		ip3912_miiphy_read(netdev->name, ip3912->phy_addr,
				ETN_PHY_SPECIAL_MODE_CONTROL_STATUS, &value);
		ip3912->speed = (value & (1 << 2)) ? 10 : 100;
		ip3912->duplex = (value & (1 << 4)) ? 1 : 0;
	}

	/* program the mac */
	writel((readl((void *)(ip3912->etn_base + ETN_SUPP)) & 0x000018fb) |
					((ip3912->speed == 100) ? (1 << 8) : 0),
					(void *)(ip3912->etn_base + ETN_SUPP));
	writel((readl((void *)(ip3912->etn_base + ETN_MAC2)) & 0x000073fe) |
			ip3912->duplex,	(void *)(ip3912->etn_base + ETN_MAC2));
	/*
	 * release rx-path, tx-path, host registers reset
	 * set Duplex, enable RMII, enable rx+tx
	 * no flow control, no frames<64b
	 */
	writel(0x00000283 | (ip3912->duplex ? (1 << 10) : 0),
				(void *)(ip3912->etn_base + ETN_COMMAND));

	udelay(100000);	/* the mac still needs some time to settle 100ms */
	ip3912_miiphy_read(netdev->name, ip3912->phy_addr,
				ETN_PHY_SPECIAL_MODE_CONTROL_STATUS, &value);
	printf(" %s %s negotiation", netdev->name,
				(value & (1 << 12)) ? "Auto" : "Manual");
	printf(" (10%s Mbit", (value & (1 << 3)) ? "0" : "");
	printf(" %sD", (value & (1 << 4)) ? "F" : "H");
	ip3912_miiphy_read(netdev->name, ip3912->phy_addr, ETN_PHY_BASIC_STATUS,
									&value);
	printf(" (%s)\n", (value & 1<<2)
			? "Link detected" : "No Link detected, trying anyway");

	return 0;
}

#if defined(CONFIG_DISCOVER_PHY)
int mii_discover_phy(void)
{
	unsigned short id1, id2;
	int phytype, phyno;
	struct eth_device *netdev;
	struct ip3912_device *ip3912;

	netdev = eth_get_dev();
	ip3912 = netdev->priv;

	for (phyno = 0; phyno <= 31 ; ++phyno) {
		ip3912_miiphy_read(netdev->name, phyno, PHY_PHYIDR1, &id1);
		if (id1 != 0xffff) {
			ip3912_miiphy_read(netdev->name, phyno, PHY_PHYIDR2,
								&id2);
			phytype = ((id1 << 16) | id2);
			puts("Using Transceiver: ");
			switch (phytype & 0xfffffff0) {
			case PHY_ID_LXT970:
						puts("LXT970");
						break;
			case PHY_ID_LXT971:
						puts("LXT971");
						break;
			case PHY_ID_82555:
						puts("82555");
						break;
			case PHY_ID_QS6612:
						puts("QS6612");
						break;
			case PHY_ID_AMD79C784:
						puts("AMD79C784");
						break;
			case PHY_ID_LSI80225:
						puts("LSI L80225");
						break;
			case PHY_ID_LSI80225B:
						puts("LSI L80225/B");
						break;
			case PHY_ID_DM9161:
						puts("Davicom DM9161");
						break;
			case PHY_ID_KSM8995M:
						puts("MICREL KS8995M");
						break;
			case PHY_ID_SMSC8700:
						puts("SMSC Lan 8700");
						break;
			default:
						printf("0x%08x", phytype);
						break;
		}
	    }
	}

	return 0;
}
#endif /* CONFIG_DISCOVER_PHY */

#endif /* defined(CONFIG_MII) || defined(CONFIG_CMD_MII) */

int ip3912_miiphy_initialize(bd_t *bis)
{
#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
	miiphy_register("ip3912", ip3912_miiphy_read, ip3912_miiphy_write);
#endif
	return 0;
}

static int ip3912_init_descriptors(struct eth_device *netdev)
{
	struct ip3912_device *ip3912;
	static void *rxbuf;
	int i;

	ip3912 = netdev->priv;

	/* fill in pointer in regs */
	writel((unsigned long)etn_rxdescriptor, (void *)(ip3912->etn_base
							+ ETN_RXDESCRIPTOR));
	writel((unsigned long)etn_rxstatus, (void *)(ip3912->etn_base
							+ ETN_RXSTATUS));
	writel(0x00000000, (void *)(ip3912->etn_base + ETN_RXCONSUMEINDEX));
	writel(CONFIG_SYS_ETN_RX_DESCRIPTOR_NUMBER - 1,
				(void *)(ip3912->etn_base
						+ ETN_RXDESCRIPTORNUMBER));

	writel((unsigned long)etn_txdescriptor,	(void *)(ip3912->etn_base
							+ ETN_TXDESCRIPTOR));
	writel((unsigned long)etn_txstatus, (void *)(ip3912->etn_base
							+ ETN_TXSTATUS));
	writel(0x00000000, (void *)(ip3912->etn_base + ETN_TXPRODUCEINDEX));
	writel(CONFIG_SYS_ETN_TX_DESCRIPTOR_NUMBER - 1,
			(void *)(ip3912->etn_base + ETN_TXDESCRIPTORNUMBER));

	/* allocate rx-buffers, but only once, we're called multiple times! */
	if (!rxbuf)
		rxbuf = malloc(MAX_ETH_FRAME_SIZE
					* CONFIG_SYS_ETN_RX_DESCRIPTOR_NUMBER);
	if (!rxbuf) {
		puts("ERROR: couldn't allocate rx buffers!\n");
		return -1;
	}

	for (i = 0; i < CONFIG_SYS_ETN_RX_DESCRIPTOR_NUMBER; i++) {
		etn_rxdescriptor[i].packet = rxbuf + i * MAX_ETH_FRAME_SIZE;
		etn_rxdescriptor[i].control = MAX_ETH_FRAME_SIZE
						- sizeof(unsigned long);
		etn_rxstatus[i].info = 0;
		etn_rxstatus[i].hashCRC = 0;
	}

	for (i = 0; i < CONFIG_SYS_ETN_TX_DESCRIPTOR_NUMBER; i++) {
		etn_txdescriptor[i].packet = 0;
		etn_txdescriptor[i].control = 0;
		etn_txstatus[i].info = 0;
	}
	return 0;
}

void ip3912_setmac(struct eth_device *netdev)
{
	struct ip3912_device *ip3912;
	unsigned char i, use_etn1addr = 0;
	char *mac_string, *pmac, *end;
	char tmp[18];

	ip3912 = netdev->priv;

	mac_string = getenv("ethaddr");

	if (ip3912->nr) {
		/* we use ETN2 */
		mac_string = getenv("eth1addr");
		if (!mac_string) {
			mac_string = getenv("ethaddr");
			use_etn1addr = 1;
		}
	}

	pmac = mac_string;
	for (i = 0; i < 6; i++) {
		netdev->enetaddr[i] = pmac ? simple_strtoul(pmac, &end, 16) : 0;
		if (pmac)
			pmac = (*end) ? end + 1 : end;
	}

	if (use_etn1addr) {
		/* flip last bit of mac address */
		debug("ip3912_setmac %s flipping last bit\n", netdev->name);
		if (netdev->enetaddr[5] & 1)
			netdev->enetaddr[5] &= 0xfe;
		else
			netdev->enetaddr[5] |= 0x01;
		sprintf(tmp, "%02X:%02X:%02X:%02X:%02X:%02X",
			 netdev->enetaddr[0], netdev->enetaddr[1],
			 netdev->enetaddr[2], netdev->enetaddr[3],
			 netdev->enetaddr[4], netdev->enetaddr[5]);
		setenv("eth1addr", tmp);
		mac_string = tmp;
	}

	debug("ip3912_setmac set %s to address %s\n", netdev->name, mac_string);

	writel((netdev->enetaddr[5] << 8) | netdev->enetaddr[4],
					(void *)(ip3912->etn_base + ETN_SA0));
	writel((netdev->enetaddr[3] << 8) | netdev->enetaddr[2],
					(void *)(ip3912->etn_base + ETN_SA1));
	writel((netdev->enetaddr[1] << 8) | netdev->enetaddr[0],
					(void *)(ip3912->etn_base + ETN_SA2));
}

int ip3912_macreset(void)
{
	struct eth_device *netdev;
	struct ip3912_device *ip3912;

	netdev = eth_get_dev();
	ip3912 = netdev->priv;

	debug("ip3912_macreset resetting %s\n", netdev->name);

	/* reset MAC layer */
	writel(0x0000cf00, (void *)(ip3912->etn_base + ETN_MAC1));
	/* release MAC soft reset */
	writel(0x00000000, (void *)(ip3912->etn_base + ETN_MAC1));
	/* reset rx-path, tx-path, host registers */
	writel(0x00000038, (void *)(ip3912->etn_base + ETN_COMMAND));
	/* reset RMII, 100Mbps MAC, 10Mbps MAC */
	writel(0x1888, (void *)(ip3912->etn_base + ETN_SUPP));
	writel(0x1000, (void *)(ip3912->etn_base + ETN_SUPP));

	return 0;
}

static int ip3912_init(struct eth_device *netdev, bd_t *bd)
{
	unsigned char i;
	struct ip3912_device *ip3912 = netdev->priv;

	/* update mac address in boardinfo */
	ip3912_setmac(netdev);

	/* before enabling the rx-path we need to set up rx-descriptors */
	if (ip3912_init_descriptors(netdev))
		return -1;

	/* set max packet length to 1536 bytes */
	writel(MAX_ETH_FRAME_SIZE, (void *)(ip3912->etn_base + ETN_MAXF));
	/* full duplex */
	writel(0x00000023, (void *)(ip3912->etn_base + ETN_MAC2));
	/* inter packet gap register */
	writel(0x15, (void *)(ip3912->etn_base + ETN_IPGT));
	writel(0x12, (void *)(ip3912->etn_base + ETN_IPGR));
	/* enable rx, receive all frames */
	writel(0x00000003, (void *)(ip3912->etn_base + ETN_MAC1));
	/* accept all multicast, broadcast and station packets */
	writel(0x00000026, (void *)(ip3912->etn_base + ETN_RXFILTERCTRL));

	/* reset MII mgmt, set MII clock */
	writel(0x0000801c, (void *)(ip3912->etn_base + ETN_MCFG));
	writel(0x0000001c, (void *)(ip3912->etn_base + ETN_MCFG));

	/* release rx-path, tx-path, host registers reset
	 * set FullDuplex, enable RMMI, enable rx+tx
	 * no flow control, no frames<64b
	 */
	writel(0x00000683, (void *)(ip3912->etn_base + ETN_COMMAND));
	ip3912_init_descriptors(netdev);

#ifdef CONFIG_DISCOVER_PHY
	mii_discover_phy();
#endif
#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
	/* check autonegotiation */
	ip3912_mii_negotiate_phy();
#endif
	return 0;
}

/* Send a packet */
static int ip3912_send(struct eth_device *netdev, volatile void *packet,
								int length)
{
	struct ip3912_device *ip3912 = netdev->priv;
	uint32_t next_packet;
	uint32_t this_packet;
	uint32_t last_packet;

	if ((length > MAX_ETH_FRAME_SIZE) || (length <= 0)) {
		printf("ERROR: cannot transmit a %d bytes frame!\n", length);
		return -1;
	}

	this_packet = readl((void *)(ip3912->etn_base + ETN_TXPRODUCEINDEX));
	next_packet = (this_packet + 1) % CONFIG_SYS_ETN_TX_DESCRIPTOR_NUMBER;
	last_packet = readl((void *)(ip3912->etn_base + ETN_TXCONSUMEINDEX));

#define ETN_TX_MAX_RETRY	1000000
	int i = 0;
	/* wait until the FIFO is ready to accept a new packet */

	while ((this_packet == ((last_packet +
					CONFIG_SYS_ETN_TX_DESCRIPTOR_NUMBER - 1)
					% CONFIG_SYS_ETN_TX_DESCRIPTOR_NUMBER))
						&& (i < ETN_TX_MAX_RETRY)) {
#ifdef ET_DEBUG
		/* debug print when FIFO full*/
		if ((i > 50000) && (!(i % 50000))) {
			this_packet =
				readl((void *)(ip3912->etn_base
							+ ETN_TXPRODUCEINDEX));
			last_packet =
				readl((void *)(ip3912->etn_base
							+ ETN_TXCONSUMEINDEX));
			printf("this=%3.d, last=%3.d, i=%d\n",
						this_packet, last_packet, i);
		}
#endif
		i++;
		last_packet = readl((void *)(ip3912->etn_base
							+ ETN_TXCONSUMEINDEX));
	}
	if (i == ETN_TX_MAX_RETRY) {
		printf("tx FAILED after %d cycles\n", i);
		return -1;
	}
	if (i)
		printf("tx after %d cycles\n", i);

	etn_txdescriptor[this_packet].packet  = packet;
	etn_txdescriptor[this_packet].control = (length - 1) |
		ETN_CONTROL_INTERRUPT | ETN_CONTROL_LAST;

	/* let the HW know a new packet is ready */
	writel(next_packet, (void *)(ip3912->etn_base + ETN_TXPRODUCEINDEX));

	return 0;
}

/* Check for received packets */
static int ip3912_recv(struct eth_device *netdev)
{
	struct ip3912_device *ip3912 = netdev->priv;

	unsigned short rxconsume = (unsigned short)
		(readl((void *)(ip3912->etn_base + ETN_RXCONSUMEINDEX)));
	unsigned short rxproduce = (unsigned short)
		(readl((void *)(ip3912->etn_base + ETN_RXPRODUCEINDEX)));
	unsigned short psize = 0;

	debug("eth_rx: receive_rsv 0x%08x\n",
				readl((void *)(ip3912->etn_base + ETN_RSV)));
	debug("eth_rx: consume 0x%04x produce 0x%04x\n",
							rxconsume, rxproduce);
	while (rxconsume != rxproduce) {
		rxproduce = (unsigned short)(readl((void *)
				(ip3912->etn_base + ETN_RXPRODUCEINDEX)));
		psize = (etn_rxstatus[rxconsume].info & 0x07ff) + 1;
		if (psize > MAX_ETH_FRAME_SIZE) {
			printf("dropping %d bytes frame (too large)!\n",
									psize);
		} else {
			NetReceive(etn_rxdescriptor[rxconsume].packet, psize);
		}
		rxconsume = (rxconsume + 1)
					% CONFIG_SYS_ETN_RX_DESCRIPTOR_NUMBER;
		writel(rxconsume,
			(void *)(ip3912->etn_base + ETN_RXCONSUMEINDEX));
	}
	return psize;
}

static void ip3912_halt(struct eth_device *netdev)
{
	struct ip3912_device *ip3912 = netdev->priv;

	/* disable rx-path, tx-path, host registers reset
	 * set FullDuplex, enable RMMI, disable rx+tx
	 * no flow control, no frames<64b
	 */
	writel(0x000006b8, (void *)(ip3912->etn_base + ETN_COMMAND));
}

int ip3912_eth_initialize(unsigned char nr, unsigned int etn_base,
	unsigned int phy_base, unsigned char phy_addr, unsigned char rmii)
{
	struct ip3912_device *ip3912;
	struct eth_device *netdev;

	netdev = malloc(sizeof(struct eth_device));
	ip3912 = malloc(sizeof(struct ip3912_device));
	if ((!ip3912) || (!netdev)) {
		printf("Error: Failed to allocate memory for ETN%d\n", nr + 1);
		return -1;
	}

	memset(ip3912, 0, sizeof(struct ip3912_device));
	memset(netdev, 0, sizeof(struct eth_device));

	ip3912->nr = nr;
	ip3912->etn_base = etn_base;
	ip3912->phy_base = phy_base;
	ip3912->phy_addr = phy_addr;
	ip3912->autonegotiate = 0;
	ip3912->rmii = rmii;
	ip3912->speed = 0;
	ip3912->duplex = 0;
	ip3912->netdev = netdev;

	sprintf(netdev->name, "ETN%d", nr + 1);
	netdev->init = ip3912_init;
	netdev->send = ip3912_send;
	netdev->recv = ip3912_recv;
	netdev->halt = ip3912_halt;
	netdev->priv = (void *)ip3912;

	eth_register(netdev);
	setenv("ethact", netdev->name);
	eth_set_current();
	ip3912_macreset();
	/* we have to set the mac address, because we have no SROM */
	ip3912_setmac(netdev);

	return 0;
}
