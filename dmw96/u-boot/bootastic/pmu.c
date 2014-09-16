#include "common.h"
#include "hardware.h"
#include "gpio.h"
#include "spi.h"
#include "pmu.h"

#define DP52_SPI_BASE			0x10	/* SPI (10 - 17) */
#define DP52_PMU_DIRECT_BASE		0x70	/* PMU registers (70 - 7F) */

#define DP52_SPI_INDADD			(DP52_SPI_BASE + 0)
#define DP52_SPI_INDDAT			(DP52_SPI_BASE + 1)

#define DP52_PMU_EN_SW			(DP52_PMU_DIRECT_BASE + 1)
#define DP52_PMU_LVL1_SW		(DP52_PMU_DIRECT_BASE + 2)
#define DP52_PMU_LVL2_SW		(DP52_PMU_DIRECT_BASE + 3)
#define DP52_PMU_LVL3_SW		(DP52_PMU_DIRECT_BASE + 4)

static void
dp52_select(void)
{
	gpio_set_value(BGPIO(30), 0);
}

static void
dp52_deselect(void)
{
	gpio_set_value(BGPIO(30), 1);
}

static int
dp52_direct_write(unsigned int reg, unsigned int value)
{
	unsigned char tmp[3];

	dp52_select();

	tmp[0] = reg;
	tmp[1] = (value >> 8) & 0xff;
	tmp[2] = value & 0xff;
	spi_write(tmp, 3);

	dp52_deselect();

	return 0;
}

void
pmu_init(void)
{
	uint16_t reg;

	/* enable regulators */
	reg = (1 <<  2) | /* dp io */
	      (1 <<  3) | /* rf */
	      (1 <<  5) | /* io */
	      (1 <<  7) | /* mem */
	      (1 <<  8) | /* core */
	      (1 <<  9) | /* core: use dcdc */
	      (1 << 10);  /* dp core */
	dp52_direct_write(DP52_PMU_EN_SW, reg);

	/* LDO_DP_CORE: 1.8V; CORE: 1.25V; LDO_MEM: 1.9V; RF: 2.4V */
	dp52_direct_write(DP52_PMU_LVL1_SW, 0x7243);
	/* LDO_SP1: 1.8V; LDO_SP2: 3.3V; LDO_SP3: 3.3V */
	dp52_direct_write(DP52_PMU_LVL2_SW, 0x0e52);
	/* LDO_IO: 3.3V; LDO_DP_IO: 3.3V */
	dp52_direct_write(DP52_PMU_LVL3_SW, 0x0099);
}
