/*------------------------------------------------------------------------------
 * $Id: macb_AL.h,v 1.8 2007/01/03 15:17:25 odedg Exp $
 *  =Id: macb_AL.h,v 1.3 2003/03/24 11:09:06 pvrose Exp $
 *------------------------------------------------------------------------------
 *
 *                                       Copyright (c) 2003
 *            CADENCE                    CADENCE DESIGN FOUNDRY
 *                                       All rights reserved.
 *
 *------------------------------------------------------------------------------
 *
 *   Filename       :   macb_AL.h
 *
 *   Author         :   Steven Ho, DICS, LDC
 *
 *   Date           :   5th Oct 2001
 *
 *   Limitations    :   Supports only the Cadence Enhanced MAC, the Cadence
 *                      Gigabit MAC will be supported in the next version.
 *                      The standard Cadence Ethernet MAC will NOT be supported
 *                      due to the different underlying architecture of the
 *                      transmit interface.
 *
 *------------------------------------------------------------------------------
 *   Description    :   Header file containing definitions for the Cadence MAC
 *                      Hardware Abstraction Layer.
 *
 *------------------------------------------------------------------------------
 *   Revision Control
 *
 *------------------------------------------------------------------------------
 */

/* Ensure that header file is only compiled once */
#ifndef __MACB_AL_h__

#define __MACB_AL_h__

/* Some simple definitions to make things easier to read */
#ifndef TRUE
    #define TRUE   (0x01)
#endif
#ifndef FALSE
    #define FALSE  (0x00)
#endif

#ifndef NULL
    #define NULL (0)
#endif

// 100/10 Full/Half definition
//#define MACB_100
//#define MACB_FULL


/* Specify the length of the IO space in bytes */
#define IO_PORT_LENGTH 0x100

/* Specify the default receive buffer size. */
#define RX_BUFF_SIZE_SHIFT 11
#define RX_BUF_SIZE (1 << RX_BUFF_SIZE_SHIFT)/*128*/


/* Versions of macb prior to release 1p10 behave differently with regard to the
 * wrapping under certain conditions and require special attention
 */
//#define macb_pre_1p10

/* Specify size of descriptor queues for software that uses it.. */
#define RBQ_LENGTH 32//3/*128*/


/* Specify whether the MDIO should be available, this is set so that for reset
 * function, appropriate options are setup.  To disable, use 0.
 */
#define MDIO_ENABLED (MDIO_EN)


/* Specify the default PCLK divisor for generating MDC.  This simply defines
 * which of the divisor control bits are active.  By default, bit 1 on and bit
 * 0 is off.
 */
#define DEF_PCLK_DIV (MDC_DIV1 | MDC_DIV0)

/* Specify default loopback mode.  0 for no loopback, other values are LB_MAC
 * and LB_PHY
 */
#define DEF_LOOP 0


/* Specify if hardware is configured for bit rate mode operation.
 * Note that the value is shifted according to the address map
 */
#define BIT_RATE_ENABLED (0)


/* Define some bit positions for registers. */

/* Bit positions for network control register */
#define TX_HALT         (1<<10)     /* Halt transmission after current frame */
#define TX_START        (1<<9)      /* Start tx (tx_go) */
#define NET_BP          (1<<8)      /* Enable back pressure i.e. force cols */
#define STATS_WR_EN     (1<<7)      /* Enable writing to statistic registers */
#define STATS_INC       (1<<6)      /* Increment statistic registers */
#define STATS_CLR       (1<<5)      /* Clear statistic registers */
#define MDIO_EN         (1<<4)      /* Enable MDIO port */
#define TX_EN           (1<<3)      /* Enable transmit circuits */
#define RX_EN           (1<<2)      /* Enable receive circuits */
#define LB_MAC          (1<<1)      /* Perform local loopback at MAC */
#define LB_PHY          (1<<0)      /* Perform external loopback through PHY */

/* Bit positions for network configuration register */
#define RX_NO_FCS       (1<<17)     /* Discard FCS from received frames. */
#define RX_LEN_CHK      (1<<16)     /* Receive length check. */
#define RX_OFFSET_BASE  14          /* Position of LSB for rx buffer offsets. */
#define RX_OFFSET1      (1<<(RX_OFFSET_BASE + 1))   /* RX offset bit 1. */
#define RX_OFFSET0      (1<<RX_OFFSET_BASE)         /* RX offset bit 0. */
#define RX_PAUSE_EN     (1<<13)     /* Enable pause reception */
#define RETRY_TEST      (1<<12)     /* Retry test for speeding up debug */
#define MDC_DIV1        (1<<11)     /* PCLK divisor for MDC, bit 1 */
#define MDC_DIV0        (1<<10)     /* PCLK divisor for MDC, bit 0 */
#define EAM_EN          (1<<9)      /* External address match enable */
#define FRAME_1536      (1<<8)      /* Enable reception of 1536 byte frames */
#define UNICAST_EN      (1<<7)      /* Receive unicast hash frames */
#define MULTICAST_EN    (1<<6)      /* Receive multicast hash frames */
#define NO_BROADCAST    (1<<5)      /* Do not receive broadcast frames */
#define COPY_ALL        (1<<4)      /* Copy all frames */
#define BIT_RATE        (1<<2)      /* Enable bit rate mode */
#define FULL_DUPLEX     (1<<1)      /* Enable full duplex */
#define SPEED_100       (1<<0)      /* Set to 100Mb mode */

/* Bit positions for network status register */
#define PHY_IDLE        (1<<2)      /* PHY management is idle */
#define MDIO_IN         (1<<1)      /* Status of mdio_in pin */
#define LINK_STATUS     (1<<0)      /* Status of link pin */

/* Bit positions for transmit status register */
#define TX_URUN         (1<<6)      /* Transmit underrun occurred */
#define TX_COMPLETE     (1<<5)      /* Transmit completed OK */
#define TX_BUF_EXH      (1<<4)      /* Transmit buffers exhausted mid frame */
#define TX_GO           (1<<3)      /* Status of tx_go internal variable */
#define TX_RETRY_EXC    (1<<2)      /* Retry limit exceeded */
#define TX_COL          (1<<1)      /* Collision occurred during frame tx */
#define TX_USED         (1<<0)      /* Used bit read in tx buffer */

/* Bit positions for receive status register */
#define RX_ORUN         (1<<2)      /* Receive overrun occurred */
#define RX_DONE         (1<<1)      /* Frame successfully received */
#define RX_BUF_USED     (1<<0)      /* Receive buffer used bit read */

/* Bit positions for interrupts */
#define IRQ_PAUSE_0     (1<<13)     /* Pause time has reached zero */
#define IRQ_PAUSE_RX    (1<<12)     /* Pause frame received */
#define IRQ_HRESP       (1<<11)     /* hresp not ok */
#define IRQ_RX_ORUN     (1<<10)     /* Receive overrun occurred */
#define IRQ_LINK        (1<<9)      /* Status of link pin changed */
#define IRQ_TX_DONE     (1<<7)      /* Frame transmitted ok */
#define IRQ_TX_ERROR    (1<<6)      /* Transmit error occurred or no buffers */
#define IRQ_RETRY_EXC   (1<<5)      /* Retry limit exceeded */
#define IRQ_TX_URUN     (1<<4)      /* Transmit underrun occurred */
#define IRQ_TX_USED     (1<<3)      /* Tx buffer used bit read */
#define IRQ_RX_USED     (1<<2)      /* Rx buffer used bit read */
#define IRQ_RX_DONE     (1<<1)      /* Frame received ok */
#define IRQ_MAN_DONE    (1<<0)      /* PHY management operation complete */
#define IRQ_ALL         (0xFFFFFFFF)/* Everything! */

/* Transmit buffer descriptor status words bit positions. */
#define TBQE_USED       (1 << 31)   /* Used bit. */
#define TBQE_WRAP       (1 << 30)   /* Wrap bit */
#define TBQE_URUN       (1 << 28)   /* Transmit underrun occurred. */
#define TBQE_RETRY_EXC  (1 << 29)   /* Retry limit exceeded. */
#define TBQE_BUF_EXH    (1 << 27)   /* Buffers exhausted mid frame. */
#define TBQE_LAST_BUF   (1 << 15)   /* Last buffer */
#define TBQE_LEN_MASK   (0x7ff)     /* Mask for length field */
#define TBQE_DUMMY      (0x800087ff)/* Dummy value to check for free buffer */
/* Receive buffer descriptor status words bit positions. */
#define RBQE_SOF        (1<<14)     /* Start of frame. */
#define RBQE_EOF        (1<<15)     /* End of frame. */

#define RBQE_LEN_MASK   (0x7FF)     /* Mask for the length field. */

/* Revision ID Register */
#define MACB_REV_ID_MODEL_MASK   (0x000F0000)    /* Model ID */
#define MACB_REV_ID_MODEL_BASE   (16)            /* For Shifting */
#define MACB_REV_ID_REG_MODEL    (0x00010000)    /* MACB module ID */
#define MACB_REV_ID_REV_MASK     (0x0000FFFF)    /* Revision ID */

/* Define some memory offsets for easier direct access to memory map. */
#define MAC_NET_CONTROL         (0x00)
#define MAC_NET_CONFIG          (0x04)
#define MAC_NET_STATUS          (0x08)
#define MAC_TX_STATUS           (0x14)
#define MAC_RX_QPTR             (0x18)
#define MAC_TX_QPTR             (0x1C)
#define MAC_RX_STATUS           (0x20)
#define MAC_IRQ_STATUS          (0x24)
#define MAC_IRQ_ENABLE          (0x28)
#define MAC_IRQ_DISABLE         (0x2C)
#define MAC_IRQ_MASK            (0x30)
#define MAC_PHY_MAN             (0x34)
#define MAC_PAUSE_TIME          (0x38)
#define MAC_STATS_PAUSE_RX      (0x3C)
#define MAC_STATS_FRAMES_TX     (0x40)
#define MAC_STATS_SINGLE_COL    (0x44)
#define MAC_STATS_MULTI_COL     (0x48)
#define MAC_STATS_FRAMES_RX     (0x4C)
#define MAC_STATS_FCS_ERRORS    (0x50)
#define MAC_STATS_ALIGN_ERRORS  (0x54)
#define MAC_STATS_DEF_TX        (0x58)
#define MAC_STATS_LATE_COL      (0x5C)
#define MAC_STATS_EXCESS_COL    (0x60)
#define MAC_STATS_TX_URUN       (0x64)
#define MAC_STATS_CRS_ERRORS    (0x68)
#define MAC_STATS_RX_RES_ERR    (0x6C)
#define MAC_STATS_RX_ORUN       (0x70)
#define MAC_STATS_RX_SYM_ERR    (0x74)
#define MAC_STATS_EXCESS_LEN    (0x78)
#define MAC_STATS_JABBERS       (0x7C)
#define MAC_STATS_USIZE_FRAMES  (0x80)
#define MAC_STATS_SQE_ERRORS    (0x84)
#define MAC_STATS_LENGTH_ERRORS (0x88)
#define MAC_STATS_PAUSE_TX      (0x8C)
#define MAC_HASH_BOT            (0x90)
#define MAC_HASH_TOP            (0x94)
#define MAC_LADDR1_BOT          (0x98)
#define MAC_LADDR1_TOP          (0x9C)
#define MAC_LADDR2_BOT          (0xA0)
#define MAC_LADDR2_TOP          (0xA4)
#define MAC_LADDR3_BOT          (0xA8)
#define MAC_LADDR3_TOP          (0xAC)
#define MAC_LADDR4_BOT          (0xB0)
#define MAC_LADDR4_TOP          (0xB4)
#define MAC_ID_CHECK            (0xB8)
#define MAC_TX_PAUSE_QUANT      (0xBC)
#define MAC_USER_IO             (0xC0)
#define MAC_REV_ID              (0xFC)
#define MAC_REG_TOP             (0xFC)

/* An enumerated type for loopback values. This can be one of three values, no
 * loopback -normal operation, local loopback with internal loopback module of
 * MAC or PHY loopback which is through the external PHY.
 */
#ifndef __MAC_LOOP_ENUM__
#define __MAC_LOOP_ENUM__
typedef enum {LB_NONE, LB_EXT, LB_LOCAL} MAC_LOOP;
#endif


/* The possible operating speeds of the MAC, currently only supporting 10 and
 * 100Mb modes.
 */
#ifndef __MAC_SPEED_ENUM__
#define __MAC_SPEED_ENUM__
typedef enum {SPEED_10M, SPEED_100M, SPEED_1000M, SPEED_1000M_PCS} MAC_SPEED;
#endif


/* Define a new type to use that should be unsigned and 32-bits in length.
 * This is currently supported with 16 and 32-bit architectures of ANSI C, this
 * may need to be adjusted for other architectures.
 */
typedef unsigned long int  UINT_32;


/* A new type for a byte definition.  This is an unsigned 8-bit 'char' value. */
typedef unsigned char BYTE;


/* Define some types for using with the HAL.  These types correspond to the
 * memory map and programming structure of the MAC device.
 * All structures are 'volatile' to indicate they can be changed by some non-
 * programming means - i.e. by the hardware itself.  This prevents the compiler
 * from making false assumptions on how to optimise the code.  Some elements
 * are also defined as 'const' to enforce some checks on the programmer.  These
 * are only for register fields that can only be changed by the hardware 
 * and are not writable.
 */


/* The Address organisation for the MAC device.  All addresses are split into
 * two 32-bit register fields.  The first one (bottom) is the lower 32-bits of
 * the address and the other field are the high order bits - this may be 16-bits
 * in the case of MAC addresses, or 32-bits for the hash address.
 * In terms of memory storage, the first item (bottom) is assumed to be at a
 * lower address location than 'top'. i.e. top should be at address location of
 * 'bottom' + 4 bytes.
 */
#ifndef __MAC_ADDR_DEF__
#define __MAC_ADDR_DEF__
typedef struct {
   UINT_32  bottom;     /* Lower 32-bits of address. */
   UINT_32  top;        /* Upper 32-bits of address. */
} volatile MAC_ADDR;
#endif


/* The following is the organisation of the address filters section of the MAC
 * registers.  The Cadence MAC contains four possible specific address match
 * addresses, if an incoming frame corresponds to any one of these four
 * addresses then the frame will be copied to memory.
 * It is not necessary for all four of the address match registers to be
 * programmed, this is application dependant.
 */
#ifndef __SPEC_ADDR_DEF__
#define __SPEC_ADDR_DEF__
typedef struct {
    MAC_ADDR    one;        /* Specific address register 1. */
    MAC_ADDR    two;        /* Specific address register 2. */
    MAC_ADDR    three;      /* Specific address register 3. */
    MAC_ADDR    four;       /* Specific address register 4. */
} volatile SPEC_ADDR;
#endif


/* The set of statistics registers implemented in the Cadence MAC.
 * The statistics registers implemented are a subset of all the statistics
 * available, but contains all the compulsory ones.
 * For full descriptions on the registers, refer to the Cadence MAC programmers
 * guide or the IEEE 802.3 specifications.
 */
typedef struct {
   UINT_32  pause_rx;           /* Number of pause frames received. */
   UINT_32  frames_tx;          /* Number of frames transmitted OK */
   UINT_32  single_col;         /* Number of single collision frames */
   UINT_32  multi_col;          /* Number of multi collision frames */
   UINT_32  frames_rx;          /* Number of frames received successfully */
   UINT_32  fcs_errors;         /* Number of frames received with crc errors */
   UINT_32  align_errors;       /* Frames received without integer no. bytes */
   UINT_32  def_tx;             /* Frames deferred due to crs */
   UINT_32  late_col;           /* Collisions occuring after slot time */
   UINT_32  excess_col;         /* Number of excessive collision frames. */
   UINT_32  tx_urun;            /* Transmit underrun errors due to DMA */
   UINT_32  crs_errors;         /* Errors caused by crs not being asserted. */
   UINT_32  rx_res_errors;      /* Number of times buffers ran out during rx */
   UINT_32  rx_orun;            /* Receive overrun errors due to DMA */
   UINT_32  rx_symbol_errors;   /* Number of times rx_er asserted during rx */
   UINT_32  excess_length;      /* Number of excessive length frames rx */
   UINT_32  jabbers;            /* Excessive length + crc or align errors. */
   UINT_32  usize_frames;       /* Frames received less than min of 64 bytes */
   UINT_32  sqe_errors;         /* Number of times col was not asserted */
   UINT_32  length_check_errors;/* Number of frames with incorrect length */
   UINT_32  pause_tx;           /* Number of pause frames transmitted. */

} volatile MAC_STATS;


/* This is the memory map for the Cadence Enhanced MAC device.
 * For full descriptions on the registers, refer to the Cadence MAC programmers
 * guide or the IEEE 802.3 specifications.
 */
typedef volatile struct {
   volatile UINT_32  net_control;            /* Network control 0x00 */
   volatile UINT_32  net_config;             /* Network config 0x04 */
   const volatile UINT_32  net_status; /* Network status, RO, 0x08 */
   const volatile UINT_32  rsvd0;      /* reserved 0x0C*/
   const volatile UINT_32  rsvd1;      /* reserved 0x10*/
   volatile UINT_32  tx_status;              /* Transmit status 0x14 */
   volatile UINT_32  rx_qptr;                /* Receive queue pointer 0x18 */
   volatile UINT_32  tx_qptr;                /* Transmit queue pointer 0x1C */
   volatile UINT_32  rx_status;              /* Receive status 0x20 */
   volatile UINT_32  irq_status;             /* Interrupt status 0x24 */
   volatile UINT_32  irq_enable;             /* Interrupt enable 0x28 */
   volatile UINT_32  irq_disable;            /* Interrupt disable 0x2C */
   const volatile UINT_32 irq_mask;    /* Interrupt mask, RO, 0x30 */
   volatile UINT_32  phy_man;                /* PHY management 0x34 */
   const volatile UINT_32  pause_time; /* Pause time register 0x38 */
   volatile MAC_STATS stats;                 /* MAC statistics 0x3C - 0x8C */
   volatile MAC_ADDR hash_addr;              /* Hash address 0x90 - 0x94 */
   volatile SPEC_ADDR address;               /* Specific addresses 0x98 - 0xB4 */
   volatile UINT_32  id_check;               /* Type ID check 0xB8 */
   volatile UINT_32  tx_pause_quant;         /* Transmit pause quantum. 0xBC*/
   volatile UINT_32  user_io;                /* User IO register, 0xC0 */
                                    /* 0xC4 to 0xF8 reserved not listed to save */
                                    /* space, Rev_id in 0xFC, not listed. */
} volatile MAC_REG;

/* This is a structure that will be passed and used for all HAL operations, it
 * consists of pointers to the various MAC structures such as the MAC register
 * block and the first descriptor element for the rx and tx buffer queues.
 * Other internal variables declared for use in function calls and to keep track
 * of where things are.
 */
typedef struct {
   	MAC_REG  *		registers;       /* Pointer to the MAC address space. */
	unsigned char  	tx_packet_buf[2048];
	unsigned char	rx_packet_buf[2048];
	unsigned char	tx_desc_buf[8];
	unsigned char	rx_desc_buf[8];
} MAC_DEVICE;


/******************************************************************************/
/*
 * Prototypes for functions of HAL
*/
/******************************************************************************/

/* Attach a physical MAC to the MAC_DEVICE structure. */
int mac_attach_device(MAC_DEVICE *mac, void *device);

/* Re-initialise device and check reset values. */
int mac_reset(MAC_DEVICE *mac);

/* Device setup. */
void mac_set_loop(MAC_DEVICE *mac, MAC_LOOP mac_loop);
int mac_get_loop(MAC_DEVICE *mac);

int mac_set_rx_oset(MAC_DEVICE *mac, unsigned char oset);
int mac_get_rx_oset(MAC_DEVICE *mac);

void mac_enable_eam(MAC_DEVICE *mac);
void mac_disable_eam(MAC_DEVICE *mac);

void mac_enable_fcs_rx(MAC_DEVICE *mac);
void mac_disable_fcs_rx(MAC_DEVICE *mac);

void mac_enable_1536_rx(MAC_DEVICE *mac);
void mac_disable_1536_rx(MAC_DEVICE *mac);

void mac_full_duplex(MAC_DEVICE *mac);
void mac_half_duplex(MAC_DEVICE *mac);

void mac_set_speed(MAC_DEVICE *mac, MAC_SPEED mac_speed);
int mac_get_speed(MAC_DEVICE *mac);

/* Pause control. */
void mac_enable_pause_rx(MAC_DEVICE *mac);
void mac_disable_pause_rx(MAC_DEVICE *mac);
UINT_32 mac_pause_time(MAC_DEVICE *mac);

/* PHY management control. */
void mac_enable_MDIO(MAC_DEVICE *mac);
void mac_disable_MDIO(MAC_DEVICE *mac);

int mac_phy_man_wr(MAC_DEVICE *mac, BYTE phy_addr, BYTE reg_addr, UINT_32 data);
int mac_phy_man_rd(MAC_DEVICE *mac, BYTE phy_addr, BYTE reg_addr);
UINT_32 mac_phy_man_data(MAC_DEVICE *mac);

int mac_phy_man_idle(MAC_DEVICE *mac);

int mac_link_status(MAC_DEVICE *mac);

/* Available for more direct access.. */
void mac_set_rx_qptr(MAC_DEVICE * mac, void * qptr);
void * mac_get_rx_qptr(MAC_DEVICE *mac);

void mac_set_tx_qptr(MAC_DEVICE * mac, void * qptr);
void * mac_get_tx_qptr(MAC_DEVICE *mac);

/* Address setup and control. */
void mac_enable_unicast(MAC_DEVICE *mac);
void mac_disable_unicast(MAC_DEVICE *mac);

void mac_enable_multicast(MAC_DEVICE *mac);
void mac_disable_multicast(MAC_DEVICE *mac);

void mac_allow_broadcast(MAC_DEVICE *mac);
void mac_no_broadcast(MAC_DEVICE *mac);

void mac_enable_copy_all(MAC_DEVICE *mac);
void mac_disable_copy_all(MAC_DEVICE *mac);

void mac_set_hash(MAC_DEVICE *mac, MAC_ADDR *hash_addr);

void mac_set_address(MAC_DEVICE *mac, SPEC_ADDR *spec_addr);

/* Functions to convert between address formats. */
int enet_addr_byte_mac(const char * enet_byte_addr, MAC_ADDR *enet_addr);
int enet_addr_mac_byte(char * enet_byte_addr, MAC_ADDR *enet_addr);

void mac_set_laddr1(MAC_DEVICE *mac, MAC_ADDR *address);
void mac_set_laddr2(MAC_DEVICE *mac, MAC_ADDR *address);
void mac_set_laddr3(MAC_DEVICE *mac, MAC_ADDR *address);
void mac_set_laddr4(MAC_DEVICE *mac, MAC_ADDR *address);

void mac_set_id_check(MAC_DEVICE *mac, UINT_32 id_check);
UINT_32 mac_get_id_check(MAC_DEVICE *mac);

void mac_enable_len_check(MAC_DEVICE *mac);
void mac_disable_len_check(MAC_DEVICE *mac);

/* Interrupt handling and masking. */
void mac_set_irq_stat(MAC_DEVICE *mac, UINT_32 irq_status);
UINT_32 mac_get_irq_stat(MAC_DEVICE *mac);

void mac_enable_irq(MAC_DEVICE *mac, UINT_32 irq_en);
void mac_mask_irq(MAC_DEVICE *mac, UINT_32 irq_mask);
UINT_32 mac_get_irq_mask(MAC_DEVICE *mac);

/* Transmit control. */
void mac_stop_tx(MAC_DEVICE *mac) ;
void mac_abort_tx(MAC_DEVICE *mac);
int mac_transmitting(MAC_DEVICE *mac);
UINT_32 mac_get_tx_stat(MAC_DEVICE *mac);
void mac_reset_tx_stat(MAC_DEVICE *mac, UINT_32 rst_status);

/* Receive control. */
void mac_disable_rx(MAC_DEVICE *mac);
int mac_receive_on(MAC_DEVICE *mac);
UINT_32 mac_get_rx_stat(MAC_DEVICE *mac);
void mac_reset_rx_stat(MAC_DEVICE *mac, UINT_32 rst_status);

/* Debug options. */
void mac_stats_wr_en(MAC_DEVICE *mac);
void mac_stats_wr_off(MAC_DEVICE *mac);
void mac_stats_inc(MAC_DEVICE *mac);
void mac_stats_clr(MAC_DEVICE *mac);

void mac_enable_bp(MAC_DEVICE *mac);
void mac_disable_bp(MAC_DEVICE *mac);

void mac_en_retry_test(MAC_DEVICE *mac);
void mac_dis_retry_test(MAC_DEVICE *mac);

/*MAC_STATS mac_get_stats(MAC_DEVICE *mac); @@yoav: this function isn't used and its return value is wierd */
void mac_set_stats(MAC_DEVICE *mac, MAC_STATS *stats);

/* Generic register access interface. */
UINT_32 mac_register_rd(MAC_DEVICE *mac, UINT_32 reg_addr);
int mac_register_wr(MAC_DEVICE *mac, UINT_32 reg_addr, UINT_32 data);

/******************************************************************************/

#endif

