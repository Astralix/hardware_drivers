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


#define  WINVER			0x0600
#define _WIN32_WINNT	0x0600
#define _WIN32_WINDOWS	0x0410
#define _WIN32_IE		0x0700

#include <stdio.h>
#include <tchar.h>
#include <afx.h>
#include <math.h>
#include "Generator.h"
#include "Formats.h"

						static gctSTRING readHeader =
							"static void " vgvPIXEL_READ "(\n"
							"\t"	"vgsPIXELWALKER_PTR Pixel,\n"
							"\t"	"vgtFLOATVECTOR4 Value\n"
							"\t"	")\n"
							"{\n";


						void _GenerateLuminanceReader(
							CStdioFile& File,
							vgsFORMAT_INFO_PTR SourceFormat,
							gctBOOL TargetLinear,
							gctBOOL TargetPremultiplied
							)
						{
							gctBOOL sourceBitAccess = (SourceFormat->l.width <  8);
							gctBOOL source1Bit      = (SourceFormat->l.width == 1);

							gctSTRING converter;

							if (SourceFormat->linear == TargetLinear)
							{
								converter = "vgmZERO2ONE";
							}
							else if (SourceFormat->linear)
							{
								converter = "vgmZERO2ONE_L2S";
							}
							else
							{
								converter = "vgmZERO2ONE_S2L";
							}


							/*****************************************************************************************************
							** Define local variables.
							*/

vgmPRINTF(vgvTAB,			"register gctUINT8 lINT;"																			);
vgmPRINTLINE("",																												);


							/*****************************************************************************************************
							** Read the current buffer value.
							*/

							if (sourceBitAccess)
							{
								/* Determine the last pixel position. */
								gctUINT lastPixelOffset = 8 - SourceFormat->l.width;

								if (source1Bit)
								{
vgmPRINTCMNT(vgvTAB,				 Get the pixel value.																		);
vgmPRINTLINE(vgvTAB,				 lINT   = * Pixel->current;																	);
vgmPRINTLINE(vgvTAB,				 lINT >>=   Pixel->bitOffset;																);
vgmPRINTLINE(vgvTAB,				 lINT  &=   0x01;																			);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				 Last pixel within the byte?																);
vgmPRINTF(vgvTAB,					"if (Pixel->bitOffset == %d)", lastPixelOffset												);
vgmPRINTLINE(vgvTAB,				 {																							);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Reset the bit offset. 																	);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->bitOffset = 0;																	);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Advance to the next byte position.														);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->current += 1;																	);
vgmPRINTLINE(vgvTAB,				 }																							);
vgmPRINTLINE(vgvTAB,				 else																						);
vgmPRINTLINE(vgvTAB,				 {																							);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Advance to the next bit position. 														);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->bitOffset += 1;																	);
vgmPRINTLINE(vgvTAB,				 }																							);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				 Convert to 0..1 range.																		);
vgmPRINTLINE(vgvTAB,				 Value[3] = 1.0f;																			);
vgmPRINTLINE(vgvTAB,				 Value[2] =																					);
vgmPRINTLINE(vgvTAB,				 Value[1] =																					);
vgmPRINTLINE(vgvTAB,				 Value[0] = (lINT == 0) ? 0.0f : 1.0f;														);
								}
								else
								{
vgmPRINTCMNT(vgvTAB,				 Get the pixel value.																		);
vgmPRINTLINE(vgvTAB,				 lINT   = * Pixel->current;																	);
vgmPRINTLINE(vgvTAB,				 lINT >>=   Pixel->bitOffset;																);
vgmPRINTF(vgvTAB,					"lINT  &=   0x%02X;", SourceFormat->l.mask													);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				 Last pixel within the byte?																);
vgmPRINTF(vgvTAB,					"if (Pixel->bitOffset == %d)", lastPixelOffset												);
vgmPRINTLINE(vgvTAB,				 {																							);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Reset the bit offset. 																	);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->bitOffset = 0;																	);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Advance to the next byte position.														);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->current += 1;																	);
vgmPRINTLINE(vgvTAB,				 }																							);
vgmPRINTLINE(vgvTAB,				 else																						);
vgmPRINTLINE(vgvTAB,				 {																							);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Advance to the next bit position. 														);
vgmPRINTF(vgvTAB vgvTAB,				"Pixel->bitOffset += %d;", SourceFormat->l.width										);
vgmPRINTLINE(vgvTAB,				 }																							);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				 Convert to 0..1 range.																		);
vgmPRINTLINE(vgvTAB,				 Value[3] = 1.0f;																			);
vgmPRINTLINE(vgvTAB,				 Value[2] =																					);
vgmPRINTLINE(vgvTAB,				 Value[1] =																					);
vgmPRINTF(vgvTAB,					"Value[0] = %s(lINT, %d);", converter, SourceFormat->l.width								);
								}
							}
							else
							{
vgmPRINTCMNT(vgvTAB,			 Get the pixel value.																			);
vgmPRINTLINE(vgvTAB,			 lINT = * Pixel->current;																		);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			 Advance to the next pixel.																		);
vgmPRINTLINE(vgvTAB,			 Pixel->current += 1;																			);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			 Convert to 0..1 range.																			);
vgmPRINTLINE(vgvTAB,			 Value[3] = 1.0f;																				);
vgmPRINTLINE(vgvTAB,			 Value[2] =																						);
vgmPRINTLINE(vgvTAB,			 Value[1] =																						);
vgmPRINTF(vgvTAB,				"Value[0] = %s(lINT, %d);", converter, SourceFormat->l.width									);
							}
						}

						void _GenerateAlphaReader(
							CStdioFile& File,
							vgsFORMAT_INFO_PTR SourceFormat,
							gctBOOL TargetLinear,
							gctBOOL TargetPremultiplied
							)
						{
							gctBOOL sourceBitAccess = (SourceFormat->a.width <  8);
							gctBOOL source1Bit      = (SourceFormat->a.width == 1);


							/*****************************************************************************************************
							** Define local variables.
							*/

vgmPRINTF(vgvTAB,			"register gctUINT8 aINT;"																			);
vgmPRINTLINE("",																												);


							/*****************************************************************************************************
							** Read the current buffer value.
							*/

							if (sourceBitAccess)
							{
								/* Determine the last pixel position. */
								gctUINT lastPixelOffset = 8 - SourceFormat->a.width;

								if (source1Bit)
								{
vgmPRINTCMNT(vgvTAB,				 Get the pixel value.																		);
vgmPRINTLINE(vgvTAB,				 aINT   = * Pixel->current;																	);
vgmPRINTLINE(vgvTAB,				 aINT >>=   Pixel->bitOffset;																);
vgmPRINTLINE(vgvTAB,				 aINT  &=   0x01;																			);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				 Last pixel within the byte?																);
vgmPRINTF(vgvTAB,					"if (Pixel->bitOffset == %d)", lastPixelOffset												);
vgmPRINTLINE(vgvTAB,				 {																							);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Reset the bit offset. 																	);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->bitOffset = 0;																	);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Advance to the next byte position.														);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->current += 1;																	);
vgmPRINTLINE(vgvTAB,				 }																							);
vgmPRINTLINE(vgvTAB,				 else																						);
vgmPRINTLINE(vgvTAB,				 {																							);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Advance to the next bit position. 														);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->bitOffset += 1;																	);
vgmPRINTLINE(vgvTAB,				 }																							);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				 Convert to 0..1 range.																		);
									 if (TargetPremultiplied)
									 {
vgmPRINTLINE(vgvTAB,					 Value[3] =																				);
vgmPRINTLINE(vgvTAB,					 Value[2] =																				);
vgmPRINTLINE(vgvTAB,					 Value[1] =																				);
vgmPRINTLINE(vgvTAB,					 Value[0] = (aINT == 0) ? 0.0f : 1.0f;													);
									 }
									 else
									 {
vgmPRINTLINE(vgvTAB,					 Value[3] = (aINT == 0) ? 0.0f : 1.0f;													);
vgmPRINTLINE(vgvTAB,					 Value[2] = 1.0f;																		);
vgmPRINTLINE(vgvTAB,					 Value[1] = 1.0f;																		);
vgmPRINTLINE(vgvTAB,					 Value[0] = 1.0f;																		);
									 }
								}
								else
								{
vgmPRINTCMNT(vgvTAB,				 Get the pixel value.																		);
vgmPRINTLINE(vgvTAB,				 aINT   = * Pixel->current;																	);
vgmPRINTLINE(vgvTAB,				 aINT >>=   Pixel->bitOffset;																);
vgmPRINTF(vgvTAB,					"aINT  &=   0x%02X;", SourceFormat->a.mask													);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				 Last pixel within the byte?																);
vgmPRINTF(vgvTAB,					"if (Pixel->bitOffset == %d)", lastPixelOffset												);
vgmPRINTLINE(vgvTAB,				 {																							);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Reset the bit offset. 																	);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->bitOffset = 0;																	);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Advance to the next byte position.														);
vgmPRINTLINE(vgvTAB vgvTAB,				 Pixel->current += 1;																	);
vgmPRINTLINE(vgvTAB,				 }																							);
vgmPRINTLINE(vgvTAB,				 else																						);
vgmPRINTLINE(vgvTAB,				 {																							);
vgmPRINTCMNT(vgvTAB vgvTAB,				 Advance to the next bit position. 														);
vgmPRINTF(vgvTAB vgvTAB,				"Pixel->bitOffset += %d;", SourceFormat->a.width										);
vgmPRINTLINE(vgvTAB,				 }																							);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				 Convert to 0..1 range.																		);
									 if (TargetPremultiplied)
									 {
vgmPRINTLINE(vgvTAB,					 Value[3] = 																			);
vgmPRINTLINE(vgvTAB,					 Value[2] =																				);
vgmPRINTLINE(vgvTAB,					 Value[1] =																				);
vgmPRINTF(vgvTAB,						"Value[0] = vgmZERO2ONE(aINT, %d);", SourceFormat->a.width								);
									 }
									 else
									 {
vgmPRINTF(vgvTAB,						"Value[3] = vgmZERO2ONE(aINT, %d);", SourceFormat->a.width								);
vgmPRINTLINE(vgvTAB,					 Value[2] = 1.0f;																		);
vgmPRINTLINE(vgvTAB,					 Value[1] = 1.0f;																		);
vgmPRINTLINE(vgvTAB,					 Value[0] = 1.0f;																		);
									 }
								}
							}
							else
							{
vgmPRINTCMNT(vgvTAB,			 Get the pixel value.																			);
vgmPRINTLINE(vgvTAB,			 aINT = * Pixel->current;																		);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			 Advance to the next pixel.																		);
vgmPRINTLINE(vgvTAB,			 Pixel->current += 1;																			);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			 Convert to 0..1 range.																			);
								 if (TargetPremultiplied)
								 {
vgmPRINTLINE(vgvTAB,				 Value[3] = 																				);
vgmPRINTLINE(vgvTAB,				 Value[2] =																					);
vgmPRINTLINE(vgvTAB,				 Value[1] =																					);
vgmPRINTF(vgvTAB,					"Value[0] = vgmZERO2ONE(aINT, %d);", SourceFormat->a.width									);
								 }
								 else
								 {
vgmPRINTF(vgvTAB,					"Value[3] = vgmZERO2ONE(aINT, %d);", SourceFormat->a.width									);
vgmPRINTLINE(vgvTAB,				 Value[2] = 1.0f;																			);
vgmPRINTLINE(vgvTAB,				 Value[1] = 1.0f;																			);
vgmPRINTLINE(vgvTAB,				 Value[0] = 1.0f;																			);
								 }
							}
						}

						void _GenerateRGBReader(
							CStdioFile& File,
							vgsFORMAT_INFO_PTR SourceFormat,
							gctBOOL TargetLinear,
							gctBOOL TargetPremultiplied
							)
						{
							gctSTRING converter;

							if (SourceFormat->linear == TargetLinear)
							{
								converter = "vgmZERO2ONE";
							}
							else if (SourceFormat->linear)
							{
								converter = "vgmZERO2ONE_L2S";
							}
							else
							{
								converter = "vgmZERO2ONE_S2L";
							}


							/*****************************************************************************************************
							** Define local variables.
							*/

vgmPRINTF(vgvTAB,			"register gctUINT%d value;", SourceFormat->bitsPerPixel												);
vgmPRINTF(vgvTAB,			"register gctUINT8 bINT, gINT, rINT;"																);
vgmPRINTLINE("",																												);


							/*****************************************************************************************************
							** Read the current buffer value.
							*/

vgmPRINTCMNT(vgvTAB,		 Get the pixel value.																				);
vgmPRINTF(vgvTAB,			"value = * (gctUINT%d_PTR) Pixel->current;", SourceFormat->bitsPerPixel								);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,		 Advance to the next pixel.																			);
vgmPRINTF(vgvTAB,			"Pixel->current += %d;", SourceFormat->bitsPerPixel / 8												);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,		Extract components.																					);

							if (SourceFormat->b.start == 0)
							{
vgmPRINTF(vgvTAB,				"bINT =  value        & 0x%02X;",     SourceFormat->b.mask										);
							}
							else
							{
vgmPRINTF(vgvTAB,				"bINT = (value >> %*d) & 0x%02X;", 2, SourceFormat->b.start, SourceFormat->b.mask				);
							}

							if (SourceFormat->g.start == 0)
							{
vgmPRINTF(vgvTAB,				"gINT =  value        & 0x%02X;",     SourceFormat->g.mask										);
							}
							else
							{
vgmPRINTF(vgvTAB,				"gINT = (value >> %*d) & 0x%02X;", 2, SourceFormat->g.start, SourceFormat->g.mask				);
							}

							if (SourceFormat->r.start == 0)
							{
vgmPRINTF(vgvTAB,				"rINT =  value        & 0x%02X;",     SourceFormat->r.mask										);
							}
							else
							{
vgmPRINTF(vgvTAB,				"rINT = (value >> %*d) & 0x%02X;", 2, SourceFormat->r.start, SourceFormat->r.mask				);
							}
vgmPRINTLINE("",																												);
vgmPRINTF(vgvTAB,			"/* Convert to 0..1 range. */"																		);
vgmPRINTF(vgvTAB,			"Value[3] = 1.0f;"																					);
vgmPRINTF(vgvTAB,			"Value[2] = %s(bINT, %d);", converter, SourceFormat->b.width										);
vgmPRINTF(vgvTAB,			"Value[1] = %s(gINT, %d);", converter, SourceFormat->g.width										);
vgmPRINTF(vgvTAB,			"Value[0] = %s(rINT, %d);", converter, SourceFormat->r.width										);
						}

						void _GenerateRGBAReader(
							CStdioFile& File,
							vgsFORMAT_INFO_PTR SourceFormat,
							gctBOOL TargetLinear,
							gctBOOL TargetPremultiplied
							)
						{
							gctSTRING extraTab = "";

							gctBOOL unpremultiply
								= SourceFormat->premultiplied
								&& (!TargetPremultiplied || (SourceFormat->linear != TargetLinear));

							gctBOOL premultiply
								= TargetPremultiplied
								&& (!SourceFormat->premultiplied || unpremultiply);

							gctSTRING converterAlpha;
							gctSTRING converterColor;

							if ((SourceFormat->linear == TargetLinear) || unpremultiply)
							{
								converterAlpha = "vgmZERO2ONE";
								converterColor = "vgmZERO2ONE";
							}
							else if (SourceFormat->linear)
							{
								converterAlpha = "vgmZERO2ONE    ";
								converterColor = "vgmZERO2ONE_L2S";
							}
							else
							{
								converterAlpha = "vgmZERO2ONE    ";
								converterColor = "vgmZERO2ONE_S2L";
							}


							/*****************************************************************************************************
							** Define local variables.
							*/

vgmPRINTF(vgvTAB,			"register gctUINT%d value;", SourceFormat->bitsPerPixel												);
vgmPRINTF(vgvTAB,			"register gctUINT8 aINT, bINT, gINT, rINT;"															);
vgmPRINTLINE("",																												);


							/*****************************************************************************************************
							** Read the current buffer value.
							*/

vgmPRINTCMNT(vgvTAB,		 Get the pixel value.																				);
vgmPRINTF(vgvTAB,			"value = * (gctUINT%d_PTR) Pixel->current;", SourceFormat->bitsPerPixel								);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,		 Advance to the next pixel.																			);
vgmPRINTF(vgvTAB,			"Pixel->current += %d;", SourceFormat->bitsPerPixel / 8												);
vgmPRINTLINE("",																												);
							if (SourceFormat->premultiplied)
							{
								extraTab = "\t";

vgmPRINTCMNT(vgvTAB,			Extract the alpha component.																	);

								if (SourceFormat->a.start == 0)
								{
vgmPRINTF(vgvTAB,					"aINT = value & 0x%02X;", SourceFormat->a.mask												);
								}
								else
								{
vgmPRINTF(vgvTAB,					"aINT = (value >> %d) & 0x%02X;", SourceFormat->a.start, SourceFormat->a.mask				);
								}
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			Zero?																							);
vgmPRINTLINE(vgvTAB,			if (aINT == 0)																					);
vgmPRINTLINE(vgvTAB,			{																								);
vgmPRINTLINE(vgvTAB vgvTAB,			Value[3] =																					);
vgmPRINTLINE(vgvTAB vgvTAB,			Value[2] =																					);
vgmPRINTLINE(vgvTAB vgvTAB,			Value[1] =																					);
vgmPRINTLINE(vgvTAB vgvTAB,			Value[0] = 0.0f;																			);
vgmPRINTLINE(vgvTAB,			}																								);
vgmPRINTLINE(vgvTAB,			else																							);
vgmPRINTLINE(vgvTAB,			{																								);
vgmPRINTCMNT(vgvTAB vgvTAB,			Extract components.																			);
							}
							else
							{
vgmPRINTCMNT(vgvTAB,			Extract components.																				);

								if (SourceFormat->a.start == 0)
								{
vgmPRINTF(vgvTAB,					"aINT =  value        & 0x%02X;", SourceFormat->a.mask										);
								}
								else
								{
vgmPRINTF(vgvTAB,					"aINT = (value >> %*d) & 0x%02X;", 2, SourceFormat->a.start, SourceFormat->a.mask			);
								}
							}

							if (SourceFormat->b.start == 0)
							{
vgmPRINTF(vgvTAB,				"%sbINT =  value        & 0x%02X;",  extraTab,    SourceFormat->b.mask							);
							}
							else
							{
vgmPRINTF(vgvTAB,				"%sbINT = (value >> %*d) & 0x%02X;", extraTab, 2, SourceFormat->b.start, SourceFormat->b.mask	);
							}

							if (SourceFormat->g.start == 0)
							{
vgmPRINTF(vgvTAB,				"%sgINT =  value        & 0x%02X;",  extraTab,    SourceFormat->g.mask							);
							}
							else
							{
vgmPRINTF(vgvTAB,				"%sgINT = (value >> %*d) & 0x%02X;", extraTab, 2, SourceFormat->g.start, SourceFormat->g.mask	);
							}

							if (SourceFormat->r.start == 0)
							{
vgmPRINTF(vgvTAB,				"%srINT =  value        & 0x%02X;",  extraTab,    SourceFormat->r.mask							);
							}
							else
							{
vgmPRINTF(vgvTAB,				"%srINT = (value >> %*d) & 0x%02X;", extraTab, 2, SourceFormat->r.start, SourceFormat->r.mask	);
							}

							if (SourceFormat->premultiplied)
							{
vgmPRINTLINE("",																												);
vgmPRINTF(vgvTAB,				"%s/* Clamp colors. */", extraTab																);
vgmPRINTF(vgvTAB,				"%sbINT = gcmMIN(bINT, aINT);", extraTab														);
vgmPRINTF(vgvTAB,				"%sgINT = gcmMIN(gINT, aINT);", extraTab														);
vgmPRINTF(vgvTAB,				"%srINT = gcmMIN(rINT, aINT);", extraTab														);
							}

vgmPRINTLINE("",																												);
vgmPRINTF(vgvTAB,			"%s/* Convert to 0..1 range. */", extraTab															);
vgmPRINTF(vgvTAB,			"%sValue[3] = %s(aINT, %d);", extraTab, converterAlpha, SourceFormat->a.width						);
vgmPRINTF(vgvTAB,			"%sValue[2] = %s(bINT, %d);", extraTab, converterColor, SourceFormat->b.width						);
vgmPRINTF(vgvTAB,			"%sValue[1] = %s(gINT, %d);", extraTab, converterColor, SourceFormat->g.width						);
vgmPRINTF(vgvTAB,			"%sValue[0] = %s(rINT, %d);", extraTab, converterColor, SourceFormat->r.width						);

							if (unpremultiply)
							{
vgmPRINTLINE("",																												);
vgmPRINTF(vgvTAB,				"%s/* Unpremultiply. */", extraTab																);
vgmPRINTF(vgvTAB,				"%sValue[2] /= Value[3];", extraTab																);
vgmPRINTF(vgvTAB,				"%sValue[1] /= Value[3];", extraTab																);
vgmPRINTF(vgvTAB,				"%sValue[0] /= Value[3];", extraTab																);

								if (SourceFormat->linear != TargetLinear)
								{
									if (SourceFormat->linear)
									{
vgmPRINTLINE("",																												);
vgmPRINTF(vgvTAB,						"%s/* Convert to non-linear color space. */", extraTab									);
vgmPRINTF(vgvTAB,						"%sValue[2] = vgfGetColorGamma(Value[2]);", extraTab									);
vgmPRINTF(vgvTAB,						"%sValue[1] = vgfGetColorGamma(Value[1]);", extraTab									);
vgmPRINTF(vgvTAB,						"%sValue[0] = vgfGetColorGamma(Value[0]);", extraTab									);
									}
									else
									{
vgmPRINTLINE("",																												);
vgmPRINTF(vgvTAB,						"%s/* Convert to linear color space. */", extraTab										);
vgmPRINTF(vgvTAB,						"%sValue[2] = vgfGetColorInverseGamma(Value[2]);", extraTab								);
vgmPRINTF(vgvTAB,						"%sValue[1] = vgfGetColorInverseGamma(Value[1]);", extraTab								);
vgmPRINTF(vgvTAB,						"%sValue[0] = vgfGetColorInverseGamma(Value[0]);", extraTab								);
									}
								}
							}

							if (premultiply)
							{
vgmPRINTLINE("",																												);
vgmPRINTF(vgvTAB,				"%s/* Premultiply. */", extraTab																);
vgmPRINTF(vgvTAB,				"%sValue[2] *= Value[3];", extraTab																);
vgmPRINTF(vgvTAB,				"%sValue[1] *= Value[3];", extraTab																);
vgmPRINTF(vgvTAB,				"%sValue[0] *= Value[3];", extraTab																);
							}

							if (SourceFormat->premultiplied)
							{
vgmPRINTLINE(vgvTAB,			}																								);
							}
						}

void GenerateReaders(
	CStdioFile& File
	)
{
	gctBOOL firstFormat = gcvTRUE;
	vgsFORMAT_LIST_PTR formatGroups;
	gctUINT groupCount;

	/* Get format groups. */
	vgfGetFormatGroupList(&formatGroups, &groupCount);

	/* Go through all swizzle groups. */
	for (gctUINT currentGroup = 0; currentGroup < groupCount; currentGroup += 1)
	{
		/* Set initial generation flags. */
		gctBOOL haveL8      = gcvFALSE;
		gctBOOL haveRGB565  = gcvFALSE;
		gctBOOL haveRGB8888 = gcvFALSE;

		/* Get the number of formats in the group. */
		gctUINT formatCount = formatGroups[currentGroup].count;

		/* Go through all formats in the swizzle group. */
		for (gctUINT currentFormat = 0; currentFormat < formatCount; currentFormat += 1)
		{
			gctBOOL firstFunction = gcvTRUE;

			/* Get the current format. */
			vgsFORMAT_INFO_PTR format = &formatGroups[currentGroup].group[currentFormat];

			/* Valid format? */
			if (format->name == gcvNULL)
			{
				continue;
			}

			/* Add space. */
			if (!firstFormat)
			{
				vgfWriteLine(File, "\n");
				vgfWriteLine(File, "\n");
			}

			firstFormat = gcvFALSE;

			/* Write the format title. */
			{
				CString formatName;
				formatName.Format(_T("VG_%s%s"), format->name, format->nativeFormat? "" : "_VIV");
				vgfGenerateCaption(File, formatName);
			}

			/* Go through all color spaces. */
			for (gctUINT currentColorSpace = 0; currentColorSpace < gcmCOUNTOF(colorSpaceArray); currentColorSpace += 1)
			{
				/* Go through all color spaces. */
				for (gctUINT currentPremul = 0; currentPremul < gcmCOUNTOF(premulArray); currentPremul += 1)
				{
					/* Luminance? */
					gctBOOL luminance
						= (format->l.width != 0);

					/* Alpha-only? */
					gctBOOL alpha
						=  (format->a.width != 0)
						&& (format->r.width == 0)
						&& (format->g.width == 0)
						&& (format->b.width == 0);

					/* RGB? */
					gctBOOL rgb
						=  (format->a.width == 0)
						&& (format->r.width != 0)
						&& (format->g.width != 0)
						&& (format->b.width != 0);

					if (luminance)
					{
						CString formatName(format->name);
						CString targetColorSpace(colorSpaceArray[currentColorSpace]);

						if (currentPremul == 1)
						{
							/* Premultiplied and not premultiplied functions are the same. */
							continue;
						}

						/* Special optimization for 8-bit luminance. */
						if ((format->l.width == 8) && (format->linear == currentColorSpace))
						{
							/* The case is already generated? */
							if (haveL8)
							{
								continue;
							}

							/* Mark as generated. */
							haveL8 = gcvTRUE;

							/* Make (s --> s) and (l --> l) cases the same. */
							formatName.Delete(0, 1);
							targetColorSpace.Empty();
						}

						if (!firstFunction)
						{
							vgfWriteLine(File, "\n");
						}

						firstFunction = gcvFALSE;

						vgfWriteLine(
							File, readHeader,
							(LPCTSTR) formatName,
							(LPCTSTR) targetColorSpace, premulArray[currentPremul]
							);

						_GenerateLuminanceReader(
							File, format, currentColorSpace, currentPremul
							);

						vgfWriteLine(File, "}\n");
					}

					else if (alpha)
					{
						if (currentColorSpace == 0)
						{
							/* No conversion is needed for alpha formats. */
							continue;
						}

						if (!firstFunction)
						{
							vgfWriteLine(File, "\n");
						}

						firstFunction = gcvFALSE;

						vgfWriteLine(
							File, readHeader,
							format->name,
							"", premulArray[currentPremul]
							);

						_GenerateAlphaReader(
							File, format, currentColorSpace, currentPremul
							);

						vgfWriteLine(File, "}\n");
					}

					else if (rgb)
					{
						CString formatName(format->name);
						CString targetColorSpace(colorSpaceArray[currentColorSpace]);

						if (currentPremul == 1)
						{
							/* Premultiplied and not premultiplied functions are the same. */
							continue;
						}

						/* Special optimization for RGB8888 formats. */
						if ((format->r.width == 8) && (format->linear == currentColorSpace))
						{
							/* The case is already generated? */
							if (haveRGB8888)
							{
								continue;
							}

							/* Mark as generated. */
							haveRGB8888 = gcvTRUE;

							/* Make (s --> s) and (l --> l) cases the same. */
							formatName.Delete(0, 1);
							targetColorSpace.Empty();
						}

						/* Special optimization for RGB565 formats. */
						if ((format->r.width == 5) && (format->linear == currentColorSpace))
						{
							/* The case is already generated? */
							if (haveRGB565)
							{
								continue;
							}

							/* Mark as generated. */
							haveRGB565 = gcvTRUE;

							/* Make (s --> s) and (l --> l) cases the same. */
							formatName.Delete(0, 1);
							targetColorSpace.Empty();
						}

						if (!firstFunction)
						{
							vgfWriteLine(File, "\n");
						}

						firstFunction = gcvFALSE;

						vgfWriteLine(
							File, readHeader,
							(LPCTSTR) formatName,
							(LPCTSTR) targetColorSpace, premulArray[currentPremul]
							);

						_GenerateRGBReader(
							File, format, currentColorSpace, currentPremul
							);

						vgfWriteLine(File, "}\n");
					}

					else
					{
						if (!firstFunction)
						{
							vgfWriteLine(File, "\n");
						}

						firstFunction = gcvFALSE;

						vgfWriteLine(
							File, readHeader,
							format->name,
							colorSpaceArray[currentColorSpace], premulArray[currentPremul]
							);

						_GenerateRGBAReader(
							File, format, currentColorSpace, currentPremul
							);

						vgfWriteLine(File, "}\n");
					}
				}
			}
		}
	}
}
