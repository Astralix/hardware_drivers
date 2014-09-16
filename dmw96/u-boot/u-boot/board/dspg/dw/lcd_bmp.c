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

#include <common.h>
#include <asm/io.h>

//
// BMP File header
//
#define BMP_B_OFFSET			0		// byte
#define BMP_M_OFFSET			1		// byte
#define BMP_FILESIZE_OFFSET		2		// 4 bytes
#define BMP_RESERVED_OFFSET		6		// 4 bytes reserved
#define BMP_STARTIMAGE_OFFSET		10		// 4 bytes

//
// BMP Info header (located right after the file header)
//
#define BMP_SIZEOFINFOHEADER_OFFSET	14		// 4 bytes (must be 40)
#define BMP_HORIZWIDTH_OFFSET		18		// 4 bytes
#define BMP_VERTWIDTH_OFFSET		22		// 4 bytes
#define BMP_NUMPLANES_OFFSET		26		// 2 bytes
#define BMP_BPP_OFFSET			28		// 2 bytes
#define BMP_COMPRESSION_OFFSET		30		// 4 bytes
#define BMP_SIZEOFPIC_OFFSET		34		// 4 bytes
#define BMP_HORIZRES_OFFSET		38		// 4 bytes
#define BMP_VERTORIZ_OFFSET		42		// 4 bytes
#define BMP_NUMUSEDCOL_OFFSET		46		// 4 bytes
#define BMP_NUMIMPCOL_OFFSET		50		// 4 bytes

//
// Values of several fields
//
#define BMP_SIZEOFINFOHEADER		40
#define BMP_COMPRESSION_NONE		0
#define BMP_COMPRESSION_RLE8		1
#define BMP_COMPRESSION_RLE4		2
#define BMP_PALETTE_SIZE_BYTES		1024		// 256 colors, 4 bytes each (B,G,R,alpha)
#define BMP_FILEHEADER_SIZE		54		// All of the header (including the BMP info header)

static unsigned short	rgb565_palette[256];

static inline unsigned short bgr888_to_rgb565( unsigned char *pbgr888 )
{
	unsigned short rgb565;

	rgb565 = (pbgr888[2] & 0xf8) << 8;	// R
	rgb565 |= (pbgr888[1] & 0xfc) << 3;	// G
	rgb565 |= (pbgr888[0] & 0xf8) >> 3;	// B

	return rgb565;
}

static unsigned int read_int_unaligned( unsigned char *p )
{
	unsigned int retval;

	// The BMP file is using little-endian, so the next code is OK
	memcpy( &retval, p, sizeof(retval) );

	return retval;
}

static unsigned short read_short_unaligned( unsigned char *p )
{
	unsigned short retval;

	// The BMP file is using little-endian, so the next code is OK
	memcpy( &retval, p, sizeof(retval) );

	return retval;
}

static int abs( int num )
{
	return num > 0 ? num : -num;
}

unsigned int lcd_decode_bmp( unsigned short *dst, unsigned char* (*read_data)(int size))
{
	unsigned char* bmp_data;
	unsigned char *palette;			// R,G,B,alpha
	unsigned char repeat;
	unsigned char palette_index;
	unsigned short rgb565val;
	unsigned int i;
	unsigned int start_palette_index;
	unsigned int bmp_width;	
	int bmp_height;
	

	// Read the header from the file
	bmp_data = read_data(BMP_FILEHEADER_SIZE);

	// Parse a little the header of the BMP file
	// Currently only RLE8 format is supported
	if (	bmp_data[BMP_B_OFFSET] != 'B' || bmp_data[BMP_M_OFFSET] != 'M'				||
		read_int_unaligned(bmp_data + BMP_SIZEOFINFOHEADER_OFFSET) != BMP_SIZEOFINFOHEADER		||
		read_short_unaligned(bmp_data + BMP_BPP_OFFSET) != 8					||
		read_int_unaligned(bmp_data + BMP_COMPRESSION_OFFSET) != BMP_COMPRESSION_RLE8		)
	{
		printf("Wrong BMP file!\n");
		return 0;
	}
	
	// Read width and height of the picture
	bmp_width = read_int_unaligned(bmp_data + BMP_HORIZWIDTH_OFFSET);
	bmp_height = (int)read_int_unaligned(bmp_data + BMP_VERTWIDTH_OFFSET);

	printf("BMP Width = %d\nBMP Height = %d\n", bmp_width, bmp_height); 

	start_palette_index = read_int_unaligned(bmp_data + BMP_STARTIMAGE_OFFSET) - BMP_PALETTE_SIZE_BYTES;

	// Read till the beginning of the palette (this data is not in use)
	read_data(start_palette_index - BMP_FILEHEADER_SIZE);

	// Read palette information
	palette = read_data( BMP_PALETTE_SIZE_BYTES );

	// Load palette into memory and convert to rgb565	
	for (i=0 ; i<256 ; i++) {
		rgb565_palette[i] = bgr888_to_rgb565( palette + (i * 4) );
	}

	// if bmp_height > 0, then order of lines is bottom-up		(this is the common case of BMP files...)
	// if bmp_height < 0, then order of lines is up-bottom 
	if ( bmp_height > 0 ) {
		// Place dst to start of last line (-1 is in order to be at the beginning of last line)
		dst += (bmp_height - 1)*bmp_width;
	}


	while (1) {
		// The stream contains
		// (x!=0,y)					put pixel palette[y] x times
		// (x=0, y>2, v1, v2...)	next y pixels are uncompressed
		// (0,0)					end of line
		// (0,1)					end of file
		// (0,2,delta_x,delt_y)		jump delta_x pixels and delta_y lines	(not supported)

		bmp_data = read_data(2);
		if (bmp_data[0] != 0) {
			palette_index = bmp_data[1];
			rgb565val = rgb565_palette[palette_index];

			repeat = bmp_data[0];
			for (i=0; i<repeat; i++) {
				*dst++ = rgb565val;
			}
		}
		else {
			repeat = bmp_data[1];

			// Special cases (used for alignment and "end marker") 
			if (repeat == 0) {
				// (0,0) = End of line
				if ( bmp_height > 0 ) {
					// If height > 0, we have to go back 2 lines (since we are at the end of current line)
					dst -= 2 * bmp_width;
				}

				continue;
			}
			else if (repeat == 1) {
				// (0,1) = End of file - exit main loop
				break;
			}
			else if (repeat == 2) {
				// Jump to new location - not implemented!!! - exit main loop
				// delta_x = *p++;
				// delta_y = *p++;				
				break;
			}

			bmp_data = read_data(repeat);
			for (i=0; i<repeat; i++) {
				palette_index = *bmp_data++;
				*dst++ = rgb565_palette[palette_index];
			}

			// Sequence should always be aligned to 2 bytes
			if ( repeat % 2 )
				read_data(1);
		}
	}

    // Return number of pixels at the picture
    return abs(bmp_height * (int)bmp_width);
}

