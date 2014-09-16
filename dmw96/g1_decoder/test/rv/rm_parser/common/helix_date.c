/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: helix_date.c,v 1.1 2009-03-11 14:09:53 kima Exp $
 * 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc.
 * All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the Real Format Source Code
 * Porting and Optimization License, available at
 * https://helixcommunity.org/2005/license/realformatsource (unless
 * RealNetworks otherwise expressly agrees in writing that you are
 * subject to a different license).  You may also obtain the license
 * terms directly from RealNetworks.  You may not use this file except
 * in compliance with the Real Format Source Code Porting and
 * Optimization License. There are no redistribution rights for the
 * source code of this file. Please see the Real Format Source Code
 * Porting and Optimization License for the rights, obligations and
 * limitations governing use of the contents of the file.
 * 
 * RealNetworks is the developer of the Original Code and owns the
 * copyrights in the portions it created.
 * 
 * This file, and the files included with this file, is distributed and
 * made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL
 * SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT
 * OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 * https://rarvcode-tck.helixcommunity.org
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#include <time.h>
#include "helix_types.h"

HX_DATETIME HX_GET_DATETIME(void)
{
    struct tm* tm;
    time_t t = time(0);
    HX_DATETIME dt;

    tm = localtime(&t);
    dt.second = tm->tm_sec;
    dt.minute = tm->tm_min;
    dt.hour = tm->tm_hour;
    dt.dayofweek = tm->tm_wday;
    dt.dayofmonth = tm->tm_mday;
    dt.dayofyear = tm->tm_yday;
    dt.month = tm->tm_mon + 1; // 0 based (11 = December)
    dt.year = tm->tm_year;
    dt.gmtDelta = 0; // or something
    return dt;
}
