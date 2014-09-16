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


#include "gc_glsl_preprocessor_int.h"

/*******************************************************************************
**
**	ppoBASE_Dump
**
**		To dump the base class.
**
**
**
**
*/
gceSTATUS
ppoBASE_Dump(
	ppoPREPROCESSOR PP,
	ppoBASE			Base
	)
{
    gceSTATUS status;

    gcmHEADER_ARG("PP=0x%x Base=0x%x", PP, Base);

    status = sloCOMPILER_Dump(
		PP->compiler,
		slvDUMP_PREPROCESSOR,
		"<BaseClass file=\"%s\" line=\"%d\" infomation=\"%s\" />",
		Base->file,
		Base->line,
		Base->info
		);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**	ppoBASE_Init
**
**		Init(Reset), not alloc a base class.
**
*/

gceSTATUS
ppoBASE_Init(
	/*00*/ IN		ppoPREPROCESSOR		PP,
	/*01*/ IN OUT	ppoBASE				YourBase,
	/*02*/ IN		gctCONST_STRING		File,
	/*03*/ IN		gctUINT				Line,
	/*04*/ IN		gctCONST_STRING		MoreInfo,
	/*05*/ IN		ppeOBJECT_TYPE		Type
	)
{
    gcmHEADER_ARG("PP=0x%x YourBase=0x%x File=0x%x Line=%u MoreInfo=0x%x Type=%d",
                  PP, YourBase, File, Line, MoreInfo, Type);

	gcmASSERT(
		/*00*/	PP
		/*01*/	&& YourBase
		/*02*/	&& File
		/*03*/	&& Line
		/*04*/	&& MoreInfo
		/*05*/	&& Type
		);

	gcoOS_MemFill(YourBase, 0, sizeof(struct _ppoBASE));

	/*00	node*/

	/*01*/	YourBase->type	=	Type;

	/*02*/	YourBase->file	=	File;

	/*03*/	YourBase->line	=	Line;

	/*04*/	YourBase->info	=	MoreInfo;

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	ppeOBJECT_TypeString
**
**		Return the name string of a type.
**
*/
gceSTATUS
ppeOBJECT_TypeString(
	IN	ppeOBJECT_TYPE		TypeEnum,
	OUT	gctCONST_STRING*	TypeString
	)
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("TypeEnum=0x%x TypeString=0x%x",
                  TypeEnum, TypeString);

	switch (TypeEnum)
	{
	case ppvOBJ_UNKNOWN:
		*TypeString = "Object Type : Unknown";
		break;
	case ppvOBJ_MACRO_MANAGER:
		*TypeString = "Object Type : Macro Manager";
		break;
	case ppvOBJ_TOKEN:
		*TypeString = "Object Type : Token";
		break;
	case ppvOBJ_PREPROCESSOR:
		*TypeString = "Object Type : PP";
		break;
	case ppvOBJ_HIDE_SET:
		*TypeString = "Object Type : Hide Set";
		break;
	case ppvOBJ_BYTE_INPUT_STREAM:
		*TypeString = "Object Type : Byte Input Stream";
		break;
	case ppvOBJ_STRING_MANAGER_IR:
		*TypeString = "Object Type : String Manager";
		break;
	case ppvOBJ_MACRO_SYMBOL:
		*TypeString = "Object Type : Macro Symbol";
		break;
	default:
		{
            *TypeString = gcvNULL;

			status = gcvSTATUS_INVALID_DATA;
            gcmFOOTER();
            return status;
		}
	}

    /* Success. */
    gcmFOOTER_ARG("*TypeString=0x%x", *TypeString);
	return gcvSTATUS_OK;
}





