#include <common.h>
#include <stdio_dev.h>
#include <asm/io.h>
#include "dw.h"
#include "gpio.h"

#define KPINT_CFG	0x5c
#define KPINT_ROW	0x60
#define KPINT_COL	0x64

#define KPINT_ROW_MASK	0x0000001F
#define KPINT_COL_MASK	0x0000003F

#define DEVNAME		"keypad"

#define KB_DISCHARGE_DELAY 20
#define KB_ACTIVATE_DELAY 20

#ifdef CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE
extern int overwrite_console (void);
#define OVERWRITE_CONSOLE overwrite_console ()
#else
#define OVERWRITE_CONSOLE 0
#endif /* CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE */

extern int dw_board;

static int dw_keypad_rows = 0;
static int dw_keypad_cols = 0;

static int debug_on = 0;

int dw_keypad_scan(int row, int col)
{
	int row_mask;

	if ((row < 0) || (row >= dw_keypad_rows) ||
	    (col < 0) || (col >= dw_keypad_cols))
		return 0;

	/* Discharge the output driver capacitance in the keypad matrix. */
	writel(0, DW_GPIO_BASE + KPINT_COL);
	udelay(KB_DISCHARGE_DELAY);

	writel(1<<col, DW_GPIO_BASE + KPINT_COL);
	udelay(KB_ACTIVATE_DELAY);

	row_mask = readl(DW_GPIO_BASE + KPINT_ROW) & ((1<<dw_keypad_cols)-1);

	if (row_mask & (1<<row))
		return 1;

	return 0;
}

void dw_keypad_debug(void)
{
	int c, r, row_mask;

	if (debug_on == 1) {
		for (c = 0; c < dw_keypad_cols; c++) {
			/* Discharge the output driver capacitance in the keypad matrix. */
			writel(0, DW_GPIO_BASE + KPINT_COL);
			udelay(KB_DISCHARGE_DELAY);

			writel(1<<c, DW_GPIO_BASE + KPINT_COL); /* |= */
			udelay(KB_ACTIVATE_DELAY);

			row_mask = readl(DW_GPIO_BASE + KPINT_ROW) & ((1<<dw_keypad_cols)-1);

			for (r = 0; r < dw_keypad_rows; r++) {
				if (row_mask & (1<<r))
					printf("row %d col %d\n", r, c);
			}
		}
	}
}

static int dw_keypad_tstc(void)
{
	unsigned int colums = 0;
	unsigned int rows = 0;

	colums = readl(DW_GPIO_BASE + KPINT_COL);

	/* disable/enable all colums */
	writel(colums & ~(KPINT_COL_MASK), DW_GPIO_BASE + KPINT_COL);
	udelay(KB_DISCHARGE_DELAY);
	writel(colums | KPINT_COL_MASK, DW_GPIO_BASE + KPINT_COL);
	udelay(KB_ACTIVATE_DELAY);

	/* read rows and check for active rows */
	rows = readl(DW_GPIO_BASE + KPINT_ROW) & KPINT_ROW_MASK;
	return (rows == 0 ? 0 : 1);
}

static int dw_keypad_getc(void)
{
	return 0;
}

void dw_keypad_setup(void)
{
	if ((dw_board == DW_BOARD_EXPEDIBLUE) ||
	    (dw_board == DW_BOARD_EXPEDIWAU_BASIC)) {
		dw_keypad_rows = 5;
		dw_keypad_cols = 5;
	} else if (dw_board == DW_BOARD_IMH) {
		dw_keypad_rows = 2;
		dw_keypad_cols = 5;
	}

	if ((dw_board == DW_BOARD_EXPEDIBLUE) ||
	    (dw_board == DW_BOARD_EXPEDIWAU_BASIC) ||
	    (dw_board == DW_BOARD_IMH)) {
		gpio_disable(BGPIO(5));  /* KCOL0 */
		gpio_disable(CGPIO(25)); /* KCOL1 */
		gpio_disable(CGPIO(26)); /* KCOL2 */
		gpio_disable(BGPIO(0));  /* KCOL3 */
		gpio_disable(BGPIO(1));  /* KCOL4 */

		gpio_disable(CGPIO(19)); /* KROW0 */
		gpio_disable(CGPIO(20)); /* KROW1 */
		gpio_disable(CGPIO(21)); /* KROW2 */
		gpio_disable(CGPIO(22)); /* KROW3 */
		gpio_disable(CGPIO(23)); /* KROW4 */

		writel(0x1f1f, DW_GPIO_BASE + KPINT_CFG);
		writel(0x1f1f, DW_GPIO_BASE + KPINT_COL);
	}
}

void dw_keypad_late_init(void)
{
	char *tmp = NULL;
	int val = 0;
	struct stdio_dev keypaddev;
	char *stdinname = getenv ("stdin");

	/* register as input device */
	memset (&keypaddev, 0, sizeof(keypaddev));
	strcpy(keypaddev.name, DEVNAME);
	keypaddev.flags =  DEV_FLAGS_INPUT | DEV_FLAGS_SYSTEM;
	keypaddev.putc = NULL;
	keypaddev.puts = NULL;
	keypaddev.getc = dw_keypad_getc;
	keypaddev.tstc = dw_keypad_tstc;

	val = stdio_register (&keypaddev);
	if (val == 0) {
		/* check if this is the standard input device */
		if(strcmp(stdinname,DEVNAME) == 0) {
			/* reassign the console */
			if(OVERWRITE_CONSOLE == 0) {
				val = console_assign(stdin, DEVNAME);
				if (val != 0) {
					/* XXX */
				}
			}
		}
	}

	tmp = getenv("keypad_debug_on");
	debug_on = tmp ? simple_strtoul(tmp, NULL, 10) : 0;
}
