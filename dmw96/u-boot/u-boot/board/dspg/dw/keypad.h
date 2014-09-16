#ifndef __KEYPAD_H

void dw_keypad_setup(void);
void dw_keypad_late_init(void);
void dw_keypad_debug(void);
int dw_keypad_scan(int row, int col);

#endif
