#include <common.h>
#include <command.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include "dw.h"
#include "clock.h"
#include "gpio.h"

#define RSTSR_OSC12  0x8
#define RSTSR_WIFIEN 0x4
#define RSTSR_DECTEN 0x2
#define RSTSR_CLKMSK (RSTSR_OSC12 | RSTSR_WIFIEN | RSTSR_DECTEN)

#if 0
unsigned int dw_get_pclk(void)
{
	u16 clk_straps = readl(DW_CMU_BASE + DW_CMU_RSTSR) & RSTSR_CLKMSK;
	u16 src_sel = readl(DW_CMU_BASE + DW_CMU_CDR6);

	/* only dect oscillator available, no 12Mhz */
	if (clk_straps == RSTSR_DECTEN)
		return 72000000;

	/* 12MHz and dect, no wifi, but we use pll2 as hclk */
	if ((clk_straps == (RSTSR_DECTEN | RSTSR_OSC12)) &&
	    !(src_sel & 0x2))
		return 72000000;

	return 80000000;
}
#endif

#define BAUDRATE 115200

static void platform_serial_init(void)
{
	unsigned int intgr, frac, div;
	unsigned int pclk = dw_get_pclk();

	intgr = (pclk / (BAUDRATE << 4));
	frac = (pclk / BAUDRATE) & 0xf;
	div = intgr | (frac << 12);

	writel(div, IO_ADDRESS(DW_UART1_BASE) + DW_UART_DIV);

	/* Enable UART */
	writel(1, IO_ADDRESS(DW_UART1_BASE) + DW_UART_CTL);
}

static void
platform_cmu_init(void)
{
	unsigned int tmp;
	unsigned short straps;

#if 0
	/* only if nomusb? */
	tmp = readl(DW_GPIO_BASE + EXPEDITOR_GPDATA);
	tmp &= ~EXPEDITOR_USB_PULLUP_MASK;
	writel(tmp, DW_GPIO_BASE + EXPEDITOR_GPDATA);
	tmp = readl(DW_GPIO_BASE + EXPEDITOR_GPDIR);
	tmp &= ~EXPEDITOR_USB_PULLUP_MASK;
	writel(tmp, DW_GPIO_BASE + EXPEDITOR_GPDIR);
	tmp = readl(DW_GPIO_BASE + EXPEDITOR_GPPULLEN);
	tmp &= ~EXPEDITOR_USB_PULLUP_MASK;
	writel(tmp, DW_GPIO_BASE + EXPEDITOR_GPPULLEN);
	tmp = readl(DW_GPIO_BASE + EXPEDITOR_GPPOD);
	tmp &= ~EXPEDITOR_USB_PULLUP_MASK;
	writel(tmp, DW_GPIO_BASE + EXPEDITOR_GPPOD);
	tmp = readl(DW_GPIO_BASE + EXPEDITOR_GPEN);
	tmp |= EXPEDITOR_USB_PULLUP_MASK;
	writel(tmp, DW_GPIO_BASE + EXPEDITOR_GPEN);
#endif

	/* skip the rest of the setup if we already are in fast mode */
	tmp = readl(DW_CMU_BASE + DW_CMU_CMR);
	if (tmp & 0x2) {
		/* enable important clocks, incl. both UARTs */
		clk_enable("ahb3");
		clk_enable("ethmaca");
		clk_enable("tmr1");
		clk_enable("uart2");
		clk_enable("uart1");
		return;
	}

	/* sysconfig */
	writel(0xC03E, DW_SYSCFG_BASE + DW_SYSCFG_GCR0);
	/* DPCLK_EN, external memory */
	/* MEMCS3 is Memory CS and not NFL Data */
	tmp = readl(DW_SYSCFG_BASE + DW_SYSCFG_GCR1);
	tmp |= 0x100;  /* MEMCS3 is SDRAM CS and not SRAM CS */
	writel(tmp, DW_SYSCFG_BASE + DW_SYSCFG_GCR1);

#ifdef DW_OSCILLATOR_CONFIG
	tmp = readl(DW_CMU_BASE + DW_CMU_OSCR);
	tmp |= 0x9400;
	tmp |= (DW_OSCILLATOR_CONFIG & 0x7) << 5;
	writel(tmp, DW_CMU_BASE + DW_CMU_OSCR);
#endif

	straps = readl(DW_CMU_BASE + DW_CMU_RSTSR);
#ifdef DW_OSCILLATOR_CONFIG
	straps |= (DW_OSCILLATOR_CONFIG & 0x7) << 1;
#endif

	/* Choose PLL2 and USB 1.1 clock divider for DW52 chip (for DW74 we
	   will override it below).
	   This tmpue is assuming PLL2 outputs 144Mhz (otherwise no way to get
	   48Mhz for USB 1.1 on DW52).
	   Set PLL2_PRECLKDIV = 0, so PLL2 input is not divided.
	   Set MCU_CLK_DIV = 0, so PLL2 output is not divided if used by ARM
	   domain */
	writel(0x2001, DW_CMU_BASE + DW_CMU_CDR2);

	/* cmu */
	if (straps & RSTSR_DECTEN) {
		/* Dect exists (assume 13.824Mhz xtal) - Set PLL2 to
		   147.546MHz generated from 13.824Mhz xtal of Dect */
		writel(0x4320, DW_CMU_BASE + DW_CMU_PLL2CR0);
	} else {
		/* Dect does not exist - Set PLL2 to 144.0000 MHz
		   generated from 20Mhz xtal of Wifi */
		writel(0x4524, DW_CMU_BASE + DW_CMU_PLL2CR0);
	}

	writel(0x0001, DW_CMU_BASE + DW_CMU_PLL2CR1); /* Turn on PLL2 */

	/* let pll2 settle before connecting it to the core */
	udelay(50);

	/* Set DMR */
	tmp = straps & (RSTSR_WIFIEN | RSTSR_DECTEN);
	if (tmp == RSTSR_WIFIEN) {
		/* Only WiFi: PLL2 will be 144Mhz. DSP_OSC = 20Mhz. ARM running from 20Mhz oscillator */
		writel(0x0000, DW_CMU_BASE + DW_CMU_DMR);
	} else if (tmp == RSTSR_DECTEN) {
		/* Only Dect: PLL2 will be 143.7696Mhz. DSP_OSC = 13.824Mhz.
		   ARM running from 13.824Mhz oscillator (PLL2) */
		writel(0x0003, DW_CMU_BASE + DW_CMU_DMR);
	} else {
		/* Both Dect and Wifi: PLL2 will be 143.7696Mhz.
		   DSP_OSC = 13.824Mhz. ARM running from 20Mhz oscillator
		   (PLL1) */
		writel(0x0002, DW_CMU_BASE + DW_CMU_DMR);
	}

	clk_enable("fc");
	clk_enable("dma");
	clk_enable("dmafifo");
	clk_enable("mpmc");
	clk_enable("shmem");
	clk_enable("intmem");
	clk_enable("etm");

	/* enable fast mode - start running from PLL */
	writel(0x0002, DW_CMU_BASE + DW_CMU_CMR);

	clk_enable("ahb3");
	clk_enable("ethmaca");
	clk_enable("tmr1");
	clk_enable("uart2");
	clk_enable("uart1");

	writel(0x0000, DW_CMU_BASE + DW_CMU_CDR0); /* PCLKDIV = 0 */
	/* DP clock divider and source (being overriden later by the
	   kernel...) */
	writel(0x00B3, DW_CMU_BASE + DW_CMU_CDR1);

	/* Hold DSP in reset */
	writel(0x0000, DW_CMU_BASE + DW_CMU_DSPRST);

	/* we are done if chip is not dw74 */
	if (readl(DW_SYSCFG_BASE + DW_SYSCFG_CHIP_ID) != 0x0740)
		return;

	/* don't output test clock */
	writel(0x0000, DW_CMU_BASE + DW_CMU_COMR);

	/* if neither WIFIEN nor OSC12 are set, 240MHz is no option */
	if ((straps & (RSTSR_WIFIEN | RSTSR_OSC12)) == 0)
		return;

	/* if OSC12 is not set, manually activate the 12MHz oscillator */
	if ((straps & RSTSR_OSC12) == 0) {
		tmp = readl(DW_CMU_BASE + DW_CMU_OSCR);
		tmp |= 0x9480;
		writel(tmp, DW_CMU_BASE + DW_CMU_OSCR);
	}

	udelay(1000); /* Wait for OSC12 to stabilize */

	/* Move to slow-mode - run from oscillator (12Mhz or 13.824Mhz or
	   20Mhz depends on strap pins by HW) */
	writel(0x0000, DW_CMU_BASE + DW_CMU_CMR);

	if (straps & RSTSR_WIFIEN) {
		/* 240MHz from PLL3 */
		writel(0x0001, DW_CMU_BASE + DW_CMU_CDR6);

		/* SDMMC clock divider is enabled and routed from PLL3. SDMMC
		   clock divider is 4 (240/(4+1)=48). PLL4 disabled */
		writel(0x0034, DW_CMU_BASE + DW_CMU_CDR5);

		/* Output 24Mhz from PLL3 (from 20Mhz oscillator) */
		writel(0x0C12, DW_CMU_BASE + DW_CMU_PLL3CR0);
		writel(0x0101, DW_CMU_BASE + DW_CMU_PLL3CR1);
	} else {
		/* 240MHz from PLL4 */
		writel(0x0003, DW_CMU_BASE + DW_CMU_CDR6);

		/* SDMMC clock divider is enabled and routed from PLL4. PLL4
		   is enabled. Its divider is set to 2 */
		writel(0x1119, DW_CMU_BASE + DW_CMU_CDR5);

		/* Output 24Mhz from PLL4 (from 12Mhz oscillator) */
		writel(0x0026, DW_CMU_BASE + DW_CMU_PLL4CR0);
		writel(0x0103, DW_CMU_BASE + DW_CMU_PLL4CR1);
	}

	/* USB 1.1 from SDMMC clock on DW74 */
	writel(0x8001, DW_CMU_BASE + DW_CMU_CDR2);

	/* let pll3 settle before connecting it to the core */
	udelay(100); /* FIXME */

#ifdef CONFIG_DPCHARGE_EN
# define MAIN_CLK_SEL 0x0001 /* slow mode (80MHz) */
#else
# define MAIN_CLK_SEL 0x0002 /* fast mode (240MHz) */
#endif
	/* switch to fast clock (or slow clock if charger is enabled) */
	writel(MAIN_CLK_SEL, DW_CMU_BASE + DW_CMU_CMR);
}

/*
 * GPIO pins used with alternate configuration have to be disabled
 */
static void
platform_gpio_init(void)
{
	/* UART1 */
	gpio_disable(AGPIO(17)); /* TXD1 */
	gpio_disable(AGPIO(18)); /* RXD1 */

	/* UART2 */
	gpio_disable(BGPIO(19)); /* TXD2 */
	gpio_disable(BGPIO(18)); /* RXD2 */

	/* NAND/LCDC */
	gpio_disable(AGPIO(8));  /* CLE */
	gpio_disable(AGPIO(9));  /* ALE */
	gpio_disable(AGPIO(10)); /* CS0 */
	gpio_disable(AGPIO(11)); /* CS1 */
	gpio_disable(AGPIO(12)); /* READY */
	gpio_disable(AGPIO(13)); /* WR */
	gpio_disable(AGPIO(14)); /* RD */

	/* NAND 8-bit interface */
	gpio_disable(CGPIO(9));  /* NFLD0 */
	gpio_disable(CGPIO(10)); /* NFLD1 */
	gpio_disable(CGPIO(11)); /* NFLD2 */
	gpio_disable(CGPIO(12)); /* NFLD3 */
	gpio_disable(CGPIO(13)); /* NFLD4 */
	gpio_disable(CGPIO(14)); /* NFLD5 */
	gpio_disable(CGPIO(15)); /* NFLD6 */
	gpio_disable(CGPIO(16)); /* NFLD7 */
	/* 16-bit interface: A1,6,24-29 */

	/* EMACA */
	gpio_disable(AGPIO(30)); /* RXDA2 */
	gpio_disable(AGPIO(31)); /* RXDA3 */
	gpio_disable(BGPIO(7));  /* RXA_ER */
	gpio_disable(BGPIO(8));  /* RXDA0 */
	gpio_disable(BGPIO(9));  /* RXDA1 */
	gpio_disable(BGPIO(10)); /* TXA_COL */
	gpio_disable(BGPIO(11)); /* TXA_CLK */
	gpio_disable(BGPIO(12)); /* TXA_EN */
	gpio_disable(BGPIO(13)); /* TXDA2 */
	gpio_disable(BGPIO(14)); /* TXDA3 */
	gpio_disable(BGPIO(15)); /* TXA_ER */
	gpio_disable(BGPIO(23)); /* RXA_CLK */
	gpio_disable(BGPIO(24)); /* TXDA1 */
	gpio_disable(BGPIO(26)); /* RXA_CRS */
	gpio_disable(BGPIO(27)); /* MDCA */
	gpio_disable(BGPIO(28)); /* RXA_DV */
	gpio_disable(BGPIO(30)); /* MDIOA */
	gpio_disable(BGPIO(31)); /* TXDA0 */

	/* wait 100ns so that the GPIO's can settle */
	udelay(1);
}

static void
platform_sram_init(void)
{
	unsigned long tmp;

	/* enable power for internal RAM */
	tmp = readl(DW_SYSCFG_BASE + DW_SYSCFG_MCU_POWER_EN0);
	tmp &= ~(0x1);
	writel(tmp, DW_SYSCFG_BASE + DW_SYSCFG_MCU_POWER_EN0);

	/* Isolation bit of internal memory should be set for old dw52 chips
	 * and cleared for newer chips. */
	tmp = readl(DW_SYSCFG_BASE + DW_SYSCFG_MCU_ISOLATION0);
	/* set/clear isolation bit (enable memory access) */
	if ((readl(DW_SYSCFG_BASE + DW_SYSCFG_CHIP_ID) == 0x5200) &&
	    (readl(DW_SYSCFG_BASE + DW_SYSCFG_CHIP_REV) < 2))
		tmp |= 0x1; /* set (old chips) */
	else
		tmp &= ~(0x1); /* clear (new chips) */
	writel(tmp, DW_SYSCFG_BASE + DW_SYSCFG_MCU_ISOLATION0);
}

int check_phy(int gpio)
{
	int ret = 0;

	/* disable PHY reset */
	gpio_set_value(gpio, 1);

	/* enable clock for Ethernet MAC A */
	clk_enable("ethmaca");

	/* wait for PHY to leave reset */
	udelay(10000);

	/* check PHY id 0x7 */
	writel(0x638e0000, DW_MACA_BASE + DW_MACA_PHYMAN);

	/* give the PHY some time to answer */
	udelay(200);

	/* we expect the id 0x8F20 */
	if ((readl(DW_MACA_BASE + DW_MACA_PHYMAN) & 0xFFF0) == 0x8F20)
		ret = 1;

	/* disable clocks for Ethernet MACs */
	clk_disable("ethmaca");

	return ret;
}

int __test_i2c_address(int address)
{
	int tmp;
	int ret = 0;

	/* the function gets the i2c address and returns 1 if we got ack for
	   it and 0 in case of nack */
	/* bit 0 of the address must be 0 */

	clk_enable("sbus");

	/* sbus_loc=10b for dw52 */
	/* sbus_loc=00b and disable gpios for dw74 */
	if (readl(DW_SYSCFG_BASE + DW_SYSCFG_CHIP_ID) == 0x5200) {
		tmp = readl(DW_SYSCFG_BASE + DW_SYSCFG_IO_LOC);
		tmp |=  0x2;
		tmp &= ~0x1;
		writel(tmp, DW_SYSCFG_BASE + DW_SYSCFG_IO_LOC);
	} else {
		tmp = readl(DW_SYSCFG_BASE + DW_SYSCFG_IO_LOC);
		tmp &= ~0x3;
		writel(tmp, DW_SYSCFG_BASE + DW_SYSCFG_IO_LOC);

		gpio_disable(AGPIO(28)); /* SDA */
		gpio_disable(AGPIO(29)); /* SCL */
	}

	/* reset i2c */
	writel(0x2, DW_SBUS_BASE + SBUS_CTL);

	/* wait some time afer the reset */
	udelay(20);

	/* configure i2c to master */
	writel(0x8200, DW_SBUS_BASE + SBUS_CFG);

	/* configure i2c rate to 100Khz */
	writel(0x320, DW_SBUS_BASE + SBUS_FREQ);

	/* wait some time after the frequency setting */
	udelay(20);

	/* write the physical address */
	writel(address, DW_SBUS_BASE + SBUS_TX);

	/* wait for the byte to be transmitted on the bus */
	/* at least 10 micros, which are 2400 cycles at 240 Mhz (worst case) */
	udelay(10);

	/* read the status (ack or nack) */
	if (readl(DW_SBUS_BASE + SBUS_STAT) & 0x2)
		ret = 1;

	/* write stop condition */
	writel(0x100, DW_SBUS_BASE + SBUS_TX);

	/* wait for the stop condition to be transmitted */
	udelay(10);

	return ret;
}

extern int dw_chip;

static int
__platform_board_detection(void)
{
#if 0
	/* This delay is needed only on the Expediblue */
	udelay(200000);
#endif

	/* enable MDIO (ethernet MAC A) */
	writel(0x10, DW_MACA_BASE + DW_MACA_CONTROL);

	/* check PHY id 0xA (Expediblue) */
	writel(0x650e0000, DW_MACA_BASE + DW_MACA_PHYMAN);

	/* give the PHY some time to answer */
	udelay(2000);

	/* if we found id 0x8F20 its Expediblue, bail out */
	if ((readl(DW_MACA_BASE + DW_MACA_PHYMAN) & 0xFFF0) == 0x8F20)
		return DW_BOARD_EXPEDIBLUE;

	clk_disable("ethmaca");

	if (dw_chip == DW_CHIP_DW52) {
		/* enable PHY reset for Expeditor 1 */
		gpio_direction_output(AGPIO(23), 0);
		gpio_enable(AGPIO(23));

		/* enable PHY reset for IMB2 */
		gpio_direction_output(DGPIO(30), 0);
		gpio_enable(DGPIO(30));

		/* enable PHY reset for Expeditor 2 */
		gpio_direction_output(BGPIO(3), 0);
		gpio_enable(BGPIO(3));
	}

	/* enable PHY reset for ExpediWAU (basic board) */
	gpio_direction_output(CGPIO(18), 0);
	gpio_enable(CGPIO(18));

	/* enable PHY reset for iMH */
	gpio_direction_output(CGPIO(2), 0);
	gpio_enable(CGPIO(2));

	/* setup strap pins */
	gpio_direction_output(AGPIO(31), 1);
	gpio_direction_output(AGPIO(30), 1);
	gpio_enable(AGPIO(31));
	gpio_enable(AGPIO(30)); /* REMOVE */

	/*********************************************************************/

	if (dw_chip == DW_CHIP_DW52) {
		/* check for Expeditor1 - AGPIO 23 */
		if (check_phy(AGPIO(23)))
			return DW_BOARD_EXPEDITOR1;

		/* check for IMB2 - DGPIO 30 */
		if (check_phy(DGPIO(30)))
			return DW_BOARD_IMB2;
	}

	/* check for iMH - CGPIO 2 */
	if (check_phy(CGPIO(2)))
		return DW_BOARD_IMH;

	/*********************************************************************/
	
	/* it is not expediblue, expeditor1, imb2 or iMH. now we should check for expeditor2, expediwwau-basic, imb and iframe.
	 * testing ethernet is not enough in these cases, since iframe doesn't always contains ethernet PHY,
	 * and also - few boards here share the same ethernet PHY reset sequence.
	 * BUT - if we have ethernet phy, we must reset it correctly
	 *
	 * sequence for detecting the specific board:
	 *	if (expeditor2 phy reset working)
	 *		if (got ack for i2c address 0x90)
	 *			imb
	 *		else
	 *			expeditor2
	 *	else if (expediwau-basic phy reset is working)
	 *		if (got ack for i2c address 0x90)
	 *			iframe
	 *		else
	 *			expediwau-basic
	 *	else [this is the case of iframe w/o ethernet phy]
	 *		iframe
	 */

	/* check for Expeditor2 */
	if ((dw_chip == DW_CHIP_DW52) && check_phy(BGPIO(3))) {
		/* if ack for i2c address 0x90, then imb. else - expeditor2 */
		if (__test_i2c_address(0x90))
			return DW_BOARD_IMB;
		else
			return DW_BOARD_EXPEDITOR2;
	}

	/*********************************************************************/

	/* disable PHY reset for ExpediWAU (basic board) - CGPIO18 */
	/* check for ExpediWAU */
	if (check_phy(CGPIO(18))) {
		/* if ack for i2c address 0x90, then iframe else
		   expediwau-basic */
		if (__test_i2c_address(0x90))
			return DW_BOARD_IFRAME;
		else
			return DW_BOARD_EXPEDIWAU_BASIC;
	}

	/*********************************************************************/

	/* currently only iframe board is allowed to be w/o ethernet PHY */
	return DW_BOARD_IFRAME;
}

int
platform_board_detection(void)
{
	int board;

	gpio_direction_output(AGPIO(30), 0); /* RXDA2 */
	gpio_direction_output(AGPIO(31), 0); /* RXDA3 */
	gpio_direction_output(BGPIO(7), 0);  /* RXA_ER */
	gpio_direction_output(BGPIO(8), 0);  /* RXDA0 */
	gpio_direction_output(BGPIO(9), 0);  /* RXDA1 */
	gpio_direction_output(BGPIO(26), 0); /* RXA_CRS */
	gpio_direction_output(BGPIO(28), 0); /* RXA_DV */
	gpio_enable(AGPIO(30)); /* RXDA2 */
	gpio_enable(AGPIO(31)); /* RXDA3 */
	gpio_enable(BGPIO(7));  /* RXA_ER */
	gpio_enable(BGPIO(8));  /* RXDA0 */
	gpio_enable(BGPIO(9));  /* RXDA1 */
	gpio_enable(BGPIO(26)); /* RXA_CRS */
	gpio_enable(BGPIO(28)); /* RXA_DV */

	board = __platform_board_detection();

	gpio_disable(AGPIO(30));
	gpio_disable(AGPIO(31));
	gpio_disable(BGPIO(7));
	gpio_disable(BGPIO(8));
	gpio_disable(BGPIO(9));
	gpio_disable(BGPIO(26));
	gpio_disable(BGPIO(28));

	return board;
}

/* MPMC static memory register definitions */
#define MPMC_STATIC_BANK0		(MPMC_BASE+0x200)
#define MPMC_STATIC_BANK1		(MPMC_BASE+0x220)
#define MPMC_STATIC_BANK2		(MPMC_BASE+0x240)
#define MPMC_STATIC_BANK3		(MPMC_BASE+0x260)

#define MPMC_STATIC_CONFIG		0x00
#define MPMC_STATIC_WAIT_WEN		0x04
#define MPMC_STATIC_WAIT_OEN		0x08
#define MPMC_STATIC_WAIT_RD		0x0C
#define MPMC_STATIC_WAIT_PAGE		0x10
#define MPMC_STATIC_WAIT_WR		0x14
#define MPMC_STATIC_WAIT_TURN		0x18

/* CS 1 Static MPMC configuration */
#define MPMC_CS1_CONFIG			0x00000081
#define MPMC_CS1_WEN			0
#define MPMC_CS1_OEN			0
#define MPMC_CS1_RD			10
#define MPMC_CS1_PAGE			1
#define MPMC_CS1_WR			5
#define MPMC_CS1_TURN			6

static void
platform_static_memory_init(void)
{
	writel(MPMC_CS1_CONFIG, MPMC_STATIC_BANK1 + MPMC_STATIC_CONFIG);
	writel(MPMC_CS1_WEN,    MPMC_STATIC_BANK1 + MPMC_STATIC_WAIT_WEN);
	writel(MPMC_CS1_OEN,    MPMC_STATIC_BANK1 + MPMC_STATIC_WAIT_OEN);
	writel(MPMC_CS1_RD,     MPMC_STATIC_BANK1 + MPMC_STATIC_WAIT_RD);
	writel(MPMC_CS1_PAGE,   MPMC_STATIC_BANK1 + MPMC_STATIC_WAIT_PAGE);
	writel(MPMC_CS1_WR,     MPMC_STATIC_BANK1 + MPMC_STATIC_WAIT_WR);
	writel(MPMC_CS1_TURN,   MPMC_STATIC_BANK1 + MPMC_STATIC_WAIT_TURN);
}

void lowlevel(void)
{
	platform_cmu_init();
	platform_gpio_init();

	/* FIXME: this is only needed on Expediblue, will consume 200ms */
	/* udelay(200000); */

	platform_sram_init();
	platform_static_memory_init();

	platform_serial_init();
}
