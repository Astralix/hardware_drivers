#ifndef __LEGACY_NAND_H
#define __LEGACY_NAND_H

int legacy_nand_read(void *buf, unsigned long off, unsigned long size);
void legacy_nand_clkchg(int clk, unsigned long freq);

#endif
