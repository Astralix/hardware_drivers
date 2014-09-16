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




#include "gc_glsh_precomp.h"

#define glmHEX2CHAR(h) \
	( ((h) < 10) ? ('0' + (h)) \
	: ((h) <= 15) ? ('A' + (h) - 10) \
	: '?' \
	)

#define glmENUM2STRING(e) \
	case e: return #e;

gctCONST_STRING
_glshGetEnum(
	GLenum Enum
	)
{
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
	static char hex[] = "0x0000";

	switch (Enum)
	{
		glmENUM2STRING(GL_FRAMEBUFFER);
		glmENUM2STRING(GL_COLOR_ATTACHMENT0);
		glmENUM2STRING(GL_DEPTH_ATTACHMENT);
		glmENUM2STRING(GL_STENCIL_ATTACHMENT);
		glmENUM2STRING(GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE);
		glmENUM2STRING(GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME);
		glmENUM2STRING(GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL);
		glmENUM2STRING(GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE);
		glmENUM2STRING(GL_FRAMEBUFFER_COMPLETE);
		glmENUM2STRING(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
		glmENUM2STRING(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
		glmENUM2STRING(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS);
		glmENUM2STRING(GL_FRAMEBUFFER_UNSUPPORTED);
		glmENUM2STRING(GL_RENDERBUFFER);
		glmENUM2STRING(GL_TEXTURE_2D);
		glmENUM2STRING(GL_TEXTURE_CUBE_MAP);

	default:
		hex[2] = glmHEX2CHAR((Enum & 0xF000) >> 12);
		hex[3] = glmHEX2CHAR((Enum & 0x0F00) >> 8);
		hex[4] = glmHEX2CHAR((Enum & 0x00F0) >> 4);
		hex[5] = glmHEX2CHAR((Enum & 0x000F) >> 0);
		return hex;
	}
#else
	return gcvNULL;
#endif
}
