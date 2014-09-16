@if "%_echo%"=="" echo off

if "%_TARGETPLATROOT%"=="" (
    echo.
	echo ERROR: not found %%_TARGETPLATROOT%%.
	echo Please execute this tool in CESHELL command window.
	echo.
	goto :eof
)

if "%VIVANTE_SDK_DIR%"=="" (
    echo.
    echo Warning: not found %%VIVANTE_SDK_DIR%%, use default setting:
	echo set VIVANTE_SDK_DIR=%%AQROOT%%\VIVANTE_SDK
	echo.
    set VIVANTE_SDK_DIR=%AQROOT%\VIVANTE_SDK
)

set VIVANTE_SDK_INC=%VIVANTE_SDK_DIR%\inc
set VIVANTE_SDK_LIB=%VIVANTE_SDK_DIR%\lib
set VIVANTE_SDK_BIN=%VIVANTE_SDK_DIR%\bin

if not exist %VIVANTE_SDK_INC% mkdir %VIVANTE_SDK_INC%
if not exist %VIVANTE_SDK_LIB% mkdir %VIVANTE_SDK_LIB%
if not exist %VIVANTE_SDK_BIN% mkdir %VIVANTE_SDK_BIN%

echo.
echo Create Vivante Driver SDK ...
echo.

if not exist %VIVANTE_SDK_INC%\HAL mkdir %VIVANTE_SDK_INC%\HAL
echo copy gc_hal.h ...
copy /y %AQROOT%\hal\inc\gc_hal.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_tpes.h ...
copy /y %AQROOT%\hal\inc\gc_hal_types.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_enum.h ...
copy /y %AQROOT%\hal\inc\gc_hal_enum.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_options.h ...
copy /y %AQROOT%\hal\inc\gc_hal_options.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_base.h ...
copy /y %AQROOT%\hal\inc\gc_hal_base.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_raster.h ...
copy /y %AQROOT%\hal\inc\gc_hal_raster.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_engine.h ...
copy /y %AQROOT%\hal\inc\gc_hal_engine.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_dump.h ...
copy /y %AQROOT%\hal\inc\gc_hal_dump.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_mem.h ...
copy /y %AQROOT%\hal\inc\gc_hal_mem.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_rename.h ...
copy /y %AQROOT%\hal\inc\gc_hal_rename.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_driver.h ...
copy /y %AQROOT%\hal\inc\gc_hal_driver.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_profiler.h ...
copy /y %AQROOT%\hal\inc\gc_hal_profiler.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_driver_vg.h ...
copy /y %AQROOT%\hal\inc\gc_hal_driver_vg.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_engine_vg.h ...
copy /y %AQROOT%\hal\inc\gc_hal_engine_vg.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_vg.h ...
copy /y %AQROOT%\hal\inc\gc_hal_vg.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gc_hal_md5.h ...
copy /y %AQROOT%\hal\inc\gc_hal_md5.h %VIVANTE_SDK_DIR%\inc\HAL\
echo copy gchal_version header files ...
copy /y %AQROOT%\hal\inc\gc_hal_version.h %VIVANTE_SDK_DIR%\inc\HAL\gc_hal_version.h

echo copy KHR header files ...
if not exist %VIVANTE_SDK_INC%\KHR mkdir %VIVANTE_SDK_INC%\KHR
copy /y %AQROOT%\sdk\inc\KHR\*.h %VIVANTE_SDK_DIR%\inc\KHR\

echo copy EGL header files ...
if not exist %VIVANTE_SDK_INC%\EGL mkdir %VIVANTE_SDK_INC%\EGL
copy /y %AQROOT%\sdk\inc\EGL\*.h %VIVANTE_SDK_DIR%\inc\EGL\

echo copy GLES header files ...
if not exist %VIVANTE_SDK_INC%\GLES mkdir %VIVANTE_SDK_INC%\GLES
copy /y %AQROOT%\sdk\inc\GLES\*.h %VIVANTE_SDK_DIR%\inc\GLES\

echo copy GLES2 header files ...
if not exist %VIVANTE_SDK_INC%\GLES2 mkdir %VIVANTE_SDK_INC%\GLES2
copy /y %AQROOT%\sdk\inc\GLES2\*.h %VIVANTE_SDK_DIR%\inc\GLES2\

echo copy VG header files ...
if not exist %VIVANTE_SDK_INC%\VG mkdir %VIVANTE_SDK_INC%\VG
copy /y %AQROOT%\sdk\inc\VG\*.h %VIVANTE_SDK_DIR%\inc\VG\

echo copy VDK header files ...
copy /y %AQROOT%\sdk\inc\gc_vdk_types.h %VIVANTE_SDK_DIR%\inc\gc_vdk_types.h
copy /y %AQROOT%\sdk\inc\gc_vdk.h %VIVANTE_SDK_DIR%\inc\gc_vdk.h

set CELIBPATH=%_TARGETPLATROOT%\lib\%_TGTCPU%\%WINCEDEBUG%
if "%_WINCEOSVER%"=="700" set CELIBPATH=%SG_OUTPUT_ROOT%\platform\%_TGTPLAT%\lib\%_TGTCPU%\%WINCEDEBUG%
echo.
echo Entering %CELIBPATH%\ ...
echo.
cd /d %CELIBPATH%\
echo copy libGAL.lib ...
copy /y libGAL.lib %VIVANTE_SDK_LIB%\

echo copy libEGL.lib ...
copy /y libEGL.lib %VIVANTE_SDK_LIB%\

echo copy libGLESv1_CM.lib ...
copy /y libGLESv1_CM.lib %VIVANTE_SDK_LIB%\

echo copy libGLESv1_CL.lib ...
copy /y libGLESv1_CL.lib %VIVANTE_SDK_LIB%\

echo copy libGLES_CM.lib ...
copy /y libGLES_CM.lib %VIVANTE_SDK_LIB%\

echo copy libGLES_CL.lib ...
copy /y libGLES_CL.lib %VIVANTE_SDK_LIB%\

echo copy libGLSLC.lib ...
copy /y libGLSLC.lib %VIVANTE_SDK_LIB%\

echo copy libGLESv2.lib ...
copy /y libGLESv2.lib %VIVANTE_SDK_LIB%\

echo copy libOpenVG.lib ...
copy /y libOpenVG.lib %VIVANTE_SDK_LIB%\

if exist %CELIBPATH%\libVDK.lib (
	echo copy libVDK.lib ...
	copy /y libVDK.lib %VIVANTE_SDK_LIB%\
)

set CETGTPATH=%_TARGETPLATROOT%\target\%_TGTCPU%\%WINCEDEBUG%
if "%_WINCEOSVER%"=="700" set CETGTPATH=%SG_OUTPUT_ROOT%\platform\%_TGTPLAT%\target\%_TGTCPU%\%WINCEDEBUG%
echo.
echo Entering %CETGTPATH%\ ...
echo.
cd /d %CETGTPATH%\

echo copy libGALCore.dll ...
copy /y libGALCore.dll %VIVANTE_SDK_BIN%\

echo copy libGAL.dll ...
copy /y libGAL.dll %VIVANTE_SDK_BIN%\

echo copy libEGL.dll ...
copy /y libEGL.dll %VIVANTE_SDK_BIN%\

echo copy libGLESv1_CM.dll ...
copy /y libGLESv1_CM.dll %VIVANTE_SDK_BIN%\

echo copy libGLESv1_CL.dll ...
copy /y libGLESv1_CL.dll %VIVANTE_SDK_BIN%\

echo copy libGLES_CM.dll ...
copy /y libGLES_CM.dll %VIVANTE_SDK_BIN%\

echo copy libGLES_CL.dll ...
copy /y libGLES_CL.dll %VIVANTE_SDK_BIN%\

echo copy libGLSLC.dll ...
copy /y libGLSLC.dll %VIVANTE_SDK_BIN%\

echo copy libGLESv2.dll ...
copy /y libGLESv2.dll %VIVANTE_SDK_BIN%\

echo copy libOpenVG.dll ...
copy /y libOpenVG.dll %VIVANTE_SDK_BIN%\

if exist %CETGTPATH%\libVDK.dll (
	echo copy libVDK.dll ...
	copy /y libVDK.dll %VIVANTE_SDK_BIN%\
)

echo.
echo  Vivante Driver SDK file list:
echo.
cd /d %VIVANTE_SDK_DIR%
tree /A /F

:eof
