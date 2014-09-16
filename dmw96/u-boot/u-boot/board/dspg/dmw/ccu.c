/*
 *  Copyright (C) 2011 DSPG Technologies GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <asm/io.h>
#include <linux/ccu.h>

#include "dmw.h"

/*
 * Adjust this to the memory controller FIFO depth (in words) to get best
 * performance.
 */
#define FIFO_DEPTH      16

#define CCU_CONT               0x000
#define CCU_CONT_MODE_RaW      (0ul << 17)
#define CCU_CONT_MODE_WriteN   (1ul << 17)
#define CCU_CONT_SWEN          (1ul << 16)
#define CCU_STAT               0x004
#define CCU_STAT_STATE_MASK    (3ul << 30)
#define CCU_STAT_STATE_IDLE    (0ul << 30)
#define CCU_STAT_STATE_WAIT    (1ul << 30)
#define CCU_STAT_STATE_BUSY    (2ul << 30)
#define CCU_STAT_ERROR         (1ul << 29)
#define CCU_STAT_SW_REQ        (1ul << 16)
#define CCU_REQ                0x008
#define CCU_SWTRIG             0x00C
#define CCU_SWTRIG_MAGIC       0x90EE
#define CCU_DUMADR             0x010
#define CCU_DUMDAT             0x014
#define CCU_RDAT               0x018

static uint32_t ccu_victim[FIFO_DEPTH] __attribute__ ((aligned(64)));

/*
 * Initiate a CCU transaction to guarantee coherency wrt. writes of other bus
 * masters. Might be called recursively from different threads.
 */
void ccu_barrier(void)
{
	/* wait for CCU to be idle */
	while (readl(DMW_CCU0_BASE + CCU_STAT) & CCU_STAT_SW_REQ)
		; /* wait */

	/* initiate coherency transaction */
	writel(CCU_SWTRIG_MAGIC, DMW_CCU0_BASE + CCU_SWTRIG);

	/* wait for CCU to finish request */
	while (readl(DMW_CCU0_BASE + CCU_STAT) & CCU_STAT_SW_REQ)
		; /* wait */
}

void ccu_init(void)
{
	/* Just enable software requests with 'N Writes' mode */
	writel(((FIFO_DEPTH-1) << 18) | CCU_CONT_MODE_WriteN | CCU_CONT_SWEN,
		DMW_CCU0_BASE + CCU_CONT);
	writel((unsigned long)(&ccu_victim), DMW_CCU0_BASE + CCU_DUMADR);
	writel(0xBADC0FFE, DMW_CCU0_BASE + CCU_DUMDAT);
}

