/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/




#include "gc_vg_precomp.h"
#include <string.h>

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

#if gcdENABLE_PERFORMANCE_PRIOR
static const gctFIXED_POINT sinx_table[1024] =
{
	0x000000,0x000097,0x0000fb,0x000160,0x0001c4,0x000229,0x00028d,0x0002f2,
	0x000356,0x0003bb,0x000420,0x000484,0x0004e9,0x00054d,0x0005b2,0x000616,
	0x00067b,0x0006df,0x000744,0x0007a8,0x00080d,0x000871,0x0008d5,0x00093a,
	0x00099e,0x000a03,0x000a67,0x000acc,0x000b30,0x000b95,0x000bf9,0x000c5d,
	0x000cc2,0x000d26,0x000d8b,0x000def,0x000e53,0x000eb8,0x000f1c,0x000f81,
	0x000fe5,0x001049,0x0010ae,0x001112,0x001176,0x0011da,0x00123f,0x0012a3,
	0x001307,0x00136c,0x0013d0,0x001434,0x001498,0x0014fc,0x001561,0x0015c5,
	0x001629,0x00168d,0x0016f1,0x001755,0x0017b9,0x00181d,0x001882,0x0018e6,
	0x00194a,0x0019ae,0x001a12,0x001a76,0x001ada,0x001b3e,0x001ba2,0x001c06,
	0x001c69,0x001ccd,0x001d31,0x001d95,0x001df9,0x001e5d,0x001ec1,0x001f24,
	0x001f88,0x001fec,0x002050,0x0020b3,0x002117,0x00217b,0x0021de,0x002242,
	0x0022a6,0x002309,0x00236d,0x0023d0,0x002434,0x002497,0x0024fb,0x00255e,
	0x0025c2,0x002625,0x002689,0x0026ec,0x00274f,0x0027b3,0x002816,0x002879,
	0x0028dd,0x002940,0x0029a3,0x002a06,0x002a69,0x002acc,0x002b30,0x002b93,
	0x002bf6,0x002c59,0x002cbc,0x002d1f,0x002d82,0x002de5,0x002e47,0x002eaa,
	0x002f0d,0x002f70,0x002fd3,0x003035,0x003098,0x0030fb,0x00315e,0x0031c0,
	0x003223,0x003285,0x0032e8,0x00334a,0x0033ad,0x00340f,0x003472,0x0034d4,
	0x003536,0x003599,0x0035fb,0x00365d,0x0036c0,0x003722,0x003784,0x0037e6,
	0x003848,0x0038aa,0x00390c,0x00396e,0x0039d0,0x003a32,0x003a94,0x003af6,
	0x003b58,0x003bb9,0x003c1b,0x003c7d,0x003cde,0x003d40,0x003da2,0x003e03,
	0x003e65,0x003ec6,0x003f28,0x003f89,0x003fea,0x00404c,0x0040ad,0x00410e,
	0x00416f,0x0041d1,0x004232,0x004293,0x0042f4,0x004355,0x0043b6,0x004417,
	0x004478,0x0044d9,0x004539,0x00459a,0x0045fb,0x00465c,0x0046bc,0x00471d,
	0x00477d,0x0047de,0x00483e,0x00489f,0x0048ff,0x00495f,0x0049c0,0x004a20,
	0x004a80,0x004ae0,0x004b40,0x004ba1,0x004c01,0x004c61,0x004cc0,0x004d20,
	0x004d80,0x004de0,0x004e40,0x004e9f,0x004eff,0x004f5f,0x004fbe,0x00501e,
	0x00507d,0x0050dd,0x00513c,0x00519b,0x0051fb,0x00525a,0x0052b9,0x005318,
	0x005377,0x0053d6,0x005435,0x005494,0x0054f3,0x005552,0x0055b0,0x00560f,
	0x00566e,0x0056cc,0x00572b,0x005789,0x0057e8,0x005846,0x0058a5,0x005903,
	0x005961,0x0059bf,0x005a1d,0x005a7b,0x005ad9,0x005b37,0x005b95,0x005bf3,
	0x005c51,0x005caf,0x005d0c,0x005d6a,0x005dc8,0x005e25,0x005e83,0x005ee0,
	0x005f3d,0x005f9b,0x005ff8,0x006055,0x0060b2,0x00610f,0x00616c,0x0061c9,
	0x006226,0x006283,0x0062e0,0x00633c,0x006399,0x0063f5,0x006452,0x0064ae,
	0x00650b,0x006567,0x0065c3,0x006620,0x00667c,0x0066d8,0x006734,0x006790,
	0x0067ec,0x006848,0x0068a3,0x0068ff,0x00695b,0x0069b6,0x006a12,0x006a6d,
	0x006ac9,0x006b24,0x006b7f,0x006bdb,0x006c36,0x006c91,0x006cec,0x006d47,
	0x006da2,0x006dfc,0x006e57,0x006eb2,0x006f0d,0x006f67,0x006fc2,0x00701c,
	0x007076,0x0070d1,0x00712b,0x007185,0x0071df,0x007239,0x007293,0x0072ed,
	0x007347,0x0073a0,0x0073fa,0x007454,0x0074ad,0x007507,0x007560,0x0075b9,
	0x007612,0x00766c,0x0076c5,0x00771e,0x007777,0x0077d0,0x007828,0x007881,
	0x0078da,0x007932,0x00798b,0x0079e3,0x007a3c,0x007a94,0x007aec,0x007b44,
	0x007b9c,0x007bf4,0x007c4c,0x007ca4,0x007cfc,0x007d54,0x007dab,0x007e03,
	0x007e5a,0x007eb2,0x007f09,0x007f60,0x007fb7,0x00800f,0x008066,0x0080bc,
	0x008113,0x00816a,0x0081c1,0x008217,0x00826e,0x0082c4,0x00831b,0x008371,
	0x0083c7,0x00841d,0x008474,0x0084ca,0x00851f,0x008575,0x0085cb,0x008621,
	0x008676,0x0086cc,0x008721,0x008777,0x0087cc,0x008821,0x008876,0x0088cb,
	0x008920,0x008975,0x0089ca,0x008a1e,0x008a73,0x008ac7,0x008b1c,0x008b70,
	0x008bc5,0x008c19,0x008c6d,0x008cc1,0x008d15,0x008d69,0x008dbc,0x008e10,
	0x008e64,0x008eb7,0x008f0b,0x008f5e,0x008fb1,0x009004,0x009057,0x0090aa,
	0x0090fd,0x009150,0x0091a3,0x0091f5,0x009248,0x00929a,0x0092ed,0x00933f,
	0x009391,0x0093e3,0x009435,0x009487,0x0094d9,0x00952b,0x00957d,0x0095ce,
	0x009620,0x009671,0x0096c2,0x009713,0x009765,0x0097b6,0x009807,0x009857,
	0x0098a8,0x0098f9,0x009949,0x00999a,0x0099ea,0x009a3a,0x009a8b,0x009adb,
	0x009b2b,0x009b7b,0x009bca,0x009c1a,0x009c6a,0x009cb9,0x009d09,0x009d58,
	0x009da7,0x009df7,0x009e46,0x009e95,0x009ee3,0x009f32,0x009f81,0x009fd0,
	0x00a01e,0x00a06c,0x00a0bb,0x00a109,0x00a157,0x00a1a5,0x00a1f3,0x00a241,
	0x00a28e,0x00a2dc,0x00a32a,0x00a377,0x00a3c4,0x00a412,0x00a45f,0x00a4ac,
	0x00a4f9,0x00a545,0x00a592,0x00a5df,0x00a62b,0x00a678,0x00a6c4,0x00a710,
	0x00a75c,0x00a7a8,0x00a7f4,0x00a840,0x00a88c,0x00a8d7,0x00a923,0x00a96e,
	0x00a9ba,0x00aa05,0x00aa50,0x00aa9b,0x00aae6,0x00ab31,0x00ab7b,0x00abc6,
	0x00ac11,0x00ac5b,0x00aca5,0x00acef,0x00ad39,0x00ad83,0x00adcd,0x00ae17,
	0x00ae61,0x00aeaa,0x00aef4,0x00af3d,0x00af86,0x00afcf,0x00b018,0x00b061,
	0x00b0aa,0x00b0f3,0x00b13b,0x00b184,0x00b1cc,0x00b215,0x00b25d,0x00b2a5,
	0x00b2ed,0x00b335,0x00b37c,0x00b3c4,0x00b40b,0x00b453,0x00b49a,0x00b4e1,
	0x00b528,0x00b56f,0x00b5b6,0x00b5fd,0x00b644,0x00b68a,0x00b6d1,0x00b717,
	0x00b75d,0x00b7a3,0x00b7e9,0x00b82f,0x00b875,0x00b8bb,0x00b900,0x00b946,
	0x00b98b,0x00b9d0,0x00ba15,0x00ba5a,0x00ba9f,0x00bae4,0x00bb28,0x00bb6d,
	0x00bbb1,0x00bbf6,0x00bc3a,0x00bc7e,0x00bcc2,0x00bd06,0x00bd4a,0x00bd8d,
	0x00bdd1,0x00be14,0x00be57,0x00be9b,0x00bede,0x00bf21,0x00bf63,0x00bfa6,
	0x00bfe9,0x00c02b,0x00c06e,0x00c0b0,0x00c0f2,0x00c134,0x00c176,0x00c1b8,
	0x00c1f9,0x00c23b,0x00c27c,0x00c2be,0x00c2ff,0x00c340,0x00c381,0x00c3c2,
	0x00c402,0x00c443,0x00c483,0x00c4c4,0x00c504,0x00c544,0x00c584,0x00c5c4,
	0x00c604,0x00c644,0x00c683,0x00c6c2,0x00c702,0x00c741,0x00c780,0x00c7bf,
	0x00c7fe,0x00c83c,0x00c87b,0x00c8ba,0x00c8f8,0x00c936,0x00c974,0x00c9b2,
	0x00c9f0,0x00ca2e,0x00ca6b,0x00caa9,0x00cae6,0x00cb23,0x00cb61,0x00cb9e,
	0x00cbda,0x00cc17,0x00cc54,0x00cc90,0x00cccd,0x00cd09,0x00cd45,0x00cd81,
	0x00cdbd,0x00cdf9,0x00ce34,0x00ce70,0x00ceab,0x00cee7,0x00cf22,0x00cf5d,
	0x00cf98,0x00cfd2,0x00d00d,0x00d047,0x00d082,0x00d0bc,0x00d0f6,0x00d130,
	0x00d16a,0x00d1a4,0x00d1de,0x00d217,0x00d250,0x00d28a,0x00d2c3,0x00d2fc,
	0x00d335,0x00d36d,0x00d3a6,0x00d3df,0x00d417,0x00d44f,0x00d487,0x00d4bf,
	0x00d4f7,0x00d52f,0x00d566,0x00d59e,0x00d5d5,0x00d60c,0x00d644,0x00d67a,
	0x00d6b1,0x00d6e8,0x00d71f,0x00d755,0x00d78b,0x00d7c1,0x00d7f8,0x00d82d,
	0x00d863,0x00d899,0x00d8ce,0x00d904,0x00d939,0x00d96e,0x00d9a3,0x00d9d8,
	0x00da0d,0x00da41,0x00da76,0x00daaa,0x00dade,0x00db12,0x00db46,0x00db7a,
	0x00dbae,0x00dbe1,0x00dc15,0x00dc48,0x00dc7b,0x00dcae,0x00dce1,0x00dd14,
	0x00dd47,0x00dd79,0x00ddab,0x00ddde,0x00de10,0x00de42,0x00de74,0x00dea5,
	0x00ded7,0x00df08,0x00df39,0x00df6b,0x00df9c,0x00dfcd,0x00dffd,0x00e02e,
	0x00e05e,0x00e08f,0x00e0bf,0x00e0ef,0x00e11f,0x00e14f,0x00e17e,0x00e1ae,
	0x00e1dd,0x00e20d,0x00e23c,0x00e26b,0x00e299,0x00e2c8,0x00e2f7,0x00e325,
	0x00e353,0x00e382,0x00e3b0,0x00e3de,0x00e40b,0x00e439,0x00e466,0x00e494,
	0x00e4c1,0x00e4ee,0x00e51b,0x00e548,0x00e574,0x00e5a1,0x00e5cd,0x00e5f9,
	0x00e626,0x00e652,0x00e67d,0x00e6a9,0x00e6d5,0x00e700,0x00e72b,0x00e756,
	0x00e781,0x00e7ac,0x00e7d7,0x00e801,0x00e82c,0x00e856,0x00e880,0x00e8aa,
	0x00e8d4,0x00e8fe,0x00e927,0x00e951,0x00e97a,0x00e9a3,0x00e9cc,0x00e9f5,
	0x00ea1e,0x00ea47,0x00ea6f,0x00ea97,0x00eac0,0x00eae8,0x00eb0f,0x00eb37,
	0x00eb5f,0x00eb86,0x00ebae,0x00ebd5,0x00ebfc,0x00ec23,0x00ec4a,0x00ec70,
	0x00ec97,0x00ecbd,0x00ece3,0x00ed09,0x00ed2f,0x00ed55,0x00ed7a,0x00eda0,
	0x00edc5,0x00edea,0x00ee0f,0x00ee34,0x00ee59,0x00ee7e,0x00eea2,0x00eec7,
	0x00eeeb,0x00ef0f,0x00ef33,0x00ef56,0x00ef7a,0x00ef9d,0x00efc1,0x00efe4,
	0x00f007,0x00f02a,0x00f04d,0x00f06f,0x00f092,0x00f0b4,0x00f0d6,0x00f0f8,
	0x00f11a,0x00f13c,0x00f15d,0x00f17f,0x00f1a0,0x00f1c1,0x00f1e2,0x00f203,
	0x00f224,0x00f244,0x00f265,0x00f285,0x00f2a5,0x00f2c5,0x00f2e5,0x00f304,
	0x00f324,0x00f343,0x00f363,0x00f382,0x00f3a1,0x00f3c0,0x00f3de,0x00f3fd,
	0x00f41b,0x00f439,0x00f457,0x00f475,0x00f493,0x00f4b1,0x00f4ce,0x00f4eb,
	0x00f509,0x00f526,0x00f543,0x00f55f,0x00f57c,0x00f598,0x00f5b5,0x00f5d1,
	0x00f5ed,0x00f609,0x00f624,0x00f640,0x00f65b,0x00f677,0x00f692,0x00f6ad,
	0x00f6c7,0x00f6e2,0x00f6fd,0x00f717,0x00f731,0x00f74b,0x00f765,0x00f77f,
	0x00f799,0x00f7b2,0x00f7cb,0x00f7e5,0x00f7fe,0x00f816,0x00f82f,0x00f848,
	0x00f860,0x00f878,0x00f891,0x00f8a9,0x00f8c0,0x00f8d8,0x00f8f0,0x00f907,
	0x00f91e,0x00f935,0x00f94c,0x00f963,0x00f97a,0x00f990,0x00f9a6,0x00f9bd,
	0x00f9d3,0x00f9e8,0x00f9fe,0x00fa14,0x00fa29,0x00fa3e,0x00fa54,0x00fa69,
	0x00fa7d,0x00fa92,0x00faa7,0x00fabb,0x00facf,0x00fae3,0x00faf7,0x00fb0b,
	0x00fb1f,0x00fb32,0x00fb45,0x00fb58,0x00fb6b,0x00fb7e,0x00fb91,0x00fba4,
	0x00fbb6,0x00fbc8,0x00fbda,0x00fbec,0x00fbfe,0x00fc10,0x00fc21,0x00fc33,
	0x00fc44,0x00fc55,0x00fc66,0x00fc76,0x00fc87,0x00fc97,0x00fca8,0x00fcb8,
	0x00fcc8,0x00fcd8,0x00fce7,0x00fcf7,0x00fd06,0x00fd15,0x00fd24,0x00fd33,
	0x00fd42,0x00fd51,0x00fd5f,0x00fd6d,0x00fd7c,0x00fd89,0x00fd97,0x00fda5,
	0x00fdb3,0x00fdc0,0x00fdcd,0x00fdda,0x00fde7,0x00fdf4,0x00fe01,0x00fe0d,
	0x00fe19,0x00fe25,0x00fe31,0x00fe3d,0x00fe49,0x00fe55,0x00fe60,0x00fe6b,
	0x00fe76,0x00fe81,0x00fe8c,0x00fe97,0x00fea1,0x00feab,0x00feb5,0x00febf,
	0x00fec9,0x00fed3,0x00fedd,0x00fee6,0x00feef,0x00fef8,0x00ff01,0x00ff0a,
	0x00ff13,0x00ff1b,0x00ff23,0x00ff2c,0x00ff34,0x00ff3b,0x00ff43,0x00ff4b,
	0x00ff52,0x00ff59,0x00ff60,0x00ff67,0x00ff6e,0x00ff75,0x00ff7b,0x00ff82,
	0x00ff88,0x00ff8e,0x00ff94,0x00ff99,0x00ff9f,0x00ffa4,0x00ffa9,0x00ffaf,
	0x00ffb4,0x00ffb8,0x00ffbd,0x00ffc1,0x00ffc6,0x00ffca,0x00ffce,0x00ffd2,
	0x00ffd5,0x00ffd9,0x00ffdc,0x00ffe0,0x00ffe3,0x00ffe6,0x00ffe8,0x00ffeb,
	0x00ffed,0x00fff0,0x00fff2,0x00fff4,0x00fff6,0x00fff7,0x00fff9,0x00fffa,
	0x00fffc,0x00fffd,0x00fffe,0x00fffe,0x00ffff,0x010000,0x010000,0x010000
};

gctFIXED_POINT sinx(
	 gctFIXED_POINT angle
	 )
{
	gctFIXED_POINT a;

	/* Keep adding 2 * pi until angle is positive. */
	for (a = angle; a < 0; a += vgvFIXED_PI_TWO) ;

	/* Divide angle by 2 * pi. */
	a = vgmFIXED_DIV(a, vgvFIXED_PI_TWO);

	/* Drop 4 bits of significance. */
	a >>= 4;

	/* Lookup sin value. */
	switch (a & 0xC00)
	{
	case 0x000:
	default:
		return sinx_table[a & 0x3FF];

	case 0x400:
		return sinx_table[0x3FF - (a & 0x3FF)];

	case 0x800:
		return -sinx_table[a & 0x3FF];

	case 0xC00:
		return -sinx_table[0x3FF - (a & 0x3FF)];
	}
}

gctFIXED_POINT cosx(
	 gctFIXED_POINT angle
	 )
{
	gctFIXED_POINT a;

	/* Keep adding 2 * pi until angle is positive. */
	for (a = angle; a < 0; a += vgvFIXED_PI_TWO) ;

	/* Divide angle by 2 * pi. */
	a = vgmFIXED_DIV(a, vgvFIXED_PI_TWO);

	/* Drop 4 bits of significance. */
	a >>= 4;

	/* Lookup cos value. */
	switch (a & 0xC00)
	{
	case 0xC00:
	default:
		return sinx_table[a & 0x3FF];

	case 0x000:
		return sinx_table[0x3FF - (a & 0x3FF)];

	case 0x400:
		return -sinx_table[a & 0x3FF];

	case 0x800:
		return -sinx_table[0x3FF - (a & 0x3FF)];
	}
}

#endif

/* Column-major identity matrix. */
static vgsMATRIX _identityMatrix =
{
	/* Matrix values. */
	{
#if gcdENABLE_PERFORMANCE_PRIOR
		vgvFIXED_ONE,
		vgvFIXED_ZERO,
		vgvFIXED_ZERO,

		vgvFIXED_ZERO,
		vgvFIXED_ONE,
		vgvFIXED_ZERO,

		vgvFIXED_ZERO,
		vgvFIXED_ZERO,
		vgvFIXED_ONE
#else
		/* First column. */
		1.0f,
		0.0f,
		0.0f,

		/* Second column. */
		0.0f,
		1.0f,
		0.0f,

		/* Third column. */
		0.0f,
		0.0f,
		1.0f
#endif
	},
	gcvTRUE,			/* Matrix values have changed. */

	/* Identity. */
	gcvTRUE,			/* Identity matrix. */
	gcvFALSE,			/* Not dirty. */

	/* Determinant. */
	1.0f,				/* Determinant = 1. */
	gcvFALSE			/* Not dirty. */
};



/*******************************************************************************
**
** _LoadIdentity
**
** Sets the specified matrix to the identity and invalidate the values.
**
** INPUT:
**
**    Container
**       Pointer to the matrix container.
**
** OUTPUT:
**
**    Nothing.
*/
static vgmINLINE void _LoadIdentity(
	IN vgsCONTEXT_PTR Context,
	IN OUT vgsMATRIXCONTAINER_PTR Container
	)
{
	/* Copy identity matrix. */
	gcmVERIFY_OK(gcoOS_MemCopy(
		vgmGETMATRIX(Container),
		&_identityMatrix,
		gcmSIZEOF(_identityMatrix)
		));

	/* Invalidate the matrix dependents. */
	Container->invalidate(Context);
}


/*******************************************************************************
**
** _InitializeContainer
**
** Initialize the specified VG matrix container.
**
** INPUT:
**
**    Matrix
**       Pointer to the matrix.
**
** OUTPUT:
**
**    Nothing.
*/

void _InitializeContainer(
	IN vgsCONTEXT_PTR Context,
	IN OUT vgsMATRIXCONTAINER_PTR Container,
	IN vgtMATRIXINVALIDATE InvalidateCallback,
	IN vgtMATRIXUPDATE UpdateCallback,
	IN gctBOOL LoadIdentity
	)
{
	/* Set the callbacks. */
	Container->invalidate = InvalidateCallback;
	Container->update     = UpdateCallback;

	/* Load identity to the matrix. */
	if (LoadIdentity)
	{
		_LoadIdentity(Context, Container);
	}
}


/*******************************************************************************
**
** _GetAffineMatrix
**
** Copy values from the specified matrix to the destination and force affinity.
**
** INPUT:
**
**    Matrix
**       Pointer to the source matrix.
**
** OUTPUT:
**
**    Result
**       Pointer to the result matrix.
*/

static void _GetAffineMatrix(
	IN const vgsMATRIX_PTR Matrix,
	OUT vgsMATRIX_PTR Result
	)
{
	/* First row. */
	vgmMAT(Result, 0, 0) = vgmMAT(Matrix, 0, 0);
	vgmMAT(Result, 0, 1) = vgmMAT(Matrix, 0, 1);
	vgmMAT(Result, 0, 2) = vgmMAT(Matrix, 0, 2);

	/* Second row. */
	vgmMAT(Result, 1, 0) = vgmMAT(Matrix, 1, 0);
	vgmMAT(Result, 1, 1) = vgmMAT(Matrix, 1, 1);
	vgmMAT(Result, 1, 2) = vgmMAT(Matrix, 1, 2);

	/* Third row. */
#if gcdENABLE_PERFORMANCE_PRIOR
	vgmMAT(Result, 2, 0) = vgvFIXED_ZERO;
	vgmMAT(Result, 2, 1) = vgvFIXED_ZERO;
	vgmMAT(Result, 2, 2) = vgvFIXED_ONE;
#else
	vgmMAT(Result, 2, 0) = 0.0f;
	vgmMAT(Result, 2, 1) = 0.0f;
	vgmMAT(Result, 2, 2) = 1.0f;
#endif
	/* Copy matrix flags. */
	Result->identity      = Matrix->identity;
	Result->identityDirty = Matrix->identityDirty;
	Result->detDirty      = gcvTRUE;
	Result->valid         = gcvTRUE;
}


/*******************************************************************************
**
** _SetUserToSurface
**
** Update HAL as necessary with the current user-to-surface transformation.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    UserToSurface
**       Pointer to the user-to-surface matrix.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _SetUserToSurface(
	IN vgsCONTEXT_PTR Context,
	IN vgsMATRIX_PTR UserToSurface
	)
{
	gceSTATUS status;

	do
	{
		/* Dump current user-to-surface. */
		vgmDUMP_MATRIX("USER-TO-SURFACE", UserToSurface);

		/* Did the matrix change? */
		if (UserToSurface->valuesDirty ||
			(Context->currentUserToSurface != UserToSurface))
		{
#if gcdENABLE_PERFORMANCE_PRIOR
			gctFLOAT m[9];
			vgmCONVERTMAT(gctFLOAT, gctFIXED, vgmFIXED_TO_FLOAT_SPECIAL, m, UserToSurface);
			/* Set the matrix in HAL. */
			gcmERR_BREAK(gcoVG_SetUserToSurface(
				Context->vg, m
				));
#else
			/* Set the matrix in HAL. */
			gcmERR_BREAK(gcoVG_SetUserToSurface(
				Context->vg, vgmGETMATRIXVALUES(UserToSurface)
				));
#endif
			/* Validate the matrix. */
			UserToSurface->valuesDirty = gcvFALSE;

			/* Set as current. */
			Context->currentUserToSurface = UserToSurface;
		}
		else
		{
			/* Success. */
			status = gcvSTATUS_OK;
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _UpdateSurfaceToPaintMatrix
**
** Updates the current specified surface-to-paint matrix as needed.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    UserToSurface
**       Pointer to the user-to-surface matrix.
**
**    PaintToUser
**       Pointer to the paint-to-user matrix.
**
**    Paint
**       Pointer to the corresponding VG paint object.
**
** OUTPUT:
**
**    SurfaceToPaint
**       Pointer to the surface-to-paint matrix to compute.
*/

static gceSTATUS _UpdateSurfaceToPaintMatrix(
	IN vgsCONTEXT_PTR Context,
	IN vgsMATRIXCONTAINER_PTR UserToSurface,
	IN vgsMATRIXCONTAINER_PTR PaintToUser,
	IN OUT vgsMATRIX_PTR SurfaceToPaint,
	IN vgsPAINT_PTR Paint
	)
{
	gceSTATUS status;

	do
	{
		gctBOOL doReload;

		/* Assume the matrix has been loaded already. */
		doReload = gcvFALSE;

		/* Update user-to-surface matrix. */
		gcmERR_BREAK(UserToSurface->update(Context));

		/* Does the matrix need to be recomputed? */
		if (SurfaceToPaint->valuesDirty)
		{
			vgsMATRIX affineUserToSurface;
			vgsMATRIX invertedSurfaceToPaint;

			/* Get the current affine user-to-surface matrix. */
			_GetAffineMatrix(
				vgmGETMATRIX(UserToSurface),
				&affineUserToSurface
				);

			/* Compute the inverted surface-to-paint matrix. */
			vgfMultiplyMatrix3x3(
				&affineUserToSurface,
				vgmGETMATRIX(PaintToUser),
				&invertedSurfaceToPaint
				);

			/* Compute the surface to paint matrix. */
			doReload = SurfaceToPaint->valid = vgfInvertMatrix(
				&invertedSurfaceToPaint,
				SurfaceToPaint
				);

			/* Notify the paint that the matrix has cahnged. */
			if (doReload)
			{
				vgfPaintMatrixChanged(Paint, SurfaceToPaint);
			}

			/* Validate. */
			SurfaceToPaint->valuesDirty = gcvFALSE;
		}

		/* Ensure that the matrix is current for the paint. */
		vgfPaintMatrixVerifyCurrent(Paint, SurfaceToPaint);
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _UpdateSurfaceToImageMatrix
**
** Updates the current specified surface-to-image matrix as needed.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    UserToSurface
**       Pointer to the user-to-surface matrix.
**
** OUTPUT:
**
**    SurfaceToImage
**       Pointer to the surface-to-image matrix to compute.
*/

static gceSTATUS _UpdateSurfaceToImageMatrix(
	IN vgsCONTEXT_PTR Context,
	IN vgsMATRIXCONTAINER_PTR UserToSurface,
	IN OUT vgsMATRIX_PTR SurfaceToImage
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		/* Update user-to-surface matrix. */
		gcmERR_BREAK(UserToSurface->update(Context));

		/* Does the matrix need to be recomputed? */
		if (SurfaceToImage->valuesDirty)
		{
			/* Compute the surface to paint matrix. */
			SurfaceToImage->valid = vgfInvertMatrix(
				vgmGETMATRIX(UserToSurface),
				SurfaceToImage
				);

			/* Set the matrix in HAL. */
			if (SurfaceToImage->valid)
			{
#if gcdENABLE_PERFORMANCE_PRIOR
				gctFLOAT m[9];
				vgmCONVERTMAT(gctFLOAT, gctFIXED, vgmFIXED_TO_FLOAT_SPECIAL, m, SurfaceToImage);

				status = gcoVG_SetSurfaceToImage(
					Context->vg, m
					);
#else
				status = gcoVG_SetSurfaceToImage(
					Context->vg, vgmGETMATRIXVALUES(SurfaceToImage)
					);
#endif
			}

			/* Validate. */
			SurfaceToImage->valuesDirty = gcvFALSE;
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** MATRIX CALLBACKS: image matrices.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    Nothing.
*/

/* imageFillSurfaceToPaint */
static void _InvalidateImageFillSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
}

static gceSTATUS _UpdateImageFillSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	return _UpdateSurfaceToPaintMatrix(
		Context,
		&Context->imageUserToSurface,
		&Context->fillPaintToUser,
		vgmGETMATRIX(&Context->imageFillSurfaceToPaint),
		Context->fillPaint
		);
}

/* glyphSurfaceToImage */
static void _InvalidateGlyphSurfaceToImage(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
}

static gceSTATUS _UpdateGlyphSurfaceToImage(
	IN vgsCONTEXT_PTR Context
	)
{
	return _UpdateSurfaceToImageMatrix(
		Context,
		&Context->translatedGlyphUserToSurface,
		vgmGETMATRIX(&Context->glyphSurfaceToImage)
		);
}

/* imageSurfaceToImage */
static void _InvalidateImageSurfaceToImage(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
}

static gceSTATUS _UpdateImageSurfaceToImage(
	IN vgsCONTEXT_PTR Context
	)
{
	return _UpdateSurfaceToImageMatrix(
		Context,
		&Context->imageUserToSurface,
		vgmGETMATRIX(&Context->imageSurfaceToImage)
		);
}


/*******************************************************************************
**
** MATRIX CALLBACKS: path matrices.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    Nothing.
*/

/* pathFillSurfaceToPaint */
static void _InvalidatePathFillSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
}

static gceSTATUS _UpdatePathFillSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	return _UpdateSurfaceToPaintMatrix(
		Context,
		&Context->pathUserToSurface,
		&Context->fillPaintToUser,
		vgmGETMATRIX(&Context->pathFillSurfaceToPaint),
		Context->fillPaint
		);
}

/* pathStrokeSurfaceToPaint */
static void _InvalidatePathStrokeSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
}

static gceSTATUS _UpdatePathStrokeSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	return _UpdateSurfaceToPaintMatrix(
		Context,
		&Context->pathUserToSurface,
		&Context->strokePaintToUser,
		vgmGETMATRIX(&Context->pathStrokeSurfaceToPaint),
		Context->strokePaint
		);
}

/* glyphFillSurfaceToPaint */
static void _InvalidateGlyphFillSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
}

static gceSTATUS _UpdateGlyphFillSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	return _UpdateSurfaceToPaintMatrix(
		Context,
		&Context->translatedGlyphUserToSurface,
		&Context->fillPaintToUser,
		vgmGETMATRIX(&Context->glyphFillSurfaceToPaint),
		Context->fillPaint
		);
}

/* glyphStrokeSurfaceToPaint */
static void _InvalidateGlyphStrokeSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
}

static gceSTATUS _UpdateGlyphStrokeSurfaceToPaint(
	IN vgsCONTEXT_PTR Context
	)
{
	return _UpdateSurfaceToPaintMatrix(
		Context,
		&Context->translatedGlyphUserToSurface,
		&Context->strokePaintToUser,
		vgmGETMATRIX(&Context->glyphStrokeSurfaceToPaint),
		Context->strokePaint
		);
}


/*******************************************************************************
**
** MATRIX CALLBACKS: intermediate matrices.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    Nothing.
*/

/* translatedGlyphUserToSurface */
static void _InvalidateTranslatedGlyphUserToSurface(
	IN vgsCONTEXT_PTR Context
	)
{
	vgfInvalidateContainer(Context, &Context->glyphFillSurfaceToPaint);
	vgfInvalidateContainer(Context, &Context->glyphStrokeSurfaceToPaint);
	vgfInvalidateContainer(Context, &Context->glyphSurfaceToImage);
}

static gceSTATUS _UpdateTranslatedGlyphUserToSurface(
	IN vgsCONTEXT_PTR Context
	)
{
	vgsMATRIX_PTR translatedGlyphUserToSurface;

	/* Get a shortcut to the matrix. */
	translatedGlyphUserToSurface
		= vgmGETMATRIX(&Context->translatedGlyphUserToSurface);

	/* Did the matrix change? */
	if (translatedGlyphUserToSurface->valuesDirty)
	{
#if gcdENABLE_PERFORMANCE_PRIOR
		vgsMATRIX_PTR glyphUserToSurface;
		gctFIXED m00, m01, m02;
		gctFIXED m10, m11, m12;
		gctFIXED translateX;
		gctFIXED translateY;

		/* Get a shortcut to the glyph-user-to-surface matrix. */
		glyphUserToSurface = vgmGETMATRIX(&Context->glyphUserToSurface);

		/* Get shortcuts to the translation values. */
		translateX = vgmFLOAT_TO_FIXED_SPECIAL(Context->glyphOffsetX);
		translateY = vgmFLOAT_TO_FIXED_SPECIAL(Context->glyphOffsetY);

		/* Read the top left 2x2 matrix values. */
		m00 = vgmMAT(glyphUserToSurface, 0, 0);
		m01 = vgmMAT(glyphUserToSurface, 0, 1);
		m02 = vgmMAT(glyphUserToSurface, 0, 2);
		m10 = vgmMAT(glyphUserToSurface, 1, 0);
		m11 = vgmMAT(glyphUserToSurface, 1, 1);
		m12 = vgmMAT(glyphUserToSurface, 1, 2);

		/* Compute the translated values. */
		m02 += vgmFIXED_MUL(m00, translateX) + vgmFIXED_MUL(m01, translateY);
		m12 += vgmFIXED_MUL(m10, translateX) + vgmFIXED_MUL(m11, translateY);

		/* Set the first row. */
		vgmMAT(translatedGlyphUserToSurface, 0, 0) = m00;
		vgmMAT(translatedGlyphUserToSurface, 0, 1) = m01;
		vgmMAT(translatedGlyphUserToSurface, 0, 2) = m02;

		/* Set the second row. */
		vgmMAT(translatedGlyphUserToSurface, 1, 0) = m10;
		vgmMAT(translatedGlyphUserToSurface, 1, 1) = m11;
		vgmMAT(translatedGlyphUserToSurface, 1, 2) = m12;

		/* Set the third row. */
		vgmMAT(translatedGlyphUserToSurface, 2, 0) = vgvFIXED_ZERO;
		vgmMAT(translatedGlyphUserToSurface, 2, 1) = vgvFIXED_ZERO;
		vgmMAT(translatedGlyphUserToSurface, 2, 2) = vgvFIXED_ONE;
#else
		vgsMATRIX_PTR glyphUserToSurface;
		gctFLOAT m00, m01, m02;
		gctFLOAT m10, m11, m12;
		gctFLOAT translateX;
		gctFLOAT translateY;

		/* Get a shortcut to the glyph-user-to-surface matrix. */
		glyphUserToSurface = vgmGETMATRIX(&Context->glyphUserToSurface);

		/* Get shortcuts to the translation values. */
		translateX = Context->glyphOffsetX;
		translateY = Context->glyphOffsetY;

		/* Read the top left 2x2 matrix values. */
		m00 = vgmMAT(glyphUserToSurface, 0, 0);
		m01 = vgmMAT(glyphUserToSurface, 0, 1);
		m02 = vgmMAT(glyphUserToSurface, 0, 2);
		m10 = vgmMAT(glyphUserToSurface, 1, 0);
		m11 = vgmMAT(glyphUserToSurface, 1, 1);
		m12 = vgmMAT(glyphUserToSurface, 1, 2);

		/* Compute the translated values. */
		m02 += m00 * translateX + m01 * translateY;
		m12 += m10 * translateX + m11 * translateY;

		/* Set the first row. */
		vgmMAT(translatedGlyphUserToSurface, 0, 0) = m00;
		vgmMAT(translatedGlyphUserToSurface, 0, 1) = m01;
		vgmMAT(translatedGlyphUserToSurface, 0, 2) = m02;

		/* Set the second row. */
		vgmMAT(translatedGlyphUserToSurface, 1, 0) = m10;
		vgmMAT(translatedGlyphUserToSurface, 1, 1) = m11;
		vgmMAT(translatedGlyphUserToSurface, 1, 2) = m12;

		/* Set the third row. */
		vgmMAT(translatedGlyphUserToSurface, 2, 0) = 0.0f;
		vgmMAT(translatedGlyphUserToSurface, 2, 1) = 0.0f;
		vgmMAT(translatedGlyphUserToSurface, 2, 2) = 1.0f;
#endif
	}

	/* Update HAL. */
	return _SetUserToSurface(Context, translatedGlyphUserToSurface);
}


/*******************************************************************************
**
** MATRIX CALLBACKS: root VG matrices.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    Nothing.
*/

/* pathUserToSurface */
static void _InvalidatePathUserToSurface(
	IN vgsCONTEXT_PTR Context
	)
{
	vgfInvalidateContainer(Context, &Context->pathFillSurfaceToPaint);
	vgfInvalidateContainer(Context, &Context->pathStrokeSurfaceToPaint);
}

static gceSTATUS _UpdatePathUserToSurface(
	IN vgsCONTEXT_PTR Context
	)
{
	return _SetUserToSurface(
		Context, vgmGETMATRIX(&Context->pathUserToSurface)
		);
}

/* glyphUserToSurface */
static void _InvalidateGlyphUserToSurface(
	IN vgsCONTEXT_PTR Context
	)
{
	vgfInvalidateContainer(Context, &Context->translatedGlyphUserToSurface);
}

static gceSTATUS _UpdateGlyphUserToSurface(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
	return gcvSTATUS_OK;
}

/* imageUserToSurface */
static void _InvalidateImageUserToSurface(
	IN vgsCONTEXT_PTR Context
	)
{
	vgfInvalidateContainer(Context, &Context->imageFillSurfaceToPaint);
	vgfInvalidateContainer(Context, &Context->imageSurfaceToImage);
}

static gceSTATUS _UpdateImageUserToSurface(
	IN vgsCONTEXT_PTR Context
	)
{
	return _SetUserToSurface(
		Context, vgmGETMATRIX(&Context->imageUserToSurface)
		);
}

/* fillPaintToUser */
static void _InvalidateFillPaintToUser(
	IN vgsCONTEXT_PTR Context
	)
{
	vgfInvalidateContainer(Context, &Context->pathFillSurfaceToPaint);
	vgfInvalidateContainer(Context, &Context->glyphFillSurfaceToPaint);
	vgfInvalidateContainer(Context, &Context->imageFillSurfaceToPaint);
}

static gceSTATUS _UpdateFillPaintToUser(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
	return gcvSTATUS_OK;
}

/* strokePaintToUser */
static void _InvalidateStrokePaintToUser(
	IN vgsCONTEXT_PTR Context
	)
{
	vgfInvalidateContainer(Context, &Context->pathStrokeSurfaceToPaint);
	vgfInvalidateContainer(Context, &Context->glyphStrokeSurfaceToPaint);
}

static gceSTATUS _UpdateStrokePaintToUser(
	IN vgsCONTEXT_PTR Context
	)
{
	/* Nothing to do here. */
	return gcvSTATUS_OK;
}


/******************************************************************************\
************************** Internal matrix functions. **************************
\******************************************************************************/

/*******************************************************************************
**
** vgfInitializeMatrixSet
**
** Initialize the VG matrix set.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    Nothing.
*/

void vgfInitializeMatrixSet(
	IN vgsCONTEXT_PTR Context
	)
{
	/***************************************************************************
	** Initialize image matrices.
	*/

	_InitializeContainer(
		Context,
		&Context->imageFillSurfaceToPaint,
		_InvalidateImageFillSurfaceToPaint,
		_UpdateImageFillSurfaceToPaint,
		gcvFALSE
		);

	_InitializeContainer(
		Context,
		&Context->glyphSurfaceToImage,
		_InvalidateGlyphSurfaceToImage,
		_UpdateGlyphSurfaceToImage,
		gcvFALSE
		);

	_InitializeContainer(
		Context,
		&Context->imageSurfaceToImage,
		_InvalidateImageSurfaceToImage,
		_UpdateImageSurfaceToImage,
		gcvFALSE
		);


	/***************************************************************************
	** Initialize path matrices.
	*/

	_InitializeContainer(
		Context,
		&Context->pathFillSurfaceToPaint,
		_InvalidatePathFillSurfaceToPaint,
		_UpdatePathFillSurfaceToPaint,
		gcvFALSE
		);

	_InitializeContainer(
		Context,
		&Context->pathStrokeSurfaceToPaint,
		_InvalidatePathStrokeSurfaceToPaint,
		_UpdatePathStrokeSurfaceToPaint,
		gcvFALSE
		);

	_InitializeContainer(
		Context,
		&Context->glyphFillSurfaceToPaint,
		_InvalidateGlyphFillSurfaceToPaint,
		_UpdateGlyphFillSurfaceToPaint,
		gcvFALSE
		);

	_InitializeContainer(
		Context,
		&Context->glyphStrokeSurfaceToPaint,
		_InvalidateGlyphStrokeSurfaceToPaint,
		_UpdateGlyphStrokeSurfaceToPaint,
		gcvFALSE
		);


	/***************************************************************************
	** Initialize intermediate matrices.
	*/

	_InitializeContainer(
		Context,
		&Context->translatedGlyphUserToSurface,
		_InvalidateTranslatedGlyphUserToSurface,
		_UpdateTranslatedGlyphUserToSurface,
		gcvFALSE
		);


	/***************************************************************************
	** Initialize root VG matrices.
	*/

	_InitializeContainer(
		Context,
		&Context->pathUserToSurface,
		_InvalidatePathUserToSurface,
		_UpdatePathUserToSurface,
		gcvTRUE
		);

	_InitializeContainer(
		Context,
		&Context->glyphUserToSurface,
		_InvalidateGlyphUserToSurface,
		_UpdateGlyphUserToSurface,
		gcvTRUE
		);

	_InitializeContainer(
		Context,
		&Context->imageUserToSurface,
		_InvalidateImageUserToSurface,
		_UpdateImageUserToSurface,
		gcvTRUE
		);

	_InitializeContainer(
		Context,
		&Context->fillPaintToUser,
		_InvalidateFillPaintToUser,
		_UpdateFillPaintToUser,
		gcvTRUE
		);

	_InitializeContainer(
		Context,
		&Context->strokePaintToUser,
		_InvalidateStrokePaintToUser,
		_UpdateStrokePaintToUser,
		gcvTRUE
		);


	/***************************************************************************
	** Set the current matrix.
	*/

	Context->matrixMode = VG_MATRIX_PATH_USER_TO_SURFACE;
	Context->matrix = &Context->pathUserToSurface;
	Context->currentUserToSurface = gcvNULL;
}


/*******************************************************************************
**
** vgfGetIdentity
**
** Returns a pointer to the identity matrix.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Pointer to the read-only identity matrix.
*/

vgsMATRIX_PTR vgfGetIdentity(
	void
	)
{
	return &_identityMatrix;
}


/*******************************************************************************
**
** vgfIsAffine
**
** A 3x3 matrix is affine if its last row is equal to (0, 0, 1).
**
** INPUT:
**
**    Matrix
**       Pointer to the matrix.
**
** OUTPUT:
**
**    Not zero if the matrix is affine.
*/

gctBOOL vgfIsAffine(
	IN const vgsMATRIX_PTR Matrix
	)
{
	return
#if gcdENABLE_PERFORMANCE_PRIOR
		(vgmMAT(Matrix, 2, 0) == vgvFIXED_ZERO) &&
		(vgmMAT(Matrix, 2, 1) == vgvFIXED_ZERO) &&
		(vgmMAT(Matrix, 2, 2) == vgvFIXED_ONE);
#else
		(vgmMAT(Matrix, 2, 0) == 0.0f) &&
		(vgmMAT(Matrix, 2, 1) == 0.0f) &&
		(vgmMAT(Matrix, 2, 2) == 1.0f);
#endif
}


/*******************************************************************************
**
** vgfIsIdentity
**
** Verifies whether the specified matrix is an identity matrix.
**
** INPUT:
**
**    Matrix
**       Pointer to the matrix.
**
** OUTPUT:
**
**    Returns gcvTRUE if the matrix is indentity.
*/

gctBOOL vgfIsIdentity(
	IN OUT vgsMATRIX_PTR Matrix
	)
{
	gcmVERIFY_BOOLEAN(Matrix->identityDirty);
	gcmVERIFY_BOOLEAN(Matrix->identity);

	if (Matrix->identityDirty)
	{
		gctINT compare;

		/* Compare the matrix values to the values of identity matrix. */
		compare = memcmp(
			vgmGETMATRIXVALUES(&_identityMatrix),
			vgmGETMATRIXVALUES(Matrix),
			gcmSIZEOF(vgtMATRIXVALUES)
			);

		/* Set idenity flags. */
		Matrix->identityDirty = gcvFALSE;
		Matrix->identity = (compare == 0);
	}

	/* Return identity flag. */
	return Matrix->identity;
}


/*******************************************************************************
**
** vgfInvalidateMatrix
**
** Invalidate the internal flags for intermediate matrices.
**
** INPUT:
**
**    Matrix
**       Pointer to the matrix.
**
** OUTPUT:
**
**    Matrix determinant.
*/

vgmINLINE void vgfInvalidateMatrix(
	IN OUT vgsMATRIX_PTR Matrix
	)
{
	/* Reset internal flags. */
	Matrix->valuesDirty = gcvTRUE;
	Matrix->detDirty    = gcvTRUE;

	/* For simplicity assume not identity. */
	Matrix->identityDirty = gcvFALSE;
	Matrix->identity      = gcvFALSE;
}


/*******************************************************************************
**
** vgfInvalidateContainer
**
** Invalidate the internal flags of a contained matrix.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Container
**       Pointer to the matrix container.
**
** OUTPUT:
**
**    Matrix determinant.
*/

vgmINLINE void vgfInvalidateContainer(
	IN vgsCONTEXT_PTR Context,
	IN OUT vgsMATRIXCONTAINER_PTR Container
	)
{
	/* Invalidate all flags. */
	vgfInvalidateMatrix(vgmGETMATRIX(Container));

	/* Invalidate the matrix dependents. */
	Container->invalidate(Context);
}


/*******************************************************************************
**
** vgfGetDeterminant
**
** Returns determinant of the specified 3x3 matrix.
**
** INPUT:
**
**    Matrix
**       Pointer to the matrix.
**
** OUTPUT:
**
**    Matrix determinant.
*/
gctFLOAT vgfGetDeterminant(
	IN OUT vgsMATRIX_PTR Matrix
	)
{
	gcmVERIFY_BOOLEAN(Matrix->detDirty);

	if (Matrix->detDirty)
	{
		Matrix->detDirty = gcvFALSE;

		if (vgfIsIdentity(Matrix))
		{
			Matrix->det = 1.0f;
		}
		else
		{
#if gcdENABLE_PERFORMANCE_PRIOR
			gctFIXED det
				= vgmFIXED_MUL(vgmMAT(Matrix, 0, 0),

						vgmFIXED_MUL(vgmMAT(Matrix, 1, 1), vgmMAT(Matrix, 2, 2)) -
						vgmFIXED_MUL(vgmMAT(Matrix, 1, 2), vgmMAT(Matrix, 2, 1))
						)

				+ vgmFIXED_MUL(vgmMAT(Matrix, 0, 1),

						vgmFIXED_MUL(vgmMAT(Matrix, 1, 2), vgmMAT(Matrix, 2, 0)) -
						vgmFIXED_MUL(vgmMAT(Matrix, 1, 0), vgmMAT(Matrix, 2, 2))
					)

				+ vgmFIXED_MUL(vgmMAT(Matrix, 0, 2),

						vgmFIXED_MUL(vgmMAT(Matrix, 1, 0), vgmMAT(Matrix, 2, 1)) -
						vgmFIXED_MUL(vgmMAT(Matrix, 1, 1), vgmMAT(Matrix, 2, 0))
					);

			Matrix->det = vgmFIXED_TO_FLOAT(det);
#else
			Matrix->det

				= vgmMAT(Matrix, 0, 0) *
					(
						vgmMAT(Matrix, 1, 1) * vgmMAT(Matrix, 2, 2) -
						vgmMAT(Matrix, 1, 2) * vgmMAT(Matrix, 2, 1)
					)

				+ vgmMAT(Matrix, 0, 1) *
					(
						vgmMAT(Matrix, 1, 2) * vgmMAT(Matrix, 2, 0) -
						vgmMAT(Matrix, 1, 0) * vgmMAT(Matrix, 2, 2)
					)

				+ vgmMAT(Matrix, 0, 2) *
					(
						vgmMAT(Matrix, 1, 0) * vgmMAT(Matrix, 2, 1) -
						vgmMAT(Matrix, 1, 1) * vgmMAT(Matrix, 2, 0)
					);
#endif
		}
	}

	/* Return identity flag. */
	return Matrix->det;
}


/*******************************************************************************
**
** vgfInvertMatrix
**
** Computes the inverse of the specified matrix.
**
** INPUT:
**
**    Matrix
**       Pointer to the matrix.
**
** OUTPUT:
**
**    Matrix determinant.
*/

gctBOOL vgfInvertMatrix(
	IN const vgsMATRIX_PTR Matrix,
	OUT vgsMATRIX_PTR Result
	)
{
	/* Identity matrix? */
	if (vgfIsIdentity(Matrix))
	{
		gcmVERIFY_OK(gcoOS_MemCopy(
			Result, Matrix, gcmSIZEOF(vgsMATRIX)
			));
	}
	else
	{
#if gcdENABLE_PERFORMANCE_PRIOR
		/* Get the matrix determinant. */
		gctFIXED det = vgmFLOAT_TO_FIXED(vgfGetDeterminant(Matrix));

		/* Can only compute inverse if the determinant is not zero. */
		if (det == vgvFIXED_ZERO)
		{
			return gcvFALSE;
		}

		/* Find the 1 over determinant. */
		det = vgmFIXED_DIV(vgvFIXED_ONE, det);

		/* ROW 0. */
		vgmMAT(Result, 0, 0)
			= vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(Matrix, 2, 2), vgmMAT(Matrix, 1, 1))
			-  vgmFIXED_MUL(vgmMAT(Matrix, 2, 1), vgmMAT(Matrix, 1, 2)) , det);

		vgmMAT(Result, 0, 1)
			= vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(Matrix, 2, 1), vgmMAT(Matrix, 0, 2))
			-  vgmFIXED_MUL(vgmMAT(Matrix, 2, 2), vgmMAT(Matrix, 0, 1)), det);

		vgmMAT(Result, 0, 2)
			= vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(Matrix, 1, 2), vgmMAT(Matrix, 0, 1))
			-  vgmFIXED_MUL(vgmMAT(Matrix, 1, 1), vgmMAT(Matrix, 0, 2)), det);

		/* ROW 1. */
		vgmMAT(Result, 1, 0)
			= vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(Matrix, 2, 0), vgmMAT(Matrix, 1, 2))
			-  vgmFIXED_MUL(vgmMAT(Matrix, 2, 2), vgmMAT(Matrix, 1, 0)), det);

		vgmMAT(Result, 1, 1)
			= vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(Matrix, 2, 2), vgmMAT(Matrix, 0, 0))
			-  vgmFIXED_MUL(vgmMAT(Matrix, 2, 0), vgmMAT(Matrix, 0, 2)), det);

		vgmMAT(Result, 1, 2)
			= vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(Matrix, 1, 0), vgmMAT(Matrix, 0, 2))
			-  vgmFIXED_MUL(vgmMAT(Matrix, 1, 2), vgmMAT(Matrix, 0, 0)), det);


		/* ROW 2. */
		if (vgfIsAffine(Matrix))
		{
			vgmMAT(Result, 2, 0) = vgvFIXED_ZERO;
			vgmMAT(Result, 2, 1) = vgvFIXED_ZERO;
			vgmMAT(Result, 2, 2) = vgvFIXED_ONE;
		}
		else
		{

			vgmMAT(Result, 2, 0)
				= vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(Matrix, 2, 1), vgmMAT(Matrix, 1, 0))
				-  vgmFIXED_MUL(vgmMAT(Matrix, 2, 0), vgmMAT(Matrix, 1, 1)), det);

			vgmMAT(Result, 2, 1)
				= vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(Matrix, 2, 0), vgmMAT(Matrix, 0, 1))
				-  vgmFIXED_MUL(vgmMAT(Matrix, 2, 1), vgmMAT(Matrix, 0, 0)), det);

			vgmMAT(Result, 2, 2)
				= vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(Matrix, 1, 1), vgmMAT(Matrix, 0, 0))
				-  vgmFIXED_MUL(vgmMAT(Matrix, 1, 0), vgmMAT(Matrix, 0, 1)), det);

		}
#else
		/* Get the matrix determinant. */
		gctFLOAT det = vgfGetDeterminant(Matrix);

		/* Can only compute inverse if the determinant is not zero. */
		if (det == 0.0f)
		{
			return gcvFALSE;
		}

		/* Find the 1 over determinant. */
		det = 1.0f / det;

		/* ROW 0. */
		vgmMAT(Result, 0, 0)
			= (vgmMAT(Matrix, 2, 2) * vgmMAT(Matrix, 1, 1)
			-  vgmMAT(Matrix, 2, 1) * vgmMAT(Matrix, 1, 2)) * det;

		vgmMAT(Result, 0, 1)
			= (vgmMAT(Matrix, 2, 1) * vgmMAT(Matrix, 0, 2)
			-  vgmMAT(Matrix, 2, 2) * vgmMAT(Matrix, 0, 1)) * det;

		vgmMAT(Result, 0, 2)
			= (vgmMAT(Matrix, 1, 2) * vgmMAT(Matrix, 0, 1)
			-  vgmMAT(Matrix, 1, 1) * vgmMAT(Matrix, 0, 2)) * det;

		/* ROW 1. */
		vgmMAT(Result, 1, 0)
			= (vgmMAT(Matrix, 2, 0) * vgmMAT(Matrix, 1, 2)
			-  vgmMAT(Matrix, 2, 2) * vgmMAT(Matrix, 1, 0)) * det;

		vgmMAT(Result, 1, 1)
			= (vgmMAT(Matrix, 2, 2) * vgmMAT(Matrix, 0, 0)
			-  vgmMAT(Matrix, 2, 0) * vgmMAT(Matrix, 0, 2)) * det;

		vgmMAT(Result, 1, 2)
			= (vgmMAT(Matrix, 1, 0) * vgmMAT(Matrix, 0, 2)
			-  vgmMAT(Matrix, 1, 2) * vgmMAT(Matrix, 0, 0)) * det;

		/* ROW 2. */
		if (vgfIsAffine(Matrix))
		{
			vgmMAT(Result, 2, 0) = 0.0f;
			vgmMAT(Result, 2, 1) = 0.0f;
			vgmMAT(Result, 2, 2) = 1.0f;
		}
		else
		{
			vgmMAT(Result, 2, 0)
				= (vgmMAT(Matrix, 2, 1) * vgmMAT(Matrix, 1, 0)
				-  vgmMAT(Matrix, 2, 0) * vgmMAT(Matrix, 1, 1)) * det;

			vgmMAT(Result, 2, 1)
				= (vgmMAT(Matrix, 2, 0) * vgmMAT(Matrix, 0, 1)
				-  vgmMAT(Matrix, 2, 1) * vgmMAT(Matrix, 0, 0)) * det;

			vgmMAT(Result, 2, 2)
				= (vgmMAT(Matrix, 1, 1) * vgmMAT(Matrix, 0, 0)
				-  vgmMAT(Matrix, 1, 0) * vgmMAT(Matrix, 0, 1)) * det;
		}
#endif
		/* Set matrix flags. */
		Result->identityDirty = gcvFALSE;
		Result->identity      = gcvFALSE;
		Result->detDirty      = gcvTRUE;
	}

	/* Success. */
	return gcvTRUE;
}


/*******************************************************************************
**
** vgfMultiplyVector2ByMatrix2x2
** vgfMultiplyVector2ByMatrix3x2
** vgfMultiplyVector3ByMatrix3x3
**
** Multiply the specified vertical vector and matrix.
**
** INPUT:
**
**    Vector
**       Pointer to the vertical vector.
**
**    Matrix
**       Pointer to the matrix.
**
** OUTPUT:
**
**    Result
**       Pointer to the result product vector.
*/

void vgfMultiplyVector2ByMatrix2x2(
	IN const vgtFLOATVECTOR2 Vector,
	IN const vgsMATRIX_PTR Matrix,
	OUT vgtFLOATVECTOR2 Result
	)
{
#if gcdENABLE_PERFORMANCE_PRIOR
	if (vgfIsIdentity(Matrix))
	{
		gcmVERIFY_OK(gcoOS_MemCopy(
			Result, Vector, gcmSIZEOF(vgtFLOATVECTOR2)
			));
	}
	else
	{
		vgtFIXEDVECTOR2 fixedVector;
		gctFIXED x, y;

		/*convert to the fixed point*/
		fixedVector[0] = vgmFLOAT_TO_FIXED_SPECIAL(Vector[0]);
		fixedVector[1] = vgmFLOAT_TO_FIXED_SPECIAL(Vector[1]);

		x
			= vgmFIXED_MUL(vgmMAT(Matrix, 0, 0), fixedVector[0])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 0, 1), fixedVector[1]);

		y
			= vgmFIXED_MUL(vgmMAT(Matrix, 1, 0), fixedVector[0])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 1, 1), fixedVector[1]);

		Result[0] = vgmFIXED_TO_FLOAT(x);
		Result[1] = vgmFIXED_TO_FLOAT(y);
	}
#else
	if (vgfIsIdentity(Matrix))
	{
		gcmVERIFY_OK(gcoOS_MemCopy(
			Result, Vector, gcmSIZEOF(vgtFLOATVECTOR2)
			));
	}
	else
	{
		VGfloat x
			= vgmMAT(Matrix, 0, 0) * Vector[0]
			+ vgmMAT(Matrix, 0, 1) * Vector[1];

		VGfloat y
			= vgmMAT(Matrix, 1, 0) * Vector[0]
			+ vgmMAT(Matrix, 1, 1) * Vector[1];

		Result[0] = x;
		Result[1] = y;
	}
#endif
}

void vgfMultiplyVector2ByMatrix3x2(
	IN const vgtFLOATVECTOR2 Vector,
	IN const vgsMATRIX_PTR Matrix,
	OUT vgtFLOATVECTOR2 Result
	)
{
#if gcdENABLE_PERFORMANCE_PRIOR
	if (vgfIsIdentity(Matrix))
	{
		gcmVERIFY_OK(gcoOS_MemCopy(
			Result, Vector, gcmSIZEOF(vgtFLOATVECTOR2)
			));
	}
	else
	{
		vgtFIXEDVECTOR2 fixedVector;
		gctFIXED x, y;

		/*convert to the fixed point*/
		fixedVector[0] = vgmFLOAT_TO_FIXED_SPECIAL(Vector[0]);
		fixedVector[1] = vgmFLOAT_TO_FIXED_SPECIAL(Vector[1]);


		x
			= vgmFIXED_MUL(vgmMAT(Matrix, 0, 0), fixedVector[0])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 0, 1), fixedVector[1])
			+ vgmMAT(Matrix, 0, 2);

		y
			= vgmFIXED_MUL(vgmMAT(Matrix, 1, 0), fixedVector[0])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 1, 1), fixedVector[1])
			+ vgmMAT(Matrix, 1, 2);

		Result[0] = vgmFIXED_TO_FLOAT(x);
		Result[1] = vgmFIXED_TO_FLOAT(y);
	}
#else
	if (vgfIsIdentity(Matrix))
	{
		gcmVERIFY_OK(gcoOS_MemCopy(
			Result, Vector, gcmSIZEOF(vgtFLOATVECTOR2)
			));
	}
	else
	{
		VGfloat x
			= vgmMAT(Matrix, 0, 0) * Vector[0]
			+ vgmMAT(Matrix, 0, 1) * Vector[1]
			+ vgmMAT(Matrix, 0, 2);

		VGfloat y
			= vgmMAT(Matrix, 1, 0) * Vector[0]
			+ vgmMAT(Matrix, 1, 1) * Vector[1]
			+ vgmMAT(Matrix, 1, 2);

		Result[0] = x;
		Result[1] = y;
	}
#endif
}


void vgfMultiplyVector3ByMatrix3x3(
	IN const vgtFLOATVECTOR3 Vector,
	IN const vgsMATRIX_PTR Matrix,
	OUT vgtFLOATVECTOR3 Result
	)
{
#if gcdENABLE_PERFORMANCE_PRIOR
	if (vgfIsIdentity(Matrix))
	{
		gcmVERIFY_OK(gcoOS_MemCopy(
			Result, Vector, gcmSIZEOF(vgtFLOATVECTOR3)
			));
	}
	else
	{
		vgtFIXEDVECTOR3 fixedVector;
		gctFIXED x, y, z;

		/*convert to the fixed point*/
		fixedVector[0] = vgmFLOAT_TO_FIXED_SPECIAL(Vector[0]);
		fixedVector[1] = vgmFLOAT_TO_FIXED_SPECIAL(Vector[1]);
		fixedVector[2] = vgmFLOAT_TO_FIXED_SPECIAL(Vector[2]);

		x
			= vgmFIXED_MUL(vgmMAT(Matrix, 0, 0), fixedVector[0])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 0, 1), fixedVector[1])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 0, 2), fixedVector[2]);

		y
			= vgmFIXED_MUL(vgmMAT(Matrix, 1, 0), fixedVector[0])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 1, 1), fixedVector[1])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 1, 2), fixedVector[2]);

		z
			= vgmFIXED_MUL(vgmMAT(Matrix, 2, 0), fixedVector[0])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 2, 1), fixedVector[1])
			+ vgmFIXED_MUL(vgmMAT(Matrix, 2, 2), fixedVector[2]);

		Result[0] = vgmFIXED_TO_FLOAT(x);
		Result[1] = vgmFIXED_TO_FLOAT(y);
		Result[2] = vgmFIXED_TO_FLOAT(z);
	}
#else
	if (vgfIsIdentity(Matrix))
	{
		gcmVERIFY_OK(gcoOS_MemCopy(
			Result, Vector, gcmSIZEOF(vgtFLOATVECTOR3)
			));
	}
	else
	{
		VGfloat x
			= vgmMAT(Matrix, 0, 0) * Vector[0]
			+ vgmMAT(Matrix, 0, 1) * Vector[1]
			+ vgmMAT(Matrix, 0, 2) * Vector[2];

		VGfloat y
			= vgmMAT(Matrix, 1, 0) * Vector[0]
			+ vgmMAT(Matrix, 1, 1) * Vector[1]
			+ vgmMAT(Matrix, 1, 2) * Vector[2];

		VGfloat z
			= vgmMAT(Matrix, 2, 0) * Vector[0]
			+ vgmMAT(Matrix, 2, 1) * Vector[1]
			+ vgmMAT(Matrix, 2, 2) * Vector[2];

		Result[0] = x;
		Result[1] = y;
		Result[2] = z;
	}
#endif
}


/*******************************************************************************
**
** vgfMultiplyMatrix3x3
**
** Multiply the specified 3x3 matrices.
**
** INPUT:
**
**    Matrix1
**       Pointer to the first matrix.
**
**    Matrix2
**       Pointer to the second matrix.
**
** OUTPUT:
**
**    Result
**       Pointer to the result matrix.
*/

void vgfMultiplyMatrix3x3(
	IN const vgsMATRIX_PTR Matrix1,
	IN const vgsMATRIX_PTR Matrix2,
	OUT vgsMATRIX_PTR Result
	)
{
	if (vgfIsIdentity(Matrix1))
	{
		gcmVERIFY_OK(gcoOS_MemCopy(
			Result, Matrix2, gcmSIZEOF(vgsMATRIX)
			));
	}

	else if (vgfIsIdentity(Matrix2))
	{
		gcmVERIFY_OK(gcoOS_MemCopy(
			Result, Matrix1, gcmSIZEOF(vgsMATRIX)
			));
	}

	else
	{
#if gcdENABLE_PERFORMANCE_PRIOR
		gctFIXED e00
			= vgmFIXED_MUL(vgmMAT(Matrix1, 0, 0), vgmMAT(Matrix2, 0, 0))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 0, 1), vgmMAT(Matrix2, 1, 0))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 0, 2), vgmMAT(Matrix2, 2, 0));

		gctFIXED e01
			= vgmFIXED_MUL(vgmMAT(Matrix1, 0, 0), vgmMAT(Matrix2, 0, 1))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 0, 1), vgmMAT(Matrix2, 1, 1))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 0, 2), vgmMAT(Matrix2, 2, 1));

		gctFIXED e02
			= vgmFIXED_MUL(vgmMAT(Matrix1, 0, 0), vgmMAT(Matrix2, 0, 2))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 0, 1), vgmMAT(Matrix2, 1, 2))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 0, 2), vgmMAT(Matrix2, 2, 2));

		gctFIXED e10
			= vgmFIXED_MUL(vgmMAT(Matrix1, 1, 0), vgmMAT(Matrix2, 0, 0))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 1, 1), vgmMAT(Matrix2, 1, 0))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 1, 2), vgmMAT(Matrix2, 2, 0));

		gctFIXED e11
			= vgmFIXED_MUL(vgmMAT(Matrix1, 1, 0), vgmMAT(Matrix2, 0, 1))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 1, 1), vgmMAT(Matrix2, 1, 1))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 1, 2), vgmMAT(Matrix2, 2, 1));

		gctFIXED e12
			= vgmFIXED_MUL(vgmMAT(Matrix1, 1, 0), vgmMAT(Matrix2, 0, 2))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 1, 1), vgmMAT(Matrix2, 1, 2))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 1, 2), vgmMAT(Matrix2, 2, 2));

		gctFIXED e20
			= vgmFIXED_MUL(vgmMAT(Matrix1, 2, 0), vgmMAT(Matrix2, 0, 0))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 2, 1), vgmMAT(Matrix2, 1, 0))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 2, 2), vgmMAT(Matrix2, 2, 0));

		gctFIXED e21
			= vgmFIXED_MUL(vgmMAT(Matrix1, 2, 0), vgmMAT(Matrix2, 0, 1))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 2, 1), vgmMAT(Matrix2, 1, 1))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 2, 2), vgmMAT(Matrix2, 2, 1));

		gctFIXED e22
			= vgmFIXED_MUL(vgmMAT(Matrix1, 2, 0), vgmMAT(Matrix2, 0, 2))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 2, 1), vgmMAT(Matrix2, 1, 2))
			+ vgmFIXED_MUL(vgmMAT(Matrix1, 2, 2), vgmMAT(Matrix2, 2, 2));
#else
		gctFLOAT e00
			= vgmMAT(Matrix1, 0, 0) * vgmMAT(Matrix2, 0, 0)
			+ vgmMAT(Matrix1, 0, 1) * vgmMAT(Matrix2, 1, 0)
			+ vgmMAT(Matrix1, 0, 2) * vgmMAT(Matrix2, 2, 0);

		gctFLOAT e01
			= vgmMAT(Matrix1, 0, 0) * vgmMAT(Matrix2, 0, 1)
			+ vgmMAT(Matrix1, 0, 1) * vgmMAT(Matrix2, 1, 1)
			+ vgmMAT(Matrix1, 0, 2) * vgmMAT(Matrix2, 2, 1);

		gctFLOAT e02
			= vgmMAT(Matrix1, 0, 0) * vgmMAT(Matrix2, 0, 2)
			+ vgmMAT(Matrix1, 0, 1) * vgmMAT(Matrix2, 1, 2)
			+ vgmMAT(Matrix1, 0, 2) * vgmMAT(Matrix2, 2, 2);

		gctFLOAT e10
			= vgmMAT(Matrix1, 1, 0) * vgmMAT(Matrix2, 0, 0)
			+ vgmMAT(Matrix1, 1, 1) * vgmMAT(Matrix2, 1, 0)
			+ vgmMAT(Matrix1, 1, 2) * vgmMAT(Matrix2, 2, 0);

		gctFLOAT e11
			= vgmMAT(Matrix1, 1, 0) * vgmMAT(Matrix2, 0, 1)
			+ vgmMAT(Matrix1, 1, 1) * vgmMAT(Matrix2, 1, 1)
			+ vgmMAT(Matrix1, 1, 2) * vgmMAT(Matrix2, 2, 1);

		gctFLOAT e12
			= vgmMAT(Matrix1, 1, 0) * vgmMAT(Matrix2, 0, 2)
			+ vgmMAT(Matrix1, 1, 1) * vgmMAT(Matrix2, 1, 2)
			+ vgmMAT(Matrix1, 1, 2) * vgmMAT(Matrix2, 2, 2);

		gctFLOAT e20
			= vgmMAT(Matrix1, 2, 0) * vgmMAT(Matrix2, 0, 0)
			+ vgmMAT(Matrix1, 2, 1) * vgmMAT(Matrix2, 1, 0)
			+ vgmMAT(Matrix1, 2, 2) * vgmMAT(Matrix2, 2, 0);

		gctFLOAT e21
			= vgmMAT(Matrix1, 2, 0) * vgmMAT(Matrix2, 0, 1)
			+ vgmMAT(Matrix1, 2, 1) * vgmMAT(Matrix2, 1, 1)
			+ vgmMAT(Matrix1, 2, 2) * vgmMAT(Matrix2, 2, 1);

		gctFLOAT e22
			= vgmMAT(Matrix1, 2, 0) * vgmMAT(Matrix2, 0, 2)
			+ vgmMAT(Matrix1, 2, 1) * vgmMAT(Matrix2, 1, 2)
			+ vgmMAT(Matrix1, 2, 2) * vgmMAT(Matrix2, 2, 2);
#endif
		Result->identityDirty = gcvFALSE;
		Result->identity      = gcvFALSE;
		Result->detDirty      = gcvTRUE;

		vgmMAT(Result, 0, 0) = e00;
		vgmMAT(Result, 0, 1) = e01;
		vgmMAT(Result, 0, 2) = e02;

		vgmMAT(Result, 1, 0) = e10;
		vgmMAT(Result, 1, 1) = e11;
		vgmMAT(Result, 1, 2) = e12;

		vgmMAT(Result, 2, 0) = e20;
		vgmMAT(Result, 2, 1) = e21;
		vgmMAT(Result, 2, 2) = e22;
	}
}


/******************************************************************************\
****************************** OpenVG Matrix API. ******************************
\******************************************************************************/

/*******************************************************************************
**
** vgLoadIdentity
**
** The vgLoadIdentity function sets the current matrix M to the identity
** matrix:
**
**        | 1 0 0 |
**    M = | 0 1 0 |
**        | 0 0 1 |
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgLoadIdentity(
	void
	)
{
	vgmENTERAPI(vgLoadIdentity)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s();\n",
			__FUNCTION__
			);

		/* Set the current matrix to identity. */
		_LoadIdentity(Context, Context->matrix);
	}
	vgmLEAVEAPI(vgLoadIdentity);
}


/*******************************************************************************
**
** vgLoadMatrix
**
** The vgLoadMatrix function loads an arbitrary set of matrix values into the
** current matrix. Nine matrix values are read from m, in the order:
**
**    { ScaleX, ShearY, w0, ShearX, ScaleY, w1, TransformX, TransformY, w2 }
**
** defining the matrix:
**
**	      | ScaleX ShearX TransformX |
**    M = | ShearY ScaleY TransformY |
**	      | w0     w1     w2         |
**
** However, if the targeted matrix is affine (i.e., the matrix mode is not
** VG_MATRIX_IMAGE_USER_TO_SURFACE), the values { w0, w1, w2 } are ignored and
** replaced by the values { 0, 0, 1 }, resulting in the affine matrix:
**
**	      | ScaleX ShearX TransformX |
**    M = | ShearY ScaleY TransformY |
**	      | 0      0      1          |
**
** INPUT:
**
**    Matrix
**       Pointer to the matrix to be loaded.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgLoadMatrix(
	const VGfloat * Matrix
	)
{
	vgmENTERAPI(vgLoadMatrix)
	{
		vgsMATRIXCONTAINER_PTR matrix;

		vgmDUMP_FLOAT_ARRAY(
			Matrix, 9
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(Matrix);\n",
			__FUNCTION__
			);

		/* Validate the input matrix. */
		if (vgmIS_INVALID_PTR(Matrix, 4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Get a shortcut to the matrix. */
		matrix = Context->matrix;

		/* Load the current matrix. */
#if gcdENABLE_PERFORMANCE_PRIOR
		{
			gctINT i, j;
			for(i = 0; i < vgvMATRIXROWS; i++)
			{
				for (j = 0; j < vgvMATRIXCOLUMNS; j++)
				{
					vgmMAT(matrix, i, j) = vgmFLOAT_TO_FIXED_SPECIAL(vgmFLOATMAT(Matrix, i, j));
				}
			}

			/* Replace the last row. */
			if (Context->matrixMode != VG_MATRIX_IMAGE_USER_TO_SURFACE)
			{
				vgmMAT(matrix, 2, 0) = vgvFIXED_ZERO;
				vgmMAT(matrix, 2, 1) = vgvFIXED_ZERO;
				vgmMAT(matrix, 2, 2) = vgvFIXED_ONE;
			}
		}

#else
		gcmVERIFY_OK(gcoOS_MemCopy(
			matrix,
			Matrix,
			gcmSIZEOF(vgtMATRIXVALUES)
			));

		/* Replace the last row. */
		if (Context->matrixMode != VG_MATRIX_IMAGE_USER_TO_SURFACE)
		{
			vgmMAT(matrix, 2, 0) = 0.0f;
			vgmMAT(matrix, 2, 1) = 0.0f;
			vgmMAT(matrix, 2, 2) = 1.0f;
		}
#endif
		/* Reset matrix. */
		vgfInvalidateContainer(Context, matrix);
	}
	vgmLEAVEAPI(vgLoadMatrix);
}


/*******************************************************************************
**
** vgGetMatrix
**
** It is possible to retrieve the value of the current transformation by
** calling vgGetMatrix. Nine values are written to Matrix in the order:
**
**    { ScaleX, ShearY, w0, ShearX, ScaleY, w1, TransformX, TransformY, w2 }
**
** For an affine matrix, w0 and w1 will always be 0 and w2 will always be 1.
**
** INPUT:
**
**    Matrix
**       Pointer to the matrix receiving the value of the current
**       transformation.
**
** OUTPUT:
**
**    Matrix
**       Copied value of the current transformation.
*/

VG_API_CALL void VG_API_ENTRY vgGetMatrix(
	VGfloat * Matrix
	)
{
	vgmENTERAPI(vgGetMatrix)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X);\n",
			__FUNCTION__,
			Matrix
			);

		/* Validate the output matrix. */
		if (vgmIS_INVALID_PTR(Matrix, 4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

#if gcdENABLE_PERFORMANCE_PRIOR
		{
			gctINT i, j;
			for(i = 0; i < vgvMATRIXROWS; i++)
			{
				for (j = 0; j < vgvMATRIXCOLUMNS; j++)
				{
					vgmFLOATMAT(Matrix, i, j) = vgmFIXED_TO_FLOAT(vgmMAT(Context->matrix, i, j));
				}
			}
		}
#else
		/* Return the current matrix. */
		gcmVERIFY_OK(gcoOS_MemCopy(
			Matrix,
			vgmGETMATRIXVALUES(Context->matrix),
			gcmSIZEOF(vgtMATRIXVALUES)
			));
#endif
	}
	vgmLEAVEAPI(vgGetMatrix);
}


/*******************************************************************************
**
** vgMultMatrix
**
** The vgMultMatrix function right-multiplies the current matrix M by a given
** matrix:
**
**	            | ScaleX ShearX TransformX |
**    M <-- M * | ShearY ScaleY TransformY |
**	            | w0     w1     w2         |
**
** Nine matrix values are read from Matrix in the order:
**
**    { ScaleX, ShearY, w0, ShearX, ScaleY, w1, TransformX, TransformY, w2 }
**
** and the current matrix is multiplied by the resulting matrix. However, if
** the targeted matrix is affine (i.e., the matrix mode is not
** VG_MATRIX_IMAGE_USER_TO_SURFACE), the values { w0, w1, w2 } are ignored
** and replaced by the values { 0, 0, 1 } prior to multiplication.
**
** INPUT:
**
**    Matrix
**       Pointer to the matrix to be multiplied with the current
**       transformation.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgMultMatrix(
	const VGfloat * Matrix
	)
{
	vgmENTERAPI(vgMultMatrix)
	{
		vgsMATRIX result;
		vgsMATRIXCONTAINER_PTR matrix;

		vgmDUMP_FLOAT_ARRAY(
			Matrix, 9
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(Matrix);\n",
			__FUNCTION__
			);

		/* Validate the input matrix. */
		if (vgmIS_INVALID_PTR(Matrix, 4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Create a shortcut to the matrix. */
		matrix = Context->matrix;

#if gcdENABLE_PERFORMANCE_PRIOR
		{
			vgtMATRIXVALUES fixedMatrix;
			gctINT i, j;

			for(i = 0; i < vgvMATRIXROWS; i++)
			{
				for (j = 0; j < vgvMATRIXCOLUMNS; j++)
				{
					vgmMAT(fixedMatrix, i, j) = vgmFLOAT_TO_FIXED_SPECIAL(vgmFLOATMAT(Matrix, i, j));
				}
			}

			if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
			{
				/* ROW 0. */
				vgmMAT(&result, 0, 0)
					= vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(fixedMatrix, 0, 0))
					+ vgmFIXED_MUL(vgmMAT(matrix, 0, 1), vgmMAT(fixedMatrix, 1, 0))
					+ vgmFIXED_MUL(vgmMAT(matrix, 0, 2), vgmMAT(fixedMatrix, 2, 0));

				vgmMAT(&result, 0, 1)
					= vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(fixedMatrix, 0, 1))
					+ vgmFIXED_MUL(vgmMAT(matrix, 0, 1), vgmMAT(fixedMatrix, 1, 1))
					+ vgmFIXED_MUL(vgmMAT(matrix, 0, 2), vgmMAT(fixedMatrix, 2, 1));

				vgmMAT(&result, 0, 2)
					= vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(fixedMatrix, 0, 2))
					+ vgmFIXED_MUL(vgmMAT(matrix, 0, 1), vgmMAT(fixedMatrix, 1, 2))
					+ vgmFIXED_MUL(vgmMAT(matrix, 0, 2), vgmMAT(fixedMatrix, 2, 2));

				/* ROW 1. */
				vgmMAT(&result, 1, 0)
					= vgmFIXED_MUL(vgmMAT(matrix, 1, 0), vgmMAT(fixedMatrix, 0, 0))
					+ vgmFIXED_MUL(vgmMAT(matrix, 1, 1), vgmMAT(fixedMatrix, 1, 0))
					+ vgmFIXED_MUL(vgmMAT(matrix, 1, 2), vgmMAT(fixedMatrix, 2, 0));

				vgmMAT(&result, 1, 1)
					= vgmFIXED_MUL(vgmMAT(matrix, 1, 0), vgmMAT(fixedMatrix, 0, 1))
					+ vgmFIXED_MUL(vgmMAT(matrix, 1, 1), vgmMAT(fixedMatrix, 1, 1))
					+ vgmFIXED_MUL(vgmMAT(matrix, 1, 2), vgmMAT(fixedMatrix, 2, 1));

				vgmMAT(&result, 1, 2)
					= vgmFIXED_MUL(vgmMAT(matrix, 1, 0), vgmMAT(fixedMatrix, 0, 2))
					+ vgmFIXED_MUL(vgmMAT(matrix, 1, 1), vgmMAT(fixedMatrix, 1, 2))
					+ vgmFIXED_MUL(vgmMAT(matrix, 1, 2), vgmMAT(fixedMatrix, 2, 2));

				/* ROW 2. */
				vgmMAT(&result, 2, 0)
					= vgmFIXED_MUL(vgmMAT(matrix, 2, 0), vgmMAT(fixedMatrix, 0, 0))
					+ vgmFIXED_MUL(vgmMAT(matrix, 2, 1), vgmMAT(fixedMatrix, 1, 0))
					+ vgmFIXED_MUL(vgmMAT(matrix, 2, 2), vgmMAT(fixedMatrix, 2, 0));

				vgmMAT(&result, 2, 1)
					= vgmFIXED_MUL(vgmMAT(matrix, 2, 0), vgmMAT(fixedMatrix, 0, 1))
					+ vgmFIXED_MUL(vgmMAT(matrix, 2, 1), vgmMAT(fixedMatrix, 1, 1))
					+ vgmFIXED_MUL(vgmMAT(matrix, 2, 2), vgmMAT(fixedMatrix, 2, 1));

				vgmMAT(&result, 2, 2)
					= vgmFIXED_MUL(vgmMAT(matrix, 2, 0), vgmMAT(fixedMatrix, 0, 2))
					+ vgmFIXED_MUL(vgmMAT(matrix, 2, 1), vgmMAT(fixedMatrix, 1, 2))
					+ vgmFIXED_MUL(vgmMAT(matrix, 2, 2), vgmMAT(fixedMatrix, 2, 2));
			}
			else
			{
				/* ROW 0. */
				vgmMAT(&result, 0, 0)
					= vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(fixedMatrix, 0, 0))
					+ vgmFIXED_MUL(vgmMAT(matrix, 0, 1), vgmMAT(fixedMatrix, 1, 0));

				vgmMAT(&result, 0, 1)
					= vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(fixedMatrix, 0, 1))
					+ vgmFIXED_MUL(vgmMAT(matrix, 0, 1), vgmMAT(fixedMatrix, 1, 1));

				vgmMAT(&result, 0, 2)
					= vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(fixedMatrix, 0, 2))
					+ vgmFIXED_MUL(vgmMAT(matrix, 0, 1), vgmMAT(fixedMatrix, 1, 2))
					+ vgmMAT(matrix, 0, 2);

				/* ROW 1. */
				vgmMAT(&result, 1, 0)
					= vgmFIXED_MUL(vgmMAT(matrix, 1, 0), vgmMAT(fixedMatrix, 0, 0))
					+ vgmFIXED_MUL(vgmMAT(matrix, 1, 1), vgmMAT(fixedMatrix, 1, 0));

				vgmMAT(&result, 1, 1)
					= vgmFIXED_MUL(vgmMAT(matrix, 1, 0), vgmMAT(fixedMatrix, 0, 1))
					+ vgmFIXED_MUL(vgmMAT(matrix, 1, 1), vgmMAT(fixedMatrix, 1, 1));

				vgmMAT(&result, 1, 2)
					= vgmFIXED_MUL(vgmMAT(matrix, 1, 0), vgmMAT(fixedMatrix, 0, 2))
					+ vgmFIXED_MUL(vgmMAT(matrix, 1, 1), vgmMAT(fixedMatrix, 1, 2))
					+ vgmMAT(matrix, 1, 2);

				/* ROW 2. */
				vgmMAT(&result, 2, 0) = vgvFIXED_ZERO;
				vgmMAT(&result, 2, 1) = vgvFIXED_ZERO;
				vgmMAT(&result, 2, 2) = vgvFIXED_ONE;
			}
		}

#else
		if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
		{
			/* ROW 0. */
			vgmMAT(&result, 0, 0)
				= vgmMAT(matrix, 0, 0) * vgmMAT(Matrix, 0, 0)
				+ vgmMAT(matrix, 0, 1) * vgmMAT(Matrix, 1, 0)
				+ vgmMAT(matrix, 0, 2) * vgmMAT(Matrix, 2, 0);

			vgmMAT(&result, 0, 1)
				= vgmMAT(matrix, 0, 0) * vgmMAT(Matrix, 0, 1)
				+ vgmMAT(matrix, 0, 1) * vgmMAT(Matrix, 1, 1)
				+ vgmMAT(matrix, 0, 2) * vgmMAT(Matrix, 2, 1);

			vgmMAT(&result, 0, 2)
				= vgmMAT(matrix, 0, 0) * vgmMAT(Matrix, 0, 2)
				+ vgmMAT(matrix, 0, 1) * vgmMAT(Matrix, 1, 2)
				+ vgmMAT(matrix, 0, 2) * vgmMAT(Matrix, 2, 2);

			/* ROW 1. */
			vgmMAT(&result, 1, 0)
				= vgmMAT(matrix, 1, 0) * vgmMAT(Matrix, 0, 0)
				+ vgmMAT(matrix, 1, 1) * vgmMAT(Matrix, 1, 0)
				+ vgmMAT(matrix, 1, 2) * vgmMAT(Matrix, 2, 0);

			vgmMAT(&result, 1, 1)
				= vgmMAT(matrix, 1, 0) * vgmMAT(Matrix, 0, 1)
				+ vgmMAT(matrix, 1, 1) * vgmMAT(Matrix, 1, 1)
				+ vgmMAT(matrix, 1, 2) * vgmMAT(Matrix, 2, 1);

			vgmMAT(&result, 1, 2)
				= vgmMAT(matrix, 1, 0) * vgmMAT(Matrix, 0, 2)
				+ vgmMAT(matrix, 1, 1) * vgmMAT(Matrix, 1, 2)
				+ vgmMAT(matrix, 1, 2) * vgmMAT(Matrix, 2, 2);

			/* ROW 2. */
			vgmMAT(&result, 2, 0)
				= vgmMAT(matrix, 2, 0) * vgmMAT(Matrix, 0, 0)
				+ vgmMAT(matrix, 2, 1) * vgmMAT(Matrix, 1, 0)
				+ vgmMAT(matrix, 2, 2) * vgmMAT(Matrix, 2, 0);

			vgmMAT(&result, 2, 1)
				= vgmMAT(matrix, 2, 0) * vgmMAT(Matrix, 0, 1)
				+ vgmMAT(matrix, 2, 1) * vgmMAT(Matrix, 1, 1)
				+ vgmMAT(matrix, 2, 2) * vgmMAT(Matrix, 2, 1);

			vgmMAT(&result, 2, 2)
				= vgmMAT(matrix, 2, 0) * vgmMAT(Matrix, 0, 2)
				+ vgmMAT(matrix, 2, 1) * vgmMAT(Matrix, 1, 2)
				+ vgmMAT(matrix, 2, 2) * vgmMAT(Matrix, 2, 2);
		}
		else
		{
			/* ROW 0. */
			vgmMAT(&result, 0, 0)
				= vgmMAT(matrix, 0, 0) * vgmMAT(Matrix, 0, 0)
				+ vgmMAT(matrix, 0, 1) * vgmMAT(Matrix, 1, 0);

			vgmMAT(&result, 0, 1)
				= vgmMAT(matrix, 0, 0) * vgmMAT(Matrix, 0, 1)
				+ vgmMAT(matrix, 0, 1) * vgmMAT(Matrix, 1, 1);

			vgmMAT(&result, 0, 2)
				= vgmMAT(matrix, 0, 0) * vgmMAT(Matrix, 0, 2)
				+ vgmMAT(matrix, 0, 1) * vgmMAT(Matrix, 1, 2)
				+ vgmMAT(matrix, 0, 2);

			/* ROW 1. */
			vgmMAT(&result, 1, 0)
				= vgmMAT(matrix, 1, 0) * vgmMAT(Matrix, 0, 0)
				+ vgmMAT(matrix, 1, 1) * vgmMAT(Matrix, 1, 0);

			vgmMAT(&result, 1, 1)
				= vgmMAT(matrix, 1, 0) * vgmMAT(Matrix, 0, 1)
				+ vgmMAT(matrix, 1, 1) * vgmMAT(Matrix, 1, 1);

			vgmMAT(&result, 1, 2)
				= vgmMAT(matrix, 1, 0) * vgmMAT(Matrix, 0, 2)
				+ vgmMAT(matrix, 1, 1) * vgmMAT(Matrix, 1, 2)
				+ vgmMAT(matrix, 1, 2);

			/* ROW 2. */
			vgmMAT(&result, 2, 0) = 0.0f;
			vgmMAT(&result, 2, 1) = 0.0f;
			vgmMAT(&result, 2, 2) = 1.0f;
		}
#endif
		/* Set the result matrix. */
		gcmVERIFY_OK(gcoOS_MemCopy(
			vgmGETMATRIXVALUES(matrix),
			vgmGETMATRIXVALUES(&result),
			gcmSIZEOF(vgtMATRIXVALUES)
			));

		/* Reset matrix. */
		vgfInvalidateContainer(Context, matrix);
	}
	vgmLEAVEAPI(vgMultMatrix);
}


/*******************************************************************************
**
** vgTranslate
**
** The vgTranslate function modifies the current transformation by appending a
** translation. This is equivalent to right-multiplying the current matrix M
** by a translation matrix:
**
**              | 1 0 TransformX |
**    M <-- M * | 0 1 TransformY |
**              | 0 0 1          |
**
** INPUT:
**
**    TranslateX, TranslateY
**       Translation matrix values.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgTranslate(
	VGfloat TranslateX,
	VGfloat TranslateY
	)
{
	vgmENTERAPI(vgTranslate)
	{
		/* Create a shortcut to the matrix. */
		vgsMATRIXCONTAINER_PTR matrix = Context->matrix;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(%.10ff, %.10ff);\n",
			__FUNCTION__,
			TranslateX, TranslateY
			);

#if gcdENABLE_PERFORMANCE_PRIOR
		{
			gctFIXED fixedTranslateX = vgmFLOAT_TO_FIXED_SPECIAL(TranslateX);
			gctFIXED fixedTranslateY = vgmFLOAT_TO_FIXED_SPECIAL(TranslateY);

			if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
			{
				vgmMAT(matrix, 0, 2)
					+= vgmFIXED_MUL(vgmMAT(matrix, 0, 0), fixedTranslateX)
					+  vgmFIXED_MUL(vgmMAT(matrix, 0, 1), fixedTranslateY);

				vgmMAT(matrix, 1, 2)
					+= vgmFIXED_MUL(vgmMAT(matrix, 1, 0), fixedTranslateX)
					+  vgmFIXED_MUL(vgmMAT(matrix, 1, 1), fixedTranslateY);

				vgmMAT(matrix, 2, 2)
					+= vgmFIXED_MUL(vgmMAT(matrix, 2, 0), fixedTranslateX)
					+  vgmFIXED_MUL(vgmMAT(matrix, 2, 1), fixedTranslateY);
			}
			else
			{
				vgmMAT(matrix, 0, 2)
					+= vgmFIXED_MUL(vgmMAT(matrix, 0, 0), fixedTranslateX)
					+  vgmFIXED_MUL(vgmMAT(matrix, 0, 1), fixedTranslateY);

				vgmMAT(matrix, 1, 2)
					+= vgmFIXED_MUL(vgmMAT(matrix, 1, 0), fixedTranslateX)
					+  vgmFIXED_MUL(vgmMAT(matrix, 1, 1), fixedTranslateY);
			}
		}

#else
		if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
		{
			vgmMAT(matrix, 0, 2)
				+= vgmMAT(matrix, 0, 0) * TranslateX
				+  vgmMAT(matrix, 0, 1) * TranslateY;

			vgmMAT(matrix, 1, 2)
				+= vgmMAT(matrix, 1, 0) * TranslateX
				+  vgmMAT(matrix, 1, 1) * TranslateY;

			vgmMAT(matrix, 2, 2)
				+= vgmMAT(matrix, 2, 0) * TranslateX
				+  vgmMAT(matrix, 2, 1) * TranslateY;
		}
		else
		{
			vgmMAT(matrix, 0, 2)
				+= vgmMAT(matrix, 0, 0) * TranslateX
				+  vgmMAT(matrix, 0, 1) * TranslateY;

			vgmMAT(matrix, 1, 2)
				+= vgmMAT(matrix, 1, 0) * TranslateX
				+  vgmMAT(matrix, 1, 1) * TranslateY;
		}
#endif
		/* Reset matrix. */
		vgfInvalidateContainer(Context, matrix);
	}
	vgmLEAVEAPI(vgTranslate);
}


/*******************************************************************************
**
** vgScale
**
** The vgScale function modifies the current transformation by appending
** a scale. This is equivalent to right-multiplying the current matrix M
** by a scale matrix:
**
**	            | ScaleX 0      0 |
**    M <-- M * | 0      ScaleY 0 |
**	            | 0      0      1 |
**
** INPUT:
**
**    ScaleX, ScaleY
**       Scale matrix values.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgScale(
	VGfloat ScaleX,
	VGfloat ScaleY
	)
{
	vgmENTERAPI(vgScale)
	{
		/* Create a shortcut to the matrix. */
		vgsMATRIXCONTAINER_PTR matrix = Context->matrix;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(%.10ff, %.10ff);\n",
			__FUNCTION__,
			ScaleX, ScaleY
			);

#if gcdENABLE_PERFORMANCE_PRIOR
		{
			gctFIXED fixedScaleX = vgmFLOAT_TO_FIXED_SPECIAL(ScaleX);
			gctFIXED fixedScaleY = vgmFLOAT_TO_FIXED_SPECIAL(ScaleY);


			if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
			{
				vgmMAT(matrix, 0, 0) = vgmFIXED_MUL(vgmMAT(matrix, 0, 0), fixedScaleX);
				vgmMAT(matrix, 0, 1) = vgmFIXED_MUL(vgmMAT(matrix, 0, 1), fixedScaleY);
				vgmMAT(matrix, 1, 0) = vgmFIXED_MUL(vgmMAT(matrix, 1, 0), fixedScaleX);
				vgmMAT(matrix, 1, 1) = vgmFIXED_MUL(vgmMAT(matrix, 1, 1), fixedScaleY);
				vgmMAT(matrix, 2, 0) = vgmFIXED_MUL(vgmMAT(matrix, 2, 0), fixedScaleX);
				vgmMAT(matrix, 2, 1) = vgmFIXED_MUL(vgmMAT(matrix, 2, 1), fixedScaleY);
			}
			else
			{
				vgmMAT(matrix, 0, 0) = vgmFIXED_MUL(vgmMAT(matrix, 0, 0), fixedScaleX);
				vgmMAT(matrix, 0, 1) = vgmFIXED_MUL(vgmMAT(matrix, 0, 1), fixedScaleY);
				vgmMAT(matrix, 1, 0) = vgmFIXED_MUL(vgmMAT(matrix, 1, 0), fixedScaleX);
				vgmMAT(matrix, 1, 1) = vgmFIXED_MUL(vgmMAT(matrix, 1, 1), fixedScaleY);
			}
		}
#else
		if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
		{
			vgmMAT(matrix, 0, 0) *= ScaleX;
			vgmMAT(matrix, 0, 1) *= ScaleY;
			vgmMAT(matrix, 1, 0) *= ScaleX;
			vgmMAT(matrix, 1, 1) *= ScaleY;
			vgmMAT(matrix, 2, 0) *= ScaleX;
			vgmMAT(matrix, 2, 1) *= ScaleY;
		}
		else
		{
			vgmMAT(matrix, 0, 0) *= ScaleX;
			vgmMAT(matrix, 0, 1) *= ScaleY;
			vgmMAT(matrix, 1, 0) *= ScaleX;
			vgmMAT(matrix, 1, 1) *= ScaleY;
		}
#endif
		/* Reset matrix. */
		vgfInvalidateContainer(Context, matrix);
	}
	vgmLEAVEAPI(vgScale);
}


/*******************************************************************************
**
** vgShear
**
** The vgShear function modifies the current transformation by appending
** a shear. This is equivalent to right-multiplying the current matrix M
** by a shear matrix:
**
**              | 1      ShearX 0 |
**    M <-- M * | ShearY 1      0 |
**              | 0      0      1 |
**
** INPUT:
**
**    ShearX, ShearY
**       Shear matrix values.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgShear(
	VGfloat ShearX,
	VGfloat ShearY
	)
{
	vgmENTERAPI(vgShear)
	{
		/* Create a shortcut to the matrix. */
		vgsMATRIXCONTAINER_PTR matrix = Context->matrix;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(%.10ff, %.10ff);\n",
			__FUNCTION__,
			ShearX, ShearY
			);

#if gcdENABLE_PERFORMANCE_PRIOR
		{
			gctFIXED fixedShearX = vgmFLOAT_TO_FIXED_SPECIAL(ShearX);
			gctFIXED fixedShearY = vgmFLOAT_TO_FIXED_SPECIAL(ShearY);

			if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
			{
				gctFIXED m00 = vgmMAT(matrix, 0, 0);
				gctFIXED m10 = vgmMAT(matrix, 1, 0);
				gctFIXED m20 = vgmMAT(matrix, 2, 0);

				vgmMAT(matrix, 0, 0)
					+= vgmFIXED_MUL(vgmMAT(matrix, 0, 1), fixedShearY);

				vgmMAT(matrix, 1, 0)
					+= vgmFIXED_MUL(vgmMAT(matrix, 1, 1), fixedShearY);

				vgmMAT(matrix, 2, 0)
					+= vgmFIXED_MUL(vgmMAT(matrix, 2, 1), fixedShearY);

				vgmMAT(matrix, 0, 1) += vgmFIXED_MUL(m00, fixedShearX);
				vgmMAT(matrix, 1, 1) += vgmFIXED_MUL(m10, fixedShearX);
				vgmMAT(matrix, 2, 1) += vgmFIXED_MUL(m20, fixedShearX);
			}
			else
			{
				gctFIXED m00 = vgmMAT(matrix, 0, 0);
				gctFIXED m10 = vgmMAT(matrix, 1, 0);

				vgmMAT(matrix, 0, 0)
					+= vgmFIXED_MUL(vgmMAT(matrix, 0, 1), fixedShearY);

				vgmMAT(matrix, 1, 0)
					+= vgmFIXED_MUL(vgmMAT(matrix, 1, 1), fixedShearY);

				vgmMAT(matrix, 0, 1) += vgmFIXED_MUL(m00, fixedShearX);
				vgmMAT(matrix, 1, 1) += vgmFIXED_MUL(m10, fixedShearX);
			}
		}
#else
		if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
		{
			VGfloat m00 = vgmMAT(matrix, 0, 0);
			VGfloat m10 = vgmMAT(matrix, 1, 0);
			VGfloat m20 = vgmMAT(matrix, 2, 0);

			vgmMAT(matrix, 0, 0)
				+= vgmMAT(matrix, 0, 1) * ShearY;

			vgmMAT(matrix, 1, 0)
				+= vgmMAT(matrix, 1, 1) * ShearY;

			vgmMAT(matrix, 2, 0)
				+= vgmMAT(matrix, 2, 1) * ShearY;

			vgmMAT(matrix, 0, 1) += m00 * ShearX;
			vgmMAT(matrix, 1, 1) += m10 * ShearX;
			vgmMAT(matrix, 2, 1) += m20 * ShearX;
		}
		else
		{
			VGfloat m00 = vgmMAT(matrix, 0, 0);
			VGfloat m10 = vgmMAT(matrix, 1, 0);

			vgmMAT(matrix, 0, 0)
				+= vgmMAT(matrix, 0, 1) * ShearY;

			vgmMAT(matrix, 1, 0)
				+= vgmMAT(matrix, 1, 1) * ShearY;

			vgmMAT(matrix, 0, 1) += m00 * ShearX;
			vgmMAT(matrix, 1, 1) += m10 * ShearX;
		}
#endif
		/* Reset matrix. */
		vgfInvalidateContainer(Context, matrix);
	}
	vgmLEAVEAPI(vgShear);
}


/*******************************************************************************
**
** vgRotate
**
** The vgRotate function modifies the current transformation by appending
** a counterclockwise rotation by a given angle (expressed in degrees) about
** the origin. This is equivalent to right-multiplying the current matrix M
** by the following matrix:
**
**              | cos(Angle) -sin(Angle) 0 |
**    M <-- M * | sin(Angle)  cos(Angle) 0 |
**              | 0           0          1 |
**
** To rotate about a center point (cx, cy) other than the origin, the
** application may perform a translation by (cx, cy), followed by
** the rotation, followed by a translation by (-cx, -cy).
**
** INPUT:
**
**    Angle
**       Rotation angle in degrees.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgRotate(
	VGfloat Angle
	)
{
	vgmENTERAPI(vgRotate)
	{
		/* Create a shortcut to the matrix. */
		vgsMATRIXCONTAINER_PTR matrix = Context->matrix;

#if gcdENABLE_PERFORMANCE_PRIOR
		/* Compute angle in radians. */
		gctFIXED radians = vgmFIXED_MUL(vgmFLOAT_TO_FIXED_SPECIAL(Angle), vgvFIXED_PI_DIV_180);

		/* Compute sine and cosine of the angle. */
		gctFIXED cosAngle = cosx(radians);
		gctFIXED sinAngle = sinx(radians);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(%.10ff);\n",
			__FUNCTION__,
			Angle
			);

		if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
		{
			gctFIXED m00 = vgmMAT(matrix, 0, 0);
			gctFIXED m10 = vgmMAT(matrix, 1, 0);
			gctFIXED m20 = vgmMAT(matrix, 2, 0);

			vgmMAT(matrix, 0, 0)
				= vgmFIXED_MUL(sinAngle, vgmMAT(matrix, 0, 1))
				+ vgmFIXED_MUL(cosAngle, m00);

			vgmMAT(matrix, 0, 1)
				= vgmFIXED_MUL(cosAngle, vgmMAT(matrix, 0, 1))
				- vgmFIXED_MUL(sinAngle, m00);

			vgmMAT(matrix, 1, 0)
				= vgmFIXED_MUL(sinAngle, vgmMAT(matrix, 1, 1))
				+ vgmFIXED_MUL(cosAngle, m10);

			vgmMAT(matrix, 1, 1)
				= vgmFIXED_MUL(cosAngle, vgmMAT(matrix, 1, 1))
				- vgmFIXED_MUL(sinAngle, m10);

			vgmMAT(matrix, 2, 0)
				= vgmFIXED_MUL(sinAngle, vgmMAT(matrix, 2, 1))
				+ vgmFIXED_MUL(cosAngle, m20);

			vgmMAT(matrix, 2, 1)
				= vgmFIXED_MUL(cosAngle, vgmMAT(matrix, 2, 1))
				- vgmFIXED_MUL(sinAngle, m20);
		}
		else
		{
			gctFIXED m00 = vgmMAT(matrix, 0, 0);
			gctFIXED m10 = vgmMAT(matrix, 1, 0);

			vgmMAT(matrix, 0, 0)
				= vgmFIXED_MUL(sinAngle, vgmMAT(matrix, 0, 1))
				+ vgmFIXED_MUL(cosAngle, m00);

			vgmMAT(matrix, 0, 1)
				= vgmFIXED_MUL(cosAngle, vgmMAT(matrix, 0, 1))
				- vgmFIXED_MUL(sinAngle, m00);

			vgmMAT(matrix, 1, 0)
				= vgmFIXED_MUL(sinAngle, vgmMAT(matrix, 1, 1))
				+ vgmFIXED_MUL(cosAngle, m10);

			vgmMAT(matrix, 1, 1)
				= vgmFIXED_MUL(cosAngle, vgmMAT(matrix, 1, 1))
				- vgmFIXED_MUL(sinAngle, m10);
		}
#else
		/* Compute angle in radians. */
		VGfloat radians = Angle * vgvPI / 180.0f;

		/* Compute sine and cosine of the angle. */
		VGfloat cosAngle = gcmCOSF(radians);
		VGfloat sinAngle = gcmSINF(radians);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(%.10ff);\n",
			__FUNCTION__,
			Angle
			);

		if (Context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE)
		{
			VGfloat m00 = vgmMAT(matrix, 0, 0);
			VGfloat m10 = vgmMAT(matrix, 1, 0);
			VGfloat m20 = vgmMAT(matrix, 2, 0);

			vgmMAT(matrix, 0, 0)
				= sinAngle * vgmMAT(matrix, 0, 1)
				+ cosAngle * m00;

			vgmMAT(matrix, 0, 1)
				= cosAngle * vgmMAT(matrix, 0, 1)
				- sinAngle * m00;

			vgmMAT(matrix, 1, 0)
				= sinAngle * vgmMAT(matrix, 1, 1)
				+ cosAngle * m10;

			vgmMAT(matrix, 1, 1)
				= cosAngle * vgmMAT(matrix, 1, 1)
				- sinAngle * m10;

			vgmMAT(matrix, 2, 0)
				= sinAngle * vgmMAT(matrix, 2, 1)
				+ cosAngle * m20;

			vgmMAT(matrix, 2, 1)
				= cosAngle * vgmMAT(matrix, 2, 1)
				- sinAngle * m20;
		}
		else
		{
			VGfloat m00 = vgmMAT(matrix, 0, 0);
			VGfloat m10 = vgmMAT(matrix, 1, 0);

			vgmMAT(matrix, 0, 0)
				= sinAngle * vgmMAT(matrix, 0, 1)
				+ cosAngle * m00;

			vgmMAT(matrix, 0, 1)
				= cosAngle * vgmMAT(matrix, 0, 1)
				- sinAngle * m00;

			vgmMAT(matrix, 1, 0)
				= sinAngle * vgmMAT(matrix, 1, 1)
				+ cosAngle * m10;

			vgmMAT(matrix, 1, 1)
				= cosAngle * vgmMAT(matrix, 1, 1)
				- sinAngle * m10;
		}
#endif
		/* Reset matrix. */
		vgfInvalidateContainer(Context, matrix);
	}
	vgmLEAVEAPI(vgRotate);
}
