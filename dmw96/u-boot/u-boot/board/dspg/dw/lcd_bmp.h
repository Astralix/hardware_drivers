/*
 * (C) Copyright 2009
 * Shlomi Mor, DSP Group, shlomi.mor@dspg.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

//
// Decode BMP image
// Parameteres:
//	dst		- Destination buffer (array of rgb565 pixels)
//	read_data	- Function pointer which is used in order to read data from the BMP file.
//			  The function should ALWAYS retutn a pointer to the data at the requested size.
//			  It is guaranteed that requested size is not bigger than 2048 bytes.
//			  The prototype is not like "fread" in order to get "zero copy" functionality (for perfromances)
// Returned value:
//	Number of pixels decoded (0 in case of error)
//
unsigned int lcd_decode_bmp( unsigned short *dst, unsigned char* (*read_data)(int size));
