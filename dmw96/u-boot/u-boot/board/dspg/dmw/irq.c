#include <asm/byteorder.h>
#include <asm/io.h>
#include <common.h>
#include <linux/ctype.h>
#include "irq.h"
#include <asm/arch/gpio.h>

void dmw_irq_ack(unsigned int irq)
{
	unsigned long reg_clr;
	
	if (irq > 31) {
		irq -= 32;
		reg_clr = DMW_PLICU_CH_CLR_INT2;
	} else {
		reg_clr = DMW_PLICU_CH_CLR_INT1;
	}

	/* clear interrupt */
	writel(1 << irq, DMW_PLICU_BASE + reg_clr);
}

void dmw_irq_mask(unsigned int irq)
{
	unsigned long reg_mask;
	
	/* Beware the inverted semantic of 'masking' in the PLICU */
	if (irq > 31) {
		irq -= 32;
		reg_mask = DMW_PLICU_CLR_CH_MASK2;
	} else {
		reg_mask = DMW_PLICU_CLR_CH_MASK1;
	}

	writel(1 << irq, DMW_PLICU_BASE + reg_mask);
}

void dmw_irq_unmask(unsigned int irq)
{
	unsigned long reg_mask;
	
	/* Beware the inverted semantic of 'masking' in the PLICU */
	if (irq > 31) {
		irq -= 32;
		reg_mask = DMW_PLICU_SET_CH_MASK2;
	} else {
		reg_mask = DMW_PLICU_SET_CH_MASK1;
	}

	writel(1 << irq, DMW_PLICU_BASE + reg_mask);
}

unsigned int dmw_is_irq_pending(unsigned int irq)
{
	unsigned long reg_mask;
	
	/* Beware the inverted semantic of 'masking' in the PLICU */
	if (irq > 31) {
		irq -= 32;
		reg_mask = DMW_PLICU_PENDING_REQ2;
	} else {
		reg_mask = DMW_PLICU_PENDING_REQ1;
	}

	return (readl(DMW_PLICU_BASE + reg_mask) & (1 << irq));
}

int irq_init(void)
{
	unsigned int i;
	/* disable all interrupt sources */
	writel(0xffffffff, DMW_PLICU_BASE + DMW_PLICU_CLR_CH_MASK1);
	writel(0xffffffff, DMW_PLICU_BASE + DMW_PLICU_CLR_CH_MASK2);

	/* set priorities of all interrupts to 1 */
	for (i = 0; i < 8; i++)
		writel(0x11111111, DMW_PLICU_BASE + DMW_PLICU_CH_PRIORITY1 + i*4);

	/* set all sw interrupts to edge trigger */
	writel(0 | (1 << DMW_IID_SW1) | (1 << DMW_IID_SW2),
	       DMW_PLICU_BASE + DMW_PLICU_CH_EDGE_LEVEL1);
	writel(0 | (1 << (DMW_IID_SW3 - 32)) | (1 << (DMW_IID_SW4 - 32)),
	       DMW_PLICU_BASE + DMW_PLICU_CH_EDGE_LEVEL2);

	return 0;
}

unsigned extint_to_gpio(int extint)
{
	return FGPIO(extint + 16);
}

unsigned extint_to_idd(int extint)
{
	if (extint >= 0 && extint <= 6) {

		return (DMW_IID_EXTERNAL_REQUEST_0 + extint);
	}
	else if (extint >= 7 && extint <= 15) {

		return (DMW_IID_EXTERNAL_REQUEST_7 + extint);
	}
}
