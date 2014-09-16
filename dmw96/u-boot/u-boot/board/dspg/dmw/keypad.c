#include <common.h>
#include <stdio_dev.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

#include "dmw.h"

#define DMW_KBD_CFG 0x2B0
#define DMW_KBD_ROW 0x2B4
#define DMW_KBD_COL 0x2B8

#define DEVNAME "keypad"

#define DMW_KEYPAD_DISCHARGE_DELAY 20 /* us */
#define DMW_KEYPAD_ACTIVATE_DELAY  20 /* us */

#ifdef CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE
extern int overwrite_console (void);
#define OVERWRITE_CONSOLE overwrite_console ()
#else
#define OVERWRITE_CONSOLE 0
#endif /* CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE */

static int dmw_keypad_rows = 0;
static int dmw_keypad_cols = 0;

static int debug_on = 0;

static inline void
dmw_keypad_discharge_all(void)
{
	/* STROBE All HiZ */
	writel(0, DMW_GPIO_BASE + DMW_KBD_COL);
	udelay(DMW_KEYPAD_DISCHARGE_DELAY);
}

static inline void
dmw_keypad_activate_col(int col)
{
	unsigned long val;

	dmw_keypad_discharge_all();

	/* STROBE col -> High, not col -> HiZ */
	val = readl(DMW_GPIO_BASE + DMW_KBD_COL);
	val |= (1 << col);
	writel(val, DMW_GPIO_BASE + DMW_KBD_COL);
	udelay(DMW_KEYPAD_ACTIVATE_DELAY);
}

static inline void
dmw_keypad_activate_all(void)
{
	dmw_keypad_discharge_all();

	/* STROBE ALL -> High */
	writel((1 << dmw_keypad_cols) - 1, DMW_GPIO_BASE + DMW_KBD_COL);
	udelay(DMW_KEYPAD_ACTIVATE_DELAY);
}

static int
dmw_keypad_key_pressed(void)
{
	return (readl(DMW_GPIO_BASE + DMW_KBD_ROW) &
	       ((1 << dmw_keypad_rows) - 1));
}

int dmw_keypad_scan(int row, int col)
{
	if ((row < 0) || (row >= dmw_keypad_rows) ||
	    (col < 0) || (col >= dmw_keypad_cols))
		return 0;

	/* Discharge the output driver capacitance in the keypad matrix. */
	dmw_keypad_activate_col(col);

	if (dmw_keypad_key_pressed() & (1<<row))
		return 1;

	return 0;
}

void dmw_keypad_debug(void)
{
	int c, r, row_mask;

	if (!debug_on)
		return;

	for (c = 0; c < dmw_keypad_cols; c++) {
		dmw_keypad_discharge_all();

		/* Discharge the output driver capacitance in the keypad matrix. */
		dmw_keypad_activate_col(c);

		row_mask = dmw_keypad_key_pressed();

		for (r = 0; r < dmw_keypad_rows; r++) {
			if (row_mask & (1 << r))
				printf("row %d col %d\n", r, c);
		}
	}
}

static int dmw_keypad_tstc(void)
{
	/* enable all colums */
	dmw_keypad_activate_all();

	/* check for any active rows */
	return !!dmw_keypad_key_pressed();
}

static int dmw_keypad_getc(void)
{
	return 0;
}

static void dmw_keypad_setup_row(int gpio)
{
	gpio_disable(gpio);
	gpio_pull_ctrl(gpio, 1, 1);
}

static void dmw_keypad_setup_col(int gpio)
{
	gpio_disable(gpio);
}

void dmw_keypad_setup(void)
{
#if defined(DMW_BOARD_EVB) || defined(DMW_BOARD_TFEVB)
	dmw_keypad_rows = 6;
	dmw_keypad_cols = 5;

	dmw_keypad_setup_row(FGPIO(0));  /* KROW0 */
	dmw_keypad_setup_row(FGPIO(1));  /* KROW1 */
	dmw_keypad_setup_row(FGPIO(2));  /* KROW2 */
	dmw_keypad_setup_row(FGPIO(3));  /* KROW3 */
	dmw_keypad_setup_row(FGPIO(4));  /* KROW4 */
	dmw_keypad_setup_row(FGPIO(5));  /* KROW5 */
	
	dmw_keypad_setup_col(FGPIO(8));  /* KCOL0 */
	dmw_keypad_setup_col(FGPIO(9));  /* KCOL1 */
	dmw_keypad_setup_col(FGPIO(10)); /* KCOL2 */
	dmw_keypad_setup_col(FGPIO(11)); /* KCOL3 */
	dmw_keypad_setup_col(FGPIO(12)); /* KCOL4 */
#endif

#if defined(DMW_BOARD_IMH3)
	dmw_keypad_rows = 4;
	dmw_keypad_cols = 2;

	dmw_keypad_setup_row(FGPIO(0));  /* KROW0 */
	dmw_keypad_setup_row(FGPIO(1));  /* KROW1 */
	dmw_keypad_setup_row(FGPIO(2));  /* KROW2 */
	dmw_keypad_setup_row(FGPIO(3));  /* KROW3 */

	dmw_keypad_setup_col(FGPIO(8));  /* KCOL0 */
	dmw_keypad_setup_col(FGPIO(9));  /* KCOL1 */
#endif

	writel( ((1 << dmw_keypad_cols) - 1) | (((1 << dmw_keypad_rows) - 1) << 8),
	       DMW_GPIO_BASE + DMW_KBD_CFG);

	writel( (1 << dmw_keypad_cols) - 1, DMW_GPIO_BASE + DMW_KBD_COL);
}

void dmw_keypad_late_init(void)
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
	keypaddev.getc = dmw_keypad_getc;
	keypaddev.tstc = dmw_keypad_tstc;

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
