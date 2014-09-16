/*=============================================================================
*                                                                       
*                Copyright (C) 2008 Hantro Products Oy     		 
* File Name: 	rm_types.h 
* User for : 	rm demux mux, 
* Creat By : 	                                                                  
* CrateDate: 	
* Last modify :     
*
* relase history:
* 		version 0.1 release by 
=============================================================================*/
#ifndef _RM_TYPES_H_
#define _RM_TYPES_H_

#include "helix_types.h"
/*
 * Seek origin definitions - these are
 * exactly the same as the respective
 * definitions in <stdio.h>
 */
#define HX_SEEK_ORIGIN_SET 0
#define HX_SEEK_ORIGIN_CUR 1
#define HX_SEEK_ORIGIN_END 2

/* Function pointer definitions */
//typedef UINT32 (*rm_read_func_ptr) (void*   pUserRead,
//                                    BYTE*   pBuf, /* Must be at least ulBytesToRead long */
//                                    UINT32  ulBytesToRead);
//typedef void (*rm_seek_func_ptr) (void*  pUserRead,
//                                  UINT32 ulOffset,
//                                  UINT32 ulOrigin);

/* Function pointer definitions */
typedef UINT32 (*rm_read_func_ptr)(void *pUserRead,
								   BYTE *pBuf,	/* Must be at least ulBytesToRead long */
								   UINT32 ulBytesToRead);
typedef UINT32 (*rm_write_func_ptr)(void *pUserRead,
									BYTE *pBuf,
									UINT32 ulBytesToRead);
typedef void (*rm_seek_func_ptr)(void *pUserRead,
								 UINT32 ulOffset,
								 UINT32 ulOrigin);
typedef INT32 (*rm_tell_func_ptr)(void *pUserRead);

/*
* rm_parser definition. This is opaque to the user.
*/
typedef void rm_parser;

#endif
