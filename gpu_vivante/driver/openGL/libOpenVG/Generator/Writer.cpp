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

						static gctSTRING fullWriteHeader =
							"%s"
							"static void " vgvFULL_PIXEL_WRITE "(\n"
							"\t"	"vgsPIXELWALKER_PTR Pixel,\n"
							"\t"	"vgtFLOATVECTOR4 Value,\n"
							"\t"	"gctUINT ChannelMask\n"
							"\t"	")\n"
							"{\n";

						static gctSTRING maskedWriteHeader =
							"%s"
							"static void " vgvMASKED_PIXEL_WRITE "(\n"
							"\t"	"vgsPIXELWALKER_PTR Pixel,\n"
							"\t"	"vgtFLOATVECTOR4 Value,\n"
							"\t"	"gctUINT ChannelMask\n"
							"\t"	")\n"
							"{\n";


						/*********************************************************************************************************
						** Dummy definitions to make _WritePixel compilable.
						*/

						typedef float vgtFLOATVECTOR4[4];
						vgtFLOATVECTOR4 Value;


						static void _GenerateLuminanceWriter(
							CStdioFile& File,
							vgsFORMAT_INFO_PTR TargetFormat,
							gctBOOL InputLinear,
							gctBOOL InputPremultiplied,
							gctSTRING PackMacroName
							)
						{
							gctBOOL targetBitAccess = (TargetFormat->l.width < 8);


							/*****************************************************************************************************
							** Define local variables.
							*/

							CString localVars;

							if (targetBitAccess)
							{
vgmPRINTF(vgvTAB,				"register gctUINT8 value;"																		);
vgmPRINTF(vgvTAB,				"register gctUINT8 mask;"																		);
							}
							else
							{
vgmPRINTF(vgvTAB,				"register gctUINT%d value;", TargetFormat->bitsPerPixel											);
							}

							if (InputPremultiplied)
							{
vgmPRINTF(vgvTAB,				"gctFLOAT trgA, trgB, trgG, trgR;"																);
							}
							else
							{
vgmPRINTF(vgvTAB,				"gctFLOAT trgB, trgG, trgR;"																	);
							}

							if (!InputLinear)
							{
vgmPRINTF(vgvTAB,				"gctFLOAT tmpB, tmpG, tmpR;"																	);
							}

							if (TargetFormat->linear)
							{
vgmPRINTF(vgvTAB,				"gctFLOAT trgL;"																				);
							}
							else
							{
vgmPRINTF(vgvTAB,				"gctFLOAT trgL, grayScale;"																		);
							}

vgmPRINTLINE("",																												);


							/*****************************************************************************************************
							** Clamp the incoming color.
							*/

							if (InputPremultiplied)
							{
vgmPRINTCMNT(vgvTAB,			Clamp the incoming color.																		);
vgmPRINTLINE(vgvTAB,			trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);															);
vgmPRINTLINE(vgvTAB,			trgB = gcmCLAMP(Value[2], 0.0f, trgA);															);
vgmPRINTLINE(vgvTAB,			trgG = gcmCLAMP(Value[1], 0.0f, trgA);															);
vgmPRINTLINE(vgvTAB,			trgR = gcmCLAMP(Value[0], 0.0f, trgA);															);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			Unpremultiply.																					);
vgmPRINTLINE(vgvTAB,			if (trgA == 0.0f)																				);
vgmPRINTLINE(vgvTAB,			{																								);
vgmPRINTLINE(vgvTAB vgvTAB,			trgB = 0.0f;																				);
vgmPRINTLINE(vgvTAB vgvTAB,			trgG = 0.0f;																				);
vgmPRINTLINE(vgvTAB vgvTAB,			trgR = 0.0f;																				);
vgmPRINTLINE(vgvTAB,			}																								);
vgmPRINTLINE(vgvTAB,			else																							);
vgmPRINTLINE(vgvTAB,			{																								);
vgmPRINTLINE(vgvTAB vgvTAB,			trgB /= trgA;																				);
vgmPRINTLINE(vgvTAB vgvTAB,			trgG /= trgA;																				);
vgmPRINTLINE(vgvTAB vgvTAB,			trgR /= trgA;																				);
vgmPRINTLINE(vgvTAB,			}																								);
							}
							else
							{
vgmPRINTCMNT(vgvTAB,			Clamp the incoming color.																		);
vgmPRINTLINE(vgvTAB,			trgB = gcmCLAMP(Value[2], 0.0f, 1.0f);															);
vgmPRINTLINE(vgvTAB,			trgG = gcmCLAMP(Value[1], 0.0f, 1.0f);															);
vgmPRINTLINE(vgvTAB,			trgR = gcmCLAMP(Value[0], 0.0f, 1.0f);															);
							}
vgmPRINTLINE("",																												);


							/*****************************************************************************************************
							** Convert the incoming color.
							*/

							if (InputLinear)
							{
								if (TargetFormat->linear)
								{
									/* lRGBA --> lL */

vgmPRINTCMNT(vgvTAB,				Compute the grayscale value.																);
vgmPRINTLINE(vgvTAB,				trgL = vgfGetGrayScale(trgR, trgG, trgB);													);
vgmPRINTLINE("",																												);
								}
								else
								{
									/* lRGBA --> sL */

vgmPRINTCMNT(vgvTAB,				Compute the grayscale value.																);
vgmPRINTLINE(vgvTAB,				grayScale = vgfGetGrayScale(trgR, trgG, trgB);												);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				Convert to non-linear color space.															);
vgmPRINTLINE(vgvTAB,				trgL = vgfGetColorGamma(grayScale);															);
vgmPRINTLINE("",																												);
								}
							}

							/* Input is not linear. */
							else
							{
								if (TargetFormat->linear)
								{
									/* sRGBA --> lL */

vgmPRINTCMNT(vgvTAB,				Convert to linear color space.																);
vgmPRINTLINE(vgvTAB,				tmpB = vgfGetColorInverseGamma(trgB);														);
vgmPRINTLINE(vgvTAB,				tmpG = vgfGetColorInverseGamma(trgG);														);
vgmPRINTLINE(vgvTAB,				tmpR = vgfGetColorInverseGamma(trgR);														);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				Compute the grayscale value.																);
vgmPRINTLINE(vgvTAB,				trgL = vgfGetGrayScale(tmpR, tmpG, tmpB);													);
vgmPRINTLINE("",																												);
								}
								else
								{
									/* sRGBA --> sL */

vgmPRINTCMNT(vgvTAB,				Convert to linear color space.																);
vgmPRINTLINE(vgvTAB,				tmpB = vgfGetColorInverseGamma(trgB);														);
vgmPRINTLINE(vgvTAB,				tmpG = vgfGetColorInverseGamma(trgG);														);
vgmPRINTLINE(vgvTAB,				tmpR = vgfGetColorInverseGamma(trgR);														);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				Compute the grayscale value.																);
vgmPRINTLINE(vgvTAB,				grayScale = vgfGetGrayScale(tmpR, tmpG, tmpB);												);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,				Convert to non-linear color space.															);
vgmPRINTLINE(vgvTAB,				trgL = vgfGetColorGamma(grayScale);															);
vgmPRINTLINE("",																												);
								}
							}


							/*****************************************************************************************************
							** Write the new buffer value.
							*/

							if (targetBitAccess)
							{
								/* Determine the last pixel position. */
								gctUINT lastPixelOffset = 8 - TargetFormat->l.width;

vgmPRINTCMNT(vgvTAB,			Last pixel within the byte?																		);
vgmPRINTF(vgvTAB,				"if (Pixel->bitOffset == %d)", lastPixelOffset													);
vgmPRINTF(vgvTAB,				"{"																								);
vgmPRINTCMNT(vgvTAB vgvTAB,			Compute the pixel value.																	);
vgmPRINTF(vgvTAB vgvTAB,			"value   = %s(trgL, %d, 8);", PackMacroName, TargetFormat->l.width							);
vgmPRINTF(vgvTAB vgvTAB,			"value <<= %d;", lastPixelOffset															);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Modify the pixel value.																		);
vgmPRINTF(vgvTAB vgvTAB,			"* Pixel->current &= ~(0x%02X << %d);", TargetFormat->l.mask, lastPixelOffset				);
vgmPRINTF(vgvTAB vgvTAB,			"* Pixel->current |= value;"																);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Advance to the next set of pixels.															);
vgmPRINTF(vgvTAB vgvTAB,			"Pixel->current++;"																			);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Reset the bit offset.																		);
vgmPRINTF(vgvTAB vgvTAB,			"Pixel->bitOffset = 0;"																		);
vgmPRINTF(vgvTAB,				"}"																								);
vgmPRINTF(vgvTAB,				"else"																							);
vgmPRINTF(vgvTAB,				"{"																								);
vgmPRINTCMNT(vgvTAB vgvTAB,			Compute the pixel mask.																		);
vgmPRINTF(vgvTAB vgvTAB,			"mask   = 0x%02X;", TargetFormat->l.mask													);
vgmPRINTF(vgvTAB vgvTAB,			"mask <<= Pixel->bitOffset;"																);
vgmPRINTF(vgvTAB vgvTAB,			"mask   = ~mask;"																			);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Compute the pixel value.																	);
vgmPRINTF(vgvTAB vgvTAB,			"value   = %s(trgL, %d, 8);", PackMacroName, TargetFormat->l.width							);
vgmPRINTF(vgvTAB vgvTAB,			"value <<= Pixel->bitOffset;"																);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Modify the pixel value.																		);
vgmPRINTF(vgvTAB vgvTAB,			"* Pixel->current &= mask;"																	);
vgmPRINTF(vgvTAB vgvTAB,			"* Pixel->current |= value;"																);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Advance to the next pixel.																	);
vgmPRINTF(vgvTAB vgvTAB,			"Pixel->bitOffset += 1;"																	);
vgmPRINTF(vgvTAB,				"}"																								);
							}
							else
							{
vgmPRINTCMNT(vgvTAB,			Convert the final pixel value.																	);
vgmPRINTF(vgvTAB,				"value = %s(trgL, %d, %d);", PackMacroName, TargetFormat->l.width, TargetFormat->bitsPerPixel	);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			Write the pixel value.																			);
vgmPRINTF(vgvTAB,				"* (gctUINT%d_PTR) Pixel->current = value;", TargetFormat->bitsPerPixel							);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			Advance to the next pixel.																		);
vgmPRINTF(vgvTAB,				"Pixel->current += %d;", TargetFormat->bitsPerPixel / 8											);
							}
						}

						static void _GenerateAlphaWriter(
							CStdioFile& File,
							vgsFORMAT_INFO_PTR TargetFormat,
							gctBOOL InputLinear,
							gctBOOL InputPremultiplied,
							gctSTRING PackMacroName
							)
						{
							gctBOOL targetBitAccess  = (TargetFormat->a.width <  8);
							gctBOOL target4BitAccess = (TargetFormat->a.width == 4);


							/*****************************************************************************************************
							** Define local variables.
							*/

							if (targetBitAccess)
							{
vgmPRINTF(vgvTAB,				"register gctUINT8 value;"																		);

								if (!target4BitAccess)
								{
vgmPRINTF(vgvTAB,					"register gctUINT8 mask;"																	);
								}
							}
							else
							{
vgmPRINTF(vgvTAB,				"register gctUINT%d value;", TargetFormat->bitsPerPixel											);
							}

vgmPRINTF(vgvTAB,			"gctFLOAT trgA;"																					);
vgmPRINTLINE("",																												);


							/*****************************************************************************************************
							** Clamp the incoming color.
							*/

vgmPRINTCMNT(vgvTAB,		Clamp the incoming alpha.																			);
vgmPRINTLINE(vgvTAB,		trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);																);
vgmPRINTLINE("",																												);


							/*****************************************************************************************************
							** Write the new buffer value.
							*/

							if (targetBitAccess)
							{
								/* Determine the last pixel position. */
								gctUINT lastPixelOffset = 8 - TargetFormat->a.width;

vgmPRINTCMNT(vgvTAB,			Last pixel within the byte?																		);
vgmPRINTF(vgvTAB,				"if (Pixel->bitOffset == %d)", lastPixelOffset													);
vgmPRINTF(vgvTAB,				"{"																								);
vgmPRINTCMNT(vgvTAB vgvTAB,			Compute the pixel value.																	);
vgmPRINTF(vgvTAB vgvTAB,			"value   = %s(trgA, %d, 8);", PackMacroName, TargetFormat->a.width							);
vgmPRINTF(vgvTAB vgvTAB,			"value <<= %d;", lastPixelOffset															);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Modify the pixel value.																		);
vgmPRINTF(vgvTAB vgvTAB,			"* Pixel->current &= ~(0x%02X << %d);", TargetFormat->a.mask, lastPixelOffset				);
vgmPRINTF(vgvTAB vgvTAB,			"* Pixel->current |= value;"																);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Advance to the next set of pixels.															);
vgmPRINTF(vgvTAB vgvTAB,			"Pixel->current++;"																			);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Reset the bit offset.																		);
vgmPRINTF(vgvTAB vgvTAB,			"Pixel->bitOffset = 0;"																		);
vgmPRINTF(vgvTAB,				"}"																								);
vgmPRINTF(vgvTAB,				"else"																							);
vgmPRINTF(vgvTAB,				"{"																								);
									if (target4BitAccess)
									{
vgmPRINTCMNT(vgvTAB vgvTAB,				Compute the pixel value.																);
vgmPRINTF(vgvTAB vgvTAB,				"value = %s(trgA, %d, 8);", PackMacroName, TargetFormat->a.width						);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,				Modify the pixel value.																	);
vgmPRINTF(vgvTAB vgvTAB,				"* Pixel->current &= ~0x0F;"															);
vgmPRINTF(vgvTAB vgvTAB,				"* Pixel->current |= value;"															);
									}
									else
									{
vgmPRINTCMNT(vgvTAB vgvTAB,				Compute the pixel mask.																	);
vgmPRINTF(vgvTAB vgvTAB,				"mask   = 0x%02X;", TargetFormat->a.mask												);
vgmPRINTF(vgvTAB vgvTAB,				"mask <<= Pixel->bitOffset;"															);
vgmPRINTF(vgvTAB vgvTAB,				"mask   = ~mask;"																		);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,				Compute the pixel value.																);
vgmPRINTF(vgvTAB vgvTAB,				"value   = %s(trgA, %d, 8);", PackMacroName, TargetFormat->a.width						);
vgmPRINTF(vgvTAB vgvTAB,				"value <<= Pixel->bitOffset;"															);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,				Modify the pixel value.																	);
vgmPRINTF(vgvTAB vgvTAB,				"* Pixel->current &= mask;"																);
vgmPRINTF(vgvTAB vgvTAB,				"* Pixel->current |= value;"															);
									}

vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB vgvTAB,			Advance to the next pixel.																	);
vgmPRINTF(vgvTAB vgvTAB,			"Pixel->bitOffset += %d;", TargetFormat->a.width											);
vgmPRINTF(vgvTAB,				"}"																								);
							}
							else
							{
vgmPRINTCMNT(vgvTAB,			Convert the final pixel value.																	);
vgmPRINTF(vgvTAB,				"value = %s(trgA, %d, %d);", PackMacroName, TargetFormat->a.width, TargetFormat->bitsPerPixel	);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			Write the pixel value.																			);
vgmPRINTF(vgvTAB,				"* (gctUINT%d_PTR) Pixel->current = value;", TargetFormat->bitsPerPixel							);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			Advance to the next pixel.																		);
vgmPRINTF(vgvTAB,				"Pixel->current += %d;", TargetFormat->bitsPerPixel / 8											);
							}
						}

						static void _ProcessAChannel(
							CStdioFile& File,
							vgsFORMAT_INFO_PTR TargetFormat,
							gctBOOL InputLinear,
							gctBOOL InputPremultiplied,
							gctBOOL FullWrite,
							gctBOOL AlphaCheck,
							gctSTRING Tab
							)
						{
							CString tab(Tab);

							if (AlphaCheck)
							{
								tab += vgvTAB;
							}

							gctSTRING operation = FullWrite ? " =" : "|=";

							if (AlphaCheck)
							{
vgmPRINTF("",				"%sif ((ChannelMask & VG_ALPHA) == VG_ALPHA)", Tab													);
vgmPRINTF("",				"%s{", Tab																							);
							}

							/* For ptemultiplied inputs alpha is clamped earlier. */
							if (!InputPremultiplied)
							{
vgmPRINTF("",					"%strgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);", (LPCTSTR) tab										);
							}

							if (!FullWrite)
							{
vgmPRINTF("",					"%svalue &= ~0x%0*X;",
									(LPCTSTR) tab, (TargetFormat->bitsPerPixel / 4),
									(TargetFormat->a.mask << TargetFormat->a.start)												);
							}

							if (TargetFormat->a.start == 0)
							{
vgmPRINTF("",					"%svalue %s vgmPACKCOMPONENT(trgA, %d, %d);",
									(LPCTSTR) tab, operation,
									TargetFormat->a.width, TargetFormat->bitsPerPixel											);
							}
							else
							{
vgmPRINTF("",					"%svalue %s vgmPACKCOMPONENT(trgA, %d, %d) << %d;",
									(LPCTSTR) tab, operation,
									TargetFormat->a.width, TargetFormat->bitsPerPixel, TargetFormat->a.start					);
							}

							if (AlphaCheck)
							{
vgmPRINTF("",				"%s}", Tab																							);
							}
vgmPRINTLINE("",																												);
						}

						static void _ProcessRGBChannel(
							CStdioFile& File,
							gctUINT Channel,
							vgsFORMAT_INFO_PTR TargetFormat,
							gctBOOL InputLinear,
							gctBOOL InputPremultiplied,
							gctBOOL FullWrite,
							gctBOOL AlphaEnabled,
							gctUINT FirstChannel,
							gctSTRING Tab
							)
						{
							CString tab1;
							CString tab2;
							vgsCHANNEL_PTR channel;
							gctSTRING floatName;
							gctSTRING intName;
							gctSTRING maskName;
							gctSTRING outAlphaName;
							gctUINT inputIndex;
							gctBOOL extraLine;
							gctBOOL skipPremultiply;
							gctBOOL processPemulColor;

							switch (Channel)
							{
							case VG_RED:
								channel     = &TargetFormat->r;
								maskName    = "VG_RED";
								floatName   = "trgR";
								intName     = "rINT";
								inputIndex  = 0;
								extraLine   = gcvFALSE;
								break;

							case VG_GREEN:
								channel     = &TargetFormat->g;
								maskName    = "VG_GREEN";
								floatName   = "trgG";
								intName     = "gINT";
								inputIndex  = 1;
								extraLine   = gcvTRUE;
								break;

							case VG_BLUE:
								channel     = &TargetFormat->b;
								maskName    = "VG_BLUE";
								floatName   = "trgB";
								intName     = "bINT";
								inputIndex  = 2;
								extraLine   = gcvTRUE;
								break;
							}

							processPemulColor
								= !FullWrite
								&& AlphaEnabled
								&& TargetFormat->premultiplied;

							skipPremultiply
								=  AlphaEnabled
								&& (InputPremultiplied == TargetFormat->premultiplied)
								&& (InputLinear        == TargetFormat->linear);

							tab1 = Tab;

							if (!FullWrite)
							{
								tab1 += vgvTAB;
							}

							tab2 = processPemulColor ? Tab : tab1;

							outAlphaName = "trgA";

							if (!FullWrite)
							{
vgmPRINTF("",				"%sif ((ChannelMask & %s) == %s)", Tab, maskName, maskName											);
vgmPRINTF("",				"%s{", Tab																							);
							}

								if (InputPremultiplied)
								{
vgmPRINTF("",						"%s%s   = gcmCLAMP(Value[%d], 0.0f, trgA);", (LPCTSTR) tab1, floatName, inputIndex			);

									if (!skipPremultiply)
									{
vgmPRINTF("",							"%s%s  /= trgA;", (LPCTSTR) tab1, floatName												);
									}
								}
								else
								{
vgmPRINTF("",						"%s%s   = gcmCLAMP(Value[%d], 0.0f, 1.0f);", (LPCTSTR) tab1, floatName, inputIndex			);
								}

								if (InputLinear && !TargetFormat->linear)
								{
vgmPRINTF("",						"%s%s   = vgfGetColorGamma(%s);", (LPCTSTR) tab1, floatName, floatName						);
								}
								else if (!InputLinear && TargetFormat->linear)
								{
vgmPRINTF("",						"%s%s   = vgfGetColorInverseGamma(%s);", (LPCTSTR) tab1, floatName, floatName				);
								}

								if (!AlphaEnabled && TargetFormat->premultiplied)
								{
vgmPRINTF("",						"%s%s  *= curA;", (LPCTSTR) tab1, floatName													);
								}

								if (processPemulColor)
								{
vgmPRINTF("",				"%s}", Tab																							);
vgmPRINTF("",				"%selse", Tab																						);
vgmPRINTF("",				"%s{", Tab																							);
									if (channel->start == 0)
									{
vgmPRINTF("",							"%s%s   = value & 0x%02X;",
											(LPCTSTR) tab1, intName, channel->mask												);
									}
									else
									{
vgmPRINTF("",							"%s%s   = (value >> %d) & 0x%02X;",
											(LPCTSTR) tab1, intName, channel->start, channel->mask								);
									}

vgmPRINTF("",						"%s%s   = vgmZERO2ONE(%s, %d);",
											(LPCTSTR) tab1, floatName, intName, channel->width									);

									if (AlphaEnabled)
									{
vgmPRINTF("",							"%s%s  /= curA;", (LPCTSTR) tab1, floatName												);
									}

									if (skipPremultiply)
									{
vgmPRINTF("",							"%s%s  *= trgA;", (LPCTSTR) tab1, floatName												);
									}
vgmPRINTF("",				"%s}", Tab																							);
vgmPRINTLINE("",																												);
								}

								if (AlphaEnabled && TargetFormat->premultiplied && !skipPremultiply)
								{
vgmPRINTF("",						"%s%s  *= %s;", (LPCTSTR) tab2, floatName, outAlphaName										);
								}

								CString operation;

								if (FullWrite)
								{
									operation = (Channel == FirstChannel) ? " =" : "|=";
								}
								else
								{
vgmPRINTF("",						"%svalue &= ~0x%0*X;",
										(LPCTSTR) tab2, (TargetFormat->bitsPerPixel / 4),
										(channel->mask << channel->start)														);
									operation = "|=";
								}

								if (channel->start == 0)
								{
vgmPRINTF("",						"%svalue %s vgmPACKCOMPONENT(%s, %d, %d);",
										(LPCTSTR) tab2, (LPCTSTR) operation, floatName,
										channel->width, TargetFormat->bitsPerPixel												);
								}
								else
								{
vgmPRINTF("",						"%svalue %s vgmPACKCOMPONENT(%s, %d, %d) << %d;",
										(LPCTSTR) tab2, (LPCTSTR) operation, floatName,
										channel->width, TargetFormat->bitsPerPixel, channel->start								);
								}

							if (!FullWrite && !processPemulColor)
							{
vgmPRINTF("",				"%s}", Tab																							);
							}

							if (extraLine)
							{
vgmPRINTLINE("",																												);
							}
						}

						static void _GenerateRGBAWriter(
							CStdioFile& File,
							vgsFORMAT_INFO_PTR TargetFormat,
							gctBOOL InputLinear,
							gctBOOL InputPremultiplied,
							gctBOOL FullWrite,
							gctBOOL AlphaEnabled,
							gctBOOL AlphaCheck,
							gctBOOL TargetRGBA
							)
						{
							gctUINT firstChannel = TargetRGBA ? VG_ALPHA : VG_BLUE;


							/*****************************************************************************************************
							** Define local variables.
							*/

vgmPRINTF(vgvTAB,			"register gctUINT%d value;", TargetFormat->bitsPerPixel												);

							if (TargetRGBA)
							{
								if (FullWrite || !TargetFormat->premultiplied)
								{
vgmPRINTF(vgvTAB,					"gctFLOAT trgA, trgB, trgG, trgR;"															);
								}

								/* Masked write to premultiplied target. */
								else
								{
									if (AlphaEnabled)
									{
vgmPRINTF(vgvTAB,						"register gctUINT8 aINT, bINT, gINT, rINT;"												);
vgmPRINTF(vgvTAB,						"gctFLOAT curA, trgA, trgB, trgG, trgR;"												);
									}
									else
									{
vgmPRINTF(vgvTAB,						"register gctUINT8 aINT;"																);

										if (InputPremultiplied)
										{
vgmPRINTF(vgvTAB,							"gctFLOAT curA, trgA, trgB, trgG, trgR;"											);
										}
										else
										{
vgmPRINTF(vgvTAB,							"gctFLOAT curA, trgB, trgG, trgR;"													);
										}
									}
								}
							}
							else if (InputPremultiplied)
							{
vgmPRINTF(vgvTAB,				"gctFLOAT trgA, trgB, trgG, trgR;"																);
							}
							else
							{
vgmPRINTF(vgvTAB,				"gctFLOAT trgB, trgG, trgR;"																	);
							}
vgmPRINTLINE("",																												);

							if (!FullWrite)
							{
vgmPRINTCMNT(vgvTAB,			Read the current destination value.																);
vgmPRINTF(vgvTAB,				"value = * (gctUINT%d_PTR) Pixel->current;", TargetFormat->bitsPerPixel							);
vgmPRINTLINE("",																												);

								if (TargetRGBA && TargetFormat->premultiplied)
								{
vgmPRINTCMNT(vgvTAB,				Convert the current value of the alpha channel.												);
									if (TargetFormat->a.start == 0)
									{
vgmPRINTF(vgvTAB,						"aINT = value & 0x%02X;", TargetFormat->a.mask											);
									}
									else
									{
vgmPRINTF(vgvTAB,						"aINT = (value >> %d) & 0x%02X;", TargetFormat->a.start, TargetFormat->a.mask			);
									}

vgmPRINTF(vgvTAB,					"curA = vgmZERO2ONE(aINT, %d);", TargetFormat->a.width										);
vgmPRINTLINE("",																												);
								}
							}

							if (InputPremultiplied)
							{
vgmPRINTCMNT(vgvTAB,			Clamp the incoming alpha.																		);
vgmPRINTLINE(vgvTAB,			trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);															);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,			Convert the incoming color.																		);
vgmPRINTLINE(vgvTAB,			if (trgA == 0.0f)																				);
vgmPRINTLINE(vgvTAB,			{																								);
									if (FullWrite || (AlphaEnabled && TargetFormat->premultiplied))
									{
vgmPRINTF(vgvTAB vgvTAB,				"value = 0;"																			);
									}
									else
									{
vgmPRINTF(vgvTAB vgvTAB,				"value &= vgmCHANNELMASK(%s, ChannelMask);", TargetFormat->name							);
									}
vgmPRINTLINE(vgvTAB,			}																								);
vgmPRINTLINE(vgvTAB,			else																							);
vgmPRINTLINE(vgvTAB,			{																								);
									if (TargetRGBA && AlphaEnabled)
									{
										_ProcessAChannel(
											File,
											TargetFormat,
											InputLinear, InputPremultiplied,
											FullWrite, AlphaCheck, vgvTAB vgvTAB
											);
									}

									_ProcessRGBChannel(
										File,
										VG_BLUE,
										TargetFormat,
										InputLinear, InputPremultiplied,
										FullWrite, AlphaEnabled, firstChannel, vgvTAB vgvTAB
										);

									_ProcessRGBChannel(
										File,
										VG_GREEN,
										TargetFormat,
										InputLinear, InputPremultiplied,
										FullWrite, AlphaEnabled, firstChannel, vgvTAB vgvTAB
										);

									_ProcessRGBChannel(
										File,
										VG_RED,
										TargetFormat,
										InputLinear, InputPremultiplied,
										FullWrite, AlphaEnabled, firstChannel, vgvTAB vgvTAB
										);
vgmPRINTLINE(vgvTAB,			}																								);
							}

							/* Input is not premultiplied. */
							else
							{
								if (TargetRGBA && AlphaEnabled)
								{
									_ProcessAChannel(
										File,
										TargetFormat,
										InputLinear, InputPremultiplied,
										FullWrite, AlphaCheck, vgvTAB
										);
								}

								_ProcessRGBChannel(
									File,
									VG_BLUE,
									TargetFormat,
									InputLinear, InputPremultiplied,
									FullWrite, AlphaEnabled, firstChannel, vgvTAB
									);

								_ProcessRGBChannel(
									File,
									VG_GREEN,
									TargetFormat,
									InputLinear, InputPremultiplied,
									FullWrite, AlphaEnabled, firstChannel, vgvTAB
									);

								_ProcessRGBChannel(
									File,
									VG_RED,
									TargetFormat,
									InputLinear, InputPremultiplied,
									FullWrite, AlphaEnabled, firstChannel, vgvTAB
									);
							}

vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,		Write the pixel value.																				);
vgmPRINTF(vgvTAB,			"* (gctUINT%d_PTR) Pixel->current = value;", TargetFormat->bitsPerPixel								);
vgmPRINTLINE("",																												);
vgmPRINTCMNT(vgvTAB,		Advance to the next pixel.																			);
vgmPRINTF(vgvTAB,			"Pixel->current += %d;", TargetFormat->bitsPerPixel / 8												);
}

void GenerateDefinitions(
	CStdioFile& File
	)
{
	/* Add definitions. */
	vgfGenerateCaption(File, "Support definitions and functions.");

	vgfWriteLine(File, "#define vgmPACKCOMPONENT(Value, ComponentWidth, PixelWidth) \\\n");
	vgfWriteLine(File,		"\t(gctUINT ## PixelWidth) _PackComponent( \\\n");
	vgfWriteLine(File,			"\t\t(Value), \\\n");
	vgfWriteLine(File,			"\t\t((1 << ComponentWidth) - 1) \\\n");
	vgfWriteLine(File,			"\t\t)\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "#define vgmPACKUPSAMPLEDCOMPONENT(Value, ComponentWidth, PixelWidth) \\\n");
	vgfWriteLine(File,		"\t(gctUINT ## PixelWidth) _PackUpsampledComponent( \\\n");
	vgfWriteLine(File,			"\t\t(Value), \\\n");
	vgfWriteLine(File,			"\t\t((1 << ComponentWidth) - 1) \\\n");
	vgfWriteLine(File,			"\t\t)\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "static gctUINT32 _PackComponent(\n");
	vgfWriteLine(File,		"\tgctFLOAT Value,\n");
	vgfWriteLine(File,		"\tgctUINT32 Mask\n");
	vgfWriteLine(File,		"\t)\n");
	vgfWriteLine(File, "{\n");
	vgfWriteLine(File,		"\t/* Compute the rounded normalized value. */\n");
	vgfWriteLine(File,		"\tgctFLOAT rounded = Value * Mask + 0.5f;\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File,		"\t/* Get the integer part. */\n");
	vgfWriteLine(File,		"\tgctINT roundedInt = (gctINT) rounded;\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File,		"\t/* Clamp to 0..1 range. */\n");
	vgfWriteLine(File,		"\tgctUINT32 clamped = gcmCLAMP(roundedInt, 0, (gctINT) Mask);\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File,		"\t/* Return result. */\n");
	vgfWriteLine(File,		"\treturn clamped;\n");
	vgfWriteLine(File, "}\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "static gctUINT32 _PackUpsampledComponent(\n");
	vgfWriteLine(File,		"\tgctFLOAT Value,\n");
	vgfWriteLine(File,		"\tgctUINT32 Mask\n");
	vgfWriteLine(File,		"\t)\n");
	vgfWriteLine(File, "{\n");
	vgfWriteLine(File,		"\t/* Round and truncate the value. */\n");
	vgfWriteLine(File,		"\tgctINT rounded = (gctINT) (Value + 0.5f);\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File,		"\t/* Determine the result. */\n");
	vgfWriteLine(File,		"\tgctUINT32 result = rounded ? Mask : 0;\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File,		"\t/* Return result. */\n");
	vgfWriteLine(File,		"\treturn result;\n");
	vgfWriteLine(File, "}\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "\n");
}

void GenerateUpsampledWriters(
	CStdioFile& File
	)
{
	gctBOOL firstFormat = gcvTRUE;
	vgsFORMAT_LIST_PTR formatList;
	gctUINT formatCount;

	/* Get format groups. */
	vgfGetUpsampledFormatList(&formatList);

	/* Get the number of formats. */
	formatCount = formatList->count;

	/* Go through all formats in the swizzle group. */
	for (gctUINT currentFormat = 0; currentFormat < formatCount; currentFormat += 1)
	{
		/* Set initial generation flags. */
		gctBOOL haveA = gcvFALSE;

		/* No space before function initially. */
		gctSTRING functionSpacing = "";

		/* Get the current format. */
		vgsFORMAT_INFO_PTR format = &formatList->group[currentFormat];
		ASSERT(format->name != gcvNULL);

		/* Determine the function name postfix. */
		CString writePostfix;
		writePostfix.Format(vgvUPSAMPLED_FUNC_NAME, format->name);

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
			formatName.Format(_T("Upsampled VG_%s"), format->name);
			vgfGenerateCaption(File, formatName);
		}

		/* Go through all color spaces. */
		for (gctUINT currentColorSpace = 0; currentColorSpace < gcmCOUNTOF(colorSpaceArray); currentColorSpace += 1)
		{
			/* Go through all color spaces. */
			for (gctUINT currentPremul = 0; currentPremul < gcmCOUNTOF(premulArray); currentPremul += 1)
			{
				/* Go through all masks. */
				for (gctUINT currentMask = 0; currentMask < gcmCOUNTOF(maskArray); currentMask += 1)
				{
					/* Luminance? */
					if (format->l.width != 0)
					{
						vgfWriteLine(
							File, fullWriteHeader,
							functionSpacing,
							colorSpaceArray[currentColorSpace], maskArray[VG_RGB], premulArray[currentPremul],
							(LPCTSTR) writePostfix
							);

						_GenerateLuminanceWriter(
							File, format, currentColorSpace, currentPremul, "vgmPACKUPSAMPLEDCOMPONENT"
							);

						vgfWriteLine(File, "}\n");
						functionSpacing = "\n";

						/* Luminance ignores the mask - only one function is required. */
						break;
					}

					/* Alpha-only? */
					if ((format->a.width != 0) && (format->r.width == 0) && (format->g.width == 0) && (format->b.width == 0))
					{
						if (haveA)
						{
							break;
						}

						vgfWriteLine(
							File, fullWriteHeader,
							functionSpacing,
							"", maskArray[VG_ALPHA], "",
							(LPCTSTR) writePostfix
							);

						_GenerateAlphaWriter(
							File, format, currentColorSpace, currentPremul, "vgmPACKUPSAMPLEDCOMPONENT"
							);

						vgfWriteLine(File, "}\n");
						functionSpacing = "\n";

						/* Mark as generated. */
						haveA = gcvTRUE;

						/* Alpha formats have only two mask variations - on and off. */
						break;
					}

					ASSERT(gcvFALSE);
				}
			}
		}
	}

	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "\n");
}

void GenerateWriters(
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
		/* Get the number of formats in the group. */
		gctUINT formatCount = formatGroups[currentGroup].count;

		/* Go through all formats in the swizzle group. */
		for (gctUINT currentFormat = 0; currentFormat < formatCount; currentFormat += 1)
		{
			/* Set initial generation flags. */
			gctBOOL haveA = gcvFALSE;

			/* No space before function initially. */
			gctSTRING functionSpacing = "";

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
					/* Go through all masks. */
					for (gctUINT currentMask = 0; currentMask < gcmCOUNTOF(maskArray); currentMask += 1)
					{
						/* Luminance? */
						if (format->l.width != 0)
						{
							vgfWriteLine(
								File, fullWriteHeader,
								functionSpacing,
								colorSpaceArray[currentColorSpace], maskArray[VG_RGB], premulArray[currentPremul],
								format->name
								);

							_GenerateLuminanceWriter(
								File, format, currentColorSpace, currentPremul, "vgmPACKCOMPONENT"
								);

							vgfWriteLine(File, "}\n");
							functionSpacing = "\n";

							/* Luminance ignores the mask - only one function is required. */
							break;
						}

						/* Alpha-only? */
						if ((format->a.width != 0) && (format->r.width == 0) && (format->g.width == 0) && (format->b.width == 0))
						{
							if (haveA)
							{
								break;
							}

							vgfWriteLine(
								File, fullWriteHeader,
								functionSpacing,
								"", maskArray[VG_ALPHA], "",
								format->name
								);

							_GenerateAlphaWriter(
								File, format, currentColorSpace, currentPremul, "vgmPACKCOMPONENT"
								);

							vgfWriteLine(File, "}\n");
							functionSpacing = "\n";

							/* Mark as generated. */
							haveA = gcvTRUE;

							/* Alpha formats have only two mask variations - on and off. */
							break;
						}

						/* RGB format? */
						if ((format->a.width == 0) && (format->r.width != 0) && (format->g.width != 0) && (format->b.width != 0))
						{
							if (currentMask == 0)
							{
								vgfWriteLine(
									File, maskedWriteHeader,
									functionSpacing,
									colorSpaceArray[currentColorSpace], maskArray[VG_RGBA], premulArray[currentPremul],
									format->name
									);
							}
							else if (currentMask == VG_RGB)
							{
								vgfWriteLine(
									File, fullWriteHeader,
									functionSpacing,
									colorSpaceArray[currentColorSpace], maskArray[VG_RGBA], premulArray[currentPremul],
									format->name
									);
							}
							else
							{
								continue;
							}

							_GenerateRGBAWriter(
								File,
								format, currentColorSpace, currentPremul,
								(currentMask == VG_RGB),
								gcvFALSE,
								gcvFALSE,
								gcvFALSE
								);

							vgfWriteLine(File, "}\n");
							functionSpacing = "\n";

							continue;
						}

						/* RGBA format? */
						if ((format->a.width != 0) && (format->r.width != 0) && (format->g.width != 0) && (format->b.width != 0))
						{
							if (currentMask == VG_RGBA)
							{
								vgfWriteLine(
									File, fullWriteHeader,
									functionSpacing,
									colorSpaceArray[currentColorSpace],
									maskArray[VG_RGBA],
									premulArray[currentPremul],
									format->name
									);

								_GenerateRGBAWriter(
									File,
									format, currentColorSpace, currentPremul,
									gcvTRUE,	/* Full write.     */
									gcvTRUE,	/* Alpha enabled.  */
									gcvFALSE,	/* No alpha check. */
									gcvTRUE		/* RGBA target.    */
									);

								vgfWriteLine(File, "}\n");
								functionSpacing = "\n";

								continue;
							}

							if (!format->premultiplied)
							{
								if (currentMask == 0)
								{
									vgfWriteLine(
										File, maskedWriteHeader,
										functionSpacing,
										colorSpaceArray[currentColorSpace],
										maskArray[VG_RGBA],
										premulArray[currentPremul],
										format->name
										);

									_GenerateRGBAWriter(
										File,
										format, currentColorSpace, currentPremul,
										gcvFALSE,	/* Partial (masked) write. */
										gcvTRUE,	/* Alpha enabled.          */
										gcvTRUE,	/* Check alpha channel.    */
										gcvTRUE		/* RGBA target.            */
										);

									vgfWriteLine(File, "}\n");
									functionSpacing = "\n";
								}

								continue;
							}
							else
							{
								if (currentMask == 0)
								{
									vgfWriteLine(
										File, maskedWriteHeader,
										functionSpacing,
										colorSpaceArray[currentColorSpace],
										maskArray[VG_RGB],
										premulArray[currentPremul],
										format->name
										);

									_GenerateRGBAWriter(
										File,
										format, currentColorSpace, currentPremul,
										gcvFALSE,	/* Partial (masked) write. */
										gcvFALSE,	/* Alpha enabled.          */
										gcvFALSE,	/* Check alpha channel.    */
										gcvTRUE		/* RGBA target.            */
										);

									vgfWriteLine(File, "}\n");
									functionSpacing = "\n";
								}
								else if (currentMask == VG_ALPHA)
								{
									vgfWriteLine(
										File, maskedWriteHeader,
										functionSpacing,
										colorSpaceArray[currentColorSpace],
										maskArray[VG_RGBA],
										premulArray[currentPremul],
										format->name
										);

									_GenerateRGBAWriter(
										File,
										format, currentColorSpace, currentPremul,
										gcvFALSE,	/* Partial (masked) write. */
										gcvTRUE,	/* Alpha enabled.          */
										gcvFALSE,	/* Check alpha channel.    */
										gcvTRUE		/* RGBA target.            */
										);

									vgfWriteLine(File, "}\n");
									functionSpacing = "\n";
								}

								continue;
							}
						}

						ASSERT(gcvFALSE);
					}
				}
			}
		}
	}
}
