#include <common.h>
#include <asm/io.h>
#include "synop_gmac_dev.h"

#define MACG_TX_TIMEOUT 30000

#define to_gmac(_nd) container_of(_nd, struct synopGMACDeviceStruct, netdev)

synop_gmac_device gmac_device;

synop_gmac_device* g_gmacdev = &gmac_device;

#if defined(CONFIG_CMD_NET)
static int gmac_send(struct eth_device *netdev, volatile void *packet, int length)
{
	s32 status = 0;
	u32 offload_needed = 0;
	synop_gmac_device *gmacdev = to_gmac(netdev);
	int i = 0;
	unsigned char *packet_on_sdram = 0;
	unsigned int timeout = MACG_TX_TIMEOUT;

	TR2("\n");
	if (packet == NULL) {
		TR0("Packet is NULL. What happened to u-boot?\n");
		return -1;
	}

	packet_on_sdram = (unsigned char *)(virt_to_bus((u32)packet)); // - 0x80000000);

	TR2("packet = 0x%x packet on sdram = 0x%x length = %d \n",
	    (unsigned int)packet, (unsigned int)packet_on_sdram , length);
	for (i = 0; i < length; i++)
	 	TR2("packet[%d] = 0x%x\n",i , packet_on_sdram[i]);

	status = synop_gmac_set_tx_qptr(gmacdev, (u32)packet_on_sdram, length,
	                                (u32)packet, 0, 0, 0, offload_needed);
	if (status < 0) {
		TR0("No More Free Tx Descriptors\n");
		return -1;
	}

	/* Now force the DMA to start transmission */
	synop_gmac_resume_dma_tx(gmacdev);

	while (synop_gmac_intr_tx_handler(gmacdev) != 0 && --timeout > 0);

	if (!timeout)
		TR0("Timeout\n");
	
	return 0;
}

static int gmac_recv(struct eth_device *netdev)
{
	synop_gmac_device *gmacdev = to_gmac(netdev);

	synop_gmac_intr_rx_handler(gmacdev);

	return 0;
}

static int gmac_init(struct eth_device *netdev, bd_t *bd)
{
	synop_gmac_device *gmacdev = to_gmac(netdev);
	TR2("\n");

	synop_gmac_setup_gmac(gmacdev);
	synop_gmac_enable_rx_chksum_offload(gmacdev);

	return 0;
}

static void gmac_halt(struct eth_device *netdev)
{
	synop_gmac_device *gmacdev = to_gmac(netdev);
	TR2("\n");

	synop_gmac_tx_disable(gmacdev);
	synop_gmac_rx_disable(gmacdev);
	/* Disable TX and RX */
	synop_gmac_disable_dma_tx(gmacdev);
	synop_gmac_disable_dma_rx(gmacdev);
}

int gmac_eth_initialize(int id, void *regs, unsigned int phy_addr)
{
	struct eth_device *netdev;
	u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;
	unsigned int ijk=0;

	memset(g_gmacdev, 0, sizeof(synop_gmac_device));

	netdev = &(g_gmacdev->netdev);

	sprintf(netdev->name,"gmac%d", id) ;
	netdev->init = gmac_init;
	netdev->halt = gmac_halt;
	netdev->send = gmac_send;
	netdev->recv = gmac_recv;

	synop_gmac_reset(g_gmacdev);

	/* Attach the device to the MAC struct. This will configure all the
	 * required base addresses such as the Mac base, configuration base,
	 * phy base address (out of 32 possible phys). */
	synop_gmac_attach(g_gmacdev, (u32)(regs + MACBASE),
	                  (u32)(regs + DMABASE),
	                  DEFAULT_PHY_UNDEFINE);

	/*
	 * Do some basic initialization so that we at least can talk
	 * to the PHY
	 */
	eth_register(netdev);

	return 0;
}
#endif

#if defined(CONFIG_CMD_MII)
int miiphy_read(unsigned char addr, unsigned char reg, unsigned short *value)
{
	if (g_gmacdev != NULL)
		synop_gmac_read_phy_reg((u32 *)g_gmacdev->MacBase, gmacdev->PhyBase, (u32)reg, value );

	return 0;
}

int miiphy_write(unsigned char addr, unsigned char reg, unsigned short value)
{
	if (g_gmacdev != NULL)
		synop_gmac_write_phy_reg((u32 *)g_gmacdev->MacBase, gmacdev->PhyBase, (u32)reg, value);

	return 0;
}
#endif
