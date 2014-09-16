/*
 * (C) Copyright 2012
 * DSPG Technologies GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdlib.h>

static unsigned int rand_state = 123;

int rand_r(unsigned int *seedp)
{
	*seedp = 1103515245 * *seedp + 12345;
	return *seedp & 0x7ffffffful;
}

int rand(void)
{
	return rand_r(&rand_state);
}

void srand(unsigned int seed)
{
	rand_state = rand_r(&seed);
}

