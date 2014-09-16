WinCE Build for ARM

Contents
========
1. Prerequisite
2. Installation
3. Building
4. Generate VIVANTE_SDK

1. Prerequisite
===============

Before you build this test suite, you need to have a sdk package, or you have built the driver suite already.


2. Installation
===============

I. set system wide environment variable for AQROOT to store the source tree

There is no restriction on what should the AQROOT should be set to, but normally user should
set it to be on the same disk as where WINCE is installed, such as C:\Vivante

	AQROOT=C:\Vivante

II. Extract the source code from VIVANTE_GAL_Unified_Src_drv_<version>.tgz package to be under $AQROOT


III. create %WINCEROOT%/public/gchal directory

IV. copy everything under $AQROOT\* to %WINCEROOT%/public/gchal

3. Building
===========

I. Build WinCE OS if not already built with target BSP environments or load the pre-built OS into
PlatformBuilder if it already exists.

For how to build WinCE OS in PlatformBuilder, please refer to Microsoft PlatformBuilder User Manual.

II. Add GC500 drvier catalog to PB OS Design by selecting it from the Catalog/Third Party/Device Drivers/GC500
catalog.

 - Open the Catalog panel
 - Check if you have Third Party\Device Drivers\GC500
 - If it is not present then double click %WINCEROOT%\PUBLIC\GCHAL\gchal.cec
 - Go to Third Party\Device Drivers\Display\GC500
 - Add it to Catalog
 - Go to Catalog pane in Platform builder and refresh the catalog

Note: If the platform builder was open when you changed the Environment Variables then close the Platform builder
and open it again.

III. Build gchal project in PlatformBuilder

Note: Make sure you do a clean build.

IV. Make runtime image in PlatformBuilder

V. Copy all resource directories under $AQROOT\resources to the release directory of OS design (defined by %_FLATRELEASEDIR%)

VI. Now you are ready to attach the device and to run the samples.

4. Generate VIVANTE_SDK
=======================

This step is used to help build test suite. Once the driver has been built
sucessfully, you need to copy them to several target folders then the test
project could be compiled correctly. You can specify the target folders using
these three Environment Variables, VIVANTE_SDK_DIR VIVANTE_SDK_INC and
VIVANTE_SDK_LIB. If you don't set them, we will use their default values:

    VIVANTE_SDK_DIR=%AQROOT%/VIVANTE_SDK
    VIVANTE_SDK_INC=%VIVANTE_SDK_DIR%/inc
    VIVANTE_SDK_LIB=%VIVANTE_SDK_DIR%/lib

Then you can use cegenvivsdk.bat, which is lied in the same folder with this
documentation, to copy driver and
some necessary head files to specified target folders.

I. Open "Platform Builder"

II. Click "Build OS"/"Open Release Directory"

III. Run cegenvivsdk.bat in the opened command window.

Then you can go to build test suite if you want.

