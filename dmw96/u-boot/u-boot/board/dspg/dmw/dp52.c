#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <spi.h>
#include <asm/arch/gpio.h>

#include "dmw.h"
#include "dp52.h"

/* convert sample to voltage (VBANDGAP is 1v, so we can measure the range 0v..2v) */
#define AUXADC_WINDOW	15
#define SAMPLE_TO_VOLTAGE(sample)	((2000 * (sample)) / ((1 << (AUXADC_WINDOW)) - 1))

#define DEFAULT_BGP		134

/* AUX comparator gain steps x4 */
static const int gain_steps[] = {
	0, 4, 5, 6, 8, 10, 12, 16, 20, 24, 32, 40, 52, 68, 88, 116, 152, 200,
	264, 344, 404
};

struct dp52_calibration_params {
	int valid;
	int aux_bgp;
	int vbat_dcin_gain;
	int vbat_dcin_attn;
};

static struct dp52_calibration_params dp52_calibp;

static struct spi_slave *dp52_spi;

int dp52_init(void)
{
	uint16_t reg;

	dp52_spi = spi_setup_slave(CONFIG_DMW_DP52_SPI_UNIT, CONFIG_DMW_GPIO_DP52_CS, 0, 0);

	reg = dp52_direct_read(DP52_CMU_CSTSR);
	if (reg == 0)
		return -ENODEV;

	/* disable touchpad; it might interfere with sampling (auto-mode) */
	dp52_direct_write(DP52_TP_CTL1, 0);
	dp52_direct_write(DP52_TP_CTL2, 0);

	dp52_calibp.valid = 0;
	return 0;
}

int parse_dp52calibration(void)
{
	char *settings = getenv("dp52calibration");
	int bgp, gain, attn;

	/* parse calibration data */
	if (settings) {
		char *pos;
		bgp = simple_strtoul(settings, &pos, 10);
		if (bgp <= 0 || bgp > 255)
			goto invalid;

		if (*pos != ':')
			goto invalid;

		gain = simple_strtoul(pos+1, &pos, 10);
		if (gain < 0 || gain >= ARRAY_SIZE(gain_steps))
			goto invalid;

		if (*pos != ':')
			goto invalid;

		attn = simple_strtoul(pos+1, &pos, 10);
		if (attn < 0 || attn > 63)
			goto invalid;

		if (*pos != 0)
			goto invalid;

		dp52_calibp.aux_bgp = bgp;
		dp52_calibp.vbat_dcin_gain = gain;
		dp52_calibp.vbat_dcin_attn = attn;
		dp52_calibp.valid = 1;

		/* write aux_agp into HW. gain and attn can be retrieved using dp52_get_calibrated_parameters() */
		dp52_aux_enable(dp52_calibp.aux_bgp);

		return 0;
	}

invalid:
	dp52_calibp.valid = 0;
	return -1;
}


int dp52_late_init(void)
{
	int res;

	res = parse_dp52calibration();

	if (res)
		printf("Invalid DP calibration data (\"dp52calibration\"). Disabling battery management!!!\n");

	return res;
}

void dp52_select(void)
{
	spi_claim_bus(dp52_spi);
}

void dp52_deselect(void)
{
	spi_release_bus(dp52_spi);
}

int dp52_direct_write(unsigned int reg, unsigned int value)
{
	unsigned char tmp[3];

	dp52_select();

	/* write 3 bytes */
	tmp[0] = reg;
	tmp[1] = (value >> 8) & 0xff;
	tmp[2] = value & 0xff;
	spi_xfer(dp52_spi, 8*3, tmp, NULL, 0);

	dp52_deselect();

	return 0;
}

int dp52_direct_read(unsigned int reg)
{
	unsigned char tmp[2];

	dp52_select();

	/* write byte */
	tmp[0] = (reg & 0x7f) | 0x80;
	spi_xfer(dp52_spi, 8*1, tmp, NULL, 0);

	/* read 2 bytes */
	spi_xfer(dp52_spi, 8*2, NULL, tmp, 0);

	dp52_deselect();

	return (tmp[0] << 8) | tmp[1];
}

void dp52_direct_write_field(int reg, int offset, int size, unsigned int value)
{
	int old = dp52_direct_read(reg);
	int mask = ((1 << size)-1) << offset;

	if (!((old ^ (value << offset)) & mask))
		return;

	dp52_direct_write(reg, (old & ~mask) | (value << offset));
}

int dp52_direct_read_field(int reg, int offset, int size)
{
	return (dp52_direct_read(reg) >> offset) & ((1 << size)-1);
}

int dp52_indirect_read(unsigned int reg)
{
	unsigned char tmp[2];

	dp52_select();

	/* write 2 bytes */
	tmp[0] = DP52_SPI_INDADD;
	tmp[1] = reg;
	spi_xfer(dp52_spi, 8*2, tmp, NULL, 0);

	/* write byte */
	tmp[0] = DP52_SPI_INDDAT | 0x80;
	spi_xfer(dp52_spi, 8*1, tmp, NULL, 0);

	/* read 2 bytes */
	spi_xfer(dp52_spi, 8*2, NULL, tmp, 0);

	dp52_deselect();

	return (tmp[0] << 8) | tmp[1];
}

int dp52_indirect_write(unsigned int reg, unsigned int value)
{
	unsigned char tmp[3];

	dp52_select();

	/* write 2 bytes */
	tmp[0] = DP52_SPI_INDADD;
	tmp[1] = reg;
	spi_xfer(dp52_spi, 8*2, tmp, NULL, 0);

	/* write 3 bytes */
	tmp[0] = DP52_SPI_INDDAT;
	tmp[1] = (value >> 8) & 0xff;
	tmp[2] = value & 0xff;
	spi_xfer(dp52_spi, 8*3, tmp, NULL, 0);

	dp52_deselect();

	return 0;
}

/*****************************************************************************/

void dp52_power_off(void)
{
	/*mdelay to let error msg output (from UART fifo) before device shut down*/
	mdelay(10);
	for (;;) {
		dp52_direct_write(DP52_PMU_CTRL, 0x1);
		dp52_direct_write(DP52_PMU_EN_SW, 0x7a8);
		dp52_direct_write(DP52_PMU_OFF, 0xaaaa);
		dp52_direct_write(DP52_PMU_OFF, 0x5555);
		dp52_direct_write(DP52_PMU_OFF, 0xaaaa);
	}
}

void dp52_aux_enable(unsigned int bandgap)
{
	/* make sure 'LDO_DP_IO Low Regulator' is enabled */
	dp52_direct_write_field(DP52_PMU_EN_SW, 2, 1, 1);

	dp52_direct_write(DP52_CMU_CCR2, 1 << 0 | 1 << 6 | 1 << 7); /* 4Mhz output, 4Mhz enable, PWM0 running on 4Mhz */
	dp52_direct_write_field(DP52_CMU_CCR0, 10, 2, 0x2); /* AUX ACD clock 4MHz OSC */
	dp52_direct_write_field(DP52_CMU_CCR0, 5, 3, 0x3); /* AUXADCCLKDIV = 4 (1Mhz) */
	dp52_direct_write_field(DP52_AUX_EN, 0, 2, 0x03); /* enable AUX ADC & BGP */
	dp52_direct_write(DP52_AUX_ADCTRL, (bandgap<<8) | ((AUXADC_WINDOW - 8) << 1));

	/* let things settle */
	mdelay(10);
}

int dp52_compute_initial_gain_attn(int vref, int *gain, int *attn, int exact)
{
	/*
	 * Find gain/attenuation setting which converts Vref to 1V BG. First
	 * find the gain which drives the voltage above 1V, then adjust the
	 * attenuation to match 1V exactly.
	 */
	*gain = 1;
	while (vref*gain_steps[*gain]/4 < 1000 && *gain < ARRAY_SIZE(gain_steps))
		(*gain)++;

	if (*gain >= ARRAY_SIZE(gain_steps)) {
		if (exact)
			return 1;
		else
			(*gain)--;
	}

	*attn = 63;
	while (vref*gain_steps[*gain]*(193+*attn)/(4*256) > 1000 && *attn >= 0)
		(*attn)--;
	if (*attn < 0) {
		if (exact)
			return 2;
		else
			*attn = 0;
	}

	return 0;
}

int dp52_get_calibrated_parameters(int *gain, int *attn)
{
	if (!dp52_calibp.valid) {
		if (parse_dp52calibration())
			return 1;
	}

	*gain = dp52_calibp.vbat_dcin_gain;
	*attn = dp52_calibp.vbat_dcin_attn;

	return 0;
}

void dp52_auxcmp_enable(int dcin, int gain, int attn)
{
	dp52_direct_write(DP52_AUX_DC0GAINCNT1+dcin, gain | (attn << 5));
	dp52_direct_write_field(DP52_AUX_EN, 4+dcin, 1, 1);
}

void dp52_auxcmp_disable(int dcin)
{
	dp52_direct_write_field(DP52_AUX_EN, 4+dcin, 1, 0);
}

static int dp52_auxcmp_get(int dcin)
{
	return !!(dp52_direct_read(DP52_AUX_RAWPMINTSTAT) & (1 << dcin));
}

int dp52_auxadc_sample(int dcin)
{
	int mux, res;

	/* update aux adc mux */
	switch (dcin) {
		case 0: mux = DP52_AUX_MUX_DCIN0_16V; break;
		case 1: mux = DP52_AUX_MUX_DCIN1_GAIN; break;
		case 2: mux = DP52_AUX_MUX_DCIN2_16V; break;
		case 3: mux = DP52_AUX_MUX_DCIN3_16V; break;

		default:
			return -1;
	}

	dp52_direct_write_field(DP52_AUX_ADCFG, 0, 5, mux);

	/* perform sample */
	dp52_direct_write_field(DP52_AUX_ADCTRL, 0, 1, 0x1);
	mdelay(10);

	/* poll until sample is finished */
	while ((res = dp52_direct_read(DP52_AUX_ADDATA)) & 0x8000)
		mdelay(1); /* busy wait for bit to clear itself */

	/* for dcin1 (charing voltage), the values are very small (~30mV) */
	/* so meausing it as-is gives incorrect values (since A2D HW is not acuurate at extreme values) */
	/* thus - we are measuring the voltage AFTER the gain (about 1v), and then we divide by the gain */
	if (mux == DP52_AUX_MUX_DCIN1_GAIN) {
		int gain_index;
		gain_index = dp52_direct_read(DP52_AUX_DC1GAINCNT1) & 0x1f;
		res = res * 4 / gain_steps[gain_index];
	}

	return res;
}

int dp52_auxadc_measure(int dcin)
{
	int sample;
	sample = dp52_auxadc_sample(dcin);
	return SAMPLE_TO_VOLTAGE(sample);
}

/*****************************************************************************/

int increase_voltage_gain_attn(int *gain, int *attn)
{
	(*attn)++;
	if (*attn > 63) {
		*attn = 0;
		(*gain)++;
		if (*gain >= ARRAY_SIZE(gain_steps))
			return -1;
	}

	return 0;
}

int decrease_voltage_gain_attn(int *gain, int *attn)
{
	(*attn)--;
	if (*attn < 0) {
		*attn = 63;
		(*gain)--;
		if (*gain < 0)
			return -1;
	}

	return 0;
}

int dp52_auxadc_sample_avg(int dcin, int num_samples)
{
	int i;
	int sample;
	int sum = 0;
	int min = 0x0fffffff;
	int max = 0x0;

	if (num_samples < 1)
		return -1;
	
	/* we take num_sample+2, throw away thighest and lowest, and return avg of the reset */	
	for (i=0 ; i < num_samples + 2 ; i++) {
		sample = dp52_auxadc_sample(dcin);
		if (sample > max)
			max = sample;
		if (sample < min)
			min = sample;

		sum += sample;
	}

	return (sum - min - max) / num_samples;
}

static int do_calibrate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long vref = 0;
	int cmp;
	int vmeasured;
	int sample;
	int step;
	int v1;
	int v2;
	int bgp1;
	int bgp2;
	int aux_bgp;
	int attn;
	int gain;
	int found_exact = 0;
	int i;
	char settings[16];

	if (argc < 2)
		return 0;

	vref = simple_strtoul(argv[1], NULL, 10);

	if (vref != 4200) {
		printf("dp52: vref must be 4200\n");
		return 1;
	}

	printf("dp52: calibrating AUX BandGap with VBAT=%lumV, voltage divider=%dk/%dk...\n",
	       vref, CONFIG_DP52_VBAT_R1, CONFIG_DP52_VBAT_R2);

	printf("dp52: please make sure you calibrate the power supply to produce accurate VBAT between the battery pads\n");
	printf("dp52: VBAT is not what you see on the power supply!!!\n");

	/* substruct 35mV from vref (because of current-sense resistor) */
	vref -= 35;

	/* Voltage divider */
	vref = vref * CONFIG_DP52_VBAT_R2 / (CONFIG_DP52_VBAT_R1+CONFIG_DP52_VBAT_R2);

	/* compute theoretical gain and attn */
	if (dp52_compute_initial_gain_attn(vref, &gain, &attn, 1)) {
		printf("unusable reference voltage!\n");
		return 1;
	}

	/* enable auxcmp with gain/attn we just calculated */
	/* this is a MUST - since sampling vbat_dcin gives different values if gain is not set!!! */
	dp52_auxcmp_enable(CONFIG_DP52_VBAT_DCIN, gain, attn);
	mdelay(100);

	/* initialize bandgap and step */
	aux_bgp = 128;
	step = 128;
	v1 = -1;
	v2 = -1;
	bgp1 = -1;
	bgp2 = -1;

	while (step > 0 && !found_exact) {
		step >>= 1;
		int aux_temp = aux_bgp;

		/* enable aux with current bandgap, and measure voltage */
		dp52_aux_enable(aux_bgp);
		sample = dp52_auxadc_sample_avg(CONFIG_DP52_VBAT_DCIN, 4);
		vmeasured = SAMPLE_TO_VOLTAGE(sample);

		if (vmeasured == vref) {
			found_exact = 1;
		}
		else if (vmeasured > vref) {
			v1 = vmeasured;
			bgp1 = aux_bgp;
			aux_bgp += step;
		}
		else {
			v2 = vmeasured;
			bgp2 = aux_bgp;
			aux_bgp -= step;
		}
	}

	if (!found_exact) {
		/* don't allow aux_bgp values of 1 or 255 */
		if (v1 == -1 || v2 == -1) {
			printf("can't calibrate bandgap\n");
			return 1;
		}

		/* select between v1 and v2 (the closer to vref) */
		if ( (v1 - (int)vref) < ((int)vref - v2))
			aux_bgp = bgp1;
		else
			aux_bgp = bgp2;
	}

	/* enable chosen bandgap value */
	dp52_aux_enable(aux_bgp);

	/* tune gain/attn until we find minimal values which generates interrupt */
	cmp = dp52_auxcmp_get(CONFIG_DP52_VBAT_DCIN);
	while (cmp == dp52_auxcmp_get(CONFIG_DP52_VBAT_DCIN)) {
		if (cmp) {
			if (decrease_voltage_gain_attn(&gain, &attn)) {
				printf("can't calibrate gain and attn\n");
				return 1;
			}
		}
		else {
			if (increase_voltage_gain_attn(&gain, &attn)) {
				printf("can't calibrate gain and attn\n");
				return 1;
			}
		}

		dp52_auxcmp_enable(CONFIG_DP52_VBAT_DCIN, gain, attn);
		mdelay(10);
	}

	/* if we were decreasing voltage until we stopped getting interrupt, we should now get back to previous values */
	/* since we want the mininal voltage which gives the interrupt */
	if (cmp) {
		if (decrease_voltage_gain_attn(&gain, &attn)) {
			printf("can't calibrate gain and attn\n");
			return 1;
		}
	}

	dp52_auxcmp_disable(CONFIG_DP52_VBAT_DCIN);

	printf("ok: bandgap=%d, gain=%d, attn=%d\n", aux_bgp, gain, attn);

	/* save in calibration params variable */
	dp52_calibp.vbat_dcin_gain = gain;
	dp52_calibp.vbat_dcin_attn = attn;
	dp52_calibp.aux_bgp = aux_bgp;
	dp52_calibp.valid = 1;

	/* save calibration values in environment variable */
	sprintf(settings, "%d:%d:%d", aux_bgp, gain, attn);
	setenv("dp52calibration", settings);
	printf("dp52: setting dp52calibration='%s'. To adopt permanently type \"saveenv\"\n", settings);

	return 0;
}

U_BOOT_CMD(
	dp_calibrate, 2, 1, do_calibrate, "calibrate DP52 bandgap",
	"<Vref>\n"
	"    - calibrate DP52 assuming Vbat is currently 'Vref' mV"
);

static int do_dp_write(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int reg;
	unsigned int val;

	if (argc != 3)
		return -1;

	reg = simple_strtoul(argv[1], NULL, 16);
	val = simple_strtoul(argv[2], NULL, 16);

	dp52_direct_write(reg, val);

	return 0;
}

U_BOOT_CMD(dp_write, 3, 1, do_dp_write, "write value to register of DP52",
	"<reg> <value>\n"
	"    - write 'value' to direct register 'reg'"
);

static int do_dp_read(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int reg;
	unsigned int val;

	if (argc != 2)
		return -1;

	reg = simple_strtoul(argv[1], NULL, 16);

	val = dp52_direct_read(reg);

	printf("0x%x\n", val);

	return 0;
}

U_BOOT_CMD(dp_read, 2, 1, do_dp_read, "read register from DP52",
	"<reg>\n"
	"    - read direct register 'reg'"
);

