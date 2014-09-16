/** \file
 * This file defines the synopsys GMAC device dependent functions.
 * Most of the operations on the GMAC device are available in this file.
 * Functions for initiliasing and accessing MAC/DMA/PHY registers and the DMA descriptors
 * are encapsulated in this file. The functions are platform/host/OS independent.
 * These functions in turn use the low level device dependent (HAL) functions to 
 * access the register space.
 * \internal
 * ------------------------REVISION HISTORY---------------------------------
 * Synopsys                 01/Aug/2007                              Created
 */

#include <common.h>
#include <asm/io.h>
#include <linux/ccu.h>
#include "synop_gmac_dev.h"

static s32 synop_gmac_set_mdc_clk_div(synop_gmac_device *gmacdev, u32 clk_div_val);
static u32 synop_gmac_get_mdc_clk_div(synop_gmac_device *gmacdev);
static s32 synop_gmac_read_phy_reg(u32 *RegBase, u32 PhyBase, u32 RegOffset, u16 *data);
static s32 synop_gmac_write_phy_reg(u32 *RegBase, u32 PhyBase, u32 RegOffset, u16 data);
#ifndef __KERNEL__
static s32 synop_gmac_phy_loopback(synop_gmac_device *gmacdev, unsigned int loopback);
#endif
static s32 synop_gmac_read_version (synop_gmac_device *gmacdev);
static s32 synop_gmac_dma_bus_mode_init(synop_gmac_device *gmacdev, u32 init_value);
static s32 synop_gmac_dma_control_init(synop_gmac_device *gmacdev, u32 init_value);
static void synop_gmac_wd_enable(synop_gmac_device *gmacdev);
static void synop_gmac_jab_enable(synop_gmac_device *gmacdev);
static void synop_gmac_frame_burst_enable(synop_gmac_device *gmacdev);
static void synop_gmac_jumbo_frame_disable(synop_gmac_device *gmacdev);
static void synop_gmac_select_gmii(synop_gmac_device *gmacdev);
static void synop_gmac_select_mii(synop_gmac_device *gmacdev);
static void synop_gmac_rx_own_enable(synop_gmac_device *gmacdev);
static void synop_gmac_loopback_off(synop_gmac_device *gmacdev);
static void synop_gmac_set_full_duplex(synop_gmac_device *gmacdev);
static void synop_gmac_set_half_duplex(synop_gmac_device *gmacdev);
static void synop_gmac_retry_enable(synop_gmac_device *gmacdev);
static void synop_gmac_pad_crc_strip_disable(synop_gmac_device *gmacdev);
static void synop_gmac_back_off_limit(synop_gmac_device *gmacdev, u32 value);
static void synop_gmac_deferral_check_disable(synop_gmac_device *gmacdev);
static void synop_gmac_rx_enable(synop_gmac_device *gmacdev);
static void synop_gmac_tx_enable(synop_gmac_device *gmacdev);
static void synop_gmac_frame_filter_enable(synop_gmac_device *gmacdev);
static void synop_gmac_src_addr_filter_disable(synop_gmac_device *gmacdev);
static void synop_gmac_dst_addr_filter_normal(synop_gmac_device *gmacdev);
static void synop_gmac_set_pass_control(synop_gmac_device *gmacdev, u32 passcontrol);
static void synop_gmac_broadcast_enable(synop_gmac_device *gmacdev);
static void synop_gmac_multicast_disable(synop_gmac_device *gmacdev);
static void synop_gmac_multicast_hash_filter_disable(synop_gmac_device *gmacdev);
static void synop_gmac_promisc_disable(synop_gmac_device *gmacdev);
static void synop_gmac_unicast_hash_filter_disable(synop_gmac_device *gmacdev);
static void synop_gmac_unicast_pause_frame_detect_disable(synop_gmac_device *gmacdev);
static void synop_gmac_rx_flow_control_enable(synop_gmac_device *gmacdev);
static void synop_gmac_rx_flow_control_disable(synop_gmac_device *gmacdev);
static void synop_gmac_tx_flow_control_enable(synop_gmac_device *gmacdev);
static void synop_gmac_tx_flow_control_disable(synop_gmac_device *gmacdev);
static void synop_gmac_pause_control(synop_gmac_device *gmacdev);
static s32 synop_gmac_mac_init(synop_gmac_device *gmacdev);
static s32 synop_gmac_check_phy_init (synop_gmac_device *gmacdev);
static s32 synop_gmac_phy_select_port(synop_gmac_device *gmacdev);
static s32 synop_gmac_set_mac_addr(synop_gmac_device *gmacdev, u32 MacHigh, u32 MacLow, u8 *MacAddr);
static s32 synop_gmac_get_mac_addr(synop_gmac_device *gmacdev, u32 MacHigh, u32 MacLow, u8 *MacAddr);
static void synop_gmac_rx_desc_init_ring(dma_desc *desc, unsigned int last_ring_desc);
static void synop_gmac_tx_desc_init_ring(dma_desc *desc, unsigned int last_ring_desc);
static void synop_gmac_rx_desc_init_chain(dma_desc *desc);
static void synop_gmac_tx_desc_init_chain(dma_desc *desc);
static s32 synop_gmac_init_tx_rx_desc_queue(synop_gmac_device *gmacdev);
static void synop_gmac_init_rx_desc_base(synop_gmac_device *gmacdev);
static void synop_gmac_init_tx_desc_base(synop_gmac_device *gmacdev);
static unsigned int synop_gmac_is_desc_owned_by_dma(dma_desc *desc);
static u32 synop_gmac_get_rx_desc_frame_length(u32 status);
static unsigned int synop_gmac_is_desc_valid(u32 status);
static unsigned int synop_gmac_is_desc_empty(dma_desc *desc);
static unsigned int synop_gmac_is_rx_desc_valid(u32 status);
static unsigned int synop_gmac_is_last_rx_desc(synop_gmac_device *gmacdev,dma_desc *desc);
static unsigned int synop_gmac_is_last_tx_desc(synop_gmac_device *gmacdev,dma_desc *desc);
static unsigned int synop_gmac_is_rx_desc_chained(dma_desc *desc);
static unsigned int synop_gmac_is_tx_desc_chained(dma_desc *desc);
static s32 synop_gmac_get_tx_qptr(synop_gmac_device *gmacdev, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1, u32 *Buffer2, u32 * Length2, u32 * Data2 );
static s32 synop_gmac_set_rx_qptr(synop_gmac_device *gmacdev, u32 Buffer1, u32 Length1, u32 Data1, u32 Buffer2, u32 Length2, u32 Data2);
static s32 synop_gmac_init_rx_qptr(synop_gmac_device *gmacdev);
static s32 synop_gmac_get_rx_qptr(synop_gmac_device *gmacdev, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1, u32 *Buffer2, u32 * Length2, u32 * Data2);
static void synop_gmac_clear_interrupt(synop_gmac_device *gmacdev);
static u32 synop_gmac_get_interrupt_type(synop_gmac_device *gmacdev, u32 clear_mask);
static void synop_gmac_enable_interrupt(synop_gmac_device *gmacdev, u32 interrupts);
static void synop_gmac_disable_interrupt_all(synop_gmac_device *gmacdev);
static void synop_gmac_enable_dma_rx(synop_gmac_device *gmacdev);
static void synop_gmac_enable_dma_tx(synop_gmac_device *gmacdev);
static void synop_gmac_resume_dma_rx(synop_gmac_device *gmacdev);
static void synop_gmac_take_desc_ownership(dma_desc *desc);
static void synop_gmac_take_desc_ownership_rx(synop_gmac_device *gmacdev);
static void synop_gmac_take_desc_ownership_tx(synop_gmac_device *gmacdev);
/*******************MMC APIs***************************************/
static void synop_gmac_disable_mmc_tx_interrupt(synop_gmac_device *gmacdev, u32 mask);
static void synop_gmac_disable_mmc_rx_interrupt(synop_gmac_device *gmacdev, u32 mask);
static void synop_gmac_disable_mmc_ipc_rx_interrupt(synop_gmac_device *gmacdev, u32 mask);
static u32  synop_gmac_is_rx_checksum_error(synop_gmac_device *gmacdev, u32 status);
static unsigned int synop_gmac_is_tx_ipv4header_checksum_error(synop_gmac_device *gmacdev, u32 status);
static unsigned int synop_gmac_is_tx_payload_checksum_error(synop_gmac_device *gmacdev, u32 status);
static s32 synop_gmac_setup_tx_desc_queue(synop_gmac_device *gmacdev, u32 no_of_desc);
static s32 synop_gmac_setup_rx_desc_queue(synop_gmac_device *gmacdev, u32 no_of_desc);

#define DATA_ON_DDR
#ifdef DATA_ON_DDR
static char rx_buffers[RECEIVE_DESC_SIZE][RX_BUF_SIZE];
static dma_desc rx_array_desc[RECEIVE_DESC_SIZE];
static dma_desc tx_array_desc[TRANSMIT_DESC_SIZE];
#else
static char *rx_buffers = 0x00200000;
static dma_desc *rx_array_desc = 0x00200000 + ((RECEIVE_DESC_SIZE)*(RX_BUF_SIZE));
static dma_desc *tx_array_desc = 0x00200000 + ((RECEIVE_DESC_SIZE)*(RX_BUF_SIZE)) + sizeof(dma_desc)*(RECEIVE_DESC_SIZE);
#endif

/**
 * The Low level function to read register contents from Hardware.
 *
 * @param[in] pointer to the base of register map
 * @param[in] Offset from the base
 * \return  Returns the register contents
 */
static u32
synop_gmac_read_reg(u32 *RegBase, u32 RegOffset)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);

	return data;
}

/**
 * The Low level function to write to a register in Hardware.
 *
 * @param[in] pointer to the base of register map
 * @param[in] Offset from the base
 * @param[in] Data to be written
 * \return  void
 */
static void
synop_gmac_write_reg(u32 *RegBase, u32 RegOffset, u32 RegData)
{
	u32 addr = (u32)RegBase + RegOffset;
	writel(RegData,(void *)addr);
}

/**
 * The Low level function to set bits of a register in Hardware.
 *
 * @param[in] pointer to the base of register map
 * @param[in] Offset from the base
 * @param[in] Bit mask to set bits to logical 1
 * \return  void
 */
static void
synop_gmac_set_bits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	data |= BitPos;
	writel(data,(void *)addr);
}

/**
 * The Low level function to clear bits of a register in Hardware.
 *
 * @param[in] pointer to the base of register map
 * @param[in] Offset from the base
 * @param[in] Bit mask to clear bits to logical 0
 * \return  void
 */
static void
synop_gmac_clear_bits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	data &= (~BitPos);
	writel(data,(void *)addr);
}

/**
  * Function to set the MDC clock for mdio transactiona
  *
  * @param[in] pointer to device structure.
  * @param[in] clk divider value.
  * \return Reuturns 0 on success else return the error value.
  */
static s32
synop_gmac_set_mdc_clk_div(synop_gmac_device *gmacdev, u32 clk_div_val)
{
	u32 orig_data;
	orig_data = synop_gmac_read_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr); //set the mdc clock to the user defined value
	orig_data &= (~ GmiiCsrClkMask);
	orig_data |= clk_div_val;
	synop_gmac_write_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr, orig_data);
	return 0;
}

/**
  * Returns the current MDC divider value programmed in the ip.
  *
  * @param[in] pointer to device structure.
  * @param[in] clk divider value.
  * \return Returns the MDC divider value read.
  */
static u32
synop_gmac_get_mdc_clk_div(synop_gmac_device *gmacdev)
{
	u32 data;
	data = synop_gmac_read_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr);
	data &= GmiiCsrClkMask;
	return data;
}

/**
  * Function to read the Phy register. The access to phy register
  * is a slow process as the data is moved accross MDI/MDO interface
  * @param[in] pointer to Register Base (It is the mac base in our case) .
  * @param[in] PhyBase register is the index of one of supported 32 PHY devices.
  * @param[in] Register offset is the index of one of the 32 phy register.
  * @param[out] u16 data read from the respective phy register (only valid iff return value is 0).
  * \return Returns 0 on success else return the error status.
  */
static s32
synop_gmac_read_phy_reg(u32 *RegBase, u32 PhyBase, u32 RegOffset, u16 * data)
{
	u32 addr;
	u32 loop_variable;
	addr = ((PhyBase << GmiiDevShift) & GmiiDevMask) | ((RegOffset << GmiiRegShift) & GmiiRegMask);
	addr = addr | GmiiBusy ; //Gmii busy bit
	synop_gmac_write_reg(RegBase, GmacGmiiAddr, addr); //write the address from where the data to be read in GmiiGmiiAddr register of synopGMAC ip

	for(loop_variable = 0; loop_variable < DEFAULT_LOOP_VARIABLE; loop_variable++){ //Wait till the busy bit gets cleared with in a certain amount of time
		if (!(synop_gmac_read_reg(RegBase, GmacGmiiAddr) & GmiiBusy)){
			break;
		}
		udelay(DEFAULT_DELAY_VARIABLE);
	}
	if(loop_variable < DEFAULT_LOOP_VARIABLE)
		*data = (u16)(synop_gmac_read_reg(RegBase, GmacGmiiData) & 0xFFFF);
	else{
		TR0("Error::: PHY not responding Busy bit didnot get cleared !!!!!!\n");
		return -ESYNOPGMACPHYERR;
	}
	return -ESYNOPGMACNOERR;
}

/**
  * Function to write to the Phy register. The access to phy register
  * is a slow process as the data is moved accross MDI/MDO interface
  * @param[in] pointer to Register Base (It is the mac base in our case) .
  * @param[in] PhyBase register is the index of one of supported 32 PHY devices.
  * @param[in] Register offset is the index of one of the 32 phy register.
  * @param[in] data to be written to the respective phy register.
  * \return Returns 0 on success else return the error status.
  */
static s32
synop_gmac_write_phy_reg(u32 *RegBase, u32 PhyBase, u32 RegOffset, u16 data)
{
	u32 addr;
	u32 loop_variable;

	synop_gmac_write_reg(RegBase,GmacGmiiData,data); // write the data in to GmacGmiiData register of synopGMAC ip

	addr = ((PhyBase << GmiiDevShift) & GmiiDevMask) | ((RegOffset << GmiiRegShift) & GmiiRegMask) | GmiiWrite;

	addr = addr | GmiiBusy ; //set Gmii clk to 20-35 Mhz and Gmii busy bit

	synop_gmac_write_reg(RegBase ,GmacGmiiAddr, addr);
	for(loop_variable = 0; loop_variable < DEFAULT_LOOP_VARIABLE; loop_variable++){
		if (!(synop_gmac_read_reg(RegBase, GmacGmiiAddr) & GmiiBusy)){
			break;
		}
		udelay(DEFAULT_DELAY_VARIABLE);
	}

	if (loop_variable < DEFAULT_LOOP_VARIABLE) {
		return -ESYNOPGMACNOERR;
	} else{
		TR0("Error::: PHY not responding Busy bit didnot get cleared !!!!!!\n");
		return -ESYNOPGMACPHYERR;
	}
}

/**
  * Function to configure the phy in loopback mode.
  *
  * @param[in] pointer to synop_gmac_device.
  * @param[in] enable or disable the loopback.
  * \return 0 on success else return the error status.
  * \note Don't get confused with mac loop-back synop_gmac_loopback_on(synop_gmac_device *)
  * and synop_gmac_loopback_off(synop_gmac_device *) functions.
  */
#ifndef __KERNEL__
static s32
synop_gmac_phy_loopback(synop_gmac_device *gmacdev, unsigned int loopback)
{
	s32 status = -ESYNOPGMACNOERR;
	if (loopback)
		status = synop_gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_CONTROL_REG, Mii_Loopback);
	else
		status = synop_gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_CONTROL_REG, Mii_NoLoopback);
	
	return status;
}
#endif

/**
  * Function to read the GMAC IP Version and populates the same in device data structure.
  * @param[in] pointer to synop_gmac_device.
  * \return Always return 0.
  */

static s32
synop_gmac_read_version(synop_gmac_device *gmacdev)
{
	u32 data = 0;
	data = synop_gmac_read_reg((u32 *)gmacdev->MacBase, GmacVersion);
	gmacdev->Version = data;
	TR2("The data read from %08x is %08x\n", (gmacdev->MacBase+GmacVersion), data);
	return 0;
}

/**
  * Function to reset the GMAC core.
  * This resets the DMA and GMAC core. After reset all the
  * registers holds their respective reset value
  * @param[in] pointer to synop_gmac_device.
  * \return 0 on success else return the error status.
  */
s32
synop_gmac_reset(synop_gmac_device *gmacdev)
{
	u32 data = 0;
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaBusMode, DmaResetOn);
	udelay(DEFAULT_LOOP_VARIABLE);
	data = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaBusMode);
	TR2("DATA after Reset = %08x\n", data);

	return 0;
}

/**
  * Function to program DMA bus mode register.
  *
  * The Bus Mode register is programmed with the value given. The bits to be set are
  * bit wise or'ed and sent as the second argument to this function.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] the data to be programmed.
  * \return 0 on success else return the error status.
  */
static s32
synop_gmac_dma_bus_mode_init(synop_gmac_device *gmacdev, u32 init_value )
{
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaBusMode ,init_value);
	return 0;
}

/**
  * Function to program DMA Control register.
  *
  * The Dma Control register is programmed with the value given. The bits to be set are
  * bit wise or'ed and sent as the second argument to this function.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] the data to be programmed.
  * \return 0 on success else return the error status.
  */
static s32
synop_gmac_dma_control_init(synop_gmac_device *gmacdev, u32 init_value)
{
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl, init_value);
	return 0;
}

/*Gmac configuration functions*/

/**
  * Enable the watchdog timer on the receiver.
  * When enabled, Gmac enables Watchdog timer, and GMAC allows no more than
  * 2048 bytes of data (10,240 if Jumbo frame enabled).
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_wd_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacWatchdog);
}

/**
  * Enables the Jabber frame support.
  * When enabled, GMAC disabled the jabber timer, and can transfer 16,384 byte frames.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_jab_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacJabber);
}

/**
  * Enables Frame bursting (Only in Half Duplex Mode).
  * When enabled, GMAC allows frame bursting in GMII Half Duplex mode.
  * Reserved in 10/100 and Full-Duplex configurations.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_frame_burst_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacFrameBurst);
}

/**
  * Disable Jumbo frame support.
  * When Disabled GMAC does not supports jumbo frames.
  * Giant frame error is reported in receive frame status.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_jumbo_frame_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacJumboFrame);
}

/**
  * Selects the GMII port.
  * When called GMII (1000Mbps) port is selected (programmable only in 10/100/1000 Mbps configuration).
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_select_gmii(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacMiiGmii);
}

/**
  * Selects the MII port.
  * When called MII (10/100Mbps) port is selected (programmable only in 10/100/1000 Mbps configuration).
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_select_mii(synop_gmac_device *gmacdev)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacMiiGmii);
}

/**
  * Enables Receive Own bit (Only in Half Duplex Mode).
  * When enaled GMAC receives all the packets given by phy while transmitting.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_rx_own_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRxOwn);
}

/**
  * Sets the GMAC in Normal mode.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_loopback_off(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacLoopback);
}

/**
  * Sets the GMAC core in Full-Duplex mode.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_set_full_duplex(synop_gmac_device *gmacdev)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacDuplex);
}

/**
  * Sets the GMAC core in Half-Duplex mode.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_set_half_duplex(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacDuplex);
}

/**
  * GMAC tries retransmission (Only in Half Duplex mode).
  * If collision occurs on the GMII/MII, GMAC attempt retries based on the
  * back off limit configured.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  * \note This function is tightly coupled with synop_gmac_back_off_limit(synopGMACdev *, u32).
  */
static void
synop_gmac_retry_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRetry);
}

/**
  * GMAC doesnot strips the Pad/FCS field of incoming frames.
  * GMAC will pass all the incoming frames to Host unmodified.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_pad_crc_strip_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacPadCrcStrip);
}

/**
  * GMAC programmed with the back off limit value.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  * \note This function is tightly coupled with synop_gmac_retry_enable(synop_gmac_device *gmacdev)
  */
static void
synop_gmac_back_off_limit(synop_gmac_device *gmacdev, u32 value)
{
	u32 data;
	data = synop_gmac_read_reg((u32 *)gmacdev->MacBase, GmacConfig);
	data &= (~GmacBackoffLimit);
	data |= value;
	synop_gmac_write_reg((u32 *)gmacdev->MacBase, GmacConfig,data);
}

/**
  * Disables the Deferral check in GMAC (Only in Half Duplex mode).
  * GMAC defers until the CRS signal goes inactive.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_deferral_check_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacDeferralCheck);
}

/**
  * Enable the reception of frames on GMII/MII.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_rx_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRx);
}

/**
  * Disable the reception of frames on GMII/MII.
  * GMAC receive state machine is disabled after completion of reception of current frame.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
void synop_gmac_rx_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRx);
}

/**
  * Enable the transmission of frames on GMII/MII.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_tx_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacTx);
}

/**
  * Disable the transmission of frames on GMII/MII.
  * GMAC transmit state machine is disabled after completion of transmission of current frame.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
void synop_gmac_tx_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacTx);
}

/*Receive frame filter configuration functions*/

/**
  * Enables reception of all the frames to application.
  * GMAC passes all the frames received to application irrespective of whether they
  * pass SA/DA address filtering or not.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_frame_filter_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacFilter);
}

/**
  * Disables Source address filtering.
  * When disabled GMAC forwards the received frames with updated SAMatch bit in RxStatus.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_src_addr_filter_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacSrcAddrFilter);
}

/**
  * Enables the normal Destination address filtering.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_dst_addr_filter_normal(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacDestAddrFilterNor);
}

/**
  * Enables forwarding of control frames.
  * When set forwards all the control frames (incl. unicast and multicast PAUSE frames).
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  * \note Depends on RFE of FlowControlRegister[2]
  */
static void
synop_gmac_set_pass_control(synop_gmac_device *gmacdev, u32 passcontrol)
{
	u32 data;
	data = synop_gmac_read_reg((u32 *)gmacdev->MacBase, GmacFrameFilter);
	data &= (~GmacPassControl);
	data |= passcontrol;
	synop_gmac_write_reg((u32 *)gmacdev->MacBase,GmacFrameFilter,data);
}

/**
  * Enables Broadcast frames.
  * When enabled Address filtering module passes all incoming broadcast frames.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_broadcast_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacBroadcast);
}

/**
  * Disable Multicast frames.
  * When disabled multicast frame filtering depends on HMC bit.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_multicast_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacMulticastFilter);
}

/**
  * Disables multicast hash filtering.
  * When disabled GMAC performs perfect destination address filtering for multicast frames, it compares
  * DA field with the value programmed in DA register.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_multicast_hash_filter_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacMcastHashFilter);
}

/**
  * Clears promiscous mode.
  * When called the GMAC falls back to normal operation from promiscous mode.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_promisc_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacPromiscuousMode);
}

/**
  * Disables multicast hash filtering.
  * When disabled GMAC performs perfect destination address filtering for unicast frames, it compares
  * DA field with the value programmed in DA register.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_unicast_hash_filter_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacUcastHashFilter);
}

/*Flow control configuration functions*/

/**
  * Disables detection of pause frames with stations unicast address.
  * When disabled GMAC only detects with the unique multicast address (802.3x).
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_unicast_pause_frame_detect_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacUnicastPauseFrame);
}

/**
  * Rx flow control enable.
  * When Enabled GMAC will decode the rx pause frame and disable the tx for a specified time.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_rx_flow_control_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacRxFlowControl);
}

/**
  * Rx flow control disable.
  * When disabled GMAC will not decode pause frame.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_rx_flow_control_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacRxFlowControl);
}

/**
  * Tx flow control enable.
  * When Enabled
  *	- In full duplex GMAC enables flow control operation to transmit pause frames.
  *	- In Half duplex GMAC enables the back pressure operation
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_tx_flow_control_enable(synop_gmac_device *gmacdev)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacTxFlowControl);
}

/**
  * Tx flow control disable.
  * When Disabled
  *	- In full duplex GMAC will not transmit any pause frames.
  *	- In Half duplex GMAC disables the back pressure feature.
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_tx_flow_control_disable(synop_gmac_device *gmacdev)
{
	synop_gmac_clear_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacTxFlowControl);
}

/**
  * This enables the pause frame generation after programming the appropriate registers.
  * presently activation is set at 3k and deactivation set at 4k. These may have to tweaked
  * if found any issues
  * @param[in] pointer to synop_gmac_device.
  * \return void.
  */
static void
synop_gmac_pause_control(synop_gmac_device *gmacdev)
{
	u32 omr_reg;
	u32 mac_flow_control_reg;
	omr_reg = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
	omr_reg |= DmaRxFlowCtrlAct4K | DmaRxFlowCtrlDeact5K | DmaEnHwFlowCtrl;
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl, omr_reg);

	mac_flow_control_reg = synop_gmac_read_reg((u32 *)gmacdev->MacBase, GmacFlowControl);
	mac_flow_control_reg |= GmacRxFlowControl | GmacTxFlowControl | 0xFFFF0000;
	synop_gmac_write_reg((u32 *)gmacdev->MacBase, GmacFlowControl, mac_flow_control_reg);
}

/**
  * Example mac initialization sequence.
  * This function calls the initialization routines to initialize the GMAC register.
  * One can change the functions invoked here to have different configuration as per the requirement
  * @param[in] pointer to synop_gmac_device.
  * \return Returns 0 on success.
  */
static s32
synop_gmac_mac_init(synop_gmac_device *gmacdev)
{
	u32 PHYreg;

	if (gmacdev->DuplexMode == FULLDUPLEX) {
		synop_gmac_wd_enable(gmacdev);
		synop_gmac_jab_enable(gmacdev);
		synop_gmac_frame_burst_enable(gmacdev);
		synop_gmac_jumbo_frame_disable(gmacdev);
		synop_gmac_rx_own_enable(gmacdev);
		synop_gmac_loopback_off(gmacdev);
		synop_gmac_set_full_duplex(gmacdev);
		synop_gmac_retry_enable(gmacdev);
		synop_gmac_pad_crc_strip_disable(gmacdev);
		synop_gmac_back_off_limit(gmacdev, GmacBackoffLimit0);
		synop_gmac_deferral_check_disable(gmacdev);
		synop_gmac_tx_enable(gmacdev);
		synop_gmac_rx_enable(gmacdev);

		if (gmacdev->Speed == SPEED1000)
			synop_gmac_select_gmii(gmacdev);
		else
			synop_gmac_select_mii(gmacdev);

		/*Frame Filter Configuration*/
		synop_gmac_frame_filter_enable(gmacdev);
		synop_gmac_set_pass_control(gmacdev, GmacPassControl0);
		synop_gmac_broadcast_enable(gmacdev);
		synop_gmac_src_addr_filter_disable(gmacdev);
		synop_gmac_multicast_disable(gmacdev);
		synop_gmac_dst_addr_filter_normal(gmacdev);
		synop_gmac_multicast_hash_filter_disable(gmacdev);
		synop_gmac_promisc_disable(gmacdev);
		synop_gmac_unicast_hash_filter_disable(gmacdev);

		/*Flow Control Configuration*/
		synop_gmac_unicast_pause_frame_detect_disable(gmacdev);
		synop_gmac_rx_flow_control_enable(gmacdev);
		synop_gmac_tx_flow_control_enable(gmacdev);
	} else { //for Half Duplex configuration
		synop_gmac_wd_enable(gmacdev);
		synop_gmac_jab_enable(gmacdev);
		synop_gmac_frame_burst_enable(gmacdev);
		synop_gmac_jumbo_frame_disable(gmacdev);
		synop_gmac_rx_own_enable(gmacdev);
		synop_gmac_loopback_off(gmacdev);
		synop_gmac_set_half_duplex(gmacdev);
		synop_gmac_retry_enable(gmacdev);
		synop_gmac_pad_crc_strip_disable(gmacdev);
		synop_gmac_back_off_limit(gmacdev, GmacBackoffLimit0);
		synop_gmac_deferral_check_disable(gmacdev);
		synop_gmac_tx_enable(gmacdev);
		synop_gmac_rx_enable(gmacdev);

		if (gmacdev->Speed == SPEED1000)
			synop_gmac_select_gmii(gmacdev);
		else
			synop_gmac_select_mii(gmacdev);

		/*Frame Filter Configuration*/
		synop_gmac_frame_filter_enable(gmacdev);
		synop_gmac_set_pass_control(gmacdev, GmacPassControl0);
		synop_gmac_broadcast_enable(gmacdev);
		synop_gmac_src_addr_filter_disable(gmacdev);
		synop_gmac_multicast_disable(gmacdev);
		synop_gmac_dst_addr_filter_normal(gmacdev);
		synop_gmac_multicast_hash_filter_disable(gmacdev);
		synop_gmac_promisc_disable(gmacdev);
		synop_gmac_unicast_hash_filter_disable(gmacdev);
		
		/*Flow Control Configuration*/
		synop_gmac_unicast_pause_frame_detect_disable(gmacdev);
		synop_gmac_rx_flow_control_disable(gmacdev);
		synop_gmac_tx_flow_control_disable(gmacdev);

		/*To set PHY register to enable CRS on Transmit*/
		synop_gmac_write_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr, GmiiBusy | 0x00000408);
		PHYreg = synop_gmac_read_reg((u32 *)gmacdev->MacBase, GmacGmiiData);
		synop_gmac_write_reg((u32 *)gmacdev->MacBase, GmacGmiiData, PHYreg   | 0x00000800);
		synop_gmac_write_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr, GmiiBusy | 0x0000040a);
	}

	return 0;
}

static s32
synop_gmac_phy_check_link_status(synop_gmac_device *gmacdev)
{
	u16 data;
	s32 status = -ESYNOPGMACNOERR;

	status = synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg1, &data);
	if (status)
		return status;

	TR2("data = 0x%x\n", data);

	if (data == Mii_error)
		return 0;

	data &= PHY_Reg1_LINK_STATUS;

	return (data ? 1 : 0);
}

static s32
synop_gmac_phy_select_port(synop_gmac_device *gmacdev)
{
	u16 phy_id1 = 0;
	u16 phy_id2 = 0;

	s32 status = -ESYNOPGMACNOERR;
	unsigned int tested_phy = 0;

	for (tested_phy = PHY0; tested_phy <= PHY31; tested_phy++) {
		status = synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, tested_phy, PHY_Reg2, &phy_id1);
		if (status)
			return status;

		status = synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, tested_phy, PHY_Reg3, &phy_id2);
		if (status)
			return status;

		TR2("phy_id1=0x%x phy_id2=0x%x\n", phy_id1 ,phy_id2);

		if ( (phy_id1 != 0xffff) && (phy_id2 != 0xffff) ) {
			TR2("The selected address of the phy is = 0x%x\n", tested_phy);
			gmacdev->PhyBase = tested_phy ;

			//Turn on the LED (for test)
			//status = synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, 0x10, &data);
			//synop_gmac_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, 0x10 , data | 0x60 );
			return 0;
		}
	}

	return -1;
}

/**
  * Checks and initialze phy.
  * This function checks whether the phy initialization is complete.
  * @param[in] pointer to synop_gmac_device.
  * \return 0 if success else returns the error number.
  */
static s32
synop_gmac_check_phy_init(synop_gmac_device *gmacdev)
{
	//u32 addr;
	s32 status = -ESYNOPGMACNOERR;
	//s32 loop_count;
	u16 phyReg0Img, phyReg1Img, phyReg4Img, phyReg5Img, phyReg6Img;
	s32 autoNegTimeOut;
	u16 matchedStat;

	//this work around the problem we face at Versatile when the PHY is configured
	//with the different address each reset
	if (gmacdev->PhyBase == DEFAULT_PHY_UNDEFINE) {
		if (synop_gmac_phy_select_port(gmacdev) != 0)
			return ESYNOPGMACPHYERR;
	}

	if (synop_gmac_phy_check_link_status(gmacdev) == 0) {
		//this case the Link is down or the cable is disconnected hence cann't perform Auto Negotiate
		gmacdev->DuplexMode = FULLDUPLEX;
		gmacdev->Speed      =   SPEED100;
		TR1("No Link\n");
		TR2("Link is down setting defauts values\n");
		return -ESYNOPGMACNOERR;
	} else {
		TR2("Link is up\n");
	}

	//Prepare the to the auto negotiation proccess
	TR2("\n");
	status = synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg0, &phyReg0Img);
	if (status)
		return status;

	if (phyReg0Img == Mii_error) {
		TR0("Error: Unable to start auto negotiation \n");
		return -ESYNOPGMACPHYERR;
	}

	phyReg0Img  &= (~PHY_Reg0_AUTONEG);
	status = synop_gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg0, phyReg0Img);
	if (status)
		return status;
	// Set Local PHY Capabilities for Auto-Negotiation Advertisement
	status =  synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, PHY_Reg4, &phyReg4Img);
	if (status)
		return status;
	
	phyReg4Img |= PHY_Reg4_5_PAUSE_OP; // Supports handling RX Pause Frame

	phyReg4Img |= (PHY_Reg4_5_FD_100M | PHY_Reg4_5_HD_100M | PHY_Reg4_5_FD_10M  | PHY_Reg4_5_HD_10M);    // | 0x01E0

	//Please remove this line (only test 10M case)
	//phyReg4Img |= (PHY_Reg4_5_FD_10M  | PHY_Reg4_5_HD_10M );
	//phyReg4Img &= ~(PHY_Reg4_5_FD_100M | PHY_Reg4_5_HD_100M);

	// Speed: 100, Duplex: Full
	phyReg0Img = PHY_Reg0_SPEED | PHY_Reg0_DUPLEX;              // = 0x2100

	// Set Auto-Negotiation Capabilities at PHY
	status = synop_gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg4, phyReg4Img);
	if (status)
		return status;
	TR2("\n");
	/* Restart PHY's Auto-Negotiation, with maximal requested setting */
	phyReg0Img |= (PHY_Reg0_AUTONEG | PHY_Reg0_AUTONEGRST);
	status = synop_gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg0, phyReg0Img);
	if (status)
		return status;
	TR2("\n");
	/* Wait for Auto-Negotiation Results */
	autoNegTimeOut = DEFAULT_LOOP_VARIABLE;
	//autoNegTimeOut = 0x7fffffff;
	while (1) {
		// Wait a few  mSeconds
		//udelay(DEFAULT_DELAY_VARIABLE);
		udelay(1000000);

		// Check for Auto-Negotiation completion
		status =  synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg1, &phyReg1Img);
		if (status){
				TR2("\n");
				return status;
		}

		if (phyReg1Img & PHY_Reg1_AUTONEGDONE) {
				// Auto-Negotiation completed (Successful or Not!)
				TR2("Auto-Negotiation completed\n");//Auto-Negotiation completed (Successful or Not!)
				break;
		}

		//if (autoNegTimeOut-- == 0) {
		//		TR0("Auto-Negotiation time out\n");
		//		return -ESYNOPGMACPHYERR;
		//}
	} // end_while
	TR2("\n");
	/* Determine Auto-Ngotiation Results */
	status =  synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg6, &phyReg6Img);
	if (status)
		return status;
	TR2("\n");
	if (phyReg6Img & PHY_Reg6_CapAutoNeg) {
		/* Remote PHY (Link Partner) is capable to Auto Negotiate.
		** Local PHY has now exchanged Duplex & Speed Capabilities
		** with the Remote PHY, and vice Versa.
		*/
		status =  synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg4, &phyReg4Img);

		if (status)
			return status;
		TR2("phyReg4Img=0x%x\n",phyReg4Img);

		status =  synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg5, &phyReg5Img);
		if (status)
			return status;
		TR2("phyReg5Img=0x%x\n",phyReg5Img);

		matchedStat = phyReg4Img & phyReg5Img;

		if (matchedStat & PHY_Reg4_5_FD_100M) {
			gmacdev->DuplexMode = FULLDUPLEX;
			gmacdev->Speed   = SPEED100;
		} else if (matchedStat & PHY_Reg4_5_HD_100M) {
			gmacdev->DuplexMode = HALFDUPLEX;
			gmacdev->Speed   = SPEED100;
		} else if (matchedStat & PHY_Reg4_5_FD_10M) {
			gmacdev->DuplexMode = FULLDUPLEX;
			gmacdev->Speed   = SPEED10;
		} else if (matchedStat & PHY_Reg4_5_HD_10M) {
			gmacdev->DuplexMode = HALFDUPLEX;
			gmacdev->Speed   = SPEED10;
		} else {
			TR2("\n");
			return -ESYNOPGMACPHYERR;
		}

		status = synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg0, &phyReg0Img);
		if (status)
			return status;
		TR2("phyReg0Img=0x%x\n",phyReg0Img);

		status = synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg5, &phyReg5Img);
		if (status)
			return status;
		TR2("phyReg5Img=0x%x\n",phyReg5Img);

		// Prepare to disable auto negotiation
		phyReg0Img &= (~PHY_Reg0_AUTONEG);

		if (gmacdev->DuplexMode == FULLDUPLEX)
			phyReg0Img |= PHY_Reg0_DUPLEX;
		else
			phyReg0Img &= (~PHY_Reg0_DUPLEX);
		if (gmacdev->Speed == SPEED100)
			phyReg0Img |= PHY_Reg0_SPEED;
		else
			phyReg0Img &= (~PHY_Reg0_SPEED);

		//write the result of the xor
		status = synop_gmac_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, PHY_Reg0, phyReg0Img);

		printf("Link is up in %s mode with %s speed\n",(gmacdev->DuplexMode == FULLDUPLEX) ? "FULL DUPLEX": "HALF DUPLEX" , 
		   (gmacdev->Speed == SPEED100) ? "100" : "10");

		status = synop_gmac_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, PHY_Reg5, &phyReg5Img );
		if (status)
			return status;
		TR2("phyReg5Img=0x%x\n",phyReg5Img);

		TR2("\n");

		// Mark Auto-Negotiation Succeeded
		// Return OK
		return -ESYNOPGMACNOERR;
	} // end_if A-N success

	return -ESYNOPGMACNOERR;
}

/**
  * Sets the Mac address in to GMAC register.
  * This function sets the MAC address to the MAC register in question.
  * @param[in] pointer to synop_gmac_device to populate mac dma and phy addresses.
  * @param[in] Register offset for Mac address high
  * @param[in] Register offset for Mac address low
  * @param[in] buffer containing mac address to be programmed.
  * \return 0 upon success. Error code upon failure.
  */
static s32
synop_gmac_set_mac_addr(synop_gmac_device *gmacdev, u32 MacHigh, u32 MacLow, u8 *MacAddr)
{
	u32 data;

	data = (MacAddr[5] << 8) | MacAddr[4];
	synop_gmac_write_reg((u32 *)gmacdev->MacBase,MacHigh,data);
	data = (MacAddr[3] << 24) | (MacAddr[2] << 16) | (MacAddr[1] << 8) | MacAddr[0];
	synop_gmac_write_reg((u32 *)gmacdev->MacBase,MacLow,data);

	return 0;
}

/**
  * Get the Mac address in to the address specified.
  * The mac register contents are read and written to buffer passed.
  * @param[in] pointer to synopGMACdevice to populate mac dma and phy addresses.
  * @param[in] Register offset for Mac address high
  * @param[in] Register offset for Mac address low
  * @param[out] buffer containing the device mac address.
  * \return 0 upon success. Error code upon failure.
  */
static s32 
synop_gmac_get_mac_addr(synop_gmac_device *gmacdev, u32 MacHigh, u32 MacLow, u8 *MacAddr)
{
	u32 data;
		
	data = synop_gmac_read_reg((u32 *)gmacdev->MacBase,MacHigh);
	MacAddr[5] = (data >> 8) & 0xff;
	MacAddr[4] = (data)        & 0xff;

	data = synop_gmac_read_reg((u32 *)gmacdev->MacBase,MacLow);
	MacAddr[3] = (data >> 24) & 0xff;
	MacAddr[2] = (data >> 16) & 0xff;
	MacAddr[1] = (data >> 8 ) & 0xff;
	MacAddr[0] = (data )      & 0xff;

	return 0;
}

/**
  * Attaches the synopGMAC device structure to the hardware.
  * Device structure is populated with MAC/DMA and PHY base addresses.
  * @param[in] pointer to synop_gmac_device to populate mac dma and phy addresses.
  * @param[in] GMAC IP mac base address.
  * @param[in] GMAC IP dma base address.
  * @param[in] GMAC IP phy base address.
  * \return 0 upon success. Error code upon failure.
  * \note This is important function. No kernel api provided by Synopsys
  */
s32
synop_gmac_attach(synop_gmac_device *gmacdev, u32 macBase, u32 dmaBase, u32 phyBase)
{
	u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;

	eth_getenv_enetaddr("ethaddr", mac_addr0);

	/*Make sure the Device data strucure is cleared before we proceed further*/
	//memset((void *) gmacdev,0,sizeof(synop_gmac_device));
	/*Populate the mac and dma base addresses*/
	gmacdev->MacBase = macBase;
	gmacdev->DmaBase = dmaBase;
	gmacdev->PhyBase = phyBase;

	TR2("gmacdev->MacBase = 0x%x gmacdev->DmaBase = 0x%x gmacdev->PhyBase = 0x%x\n",gmacdev->MacBase , gmacdev->DmaBase , gmacdev->PhyBase);

	/* Program/flash in the station/IP's Mac address */
	synop_gmac_set_mac_addr(gmacdev, GmacAddr0High, GmacAddr0Low, mac_addr0);

	return 0;
}

/**
  * Initialize the rx descriptors for ring or chain mode operation.
  *	- Status field is initialized to 0.
  *	- EndOfRing set for the last descriptor.
  *	- buffer1 and buffer2 set to 0 for ring mode of operation. (note)
  *	- data1 and data2 set to 0. (note)
  * @param[in] pointer to dma_desc structure.
  * @param[in] whether end of ring
  * \return void.
  * \note Initialization of the buffer1, buffer2, data1,data2 and status are not done here. This only initializes whether one wants to use this descriptor
  * in chain mode or ring mode. For chain mode of operation the buffer2 and data2 are programmed before calling this function.
  */
static void
synop_gmac_rx_desc_init_ring(dma_desc *desc, unsigned int last_ring_desc)
{
	desc->status = 0;
	desc->length = last_ring_desc ? RxDescEndOfRing : 0;
	desc->buffer1 = 0;
	desc->buffer2 = 0;
	desc->data1 = 0;
	desc->data2 = 0;
}

/**
  * Initialize the tx descriptors for ring or chain mode operation.
  * - Status field is initialized to 0.
  *	- EndOfRing set for the last descriptor.
  *	- buffer1 and buffer2 set to 0 for ring mode of operation. (note)
  *	- data1 and data2 set to 0. (note)
  * @param[in] pointer to dma_desc structure.
  * @param[in] whether end of ring
  * \return void.
  * \note Initialization of the buffer1, buffer2, data1,data2 and status are not done here. This only initializes whether one wants to use this descriptor
  * in chain mode or ring mode. For chain mode of operation the buffer2 and data2 are programmed before calling this function.
  */
static void
synop_gmac_tx_desc_init_ring(dma_desc *desc, unsigned int last_ring_desc)
{
	desc->length = last_ring_desc? TxDescEndOfRing : 0;
	desc->buffer1 = 0;
	desc->buffer2 = 0;
	desc->data1 = 0;
	desc->data2 = 0;
	desc->status = 0;
}

/**
  * Initialize the rx descriptors for chain mode of operation.
  *	- Status field is initialized to 0.
  *	- EndOfRing set for the last descriptor.
  *	- buffer1 and buffer2 set to 0.
  *	- data1 and data2 set to 0.
  * @param[in] pointer to dma_desc structure.
  * @param[in] whether end of ring
  * \return void.
  */

static void
synop_gmac_rx_desc_init_chain(dma_desc * desc)
{
	desc->status = 0;
	desc->length = RxDescChain;
	desc->buffer1 = 0;
	desc->data1 = 0;
}

/**
  * Initialize the rx descriptors for chain mode of operation.
  *	- Status field is initialized to 0.
  *	- EndOfRing set for the last descriptor.
  *	- buffer1 and buffer2 set to 0.
  *	- data1 and data2 set to 0.
  * @param[in] pointer to dma_desc structure.
  * @param[in] whether end of ring
  * \return void.
  */
static void
synop_gmac_tx_desc_init_chain(dma_desc * desc)
{
	desc->length = TxDescChain;
	desc->buffer1 = 0;
	desc->data1 = 0;
}

static s32
synop_gmac_init_tx_rx_desc_queue(synop_gmac_device *gmacdev)
{
	s32 i;
	for(i =0; i < gmacdev -> TxDescCount; i++)
		synop_gmac_tx_desc_init_ring(gmacdev->TxDesc + i, (i == gmacdev->TxDescCount-1) ? 1 : 0);

	TR2("At\n");
	for(i =0; i < gmacdev -> RxDescCount; i++)
		synop_gmac_rx_desc_init_ring(gmacdev->RxDesc + i, (i == gmacdev->RxDescCount-1) ? 1 : 0);

	gmacdev->TxNext = 0;
	gmacdev->TxBusy = 0;
	gmacdev->RxNext = 0;
	gmacdev->RxBusy = 0;

	return -ESYNOPGMACNOERR;
}

/**
  * Programs the DmaRxBaseAddress with the Rx descriptor base address.
  * Rx Descriptor's base address is available in the gmacdev structure. This function progrms the
  * Dma Rx Base address with the starting address of the descriptor ring or chain.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_init_rx_desc_base(synop_gmac_device *gmacdev)
{
	u32 address = 0;

	address = virt_to_bus((u32)gmacdev->RxDesc);

	TR2("0x%x\n",address);
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase,DmaRxBaseAddr,address);
}

/**
  * Programs the DmaTxBaseAddress with the Tx descriptor base address.
  * Tx Descriptor's base address is available in the gmacdev structure. This function progrms the
  * Dma Tx Base address with the starting address of the descriptor ring or chain.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_init_tx_desc_base(synop_gmac_device *gmacdev)
{
	u32 address = 0;

	address = virt_to_bus((u32)gmacdev->TxDesc);

	TR2("0x%x\n",address);
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase,DmaTxBaseAddr,address);
}

/**
  * Checks whether the descriptor is owned by DMA.
  * If descriptor is owned by DMA then the OWN bit is set to 1. This API is same for both ring and chain mode.
  * @param[in] pointer to dma_desc structure.
  * \return returns true if Dma owns descriptor and false if not.
  */
static unsigned int
synop_gmac_is_desc_owned_by_dma(dma_desc *desc)
{
	return ( ((desc->status & DescOwnByDma) == DescOwnByDma) ? 1 : 0 );
}

/**
  * returns the byte length of received frame including CRC.
  * This returns the no of bytes received in the received ethernet frame including CRC(FCS).
  * @param[in] pointer to dma_desc structure.
  * \return returns the length of received frame lengths in bytes.
  */
static u32
synop_gmac_get_rx_desc_frame_length(u32 status)
{
	return ((status & DescFrameLengthMask) >> DescFrameLengthShift);
}

/**
  * Checks whether the descriptor is valid
  * if no errors such as CRC/Receive Error/Watchdog Timeout/Late collision/Giant Frame/Overflow/Descriptor
  * error the descritpor is said to be a valid descriptor.
  * @param[in] pointer to dma_desc structure.
  * \return True if desc valid. false if error.
  */
static unsigned int
synop_gmac_is_desc_valid(u32 status)
{
	return (((status & DescError) == 0) ? 1 : 0);
}

/**
  * Checks whether the descriptor is empty.
  * If the buffer1 and buffer2 lengths are zero in ring mode descriptor is empty.
  * In chain mode buffer2 length is 0 but buffer2 itself contains the next descriptor address.
  * @param[in] pointer to dma_desc structure.
  * \return returns true if descriptor is empty, false if not empty.
  */
static unsigned int
synop_gmac_is_desc_empty(dma_desc *desc)
{
	//if both the buffer1 length and buffer2 length are zero desc is empty
	return(( ((desc->length  & DescSize1Mask) == 0) && ((desc->length  & DescSize2Mask) == 0)) ? 1 : 0 );
}

/**
  * Checks whether the rx descriptor is valid.
  * if rx descripor is not in error and complete frame is available in the same descriptor
  * @param[in] pointer to dma_desc structure.
  * \return returns true if no error and first and last desc bits are set, otherwise it returns false.
  */
static unsigned int
synop_gmac_is_rx_desc_valid(u32 status)
{
	return ( ((status & DescError) == 0) && ((status & DescRxFirst) == DescRxFirst) && ((status & DescRxLast) == DescRxLast) ? 1 : 0);
}

/**
  * Checks whether this rx descriptor is last rx descriptor.
  * This returns true if it is last descriptor either in ring mode or in chain mode.
  * @param[in] pointer to devic structure.
  * @param[in] pointer to dma_desc structure.
  * \return returns true if it is last descriptor, false if not.
  * \note This function should not be called before initializing the descriptor using synop_gmac_desc_init().
  */
static unsigned int
synop_gmac_is_last_rx_desc(synop_gmac_device *gmacdev, dma_desc *desc)
{
	return ( ( ((desc->length & RxDescEndOfRing) == RxDescEndOfRing) || ((u32)gmacdev->RxDesc == desc->data2) ) ? 1 : 0);
}

/**
  * Checks whether this tx descriptor is last tx descriptor.
  * This returns true if it is last descriptor either in ring mode or in chain mode.
  * @param[in] pointer to devic structure.
  * @param[in] pointer to dma_desc structure.
  * \return returns true if it is last descriptor, false if not.
  * \note This function should not be called before initializing the descriptor using synop_gmac_desc_init().
  */
static unsigned int
synop_gmac_is_last_tx_desc(synop_gmac_device *gmacdev, dma_desc *desc)
{
	return ( ((desc->length & TxDescEndOfRing) == TxDescEndOfRing) || ((u32)gmacdev->TxDesc == desc->data2) ? 1 : 0 );
}

/**
  * Checks whether this rx descriptor is in chain mode.
  * This returns true if it is this descriptor is in chain mode.
  * @param[in] pointer to dma_desc structure.
  * \return returns true if chain mode is set, false if not.
  */
static unsigned int
synop_gmac_is_rx_desc_chained(dma_desc *desc)
{
	return (((desc->length & RxDescChain) == RxDescChain) ? 1 : 0);
}

/**
  * Checks whether this tx descriptor is in chain mode.
  * This returns true if it is this descriptor is in chain mode.
  * @param[in] pointer to dma_desc structure.
  * \return returns true if chain mode is set, false if not.
  */
static unsigned int
synop_gmac_is_tx_desc_chained(dma_desc * desc)
{
	return (((desc->length & TxDescChain) == TxDescChain) ? 1 : 0);
}

/**
  * Get the index and address of Tx desc.
  * This api is same for both ring mode and chain mode.
  * This function tracks the tx descriptor the DMA just closed after the transmission of data from this descriptor is
  * over. This returns the descriptor fields to the caller.
  * @param[in] pointer to synop_gmac_device.
  * @param[out] status field of the descriptor.
  * @param[out] Dma-able buffer1 pointer.
  * @param[out] length of buffer1 (Max is 2048).
  * @param[out] virtual pointer for buffer1.
  * @param[out] Dma-able buffer2 pointer.
  * @param[out] length of buffer2 (Max is 2048).
  * @param[out] virtual pointer for buffer2.
  * @param[out] u32 data indicating whether the descriptor is in ring mode or chain mode.
  * \return returns present tx descriptor index on success. Negative value if error.
  */
static s32
synop_gmac_get_tx_qptr(synop_gmac_device *gmacdev, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1, u32 *Buffer2, u32 * Length2, u32 * Data2 )
{
	u32 txover = gmacdev->TxBusy;
	dma_desc *txdesc = gmacdev->TxBusyDesc;
	
	if (synop_gmac_is_desc_owned_by_dma(txdesc))
		return -1;
	if (synop_gmac_is_desc_empty(txdesc))
		return -1;

	(gmacdev->BusyTxDesc)--; //busy tx descriptor is reduced by one as it will be handed over to Processor now

	if (Status != 0)
		*Status = txdesc->status;

	if (Buffer1 != 0)
		*Buffer1 = txdesc->buffer1;
	if (Length1 != 0)
		*Length1 = (txdesc->length & DescSize1Mask) >> DescSize1Shift;
	if (Data1 != 0)
		*Data1 = txdesc->data1;

	if (Buffer2 != 0)
		*Buffer2 = txdesc->buffer2;
	if (Length2 != 0)
		*Length2 = (txdesc->length & DescSize2Mask) >> DescSize2Shift;
	if (Data1 != 0)
		*Data2 = txdesc->data2;

	gmacdev->TxBusy     = synop_gmac_is_last_tx_desc(gmacdev,txdesc) ? 0 : txover + 1;

	if (synop_gmac_is_tx_desc_chained(txdesc)) {
		gmacdev->TxBusyDesc = (dma_desc *)txdesc->data2;
		synop_gmac_tx_desc_init_chain(txdesc);
	} else {
		gmacdev->TxBusyDesc = synop_gmac_is_last_tx_desc(gmacdev,txdesc) ? gmacdev->TxDesc : (txdesc + 1);
		synop_gmac_tx_desc_init_ring(txdesc, synop_gmac_is_last_tx_desc(gmacdev, txdesc));
	}
	TR2("%02d %08x %08x %08x %08x %08x %08x %08x\n", txover, (u32)txdesc, txdesc->status, txdesc->length, txdesc->buffer1,txdesc->buffer2,txdesc->data1,txdesc->data2);

	return txover;
}

/**
  * Populate the tx desc structure with the buffer address.
  * Once the driver has a packet ready to be transmitted, this function is called with the
  * valid dma-able buffer addresses and their lengths. This function populates the descriptor
  * and make the DMA the owner for the descriptor. This function also controls whetther Checksum
  * offloading to be done in hardware or not.
  * This api is same for both ring mode and chain mode.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] Dma-able buffer1 pointer.
  * @param[in] length of buffer1 (Max is 2048).
  * @param[in] virtual pointer for buffer1.
  * @param[in] Dma-able buffer2 pointer.
  * @param[in] length of buffer2 (Max is 2048).
  * @param[in] virtual pointer for buffer2.
  * @param[in] u32 data indicating whether the descriptor is in ring mode or chain mode.
  * @param[in] u32 indicating whether the checksum offloading in HW/SW.
  * \return returns present tx descriptor index on success. Negative value if error.
  */
s32
synop_gmac_set_tx_qptr(synop_gmac_device *gmacdev, u32 Buffer1, u32 Length1, u32 Data1, u32 Buffer2, u32 Length2, u32 Data2,u32 offload_needed)
{
	u32 txnext = gmacdev->TxNext;
	dma_desc *txdesc = gmacdev->TxNextDesc;

	if (!synop_gmac_is_desc_empty(txdesc)) {
		TR0("TX descriptor is not empty\n");
		return -1;
	}

	(gmacdev->BusyTxDesc)++; //busy tx descriptor is reduced by one as it will be handed over to Processor now

	txdesc->length |= (((Length1 <<DescSize1Shift) & DescSize1Mask) | ((Length2 <<DescSize2Shift) & DescSize2Mask));

	txdesc->length |=  (DescTxFirst | DescTxLast | DescTxIntEnable); //Its always assumed that complete data will fit in to one descriptor

	txdesc->buffer1 = Buffer1;
	txdesc->data1 = Data1;

	txdesc->buffer2 = Buffer2;
	txdesc->data2 = Data2;

	txdesc->status = DescOwnByDma;

	gmacdev->TxNext = synop_gmac_is_last_tx_desc(gmacdev,txdesc) ? 0 : txnext + 1;
	gmacdev->TxNextDesc = synop_gmac_is_last_tx_desc(gmacdev,txdesc) ? gmacdev->TxDesc : (txdesc + 1);

	TR2("%02d %08x %08x %08x %08x %08x %08x %08x\n",txnext,(u32)txdesc,txdesc->status,txdesc->length,txdesc->buffer1,txdesc->buffer2,txdesc->data1,txdesc->data2);

	return txnext;
}

/**
  * Prepares the descriptor to receive packets.
  * The descriptor is allocated with the valid buffer addresses (sk_buff address) and the length fields
  * and handed over to DMA by setting the ownership. After successful return from this function the
  * descriptor is added to the receive descriptor pool/queue.
  * This api is same for both ring mode and chain mode.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] Dma-able buffer1 pointer.
  * @param[in] length of buffer1 (Max is 2048).
  * @param[in] Dma-able buffer2 pointer.
  * @param[in] length of buffer2 (Max is 2048).
  * @param[in] u32 data indicating whether the descriptor is in ring mode or chain mode.
  * \return returns present rx descriptor index on success. Negative value if error.
  */
static s32
synop_gmac_set_rx_qptr(synop_gmac_device *gmacdev, u32 Buffer1, u32 Length1, u32 Data1, u32 Buffer2, u32 Length2, u32 Data2)
{
	u32 rxnext = gmacdev->RxNext;
	dma_desc * rxdesc = gmacdev->RxNextDesc;

	if (!synop_gmac_is_desc_empty(rxdesc))
		return -1;

	if (synop_gmac_is_rx_desc_chained(rxdesc)) {
		TR2("Chain \n");
		rxdesc->length |= ((Length1 <<DescSize1Shift) & DescSize1Mask);

		rxdesc->buffer1 = Buffer1;
		rxdesc->data1 = Data1;

		if ((rxnext % MODULO_INTERRUPT) != 0)
			rxdesc->length |= RxDisIntCompl;

		rxdesc->status = DescOwnByDma;

		gmacdev->RxNext     = synop_gmac_is_last_rx_desc(gmacdev,rxdesc) ? 0 : rxnext + 1;
		gmacdev->RxNextDesc = (dma_desc *)rxdesc->data2;
	} else {
		TR2("Ring \n");
		rxdesc->length |= (((Length1 <<DescSize1Shift) & DescSize1Mask) | ((Length2 << DescSize2Shift) & DescSize2Mask));

		rxdesc->buffer1 = Buffer1;
		rxdesc->data1 = Data1;

		rxdesc->buffer2 = Buffer2;
		rxdesc->data2 = Data2;

		if ((rxnext % MODULO_INTERRUPT) != 0)
			rxdesc->length |= RxDisIntCompl;

		rxdesc->status = DescOwnByDma;

		gmacdev->RxNext     = synop_gmac_is_last_rx_desc(gmacdev,rxdesc) ? 0 : rxnext + 1;
		gmacdev->RxNextDesc = synop_gmac_is_last_rx_desc(gmacdev,rxdesc) ? gmacdev->RxDesc : (rxdesc + 1);
	}
	TR2("%02d %08x %08x %08x %08x %08x %08x %08x\n",rxnext,(u32)rxdesc,rxdesc->status,rxdesc->length,rxdesc->buffer1,rxdesc->buffer2,rxdesc->data1,rxdesc->data2);
	(gmacdev->BusyRxDesc)++; //One descriptor will be given to Hardware. So busy count incremented by one

	return rxnext;
}

static s32
synop_gmac_init_rx_qptr(synop_gmac_device *gmacdev)
{
	void *skb = NULL;
	void *skb_on_sdram = NULL;
	unsigned int i = 0;

	do {
#ifdef DATA_ON_DDR
		skb = (void *)(&rx_buffers[i][0]);
#else
		skb = (void *)(rx_buffers + i*(RX_BUF_SIZE));
#endif

		skb_on_sdram = virt_to_bus(skb);

		TR2("skb_on_sdram = 0x%x skb = 0x%x i = %d\n", (u32)skb_on_sdram , (u32)skb , i);

		synop_gmac_set_rx_qptr(gmacdev,(u32)skb_on_sdram, RX_BUF_SIZE, (u32)skb,0,0,0);

		i++;
	} while (i < RECEIVE_DESC_SIZE);

	return 0;
}

/**
  * Get back the descriptor from DMA after data has been received.
  * When the DMA indicates that the data is received (interrupt is generated), this function should be
  * called to get the descriptor and hence the data buffers received. With successful return from this
  * function caller gets the descriptor fields for processing. check the parameters to understand the
  * fields returned.`
  * @param[in] pointer to synop_gmac_device.
  * @param[out] pointer to hold the status of DMA.
  * @param[out] Dma-able buffer1 pointer.
  * @param[out] pointer to hold length of buffer1 (Max is 2048).
  * @param[out] virtual pointer for buffer1.
  * @param[out] Dma-able buffer2 pointer.
  * @param[out] pointer to hold length of buffer2 (Max is 2048).
  * @param[out] virtual pointer for buffer2.
  * \return returns present rx descriptor index on success. Negative value if error.
  */
static s32
synop_gmac_get_rx_qptr(synop_gmac_device *gmacdev, u32 * Status, u32 * Buffer1, u32 * Length1, u32 * Data1, u32 * Buffer2, u32 * Length2, u32 * Data2)
{
	u32 rxnext = gmacdev->RxBusy;	// index of descriptor the DMA just completed. May be useful when data
					//is spread over multiple buffers/descriptors
	dma_desc *rxdesc = gmacdev->RxBusyDesc;

	if (synop_gmac_is_desc_owned_by_dma(rxdesc))
		return -1;
	if (synop_gmac_is_desc_empty(rxdesc))
		return -1;

	if (Status != 0)
		*Status = rxdesc->status;// send the status of this descriptor

	if (Length1 != 0)
		*Length1 = (rxdesc->length & DescSize1Mask) >> DescSize1Shift;
	if (Buffer1 != 0)
		*Buffer1 = rxdesc->buffer1;
	if (Data1 != 0)
		*Data1 = rxdesc->data1;

	if (Length2 != 0)
		*Length2 = (rxdesc->length & DescSize2Mask) >> DescSize2Shift;
	if (Buffer2 != 0)
		*Buffer2 = rxdesc->buffer2;
	if (Data1 != 0)
		*Data2 = rxdesc->data2;

	gmacdev->RxBusy = synop_gmac_is_last_rx_desc(gmacdev, rxdesc) ? 0 : rxnext + 1;

	if (synop_gmac_is_rx_desc_chained(rxdesc)){
		gmacdev->RxBusyDesc = (dma_desc *)rxdesc->data2;
		synop_gmac_rx_desc_init_chain(rxdesc);
		//synop_gmac_desc_init_chain(rxdesc, synop_gmac_is_last_rx_desc(gmacdev,rxdesc),0,0);
	} else {
		gmacdev->RxBusyDesc = synop_gmac_is_last_rx_desc(gmacdev,rxdesc) ? gmacdev->RxDesc : (rxdesc + 1);
		synop_gmac_rx_desc_init_ring(rxdesc, synop_gmac_is_last_rx_desc(gmacdev,rxdesc));
	}
	TR2("%02d %08x %08x %08x %08x %08x %08x %08x\n",rxnext,(u32)rxdesc,rxdesc->status,rxdesc->length,rxdesc->buffer1,rxdesc->buffer2,rxdesc->data1,rxdesc->data2);
	(gmacdev->BusyRxDesc)--; //This returns one descriptor to processor. So busy count will be decremented by one

	return rxnext;
}

/**
  * Clears all the pending interrupts.
  * If the Dma status register is read then all the interrupts gets cleared
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_clear_interrupt(synop_gmac_device *gmacdev)
{
	u32 data;
	data = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaStatus);
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaStatus, data);
}

/**
  * Returns the all unmasked interrupt status after reading the DmaStatus register.
  * @param[in] pointer to synop_gmac_device.
  * \return 0 upon success. Error code upon failure.
  */
static u32
synop_gmac_get_interrupt_type(synop_gmac_device *gmacdev, u32 clear_mask)
{
	u32 data;
	u32 interrupts = 0;
	data = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaStatus);
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaStatus ,data & clear_mask); //manju: I think this is the appropriate location to clear the interrupts
	TR2("DMA status reg is  %08x\n",data);
	if (data & DmaIntErrorMask)	interrupts |= synopGMACDmaError;
	if (data & DmaIntRxNormMask)	interrupts |= synopGMACDmaRxNormal;
	if (data & DmaIntRxAbnMask)	interrupts |= synopGMACDmaRxAbnormal;
	if (data & DmaIntRxStoppedMask)	interrupts |= synopGMACDmaRxStopped;
	if (data & DmaIntTxNormMask)	interrupts |= synopGMACDmaTxNormal;
	if (data & DmaIntTxAbnMask)	interrupts |= synopGMACDmaTxAbnormal;
	if (data & DmaIntTxStoppedMask)	interrupts |= synopGMACDmaTxStopped;

	if (data & (0x1 << 0) )
		TR2("DMA status transmission is finished\n");
	if (data & (0x1 << 1) )
		TR2("DMA status transmission is stopped\n");

	if (data & (0x1 << 2) ) {
		TR2("DMA status Transmit Buffer Unavailable - Transmission is suspended\n");

		if( ((data & (0x00700000)) >> 20)  == 0x0)  TR2("Stopped; Reset or Stop Transmit Command issued.\n");
		if( ((data & (0x00700000)) >> 20)  == 0x1) TR2("Running; Fetching Transmit Transfer Descriptor.\n");
		if( ((data & (0x00700000)) >> 20)  == 0x2) TR2("Running; Waiting for status.\n");
		if( ((data & (0x00700000)) >> 20)  == 0x3) TR2("Running; Reading Data from host memory buffer and queuing it to\n");

		if( ((data & (0x00700000)) >> 20)  == 0x4) TR2("TIME_STAMP write state.\n");
		if( ((data & (0x00700000)) >> 20)  == 0x5) TR2("Reserved for future use.\n");
		if( ((data & (0x00700000)) >> 20)  == 0x6) TR2("Suspended; Transmit Descriptor Unavailable or Transmit Buffer Underflow.\n");
		if( ((data & (0x00700000)) >> 20)  == 0x7) TR2("Running; Closing Transmit Descriptor.\n");
	}

	if (data & (0x1 << 3) )	TR2("DMA status Transmit Jabber Timeout - transmitter had been excessively active\n");
	if (data & (0x1 << 4) )	TR1("DMA status Receive Overflow\n");
	if (data & (0x1 << 5) )	TR1("DMA status Transmit Underflow\n");
	if (data & (0x1 << 6) )	TR2("DMA status Receive Interrupt - completion of frame reception\n");
	if (data & (0x1 << 7) )	TR2("DMA status Receive Buffer Unavailable - Receive Process is suspended\n");
	if (data & (0x1 << 8) )	TR2("DMA status Receive Process Stopped\n");
	if (data & (0x1 << 9) )	TR2("DMA status Receive Watchdog Timeout - a frame with a length greater than 2,048 bytes is received\n");
	if (data & (0x1 << 10) )	TR2("DMA status Early Transmit Interrupt - the frame to be transmitted was fully transferred to the MTL transmit FIFO\n");

	if (data & (0x1 << 13) ) {
		TR0("DMA status Fatal Bus Error Interrupt - \n");
		if(data & (0x1 << 23) )
			TR0("Error during data transfer by TxDMA\n");
		else
			TR0("Error during data transfer by RxDMA\n");

		if(data & (0x1 << 24) )
			TR0("Error during read transfer\n");
		else
			TR0("Error during write transfer\n");

		if(data & (0x1 << 25) )
			TR0("Error during descriptor access\n");
		else
			TR0("Error during data buffer access\n");
	}

	if (data & (0x1 << 14) )	TR2("DMA status Early Receive Interrupt - the DMA had filled the first data buffer of the packet\n");

	if ( ((data & (0x000E0000)) >> 17)  == 0x0) TR2("Stopped: Reset or Stop Receive Command issued.\n");
	if ( ((data & (0x000E0000)) >> 17)  == 0x1) TR2("Running: Fetching Receive Transfer Descriptor.\n");
	if ( ((data & (0x000E0000)) >> 17)  == 0x2) TR2("Reserved for future use.\n");
	if ( ((data & (0x000E0000)) >> 17)  == 0x3) TR2("Running: Waiting for receive packet.\n");
	if ( ((data & (0x000E0000)) >> 17)  == 0x4) TR2("Suspended: Receive Descriptor Unavailable.\n");
	if ( ((data & (0x000E0000)) >> 17)  == 0x5) TR2("Running: Closing Receive Descriptor.\n");
	if ( ((data & (0x000E0000)) >> 17)  == 0x6) TR2("TIME_STAMP write state.\n");
	if ( ((data & (0x000E0000)) >> 17)  == 0x7) TR2("Running: Transferring the receive packet data from receive buffer to host memory.\n");

	return interrupts;
}

/**
  * Enable all the interrupts.
  * Enables the DMA interrupt as specified by the bit mask.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] bit mask of interrupts to be enabled.
  * \return returns void.
  */
static void
synop_gmac_enable_interrupt(synop_gmac_device *gmacdev, u32 interrupts)
{
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaInterrupt, interrupts);
}

/**
  * Disable all the interrupts.
  * Disables all DMA interrupts.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  * \note This function disabled all the interrupts, if you want to disable a particular interrupt then
  *  use synop_gmac_disable_interrupt().
  */
static void
synop_gmac_disable_interrupt_all(synop_gmac_device *gmacdev)
{
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaInterrupt, DmaIntDisable);
}

/**
  * Enable the DMA Reception.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_enable_dma_rx(synop_gmac_device *gmacdev)
{
	u32 data;
//	synop_gmac_set_bits((u32 *)gmacdev->DmaBase, DmaControl, DmaRxStart);
	data = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
	data |= DmaRxStart;
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl, data);
}

/**
  * Enable the DMA Transmission.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_enable_dma_tx(synop_gmac_device *gmacdev)
{
	u32 data;
//	synop_gmac_set_bits((u32 *)gmacdev->DmaBase, DmaControl, DmaTxStart);
	data = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
	data |= DmaTxStart;
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl, data);
}

/**
  * Resumes the DMA Transmission.
  * the DmaTxPollDemand is written. (the data writeen could be anything).
  * This forces the DMA to resume transmission.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
void
synop_gmac_resume_dma_tx(synop_gmac_device *gmacdev)
{
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaTxPollDemand, 0);
}

/**
  * Resumes the DMA Reception.
  * the DmaRxPollDemand is written. (the data writeen could be anything).
  * This forces the DMA to resume reception.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_resume_dma_rx(synop_gmac_device *gmacdev)
{
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaRxPollDemand, 0);
}

/**
  * Take ownership of this Descriptor.
  * The function is same for both the ring mode and the chain mode DMA structures.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
static void
synop_gmac_take_desc_ownership(dma_desc * desc)
{
	if (desc) {
		desc->status &= ~DescOwnByDma;  //Clear the DMA own bit
		//desc->status |= DescError;	// Set the error to indicate this descriptor is bad
	}
}

/**
  * Take ownership of all the rx Descriptors.
  * This function is called when there is fatal error in DMA transmission.
  * When called it takes the ownership of all the rx descriptor in rx descriptor pool/queue from DMA.
  * The function is same for both the ring mode and the chain mode DMA structures.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  * \note Make sure to disable the transmission before calling this function, otherwise may result in racing situation.
  */
static void
synop_gmac_take_desc_ownership_rx(synop_gmac_device *gmacdev)
{
	s32 i;
	dma_desc *desc = gmacdev->RxDesc;

	for (i = 0; i < gmacdev->RxDescCount; i++){
		if (synop_gmac_is_rx_desc_chained(desc)){ //This descriptor is in chain mode
			synop_gmac_take_desc_ownership(desc);
			desc = (dma_desc *)desc->data2;
		} else {
			synop_gmac_take_desc_ownership(desc + i);
		}
	}
}

/**
  * Take ownership of all the rx Descriptors.
  * This function is called when there is fatal error in DMA transmission.
  * When called it takes the ownership of all the tx descriptor in tx descriptor pool/queue from DMA.
  * The function is same for both the ring mode and the chain mode DMA structures.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  * \note Make sure to disable the transmission before calling this function, otherwise may result in racing situation.
  */
static void
synop_gmac_take_desc_ownership_tx(synop_gmac_device *gmacdev)
{
	s32 i;
	dma_desc *desc;
	desc = gmacdev->TxDesc;
	for (i = 0; i < gmacdev->TxDescCount; i++){
		if (synop_gmac_is_tx_desc_chained(desc)) { //This descriptor is in chain mode
			synop_gmac_take_desc_ownership(desc);
			desc = (dma_desc *)desc->data2;
		} else {
			synop_gmac_take_desc_ownership(desc + i);
		}
	}
}

/**
  * Disable the DMA for Transmission.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
void
synop_gmac_disable_dma_tx(synop_gmac_device *gmacdev)
{
	u32 data;
	//synop_gmac_clear_bits((u32 *)gmacdev->DmaBase, DmaControl, DmaTxStart);
	data = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
	data &= (~DmaTxStart);
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl, data);
}

/**
  * Disable the DMA for Reception.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
void
synop_gmac_disable_dma_rx(synop_gmac_device *gmacdev)
{
	u32 data;
	synop_gmac_clear_bits((u32 *)gmacdev->DmaBase, DmaControl, DmaRxStart);
	data = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
	data &= (~DmaRxStart);
	synop_gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl, data);
}

/*******************MMC APIs***************************************/

//#if defined CONFIG_GMAC_DEBUG_NOTICE
/**
  * Read the MMC Rx interrupt status.
  * @param[in] pointer to synop_gmac_device.
  * \return returns the Rx interrupt status.
  */
static u32
synop_gmac_read_mmc_rx_int_status(synop_gmac_device *gmacdev)
{
	return (synop_gmac_read_reg((u32 *)gmacdev->MacBase, GmacMmcIntrRx));
}

/**
  * Read the MMC Tx interrupt status.
  * @param[in] pointer to synop_gmac_device.
  * \return returns the Tx interrupt status.
  */
static u32
synop_gmac_read_mmc_tx_int_status(synop_gmac_device *gmacdev)
{
	return (synop_gmac_read_reg((u32 *)gmacdev->MacBase, GmacMmcIntrTx));
}
//#endif

/**
  * Disable the MMC Tx interrupt.
  * The MMC tx interrupts are masked out as per the mask specified.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] tx interrupt bit mask for which interrupts needs to be disabled.
  * \return returns void.
  */
static void
synop_gmac_disable_mmc_tx_interrupt(synop_gmac_device *gmacdev, u32 mask)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase,GmacMmcIntrMaskTx,mask);
}

/**
  * Disable the MMC Rx interrupt.
  * The MMC rx interrupts are masked out as per the mask specified.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] rx interrupt bit mask for which interrupts needs to be disabled.
  * \return returns void.
  */
static void
synop_gmac_disable_mmc_rx_interrupt(synop_gmac_device *gmacdev, u32 mask)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacMmcIntrMaskRx, mask);
}

/**
  * Disable the MMC ipc rx checksum offload interrupt.
  * The MMC ipc rx checksum offload interrupts are masked out as per the mask specified.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] rx interrupt bit mask for which interrupts needs to be disabled.
  * \return returns void.
  */
static void synop_gmac_disable_mmc_ipc_rx_interrupt(synop_gmac_device *gmacdev, u32 mask)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacMmcRxIpcIntrMask, mask);
}

/*******************Ip checksum offloading APIs***************************************/

/**
  * Enables the ip checksum offloading in receive path.
  * When set GMAC calculates 16 bit 1's complement of all received ethernet frame payload.
  * It also checks IPv4 Header checksum is correct. GMAC core appends the 16 bit checksum calculated
  * for payload of IP datagram and appends it to Ethernet frame transferred to the application.
  * @param[in] pointer to synop_gmac_device.
  * \return returns void.
  */
void
synop_gmac_enable_rx_chksum_offload(synop_gmac_device *gmacdev)
{
	synop_gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRxIpcOffload);
}

/**
  * Decodes the Rx Descriptor status to various checksum error conditions.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] u32 status field of the corresponding descriptor.
  * \return returns decoded enum (u32) indicating the status.
  */
static u32
synop_gmac_is_rx_checksum_error(synop_gmac_device *gmacdev, u32 status)
{
	if     (((status & DescRxChkBit5) == 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) == 0))
		return RxLenLT600;
	else if(((status & DescRxChkBit5) == 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) != 0))
		return RxIpHdrPayLoadChkBypass;
	else if(((status & DescRxChkBit5) == 0) && ((status & DescRxChkBit7) != 0) && ((status & DescRxChkBit0) != 0))
		return RxChkBypass;
	else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) == 0))
		return RxNoChkError;
	else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) != 0))
		return RxPayLoadChkError;
	else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) != 0) && ((status & DescRxChkBit0) == 0))
		return RxIpHdrChkError;
	else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) != 0) && ((status & DescRxChkBit0) != 0))
		return RxIpHdrPayLoadChkError;
	else
		return RxIpHdrPayLoadRes;
}

/**
  * Checks if any Ipv4 header checksum error in the frame just transmitted.
  * This serves as indication that error occureed in the IPv4 header checksum insertion.
  * The sent out frame doesnot carry any ipv4 header checksum inserted by the hardware.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] u32 status field of the corresponding descriptor.
  * \return returns true if error in ipv4 header checksum, else returns false.
  */
static unsigned int
synop_gmac_is_tx_ipv4header_checksum_error(synop_gmac_device *gmacdev, u32 status)
{
	return (((status & DescTxIpv4ChkError) == DescTxIpv4ChkError) ? 1 : 0);
}

/**
  * Checks if any payload checksum error in the frame just transmitted.
  * This serves as indication that error occureed in the payload checksum insertion.
  * The sent out frame doesnot carry any payload checksum inserted by the hardware.
  * @param[in] pointer to synop_gmac_device.
  * @param[in] u32 status field of the corresponding descriptor.
  * \return returns true if error in ipv4 header checksum, else returns false.
  */
static unsigned int
synop_gmac_is_tx_payload_checksum_error(synop_gmac_device *gmacdev, u32 status)
{
	return (((status & DescTxPayChkError) == DescTxPayChkError) ? 1 : 0);
}

/*******************Ip checksum offloading APIs***************************************/

static s32
synop_gmac_setup_tx_desc_queue(synop_gmac_device *gmacdev, u32 no_of_desc)
{
	s32 i;
	//u8* test = NULL;
	dma_desc *first_desc = NULL;
	gmacdev->TxDescCount = 0;

	TR2("Total size of memory required for Tx Descriptors in Ring Mode = 0x%08x\n",((sizeof(dma_desc) * no_of_desc)));

	first_desc = (dma_desc *)((void*)tx_array_desc);
	if (first_desc == NULL){
		TR0("Error in Tx Descriptors memory allocation\n");
		return -ESYNOPGMACNOMEM;
	}

	gmacdev->TxDescCount = no_of_desc;
	gmacdev->TxDesc      = first_desc;
	//gmacdev->TxDescDma   = dma_addr;

	TR2("gmacdev->TxDesc = 0x%x \n",(unsigned int)gmacdev->TxDesc );

	for (i = 0; i < gmacdev->TxDescCount; i++) {
		synop_gmac_tx_desc_init_ring(gmacdev->TxDesc + i, (i == gmacdev->TxDescCount-1) ? 1 : 0);
		TR2("%02d %08x \n",i, (unsigned int)(gmacdev->TxDesc + i));
	}

	gmacdev->TxNext = 0;
	gmacdev->TxBusy = 0;
	gmacdev->TxNextDesc = gmacdev->TxDesc;
	gmacdev->TxBusyDesc = gmacdev->TxDesc;
	gmacdev->BusyTxDesc = 0;

	return -ESYNOPGMACNOERR;
}

static s32
synop_gmac_setup_rx_desc_queue(synop_gmac_device *gmacdev ,u32 no_of_desc)
{
	s32 i;

	dma_desc *first_desc = NULL;
	gmacdev->RxDescCount = 0;

	TR2("total size of memory required for Rx Descriptors in Ring Mode = 0x%08x\n",((sizeof(dma_desc) * no_of_desc)));

	first_desc = (dma_desc *)((void*)rx_array_desc);

	if(first_desc == NULL){
		TR0("Error in Rx Descriptor Memory allocation in Ring mode\n");
		return -ESYNOPGMACNOMEM;
	}
	gmacdev->RxDescCount = no_of_desc;
	gmacdev->RxDesc      = first_desc;
	//gmacdev->RxDescDma   = dma_addr;

	TR2("gmacdev->RxDesc = 0x%x \n",(unsigned int)gmacdev->RxDesc );

	for (i = 0; i < gmacdev->RxDescCount; i++) {
		synop_gmac_rx_desc_init_ring(gmacdev->RxDesc + i, (i == gmacdev->RxDescCount-1) ? 1 : 0);
		TR1("%02d %08x \n",i, (unsigned int)(gmacdev->RxDesc + i));
	}

	gmacdev->RxNext = 0;
	gmacdev->RxBusy = 0;
	gmacdev->RxNextDesc = gmacdev->RxDesc;
	gmacdev->RxBusyDesc = gmacdev->RxDesc;

	gmacdev->BusyRxDesc   = 0;

	return -ESYNOPGMACNOERR;
}

static void
synop_handle_transmit_over(synop_gmac_device *gmacdev)
{
	s32 desc_index;
	u32 data1, data2;
	u32 status;
	u32 length1, length2;
	u32 dma_addr1, dma_addr2;

	do {
		desc_index = synop_gmac_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);

		if (desc_index >= 0 && data1 != 0) {
			TR2("Finished Transmit at Tx Descriptor %d for skb 0x%08x and buffer = %08x whose status is %08x \n", desc_index,data1,dma_addr1,status);

			if (synop_gmac_is_tx_ipv4header_checksum_error(gmacdev, status))
				TR0("Harware Failed to Insert IPV4 Header Checksum\n");
			if (synop_gmac_is_tx_payload_checksum_error(gmacdev, status))
				TR0("Harware Failed to Insert Payload Checksum\n");

			if (!synop_gmac_is_desc_valid(status))
				TR0("Error in Status %08x\n",status);
		}
	} while (desc_index >= 0);
}

void
synop_handle_received_data(synop_gmac_device *gmacdev)
{
	s32 desc_index;

	u32 data1;
	u32 data2;
	u32 len;
	u32 status;
	u32 dma_addr1;
	u32 dma_addr2;
	void* skb_on_sdram = NULL;
	void *skb; //This is the pointer to hold the received data

	//Handle the Receive Descriptors
	do {
		desc_index = synop_gmac_get_rx_qptr(gmacdev, &status,&dma_addr1,NULL, &data1,&dma_addr2,NULL,&data2);

		if(desc_index >= 0 && data1 != 0){
			TR2("Received Data at Rx Descriptor %d for skb 0x%08x whose status is %08x\n",desc_index,data1,status);
			//At first step unmapped the dma address
			//dma_unmap_single(apbdev == NULL ? NULL : &apbdev->dev, dma_addr1, 0, 2 );

			skb = (void *)data1;
			if(synop_gmac_is_rx_desc_valid(status)){

				len =  synop_gmac_get_rx_desc_frame_length(status) - 4; //Not interested in Ethernet CRC bytes

				/*
				TR2("recive packet on sdram = 0x%x length = %d \n", (unsigned int)skb, (unsigned int)len);
				{
					unsigned int i = 0;
					unsigned char* data = (unsigned char *)data1;
					for (i = 0 ; i < len + 2 ; i++) {
						TR2("PACKET[%d] = 0x%x\n",i , data[i]);
					}
				}
				*/

				//memcpy((void *)NetRxPackets[0], skb, len );

				NetReceive(skb, len);

				// Now lets check for the IPC offloading
				//  Since we have enabled the checksum offloading in hardware, lets inform the kernel
				//    not to perform the checksum computation on the incoming packet. Note that ip header 
				//    checksum will be computed by the kernel immaterial of what we inform. Similary TCP/UDP/ICMP
				//    pseudo header checksum will be computed by the stack. What we can inform is not to perform
				//    payload checksum.
				//    When CHECKSUM_UNNECESSARY is set kernel bypasses the checksum computation.		    
				//

				TR2("Checksum Offloading will be done now\n");

				if (synop_gmac_is_rx_checksum_error(gmacdev, status) == RxNoChkError)
					TR2("Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 4>\n");

				if (synop_gmac_is_rx_checksum_error(gmacdev, status) == RxIpHdrChkError)
					//Linux Kernel doesnot care for ipv4 header checksum. So we will simply proceed by printing a warning ....
					TR0(" Error in 16bit IPV4 Header Checksum <Chk Status = 6>\n");

				if (synop_gmac_is_rx_checksum_error(gmacdev, status) == RxLenLT600)
					TR2("IEEE 802.3 type frame with Length field Lesss than 0x0600 <Chk Status = 0>\n");

				if (synop_gmac_is_rx_checksum_error(gmacdev, status) == RxIpHdrPayLoadChkBypass)
					TR2("Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 1>\n");

				if (synop_gmac_is_rx_checksum_error(gmacdev, status) == RxChkBypass)
					TR2("Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 3>\n");

				if (synop_gmac_is_rx_checksum_error(gmacdev, status) == RxPayLoadChkError)
					TR0(" TCP/UDP payload checksum Error <Chk Status = 5>\n");

				if (synop_gmac_is_rx_checksum_error(gmacdev, status) == RxIpHdrChkError)
					//Linux Kernel doesnot care for ipv4 header checksum. So we will simply proceed by printing a warning ....
					TR0(" Both IP header and Payload Checksum Error <Chk Status = 7>\n");
			}
			else {
				printf("Rx descriptor error. Packet dropped. Status = 0x%08x\n", status);
			}


#ifdef DATA_ON_DDR
			skb = (void *)(&rx_buffers[desc_index][0]);
#else
			skb = (void *)(rx_buffers + desc_index*(RX_BUF_SIZE));
#endif


			skb_on_sdram = virt_to_bus(skb);

			TR2("skb_on_sdram = 0x%x skb = 0x%x desc_index = %d \n", (u32)skb_on_sdram , (u32)skb , desc_index); 

			desc_index = synop_gmac_set_rx_qptr(gmacdev, (u32)skb_on_sdram, RX_BUF_SIZE, (u32)skb, 0, 0, 0);

			if (desc_index < 0)
				TR0("Cannot set Rx Descriptor for skb %08x\n",(u32)skb);
		}
	} while (desc_index >= 0);
}

int synop_gmac_intr_rx_handler(synop_gmac_device *gmacdev)
{
	u32 interrupt,dma_status_reg;
	s32 status;
	void* skb = NULL;
	void* skb_on_sdram = NULL;

	/*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
	dma_status_reg = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaStatus);

	if(dma_status_reg == 0)
		return 0;

//	synop_gmac_disable_interrupt_all(gmacdev);

	TR2("Dma Rx Status Reg: 0x%08x\n",dma_status_reg);

	//if(dma_status_reg & GmacPmtIntr){
	//	TR2(": Interrupt due to PMT module\n");
	//	synop_gmac_linux_powerup_mac(gmacdev);
	//}

	if(dma_status_reg & GmacMmcIntr){
		TR2(": Interrupt due to MMC module\n");
		TR2(": synop_gmac_rx_int_status = %08x\n",synop_gmac_read_mmc_rx_int_status(gmacdev));
		TR2(": synop_gmac_tx_int_status = %08x\n",synop_gmac_read_mmc_tx_int_status(gmacdev));
	}

	if(dma_status_reg & GmacLineIntfIntr){
		TR2(": Interrupt due to GMAC LINE module\n");
	}

	/*Now lets handle the DMA interrupts
	 this is also clear the interrupts ! ! !*/
	interrupt = synop_gmac_get_interrupt_type(gmacdev, DmaIntRxOnly);

	/*
	 * ATTENTION: The following call will guarantee that all memory
	 * transactions indicated by the device irq flags have actually hit the
	 * memory. Just process the flags which have been sampled. When reading
	 * the status again any new IRQ flags MUST NOT be processed until
	 * ccu_barrier() is called again!
	 */
	ccu_barrier();

	TR2(":Interrupts to be handled: 0x%08x\n",interrupt);

	if (interrupt & synopGMACDmaError) {
		u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;//after soft reset, configure the MAC address to default value
		memcpy(mac_addr0, gmacdev->netdev.enetaddr, 6);
		TR0(":Fatal Bus Error Inetrrupt Seen\n");
		synop_gmac_disable_dma_tx(gmacdev);
		synop_gmac_disable_dma_rx(gmacdev);

		synop_gmac_take_desc_ownership_tx(gmacdev);
		synop_gmac_take_desc_ownership_rx(gmacdev);

		synop_gmac_init_tx_rx_desc_queue(gmacdev);

		synop_gmac_reset(gmacdev);//reset the DMA engine and the GMAC ip

		synop_gmac_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, mac_addr0);
		synop_gmac_dma_bus_mode_init(gmacdev,DmaFixedBurstEnable| DmaBurstLength8 | DmaDescriptorSkip2 );
		synop_gmac_dma_control_init(gmacdev,DmaStoreAndForward);
		synop_gmac_init_rx_desc_base(gmacdev);
		synop_gmac_init_tx_desc_base(gmacdev);
		synop_gmac_mac_init(gmacdev);
		synop_gmac_enable_dma_rx(gmacdev);
		synop_gmac_enable_dma_tx(gmacdev);
		TR0("\n");
	}

	if(interrupt & synopGMACDmaRxNormal){
		TR2(": Rx Normal \n");
		synop_handle_received_data(gmacdev);
	}

	if(interrupt & synopGMACDmaRxAbnormal){
		TR1(":Abnormal Rx Interrupt Seen\n");
		synop_gmac_resume_dma_rx(gmacdev);//To handle GBPS with 12 descriptors
	}

	if(interrupt & synopGMACDmaRxStopped){
		TR1(":Receiver stopped seeing Rx interrupts\n"); //Receiver gone in to stopped state
		do {

#ifdef DATA_ON_DDR
			skb = (void *)(&rx_buffers[gmacdev->RxNext][0]);
#else
			skb = (void *)(rx_buffers + gmacdev->RxNext*(RX_BUF_SIZE));
#endif


			skb_on_sdram = virt_to_bus(skb);

			TR2("skb_on_sdram = 0x%x skb = 0x%x \n", (u32)skb_on_sdram , (u32)skb);

			status = synop_gmac_set_rx_qptr(gmacdev,(u32)skb_on_sdram, RX_BUF_SIZE, (u32)skb,0,0,0);
		} while (status >= 0);

		synop_gmac_enable_dma_rx(gmacdev);
	}

	/* Enable the interrrupt before returning from ISR*/
//	synop_gmac_enable_interrupt(gmacdev,(u32)DmaIntEnable);

	return 0;
}

int synop_gmac_intr_tx_handler(synop_gmac_device *gmacdev)
{
	u32 interrupt,dma_status_reg;
	s32 status = -1;
	/*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
	dma_status_reg = synop_gmac_read_reg((u32 *)gmacdev->DmaBase, DmaStatus);

	if (dma_status_reg == 0)
		return 0;

	synop_gmac_disable_interrupt_all(gmacdev);

	TR2("Dma Tx Status Reg: 0x%08x\n",dma_status_reg);

	if (dma_status_reg & GmacMmcIntr) {
		TR2(": Interrupt due to MMC module\n");
		TR2(": synop_gmac_rx_int_status = %08x\n",synop_gmac_read_mmc_rx_int_status(gmacdev));
		TR2(": synop_gmac_tx_int_status = %08x\n",synop_gmac_read_mmc_tx_int_status(gmacdev));
	}

	if (dma_status_reg & GmacLineIntfIntr) {
		TR2(": Interrupt due to GMAC LINE module\n");
	}

	/*Now lets handle the DMA interrupts
	 this is also clear the interrupts ! ! !*/
	interrupt = synop_gmac_get_interrupt_type(gmacdev, DmaIntTxOnly);

	/*
	 * ATTENTION: The following call will guarantee that all memory
	 * transactions indicated by the device irq flags have actually hit the
	 * memory. Just process the flags which have been sampled. When reading
	 * the status again any new IRQ flags MUST NOT be processed until
	 * ccu_barrier() is called again!
	 */
	ccu_barrier();

	TR2(":Interrupts to be handled: 0x%08x\n",interrupt);

	if (interrupt & synopGMACDmaError) {
		u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;//after soft reset, configure the MAC address to default value
		memcpy(mac_addr0, gmacdev->netdev.enetaddr, 6);
		TR0(":Fatal Bus Error Inetrrupt Seen\n");
		synop_gmac_disable_dma_tx(gmacdev);
		synop_gmac_disable_dma_rx(gmacdev);

		synop_gmac_take_desc_ownership_tx(gmacdev);
		synop_gmac_take_desc_ownership_rx(gmacdev);

		synop_gmac_init_tx_rx_desc_queue(gmacdev);

		synop_gmac_reset(gmacdev);//reset the DMA engine and the GMAC ip

		synop_gmac_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, mac_addr0);
		synop_gmac_dma_bus_mode_init(gmacdev,DmaFixedBurstEnable| DmaBurstLength8 | DmaDescriptorSkip2 );
		synop_gmac_dma_control_init(gmacdev,DmaStoreAndForward);
		synop_gmac_init_rx_desc_base(gmacdev);
		synop_gmac_init_tx_desc_base(gmacdev);
		synop_gmac_mac_init(gmacdev);
		synop_gmac_enable_dma_rx(gmacdev);
		synop_gmac_enable_dma_tx(gmacdev);
		status = -1;
		TR0("\n");
	}

	if (interrupt & synopGMACDmaTxNormal) {
		//xmit function has done its job
		TR2(":Finished Normal Transmission \n");
		synop_handle_transmit_over(gmacdev);//Do whatever you want after the transmission is over
		status = 0;
	}

	if (interrupt & synopGMACDmaTxAbnormal) {
		TR0(":Abnormal Tx Interrupt Seen\n");
		synop_handle_transmit_over(gmacdev);
		status = -1;
	}

	if (interrupt & synopGMACDmaTxStopped) {
		TR2(":Transmitter stopped sending the packets\n");
		synop_gmac_disable_dma_tx(gmacdev);
		synop_gmac_take_desc_ownership_tx(gmacdev);
		synop_gmac_enable_dma_tx(gmacdev);
		TR2(":Transmission Resumed\n");
		status = -1;
	}

	/* Enable the interrrupt before returning from ISR*/
	synop_gmac_enable_interrupt(gmacdev,(u32)DmaIntEnable);
	return status;
}


int synop_gmac_setup_gmac(synop_gmac_device *gmacdev)
{
	s32 status;
	u16 data;
	u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;
	unsigned int retry = 10;
	memcpy(mac_addr0, gmacdev->netdev.enetaddr, 6);
	// after soft reset, configure the MAC address to default value

	/* Lets read the version of ip in to device structure */
	synop_gmac_read_version(gmacdev);

	/* Program/flash in the station/IP's Mac address */
	synop_gmac_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, mac_addr0);

	/* Check for Phy initialization */
	synop_gmac_set_mdc_clk_div(gmacdev,GmiiCsrClk2);
	gmacdev->ClockDivMdc = synop_gmac_get_mdc_clk_div(gmacdev);

	if (synop_gmac_phy_select_port(gmacdev) != 0) {
		printf("No availble PHY was found, please check network connection\n");
		return -1;
	}

	/*Soft reset*/
	synop_gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_Reg0, 1 << 15);
	udelay(100);

	while (synop_gmac_phy_check_link_status(gmacdev) == 0 && retry){
		status = synop_gmac_check_phy_init(gmacdev);
		synop_gmac_mac_init(gmacdev);
		udelay(1000000);
		retry-- ;
	}
	
	if (!retry) {
		printf("No link please check network connection\n");
		return -1;
	}

	/* Set up the tx and rx descriptor queue/ring */
	synop_gmac_setup_tx_desc_queue(gmacdev, TRANSMIT_DESC_SIZE);
	synop_gmac_init_tx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr
	synop_gmac_setup_rx_desc_queue(gmacdev, RECEIVE_DESC_SIZE);
	synop_gmac_init_rx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr

	synop_gmac_dma_bus_mode_init(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2);	//pbl32 incr with rxthreshold 128
	synop_gmac_dma_control_init(gmacdev, DmaStoreAndForward | DmaTxSecondFrame|DmaRxThreshCtrl128);

	synop_gmac_mac_init(gmacdev);
	synop_gmac_pause_control(gmacdev); // This enables the pause control in Full duplex mode of operation

	synop_gmac_init_rx_qptr(gmacdev); //init the buffers of each descripor

	synop_gmac_clear_interrupt(gmacdev);
	/*
	Disable the interrupts generated by MMC and IPC counters.
	If these are not disabled ISR should be modified accordingly to handle these interrupts.
	*/
	synop_gmac_disable_mmc_tx_interrupt(gmacdev, 0xFFFFFFFF);
	synop_gmac_disable_mmc_rx_interrupt(gmacdev, 0xFFFFFFFF);
	synop_gmac_disable_mmc_ipc_rx_interrupt(gmacdev, 0xFFFFFFFF);

	synop_gmac_enable_interrupt(gmacdev, DmaIntEnable);
	synop_gmac_enable_dma_rx(gmacdev);
	synop_gmac_enable_dma_tx(gmacdev);

	return 0;
}
