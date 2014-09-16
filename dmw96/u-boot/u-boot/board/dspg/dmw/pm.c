#include <asm/io.h>
#include <common.h>
#include "dmw.h"
#include "clock.h"
#include "pm.h"


#define SLEEP_QUIRK_DIS_INP	0x10000	/* Disable input buffer in self refresh */

#define param_lowpower_auto_enable_bits 5
#define param_lowpower_auto_enable_reg  22
#define param_lowpower_auto_enable_offset 0
#define param_lowpower_control_bits 5
#define param_lowpower_control_reg  20
#define param_lowpower_control_offset 8
#define param_lowpower_external_cnt_bits 16
#define param_lowpower_external_cnt_reg  21
#define param_lowpower_external_cnt_offset 16
#define param_lowpower_internal_cnt_bits 16
#define param_lowpower_internal_cnt_reg  22
#define param_lowpower_internal_cnt_offset 8
#define param_lowpower_power_down_cnt_bits 16
#define param_lowpower_power_down_cnt_reg  20
#define param_lowpower_power_down_cnt_offset 16
#define param_lowpower_refresh_enable_bits 2
#define param_lowpower_refresh_enable_reg  22
#define param_lowpower_refresh_enable_offset 24
#define param_lowpower_self_refresh_cnt_bits 16
#define param_lowpower_self_refresh_cnt_reg  21
#define param_lowpower_self_refresh_cnt_offset 0

/* defined by DSPG */
#define param_dram_type_bits 4
#define param_dram_type_reg  129
#define param_dram_type_offset 16

/* denali parameter access */
#define denali_readl(_regno)		readl(DMW_MPMC_BASE + (_regno) * 4)
#define denali_writel(_val, _regno)	writel((_val), DMW_MPMC_BASE + (_regno) * 4)

#define denali_set(_name, _val)                               \
({                                                            \
	unsigned int msk = (1 << param_##_name##_bits) - 1;       \
	unsigned int reg;                                         \
	                                                      \
	reg = denali_readl(param_##_name##_reg);              \
	reg &= ~(msk << param_##_name##_offset);              \
	reg |= (_val) << param_##_name##_offset;              \
	denali_writel(reg, param_##_name##_reg);              \
})

#define denali_get(_name)                                     \
({                                                            \
	unsigned int msk = (1 << param_##_name##_bits) - 1;       \
	unsigned int reg;                                         \
	                                                      \
	reg = denali_readl(param_##_name##_reg);              \
	reg >>= param_##_name##_offset;                       \
	reg & msk;                                            \
})



void wfi_mode (void)
{
	__asm__ __volatile__ ("wfi" : : : "memory");
}


unsigned int pm_clk_devs(unsigned int enb){

	if (enb) {
		clk_enable("nfc");
		clk_enable("usb_phy");
		clk_enable("usb_mac");
		clk_enable("sdmmc");
		clk_enable("i2c1");
		clk_enable("i2c2");
		clk_enable("smc");
		clk_enable("gdmac");

	}else {
		clk_disable("nfc");
		clk_disable("usb_phy");
		clk_disable("usb_mac");
		clk_disable("sdmmc");
		clk_disable("i2c1");
		clk_disable("i2c2");
		clk_disable("smc");
		clk_disable("gdmac");
	}

}


void go_to_self_refresh(int req_mode){

	int quirks;
	int mode_flag;

	/* apply quirks as necessary */
	if (dmw_chip_revision() == DMW_CHIP_DMW96_REV_A)
		quirks = SLEEP_QUIRK_DIS_INP;

	mode_flag = 1 << SLEEP_DDR_MODE(req_mode |= quirks);

	/* For both DDR2 and LPDDR2*/
	denali_set(lowpower_power_down_cnt, (3+2) * 3);
	denali_set(lowpower_self_refresh_cnt, (3+200) * 2);

	denali_set(lowpower_auto_enable, mode_flag);
	denali_set(lowpower_control, mode_flag);

	wfi_mode();

	denali_set(lowpower_control, 0);
	denali_set(lowpower_auto_enable, 0);

}

