@REM #########################################################################
@REM #
@REM #  Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
@REM #
@REM #  The material in this file is confidential and contains trade secrets
@REM #  of Vivante Corporation. This is proprietary information owned by
@REM #  Vivante Corporation. No part of this work may be disclosed,
@REM #  reproduced, copied, transmitted, or used in any way for any purpose,
@REM #  without the express written permission of Vivante Corporation.
@REM #
@REM #########################################################################



if /i not "%1"=="preproc" goto :Not_Preproc
    goto :EOF
:Not_Preproc
if /i not "%1"=="pass1" goto :Not_Pass1
    goto :EOF
:Not_Pass1
if /i not "%1"=="pass2" goto :Not_Pass2
    goto :EOF
:Not_Pass2
if /i not "%1"=="report" goto :Not_Report
    goto :EOF
:Not_Report
echo %0 Invalid parameter %1
