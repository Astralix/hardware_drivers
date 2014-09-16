/*------------------------------------------------------------------------------
 * $Id: macb_AL.c,v 1.6 2007/01/11 16:18:30 odedg Exp $
 *  =Id: macb_AL.c,v 1.4 2003/04/28 08:00:44 stevenh Exp $
 *------------------------------------------------------------------------------
 *
 *                                        Copyright (c) 2003
 *            CADENCE                    CADENCE DESIGN FOUNDRY
 *                                       All rights reserved.
 *
 *------------------------------------------------------------------------------
 *
 *   Filename       :   macb_AL.c
 *
 *   Author         :   Steven Ho, DICS, LDC
 *
 *   Date           :   9th Oct 2001
 *
 *   Limitations    :   Supports only the Cadence Enhanced MAC, the Cadence
 *                      Gigabit MAC will be supported in the next version.
 *                      The standard Cadence Ethernet MAC will NOT be supported
 *                      due to the different underlying architecture of the
 *                      transmit interface.
 *
 *------------------------------------------------------------------------------
 *   Description    :   A Hardware Abstraction Layer for the Cadence Enhanced MAC
 *                      comprising of a set of functions and macros to provide a
 *                      simple abstracted view to operating the MAC enabling the
 *                      MAC software development in the minimal amount of time.
 *
 *------------------------------------------------------------------------------
 *   Revision Control
 *
 *------------------------------------------------------------------------------
 */

#include <common.h>
#include <config.h>
#include <malloc.h>
#include <net.h>

#include "dw.h"
#include "macb_AL.h"

#ifdef CONFIG_DRIVER_MACB
#define PHY0_ID_EXPEDIBLUE	0xA
#define PHY1_ID_EXPEDIBLUE	0xB
#define PHY0_ID_EXPEDITOR	0x7
#define PHY1_ID_EXPEDITOR	0x7
//#define ETH1_PRIMARY_IF //Use ETH1 as primary interface (for boot)

extern int dw_board;

/******************************************************************************/
/*
 * Attach a physical MAC instance to a MAC_DEVICE structure.
 * This must be called to attach the memory map of the physical MAC device to
 * the MAC_REG pointer in the MAC_DEVICE structure.
 * This function must be called prior to anything else!
 * Note that this function takes pointers as arguments rather than returning a
 * pointer to a structure since we are avoiding use of any memory allocation
 * routines in the HAL.
 *
 * Return value:
 *  0   :   OK
 *  -1  :   Invalid values for pointer arguments or wrong device.
 */
/******************************************************************************/
int mac_attach_device
(
    MAC_DEVICE      *mac,       /* IN/OUT.  Pointer to new structure to connect to. */
    void            *device     /* IN.  Pointer to device base register address. */
)
{
    if ( (mac == NULL) || (device == NULL) )
    {
        return (-1);
    }
    else
    {
        mac->registers = (MAC_REG *) device;
        if ((mac_register_rd(mac,MAC_REV_ID) & MACB_REV_ID_MODEL_MASK) != MACB_REV_ID_REG_MODEL)
        {
            return (-1);
        }
        else
        {
            return 0;
        }
    }
}
/******************************************************************************/


/* Some functions to set/reset and get specific bits in the MAC registers
 * Note that all functions operate on a read-modify-write basis
 */




/******************************************************************************/
/*
 * Halt transmission after current frame has completed.  This is accomplished
 * simply by writing to the TX_HALT bit in the network control register, which
 * should cause the MAC to complete its current frame then stop.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_stop_tx(MAC_DEVICE *mac)
{
    mac->registers->net_control |= TX_HALT;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Abort transmission immediately WITHOUT waiting for completion of any current
 * frame.
 * Note that after this operation, the transmit buffer descriptor will be reset
 * to point to the first buffer in the descriptor list!
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_abort_tx(MAC_DEVICE *mac)
{
    mac->registers->net_control &= (~TX_EN);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Obtains status of transmission circuitry, whether it is transmitting or idle.
 *
 * Return value:
 *  0   :   Transmitter is idle.
 *  1   :   Transmitter active.
 */
/******************************************************************************/
int mac_transmitting(MAC_DEVICE *mac)
{
    return ((mac->registers->tx_status & TX_GO) == TX_GO);
}
/******************************************************************************/




/******************************************************************************/
/*
 * Disable the receive circuitry.  This will stop reception of any frame
 * immediately, note that the descriptor pointer is not changed.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_rx(MAC_DEVICE *mac)
{
    mac->registers->net_control &= (~RX_EN);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Obtain the status of the receive circuitry, whether it is enabled.
 *
 * Return value:
 *  0   :   Receive circuitry disabled.
 *  -1  :   Receive circuits enabled.
 */
/******************************************************************************/
int mac_receive_on(MAC_DEVICE *mac)
{
    return ((mac->registers->net_control & RX_EN) == RX_EN);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set the loopback mode of the MAC.  This can be either no loopback for normal
 * operation, local loopback through MAC internal loopback module or PHY
 * loopback for external loopback through a PHY.  This asserts the external loop
 * pin.
 * The function parameters are a pointer to the device and an enumerated type
 * specifying the type of loopback required.
 *
 * Note: if an invalid loopback mode is specified, loop back will be disabled.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_set_loop
(
    MAC_DEVICE  *mac,       /* Pointer to the device structure. */
    MAC_LOOP    mac_loop    /* Loopback mode. */
)
{
    switch (mac_loop) {
        case LB_LOCAL:
            mac->registers->net_control &= (~LB_PHY);
            mac->registers->net_control |= (LB_MAC);
            break;
        case LB_EXT:
            mac->registers->net_control &= (~LB_MAC);
            mac->registers->net_control |= (LB_PHY);
            break;
        default:
            mac->registers->net_control &= (~(LB_MAC | LB_PHY));
    }
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the loopback mode of the MAC.  This can be either no loopback for normal
 * operation, local loopback through MAC internal loopback module or PHY
 * loopback for external loopback through a PHY.  This asserts the external loop
 * pin.
 * The function parameters are a pointer to the device and an enumerated type
 * specifying the type of loopback required.
 *
 * Return value:
 *  LB_LOCAL    :   Local loop back active.
 *  LB_PHY      :   External loop back active.
 *  LB_NONE     :   Loop back disabled.
 *  -1          :   Unknown mode.
 */
/******************************************************************************/
int mac_get_loop (MAC_DEVICE  *mac)
{
    UINT_32 lb_mode;

    lb_mode = mac->registers->net_control & (LB_PHY | LB_MAC);

    switch (lb_mode) {
        case LB_MAC:
            return LB_LOCAL;
            break;
        case LB_PHY:
            return LB_EXT;
            break;
        case 0:
            return LB_NONE;
            break;
        default:
            return -1;
    }
}
/******************************************************************************/


/******************************************************************************/
/*
 * Activate the Management interface.  This is required to program the PHY
 * registers through the MDIO port.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_enable_MDIO(MAC_DEVICE *mac)
{
    mac->registers->net_control |= MDIO_EN;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable the Management interface.  In this state, the MDIO is placed in a
 * high impedance state and the MDC clock is driven low.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_MDIO(MAC_DEVICE *mac)
{
    mac->registers->net_control &= (~MDIO_EN);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable writing to the statistic registers.  This is for debug purposes only
 * and should not be active during normal operation.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_stats_wr_en(MAC_DEVICE *mac)
{
    mac->registers->net_control |= STATS_WR_EN;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable writing to the statistic registers.  Under normal operation this is
 * not necessary as the writing to statistics registers should be off by default
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_stats_wr_off(MAC_DEVICE *mac)
{
    mac->registers->net_control &= (~STATS_WR_EN);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Increment all the statistic registers by 1.  This is for debug purposes only.
 * Note that the statistic registers are automatically cleared on read!
 *
 * No return value.
 */
/******************************************************************************/
void mac_stats_inc(MAC_DEVICE *mac)
{
    mac->registers->net_control |= STATS_INC;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Clear all the statistic registers.
 *
 * No return value.
 */
/******************************************************************************/
void mac_stats_clr(MAC_DEVICE *mac)
{
    mac->registers->net_control |= STATS_CLR;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable back pressure mode on the MAC.  In this mode, the MAC will force a
 * collision on every incoming frame.  This can be used to implement some kind
 * of flow control.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_enable_bp(MAC_DEVICE *mac)
{
    mac->registers->net_control |= NET_BP;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable back pressure mode on the MAC.   This is the normal operating mode of
 * the MAC.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_bp(MAC_DEVICE *mac)
{
    mac->registers->net_control &= (~NET_BP);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set the receive buffer offset.  This can be a value between 0 and 3 and
 * specifies the amount by which the received data should be offset from the
 * start of the receive buffer.
 * Note that this should not be set while the MAC is receiving, it is
 * recommended that the receive circuitry be de-activated prior to performing
 * this operation.
 *
 * Return value:
 *  0   :   OK
 *  -1  :   Invalid arguments.
 */
/******************************************************************************/
int mac_set_rx_oset
(
    MAC_DEVICE      *mac,
    unsigned char   oset
)
{
    if ( (oset > 3))
    {
        return (-1);
    }
    else
    {
        mac->registers->net_config &= (~(RX_OFFSET1 | RX_OFFSET0));
        mac->registers->net_config |= (oset << 14);
        return 0;
    }
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the current receive buffer offset.
 * This returns a value between 0 and 3 to indicate the amount by which received
 * data is offset from the start of the receive buffer.
 * The offset should not be changed during normal operation, it should be setup
 * properly prior to any reception.
 *
 * Return value:
 *  0-3 : Offset to the receive buffer
 */
/******************************************************************************/
int mac_get_rx_oset(MAC_DEVICE *mac)
{
    return ((mac->registers->net_config &
            (RX_OFFSET1 | RX_OFFSET0)) >> RX_OFFSET_BASE);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable pause frame reception.  With this enabled, if a valid pause frame is
 * received, transmission will halt for the specified time after the current
 * frame has completed transmission.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_enable_pause_rx(MAC_DEVICE *mac)
{
    mac->registers->net_config |= RX_PAUSE_EN;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable pause frame reception.  Incoming pause frames are ignored.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_pause_rx(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~RX_PAUSE_EN);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set retry test bit, this is used for debug purposes only to speed up testing.
 * This should not be enabled for normal operation.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_en_retry_test(MAC_DEVICE *mac)
{
    mac->registers->net_config |= RETRY_TEST;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable retry test bit.  For normal operation this bit should not be set.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_dis_retry_test(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~RETRY_TEST);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable external address match via the eam pin, which when active will copy
 * the current frame to memory.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_enable_eam(MAC_DEVICE *mac)
{
    mac->registers->net_config |= EAM_EN;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable external address match capability.  The MAC will ignore the status of
 * the eam pin.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_eam(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~EAM_EN);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable storing of the receive frame check sequence into memory.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_enable_fcs_rx(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~RX_NO_FCS);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable storing of the receive frame check sequence into memory.  The last 4
 * bytes from the incoming frame will be checked for valid CRC, however this
 * will not be stored into memory.  The frame length will be updated
 * accordingly.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_fcs_rx(MAC_DEVICE *mac)
{
    mac->registers->net_config |= RX_NO_FCS;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable reception of long frames up to 1536 bytes in length.
 * These are not standard IEEE 802.3 frames.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_enable_1536_rx(MAC_DEVICE *mac)
{
    mac->registers->net_config |= FRAME_1536;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable reception of frames greater than 1518 bytes in length.
 * This is normal operation mode for the MAC for compatibility with IEEE 802.3
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_1536_rx(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~FRAME_1536);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable reception of unicast hashed frames.  The frame will be received when
 * the 6 bit hash function of the frame's destination address points a bit that
 * is set in the 64-bit hash register and is signalled as a unicast frame.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_enable_unicast(MAC_DEVICE *mac)
{
    mac->registers->net_config |= UNICAST_EN;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable reception of unicast hashed frames.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_unicast(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~UNICAST_EN);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable reception of multicast hashed frames.  The frame will be received when
 * the 6 bit hash function of the frame's destination address points a bit that
 * is set in the 64-bit hash register and is signalled as a multicast frame.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_enable_multicast(MAC_DEVICE *mac)
{
    mac->registers->net_config |= MULTICAST_EN;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable reception of multicast hashed frames.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_multicast(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~MULTICAST_EN);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Allow reception of broadcast frames (frames with address set to all 1's)
 * This is normal operating mode for the MAC.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_allow_broadcast(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~NO_BROADCAST);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Do not allow reception of broadcast frames, such frames will be ignored.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_no_broadcast(MAC_DEVICE *mac)
{
    mac->registers->net_config |= NO_BROADCAST;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable copy all frames.  In this mode, the MAC will copy all valid received
 * frames to memory regardless of the destination address.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_enable_copy_all(MAC_DEVICE *mac)
{
    mac->registers->net_config |= COPY_ALL;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Do not copy all frames.  Normal operating mode for the MAC, frames will only
 * be copied to memory if it matches one of the specific or hash addresses.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_disable_copy_all(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~COPY_ALL);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set MAC into full duplex mode.  The crs and col signals will be ignored in
 * this mode.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_full_duplex(MAC_DEVICE *mac)
{
    mac->registers->net_config |= FULL_DUPLEX;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set MAC into half duplex mode.  The crs and col signals are used to detect
 * collisions and perform deference where necessary.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_half_duplex(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~FULL_DUPLEX);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set the operating speed of the MAC, currently this has no functional effect
 * on the MAC, but simply asserts an external speed pin accordingly.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_set_speed(MAC_DEVICE *mac, MAC_SPEED mac_speed)
{
    switch (mac_speed)
    {
        case SPEED_10M:
            mac->registers->net_config &= (~SPEED_100);
            break;
        case SPEED_100M:
            mac->registers->net_config |= SPEED_100;
            break;
        default:
            mac->registers->net_config |= SPEED_100;
    }
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the operating speed of the MAC, currently this has no functional effect
 * on the MAC.
 *
 * Return value:
 *  0   :   MAC in 10Mb/s mode.
 *  1   :   MAC in 100Mb/s mode.
 */
/******************************************************************************/
int mac_get_speed(MAC_DEVICE *mac)
{
    return ( (mac->registers->net_config & SPEED_100) == SPEED_100 );
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the current link status as returned by the PHY
 *
 * Return value:
 *  0   :   Link is down.
 *  1   :   Link active.
 */
/******************************************************************************/
int mac_link_status(MAC_DEVICE *mac)
{
    return ((mac->registers->net_status & LINK_STATUS) == LINK_STATUS);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Check if the PHY management logic is idle, i.e. completed management
 * operation.
 *
 * Return value:
 *  0   :   PHY management not-idle.
 *  1   :   PHY management completed.
 */
/******************************************************************************/
int mac_phy_man_idle(MAC_DEVICE *mac)
{
    return ((mac->registers->net_status & PHY_IDLE) == PHY_IDLE);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the value of the transmit status register.
 * The return value is an unsigned 32-bit integer containing the contents of the
 * register.  This should be masked appropriately to obtain the relevant status.
 *
 * Return value:
 * Returns current value of transmit status register.
 */
/******************************************************************************/
UINT_32 mac_get_tx_stat(MAC_DEVICE *mac)
{
    return (mac->registers->tx_status);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Reset the specified bits of the transmit status register.
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_reset_tx_stat
(
    MAC_DEVICE  *mac,       /* Pointer to device structure. */
    UINT_32     rst_status  /* Status to reset. */
)
{
    mac->registers->tx_status |= rst_status;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the value of the receive status register.
 * The return value is an unsigned 32-bit integer containing the contents of the
 * register.  This should be masked appropriately to obtain the relevant status.
 *
 * Returns current receive status.
 */
/******************************************************************************/
UINT_32 mac_get_rx_stat(MAC_DEVICE *mac)
{
    return (mac->registers->rx_status);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Reset the specified bits of the receive status register.
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_reset_rx_stat
(
    MAC_DEVICE  *mac,       /* Pointer to device structure. */
    UINT_32     rst_status  /* Status to reset. */
)
{
    mac->registers->rx_status |= rst_status;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set the receive queue descriptor pointer.
 * The input to this function is a pointer to the memory location, this is
 * cast into the appropriate type before being stored.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_set_rx_qptr
(
    MAC_DEVICE  *mac,
    void        *qptr       /* Pointer to receive descriptor queue. */
)
{
    // odedg - mac->registers->rx_qptr = (UINT_32) qptr;
	mac->registers->rx_qptr = (UINT_32)(qptr);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the receive queue descriptor pointer.
 *
 * Return value is a pointer to where the MAC is pointing to for the receive
 * buffer descriptors.
 */
/******************************************************************************/
void * mac_get_rx_qptr(MAC_DEVICE *mac)
{
    // odedg - return ( (void *) mac->registers->rx_qptr );
	return ( (void *)(mac->registers->rx_qptr));
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set the transmit queue descriptor pointer.
 * The input to this function is a pointer to the memory location, this is
 * cast into the appropriate type before being stored.
 *
 * There is no return value for this function.
 */
/******************************************************************************/
void mac_set_tx_qptr
(
    MAC_DEVICE  *mac,
    void        *qptr       /* Pointer to transmit descriptor queue. */
)
{
    // odedg - mac->registers->tx_qptr = (UINT_32) qptr;
	mac->registers->tx_qptr = (UINT_32)(qptr);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the transmit queue descriptor pointer.
 *
 * Return value is a pointer to where the MAC is pointing to for the transmit
 * buffer descriptors.
 */
/******************************************************************************/
void * mac_get_tx_qptr(MAC_DEVICE *mac)
{
    // odedg - return ( (void *) mac->registers->tx_qptr );
	return ( (void *)(mac->registers->tx_qptr));
}
/******************************************************************************/


/******************************************************************************/
/*
 * Read the interrupt status register.
 * This returns an unsigned 32-bit integer with the current interrupt status,
 * this should be masked appropriately to get the required status.
 * Note that the interrupt status register is automatically reset on read, so
 * the returned value should be stored if further processing required.
 *
 * Returns the current interrupt status.
 */
/******************************************************************************/
UINT_32 mac_get_irq_stat(MAC_DEVICE *mac)
{
    return (mac->registers->irq_status);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set specified bits in the interrupt status register.
 * This can be used for debug purposes to manually activate an interrupt.
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_set_irq_stat(MAC_DEVICE *mac, UINT_32 irq_status)
{
    mac->registers->irq_status |= irq_status;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Enable specified interrupts.
 * The specified interrupt bits are enabled by unmasking them.
 * Note that this appends to the existing interrupt enable list.
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_enable_irq(MAC_DEVICE *mac, UINT_32 irq_en)
{
    mac->registers->irq_enable |= irq_en;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable specified interrupts.
 * The specified interrupts are masked out so that they do not generate an
 * interrupt.
 * Note that this appends to the existing interrupt mask list.
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_mask_irq(MAC_DEVICE *mac, UINT_32 irq_mask)
{
    mac->registers->irq_disable |= irq_mask;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Obtain the current interrupt mask value.
 * The return value indicates which interrupts are currently masked out i.e. do
 * not generate an interrupt.
 *
 * Returns the interrupt mask status.
 */
/******************************************************************************/
UINT_32 mac_get_irq_mask(MAC_DEVICE *mac)
{
    return (mac->registers->irq_mask);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Write to the PHY management registers.
 * This function simply sets off the PHY management operation, completion will
 * be indicated by an interrupt.
 * The input parameters are the PHY address, register address, and the 16-bit
 * data to be written.
 * Note that the MDIO enable register must be on.
 *
 * Return value:
 *  0   :   OK
 *  -1  :   Invalid input range.
 */
/******************************************************************************/
int mac_phy_man_wr
(
    MAC_DEVICE  *mac,
    BYTE        phy_addr,
    BYTE        reg_addr,
    UINT_32     data
)
{
    UINT_32 write_data;

    if ( (phy_addr > 0x1F) || (reg_addr > 0x1F) || (data > 0xFFFF) )
    {
        return -1;
    }
    else
    {
        write_data = 0x50020000;
        write_data |= ( (phy_addr << 23) | (reg_addr << 18) | data );
        mac->registers->phy_man = write_data;
        return 0;
    }
}
/******************************************************************************/


/******************************************************************************/
/*
 * Perform PHY management register read operation.
 * This function simply sets off the PHY management operation, completion will
 * be indicated by an interrupt.
 * The input parameters are the PHY address and the register address to be read
 *
 * Return value:
 *  0   :   OK
 *  -1  :   Invalid input range.
 */
/******************************************************************************/
int mac_phy_man_rd
(
    MAC_DEVICE  *mac,
    BYTE        phy_addr,
    BYTE        reg_addr
)
{
    UINT_32 write_data;

    if ( (phy_addr > 0x1F) || (reg_addr > 0x1F) )
    {
        return -1;
    }
    else
    {
        write_data = 0x60020000;
        write_data |= ( (phy_addr << 23) | (reg_addr << 18) );
        mac->registers->phy_man = write_data;
        return 0;
    }
}
/******************************************************************************/


/******************************************************************************/
/*
 * Read the data section of the PHY management register.  After a read operation
 * the data from the PHY will be stored here.
 *
 * Return value is the lower 16-bits of the PHY management register.
 */
/******************************************************************************/
UINT_32 mac_phy_man_data(MAC_DEVICE *mac)
{
    return (mac->registers->phy_man & 0xFFFF);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Read the pause time register.
 *
 * Returns the current value in the pause time register which will
 * decrement when the MAC has gone into pause mode.
 */
/******************************************************************************/
UINT_32 mac_pause_time(MAC_DEVICE *mac)
{
    return (mac->registers->pause_time);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set the id-check register of the MAC.
 * This register is used to check the type-id field of the incoming frames, if
 * matched, the appropriate status bit will be set in word 1 of the receive
 * descriptor for that frame.
 * The input parameter is truncated to 16-bits.
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_set_id_check
(
    MAC_DEVICE  *mac,
    UINT_32     id_check
)
{
    mac->registers->id_check = (id_check & 0xFFFF);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the value of the id-check register in the MAC.
 *
 * Return value:
 *  Value of ID check register.
 */
/******************************************************************************/
UINT_32 mac_get_id_check(MAC_DEVICE *mac)
{
    return (mac->registers->id_check);
}
/******************************************************************************/


/******************************************************************************/
/*
 * Set the hash register of the MAC.
 * This register is used for matching unicast and multicast frames.
 * The parameter of this function should be a pointer to type MAC_ADDR as
 * defined in the header file.
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_set_hash
(
    MAC_DEVICE  *mac,
    MAC_ADDR    *hash_addr
)
{
    mac->registers->hash_addr = *hash_addr;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the current value of the hash registers of the MAC.
 *
 * This function returns a value of type MAC_ADDR
 */
/******************************************************************************/
/*MAC_ADDR mac_get_hash(MAC_DEVICE *mac) @@yoav: this function isn't used and its return value is wierd
{
    return (mac->registers->hash_addr);
}*/
/******************************************************************************/


/******************************************************************************/
/*
 * Setup all the specific address registers for the MAC.
 * These registers are matched against incoming frames to determine whether the
 * frame should be copied to memory.
 * The input parameter to this function should be a pointer to type SPEC_ADDR
 * as defined in the header file.
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_set_address(MAC_DEVICE *mac, SPEC_ADDR *spec_addr)
{
    mac->registers->address = *spec_addr;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Get the current set of specific match addresses for the MAC.
 * Note that a pointer is not returned as this would give direct access to the
 * MAC address space which may cause unpredictable behaviour if wrongly used.
 *
 * Return type is of type SPEC_ADDR as defined in the header file.
 */
/******************************************************************************/
/*SPEC_ADDR mac_get_address(MAC_DEVICE *mac) @@yoav: this function isn't used and its return value is wierd
{
    return (mac->registers->address);
}*/
/******************************************************************************/


/******************************************************************************/
/*
 * Set specific local addresses of the MAC.
 * Rather than setting up all four specific addresses, this function sets them
 * up individually.  The input parameter should be a pointer to type MAC_ADDR.
 *
 * There are no return values.
 *
 * odedg - IMPORTANT - The order we write the words are important !!!
 * TOP must be written last to enable the MAC address comparison
 *
 */
/******************************************************************************/
void mac_set_laddr1(MAC_DEVICE *mac, MAC_ADDR *address)
{
	mac->registers->address.one.bottom		= address->bottom;
	mac->registers->address.one.top			= address->top;
}
void mac_set_laddr2(MAC_DEVICE *mac, MAC_ADDR *address)
{
    mac->registers->address.two.bottom		= address->bottom;
	mac->registers->address.two.top			= address->top;
}
void mac_set_laddr3(MAC_DEVICE *mac, MAC_ADDR *address)
{
    mac->registers->address.three.bottom	= address->bottom;
	mac->registers->address.three.top		= address->top;
}
void mac_set_laddr4(MAC_DEVICE *mac, MAC_ADDR *address)
{
    mac->registers->address.four.bottom		= address->bottom;
	mac->registers->address.four.top		= address->top;
}
/******************************************************************************/

/******************************************************************************/
/*
 * Set the values of the statistics registers.
 * This is for debug only and allows reading and writing to the statistic
 * registers to verify functionality.
 *
 * There is no return value.
 */
/******************************************************************************/
void mac_set_stats(MAC_DEVICE *mac, MAC_STATS *stats)
{
    mac->registers->stats = *stats;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Write to a specific MAC memory address.  The defines in the header file
 * should be used for this.
 * Note: care should be taken when using this function so as not to pass null
 * pointers or write to read only registers etc.
 *
 * Return value:
 *  0   :   OK
 *  -1  :   Invalid input range.
 */
/******************************************************************************/
int mac_register_wr
(
    MAC_DEVICE  *mac,
    UINT_32     reg_addr,
    UINT_32     data
)
{
    if (reg_addr > MAC_REG_TOP)
    {
        return -1;
    }
    else
    {
        *( (UINT_32 *) (((char *) mac->registers ) + reg_addr) ) = data;
        return 0;
    }
}
/******************************************************************************/


/******************************************************************************/
/*
 * Read from a specific MAC memory address. The defines in the header file
 * should be used for this.
 * Note that the range of the address requested is not checked.
 *
 * Returns register contents.
 */
/******************************************************************************/
UINT_32 mac_register_rd(MAC_DEVICE *mac, UINT_32 reg_addr)
{
   return (*( (UINT_32 *) ( ((char *) mac->registers ) + reg_addr ) ));
}
/******************************************************************************/


/******************************************************************************/
/*
 * Reset the MAC device to its default value.  The MAC will be held in
 * quiescent state.
 * This function should be called to initialise and check the device prior to
 * setting up the buffer queues and enabling the MAC.  If it is called mid way
 * through operation, the MAC is reset to default value and any pending frames
 * will be lost.
 * Note that the values in the MAC_DEVICE software structure are not reset, only
 * the MAC registers are reset.  This is to allow, if necessary to recover the
 * buffers and reload them into the MAC, however prior to doing this, they
 * should be cleared first.
 *
 * Return value:
 *  0   :   OK
 *  -1  :   Error in write/read check on initialisation.
 */
/******************************************************************************/
int mac_reset(MAC_DEVICE *mac)
{
    MAC_ADDR zero_address = {0x00000000, 0x00000000};
    MAC_ADDR enet_address = zero_address;

    int stats_length;
    int loop_i;

    stats_length = sizeof(MAC_STATS)/4;

    /* Write to registers and set default values. */
    mac->registers->net_control = 0x00000000 | STATS_CLR | MDIO_ENABLED |
                                    DEF_LOOP;
    mac->registers->net_config = 0x00000000 | DEF_PCLK_DIV | BIT_RATE_ENABLED;
    mac->registers->tx_status = 0xFFFFFFFF;
    mac->registers->rx_qptr = 0x00000000;
    mac->registers->tx_qptr = 0x00000000;
    mac->registers->rx_status = 0xFFFFFFFF;
    mac->registers->irq_disable = 0xFFFFFFFF;
    mac->registers->irq_status = 0x00000000;
    mac->registers->phy_man = 0x00000000;
    mac->registers->hash_addr = zero_address;
    mac->registers->address.one = zero_address;
    mac->registers->address.two = zero_address;
    mac->registers->address.three = zero_address;
    mac->registers->address.four = zero_address;
    mac->registers->id_check = 0x00000000;

    /* Now read back values and return if not correct. */
    if ( (mac->registers->id_check != 0x00000000) ||
        (mac->registers->address.four.bottom != zero_address.bottom) ||
        (mac->registers->address.four.top != zero_address.top) ||
        (mac->registers->address.three.bottom != zero_address.bottom) ||
        (mac->registers->address.three.top != zero_address.top) ||
        (mac->registers->address.two.bottom != zero_address.bottom) ||
        (mac->registers->address.two.top != zero_address.top) ||
        (mac->registers->address.one.bottom != enet_address.bottom) ||
        (mac->registers->address.one.top != enet_address.top) ||
        (mac->registers->hash_addr.bottom != zero_address.bottom) ||
        (mac->registers->hash_addr.top != zero_address.top) ||
        (mac->registers->phy_man != 0x00000000) ||
        (mac->registers->irq_status != 0x00000000) ||
        (mac->registers->irq_mask != 0x00003FFF) ||
        (mac->registers->rx_status != 0x00000000) ||
        (mac->registers->tx_qptr != 0x00000000) ||
        (mac->registers->rx_qptr != 0x00000000) ||
        (mac->registers->tx_status != 0x00000000)
       )
    {
        return -1;
    }
    else if ( (mac->registers->net_control != (0x00000000 | MDIO_ENABLED | DEF_LOOP) ) ||
              (mac->registers->net_config != (0x00000000 | DEF_PCLK_DIV | BIT_RATE_ENABLED) )
            )
    {
        return -1;
    }
    else
    {
        for (loop_i=0;loop_i<stats_length;loop_i++)
        {
            if ( *( ((UINT_32 *) &mac->registers->stats) + loop_i ) !=
                        0x00000000 )
            {
                return -1;
            }
        }

        return 0;
    }
}
/******************************************************************************/

/******************************************************************************/
/*
 * Enable length field checking feature.
 * The length field check feature automatically discards frames that has a frame
 * length smaller than that reported in the length field of the header.
 *
 * Note that in accordance with the IEEE spec, frames that are longer than that
 * reported in length field is still accepted as a valid frame.
 *
 * This function has no return value.
 */
/******************************************************************************/
void mac_enable_len_check(MAC_DEVICE *mac)
{
    mac->registers->net_config |= RX_LEN_CHK;
}
/******************************************************************************/


/******************************************************************************/
/*
 * Disable length field checking feature.
 *
 * This function has no return value.
 */
/******************************************************************************/
void mac_disable_len_check(MAC_DEVICE *mac)
{
    mac->registers->net_config &= (~RX_LEN_CHK);
}
/******************************************************************************/

/******************************************************************************/
/*
 * Convert standard byte style ethernet address to format compatible with MAC.
 *
 * Input    :   Pointer to beginning of 6 byte address.
 *              Pointer to MAC_ADDR structure.
 * Return values:
 *  0   :   OK
 *  -1  :   Invalid inputs.
 */
/******************************************************************************/
int enet_addr_byte_mac(const char * enet_byte_addr, MAC_ADDR *enet_addr)
{
    if ((enet_byte_addr == NULL) || (enet_addr == NULL))
    {
        return -1;
    }
    else
    {
        enet_addr->bottom = enet_byte_addr[0] |
                            (enet_byte_addr[1] << 8) |
                            (enet_byte_addr[2] << 16) |
                            (enet_byte_addr[3] << 24);
        enet_addr->top = enet_byte_addr[4] |
                         (enet_byte_addr[5] << 8);
        return 0;
    }
}
/******************************************************************************/


/******************************************************************************/
/*
 * Convert MAC type ethernet address to standard byte style ethernet address.
 *
 * Input    :   Pointer to beginning of free space for 6 byte address.
 *              Pointer to MAC_ADDR structure.
 * Return values:
 *  0   :   OK
 *  -1  :   Invalid inputs.
 */
/******************************************************************************/
int enet_addr_mac_byte(char * enet_byte_addr, MAC_ADDR *enet_addr)
{
    if ((enet_byte_addr == NULL) || (enet_addr == NULL))
    {
        return -1;
    }
    else
    {
        enet_byte_addr[0] = enet_addr->bottom & 0xFF;
        enet_byte_addr[1] = (enet_addr->bottom >> 8) & 0xFF;
        enet_byte_addr[2] = (enet_addr->bottom >> 16) & 0xFF;
        enet_byte_addr[3] = (enet_addr->bottom >> 24) & 0xFF;

        enet_byte_addr[4] = enet_addr->top & 0xFF;
        enet_byte_addr[5] = (enet_addr->top >> 8) & 0xFF;

        return 0;
    }
}
/******************************************************************************/

/******************************************************************************/
/********************************** U-Boot driver ****************************/
/******************************************************************************/

#define MDIO_TIMEOUT            100000            /* instruction cycles? */    
#define MACB_RCV_LENGTH_MASK    0x000007ff

/* Prototypes for u-boot etherenet driver */
int dw_macb_initialize(bd_t *bd);
int macb_init(struct eth_device *dev, bd_t *bd);
void macb_halt(struct eth_device *netdev);
int macb_rx(struct eth_device *netdev);
int macb_send(struct eth_device *netdev, volatile void *packet, int length);

static int inited = 0;
static MAC_DEVICE mac_dev0;
//static MAC_DEVICE mac_dev1;

/*
 * PHY MDIO timeout
 */
static int macb_PHY_timeout(MAC_DEVICE * macdev, int timeout)
{
    while (!mac_phy_man_idle(macdev))
    {
        if (timeout >0)
        {
            timeout--;
        }
        else
        {
            return (-1);
        }
    }
    return 0;
}

int dw_macb_initialize(bd_t *bd)
{
	int eth_nr = 0;
	struct eth_device *dev;

	dev = (struct eth_device *)malloc(sizeof *dev);

	sprintf(dev->name, "MACB%d", eth_nr);

	dev->iobase = (int)MACB_IO_ADDR0;
	dev->init = macb_init;
	dev->halt = macb_halt;
	dev->send = macb_send;
	dev->recv = macb_rx;

	eth_register(dev);

	eth_nr++;

	/* TODO: register second ethernet port, too */

	/* Call eth_init already here because we want the ethernet addresses
	 * to be initialized for the Linux kernel even if u-boot doesn't use
	 * tftp */
	eth_init(bd); /* or: macb_init(dev, bd); */

	return eth_nr;
}

int macb_init(struct eth_device *dev, bd_t *bd)
{
    BYTE phy_id;
    u32 mac_phy_reg4;
    u32 mac_phy_reg5;
    u32 mac_phy_status;
    MAC_ADDR    hash_addr;          /* hash register structure */
    MAC_ADDR    mac_addr;
    u32 *rx_bd;
	char s_env_mac[64];
	unsigned char* macb_mac;
	unsigned int data;
	unsigned int phy_control_reg;
	char mac0[6];

    /* If we're inited then nothing to do */
    if (inited) {
        return 0;
    }

    /* Init the global device descriptor to zeros */
    memset(&mac_dev0, 0, sizeof(mac_dev0));
//	memset(&mac_dev1, 0, sizeof(mac_dev1));

    /* Get H/W revision number */
    //printf("MACB0 revision 0x%4.4x\n", *(u32*)(MACB_IO_ADDR0 + MAC_REV_ID));
//	printf("MACB1 revision 0x%4.4x\n", *(u32*)(MACB_IO_ADDR1 + MAC_REV_ID));

    if (mac_attach_device(&mac_dev0, (void*)MACB_IO_ADDR0) < 0)
        return -1;
//	if (mac_attach_device(&mac_dev1, (void*)MACB_IO_ADDR1) < 0)
//		return -1;

    /* Stop tx */
	mac_abort_tx(&mac_dev0);
    /* Stop rx */
    mac_disable_rx(&mac_dev0);
    /* Mask all interrupts */
	mac_mask_irq(&mac_dev0, IRQ_ALL);
    /* Read interrupt status in order to clear it */
    mac_get_irq_stat(&mac_dev0);
    /* Reset the device */
    if (mac_reset(&mac_dev0) != 0)
        printf("Error reseting MACB0 device\n");

	/* Stop tx */
//	mac_abort_tx(&mac_dev1);
    /* Stop rx */
//    mac_disable_rx(&mac_dev1);
    /* Mask all interrupts */
//	mac_mask_irq(&mac_dev1, IRQ_ALL);
    /* Read interrupt status in order to clear it */
//    mac_get_irq_stat(&mac_dev1);
    /* Reset the device */
//    if (mac_reset(&mac_dev1) != 0)
//        printf("Error reseting MACB1 device\n");

    /* Take phy out of reset the using GPIO */
//    *(volatile u32*)(DW_GPIO_BASE + 0x00) = 0xC0000000;
//	udelay(100000); /* Wait a bit (this seems to make it work) */

	phy_control_reg = 0;

#ifdef MACB_100
	mac_set_speed(&mac_dev0,SPEED_100M);
	phy_control_reg |= 0x2000;
#else
	mac_set_speed(&mac_dev0,SPEED_10M);
#endif

#ifdef MACB_FULL
	mac_full_duplex(&mac_dev0);
	phy_control_reg |= 0x0100;
#else
	mac_half_duplex(&mac_dev0);
#endif

    /* Configure the PHY */
    mac_enable_MDIO(&mac_dev0);
//	mac_enable_MDIO(&mac_dev1);

#ifndef ETH1_PRIMARY_IF
	if (dw_board == DW_BOARD_EXPEDIBLUE)
		phy_id = PHY0_ID_EXPEDIBLUE;
	else
		phy_id = PHY0_ID_EXPEDITOR;
#else
	if (dw_board == DW_BOARD_EXPEDIBLUE)
		phy_id = PHY1_ID_EXPEDIBLUE;
	else
		phy_id = PHY1_ID_EXPEDITOR;
#endif

	udelay(100);

	mac_phy_man_rd(&mac_dev0, phy_id, 3);
	if (macb_PHY_timeout(&mac_dev0, MDIO_TIMEOUT) != 0)
		return -1;
	data = mac_phy_man_data(&mac_dev0);

	if ((data & 0xFFF0) == 0x8F20)
	{
		//printf("Found phy at mdio address 0x%02X\n", phy_id);
	}
	else
	{
		printf("Could Not Find phy %d\n", phy_id);
		return -1;
	}

	/* Set phy interrupt to go through mdio */
	mac_phy_man_wr(&mac_dev0, phy_id, 0x10, 0x0008 | (phy_id << 11));
	if (macb_PHY_timeout(&mac_dev0, MDIO_TIMEOUT) != 0)
		return (-1);

	/*  No auto negotiation */
	mac_phy_man_wr(&mac_dev0, phy_id, 0x4, 0x0000);
	if (macb_PHY_timeout(&mac_dev0, MDIO_TIMEOUT) != 0)
		return (-1);

	// Configure phy
	mac_phy_man_wr(&mac_dev0, phy_id, 0x0, phy_control_reg ); 
	if (macb_PHY_timeout(&mac_dev0, MDIO_TIMEOUT) != 0)
		return (-1);

	/* Read PHY status register */
	mac_phy_man_rd(&mac_dev0, phy_id, 0x01);
	if (macb_PHY_timeout(&mac_dev0, MDIO_TIMEOUT) != 0)
		return (-1);
	mac_phy_status = mac_phy_man_data(&mac_dev0);

	/* read the PHY register reg4 */
	mac_phy_man_rd(&mac_dev0, phy_id, 0x04);
	if (macb_PHY_timeout(&mac_dev0, MDIO_TIMEOUT) != 0)
		return (-1);
	mac_phy_reg4 = mac_phy_man_data(&mac_dev0);
	/* read the PHY register reg5 */
	mac_phy_man_rd(&mac_dev0, phy_id, 0x05);
	if (macb_PHY_timeout(&mac_dev0, MDIO_TIMEOUT) != 0)
		return (-1);
	mac_phy_reg5 = mac_phy_man_data(&mac_dev0);
	/* determine actual status after auto-negotiation*/
	mac_phy_status = mac_phy_reg4 & mac_phy_reg5;
	/* For now we force 10mb-hd until issues with phy are resolved (maybe this should stay this way cause we don't
	   support dynamic changing of the link type */

    /* De-activate MDIO interface */
    mac_disable_MDIO(&mac_dev0);
//	mac_disable_MDIO(&mac_dev1);

    mac_disable_copy_all(&mac_dev0);
    mac_allow_broadcast(&mac_dev0);
    mac_disable_unicast(&mac_dev0);
    mac_disable_multicast(&mac_dev0);

    mac_set_rx_oset(&mac_dev0, 0);

    hash_addr.bottom = 0x0;
    hash_addr.top = 0x0;
    mac_set_hash(&mac_dev0, &hash_addr);

//	mac_disable_copy_all(&mac_dev1);
//	mac_allow_broadcast(&mac_dev1);
//	mac_disable_unicast(&mac_dev1);
//	mac_disable_multicast(&mac_dev1);

//	mac_set_rx_oset(&mac_dev1, 0);

//	hash_addr.bottom = 0x0;
//	hash_addr.top = 0x0;
//	mac_set_hash(&mac_dev1, &hash_addr);

/* Base memory used by MACB starts 4mb after MPMC's base memory this is so
   it won't collide with the kernel when we load it to MPMC memory. */

#ifndef ETH1_PRIMARY_IF
	/* Set the transmit queue pointer */
	mac_set_tx_qptr(&mac_dev0, (void*)mac_dev0.tx_desc_buf);
    /* Set revceive queue pointer */
    mac_set_rx_qptr(&mac_dev0, (void*)mac_dev0.rx_desc_buf);
#else
	mac_set_tx_qptr(&mac_dev1, (void*)mac_dev1.tx_desc_buf);
	mac_set_rx_qptr(&mac_dev1, (void*)mac_dev1.rx_desc_buf);
#endif

    /* Mark the driver as "initialized" */
    inited = 1;

	if ((dw_board != DW_BOARD_EXPEDITOR1) && (dw_board != DW_BOARD_EXPEDITOR2) && (dw_board != DW_BOARD_EXPEDIWAU_BASIC)) {
		macb_mac = dw_get_mac(DW_MACID_MACB1);
		if (macb_mac != NULL)
		{
			sprintf(s_env_mac, "%02X:%02X:%02X:%02X:%02X:%02X",
					macb_mac[0], macb_mac[1], macb_mac[2], macb_mac[3],
					macb_mac[4], macb_mac[5]);
			setenv ("ethaddr1", s_env_mac);
			//printf("MACB1 using MAC address: %s\n", s_env_mac);
		} 
		else                            
			printf("Error settting MACB0's MAC address, perhaps EEPROM is corrupt\n");
	}

	macb_mac = dw_get_mac(DW_MACID_MACB0);
	if (macb_mac != NULL)
	{
		dev->enetaddr[0] = macb_mac[0];
		dev->enetaddr[1] = macb_mac[1];
		dev->enetaddr[2] = macb_mac[2];
		dev->enetaddr[3] = macb_mac[3];
		dev->enetaddr[4] = macb_mac[4];
		dev->enetaddr[5] = macb_mac[5];

		sprintf(s_env_mac, "%02X:%02X:%02X:%02X:%02X:%02X",
				macb_mac[0], macb_mac[1], macb_mac[2], macb_mac[3],
				macb_mac[4], macb_mac[5]);
		setenv ("ethaddr0", s_env_mac);
		setenv ("ethaddr", s_env_mac);
		//printf("MACB0 using MAC address: %s\n", s_env_mac);
	} 
	else
		printf("Error settting MACB's MAC address, perhaps EEPROM is corrupt\n");

    /* Set mac address filter */
    if (eth_getenv_enetaddr("ethaddr", mac0))
        enet_addr_byte_mac(mac0, &mac_addr);
#ifndef ETH1_PRIMARY_IF
    mac_set_laddr1(&mac_dev0, &mac_addr);

    /* Configure RX BD */
    rx_bd = (u32*)mac_dev0.rx_desc_buf;

    rx_bd[0] = ((u32)mac_dev0.rx_packet_buf) | (1 << 1);
    rx_bd[1] = 0;

    /* Enable RX */
    mac_dev0.registers->net_control |= RX_EN;
#else
	mac_set_laddr1(&mac_dev1, &mac_addr);

    /* Configure RX BD */
    rx_bd = (u32*)mac_dev1.rx_desc_buf;

    rx_bd[0] = ((u32)mac_dev1.rx_packet_buf) | (1 << 1);
    rx_bd[1] = 0;

    /* Enable RX */
	mac_dev1.registers->net_control |= RX_EN;
#endif

    return 0;
}

void macb_halt(struct eth_device *netdev)
{
    /* If we aren't inited yet, then return */
    if (!inited) {
        return;
    }

    /* Nothing really to be done here, driver should continue working (eth_rx/eth_send) in future calls
	   without performing any special "halt" function. */
}

int macb_rx(struct eth_device *netdev)
{
    u32 rx_status;
    u32 len;
	u32 *rx_bd;

    /* Check if we got a packet */
#ifndef ETH1_PRIMARY_IF
    rx_status = mac_get_rx_stat(&mac_dev0);
#else
	rx_status = mac_get_rx_stat(&mac_dev1);
#endif

    /* Check if we revceived anything */
    if (!(rx_status & RX_DONE))
		return 0;

	/* Clear status bits */
#ifndef ETH1_PRIMARY_IF
	mac_reset_rx_stat(&mac_dev0, rx_status);
#else
	mac_reset_rx_stat(&mac_dev1, rx_status);
#endif

	/* Check if frame received successfully */
#ifndef ETH1_PRIMARY_IF
    rx_bd = (u32*)mac_dev0.rx_desc_buf;
#else
    rx_bd = (u32*)mac_dev1.rx_desc_buf;
#endif
    rx_status = rx_bd[1];
    if (!(rx_status & RBQE_SOF) || !(rx_status & RBQE_EOF)) {
        printf("Error, incomplete frame received\n");
        len = 0;
    }
    else
    {
        /* Get received frame length */
        len = rx_status & MACB_RCV_LENGTH_MASK;
        /* Pass the packet up to the protocol layers. */
#ifndef ETH1_PRIMARY_IF
        NetReceive((volatile uchar*)mac_dev0.rx_packet_buf, len);
#else
        NetReceive((volatile uchar*)mac_dev1.rx_packet_buf, len);
#endif
        /* @@ dbg print pkt */
        /*
		{
        u32 i;
		for (i = 0; i < len; i++) {
            if (!(i & 0xF))
				printf("\n%03X:", i);
#ifndef ETH1_PRIMARY_IF
			printf(" %02X", ((volatile char*)mac_dev0.rx_packet_buf)[i]);
#else
			printf(" %02X", ((volatile char*)mac_dev1.rx_packet_buf)[i]);
#endif
        }
		}
		*/

    }

    /* Reconfigure rx bd's for next frame */
#ifndef ETH1_PRIMARY_IF
    rx_bd[0] = ((u32)mac_dev0.rx_packet_buf) | (1 << 1);
#else
	rx_bd[0] = ((u32)mac_dev1.rx_packet_buf) | (1 << 1);
#endif

    return len;
}

int macb_send(struct eth_device *netdev, volatile void *packet, int length)
{
    u32 *tx_bd;
	u32 tx_status;

    /* Write TX BD */
#ifndef ETH1_PRIMARY_IF
    tx_bd 		= (u32*)mac_dev0.tx_desc_buf;
    tx_bd[0] 	= (u32)mac_dev0.tx_packet_buf;
    tx_bd[1] 	= (1 << 30) | (1 << 15) | length;

    /* Copy data to trasmit buffer */
    memcpy((void*)mac_dev0.tx_packet_buf, (void*)packet, length);

    /* Start transmitting */
    mac_dev0.registers->net_control |= ( TX_START | TX_EN );

	/* Wait for tx complete..*/
	while (mac_transmitting(&mac_dev0)) 

	udelay(20);

	/* Clear trasmition status */
	tx_status = mac_get_tx_stat(&mac_dev0);
	mac_reset_tx_stat(&mac_dev0, tx_status);

#else
    tx_bd 		= (u32*)mac_dev1.tx_desc_buf;
    tx_bd[0] 	= (u32)mac_dev0.tx_packet_buf;
    tx_bd[1] 	= (1 << 30) | (1 << 15) | length;

    /* Copy data to trasmit buffer */
    memcpy((void*)mac_dev0.tx_packet_buf, (void*)packet, length);

    /* Start transmitting */
    mac_dev1.registers->net_control |= ( TX_START | TX_EN );

	/* Wait for tx complete..*/
	while (mac_transmitting(&mac_dev1)) 

	udelay(20);

	/* Clear trasmition status */
	tx_status = mac_get_tx_stat(&mac_dev1);
	mac_reset_tx_stat(&mac_dev1, tx_status);

#endif


	/* Make sure we are not in any tx error state */
	if (tx_status & (TX_RETRY_EXC | TX_URUN | TX_BUF_EXH))
	{
		printf("Error sending packet - tx in error state\n");
		return 0;
	}

	return length;
}

#endif

