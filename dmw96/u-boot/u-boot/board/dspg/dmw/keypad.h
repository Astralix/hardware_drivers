#ifndef __KEYPAD_H

void dmw_keypad_setup(void);
void dmw_keypad_late_init(void);
void dmw_keypad_debug(void);
int  dmw_keypad_scan(int row, int col);

#endif
