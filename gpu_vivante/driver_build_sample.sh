#!/bin/bash

########################################################
# establish build environment and build options value
# Please modify the following items according your build environment

ARCH=arm
if [ ! -z $1 ]; then
    ARCH=$1
fi

export AQROOT=`pwd`
export AQARCH=$AQROOT/arch/XAQ2
export AQVGARCH=$AQROOT/arch/GC350

export SDK_DIR=$AQROOT/build/sdk

case "$ARCH" in

arm)
    export ARCH_TYPE=$ARCH
    export CPU_TYPE=arm920
    export FIXED_ARCH_TYPE=arm

    export KERNEL_DIR=/home/software/Linux/linux-2.6.21-arm1
    export DFB_DIR=/home/software/Linux/dfb
    export CROSS_COMPILE=arm-none-linux-gnueabi-
    export TOOLCHAIN=/home/software/Linux/toolchain
    export LIB_DIR=$TOOLCHAIN/arm-none-linux-gnueabi/libc/usr/lib

;;

arm-his-eabi)
    ARCH=arm
    export ARCH_TYPE=$ARCH
    export FIXED_ARCH_TYPE=arm-his-arm-hismall-linux
    export CPU_TYPE=arm920
    export KERNEL_DIR=/home/software/Linux/linux-2.6.29-hisi
    export CROSS_COMPILE=arm-hisi_vfpv3d16-linux-
    export TOOLCHAIN=/opt/hisi-linux/x86-arm/gcc-4.4.0-uClibc-0.9.30.2-softvfpv3
    export LIB_DIR=$TOOLCHAIN/usr/lib
;;

arm-his-oabi)
    ARCH=arm
    export ARCH_TYPE=$ARCH
    export FIXED_ARCH_TYPE=arm-his-arm-hismall-linux
    export CPU_TYPE=arm920
    export KERNEL_DIR=/home/software/Linux/linux-2.6.29-hisi
    export CROSS_COMPILE=arm-hismall-linux-
    export TOOLCHAIN=/opt/hisi-linux/x86-arm/gcc-3.4.3-uClibc-0.9.28
    export LIB_DIR=$TOOLCHAIN/usr/lib
;;

unicore)
    export ARCH_TYPE=unicore
    export CPU_TYPE=0
    export CPU_ARCH=0
    export FIXED_ARCH_TYPE=unicore

    export KERNEL_DIR=/home/software/Linux/linux-2.6.32
    export CROSS_COMPILE=unicore32-linux-
    export TOOLCHAIN=/home/software/Linux/uc4-1.0-beta-hard-RHELAS5
    export LIB_DIR=$TOOLCHAIN/unicore32-linux/lib
;;

tensilica)
    export ARCH_TYPE=$ARCH
    export CPU_TYPE=0
    export CPU_ARCH=0
    export FIXED_ARCH_TYPE=tensilica

#    KERNEL_DIR=/home/software/Linux/linux-2.6.24.7
    CROSS_COMPILE=xtensa_venus-linux-
    TOOLCHAIN=/home/software/Linux/xtensa/staging_dir/usr
    LIB_DIR=$TOOLCHAIN/lib
;;

ppc-be)
    export ARCH_TYPE=powerpc
    export CPU_TYPE=440

    # '-be' mens big-endian
    export FIXED_ARCH_TYPE=ppc-be

    # set ENDIANNESS to build driver with little-endian
    #export ENDIANNESS=-mlittle-endian

#    KERNEL_DIR=/home/software/Linux/linux-2.6.27
    export CROSS_COMPILE=ppc_4xx-
    export TOOLCHAIN=/home/software/eldk/usr
    export DEPMOD=$TOOLCHAIN/bin/depmod.pl
    export LIB_DIR=/home/software/eldk/ppc_4xx/lib

;;

mips-le)

    export ARCH_TYPE=$ARCH
    export CPU_TYPE=0
    export ARCH_TYPE=mips
    export CPU_ARCH=34kf

    #
    # to select the right ES20 pre-built files
    #
    export FIXED_ARCH_TYPE=mips-le

    #
    # to build driver with little endin
    #
    export ENDIANNESS=-mel

    export KERNEL_DIR=/home/software/Linux/linux-2.6.19-mips.le
    export CROSS_COMPILE=mips-linux-gnu-
    export TOOLCHAIN=/home/software/Linux/mips-4.4-5
    export LIB_DIR=$TOOLCHAIN/mips-linux-gnu/libc/el/usr/lib
;;

mips-be)

    export ARCH_TYPE=$ARCH
    export CPU_TYPE=0
    export ARCH_TYPE=mips
    export CPU_ARCH=34kf

    #
    # to select the right ES20 pre-built files
    #
    export FIXED_ARCH_TYPE=mips-be

    #
    # to build driver with little endin
    #
    export ENDIANNESS=-meb

    export KERNEL_DIR=/home/software/Linux/linux-2.6.19-mips.be
    export CROSS_COMPILE=mips-linux-gnu-
    export TOOLCHAIN=/home/software/Linux/mips-4.4-5
    export LIB_DIR=$TOOLCHAIN/lib
;;

mips-le-24kc)
    export ARCH_TYPE=mips
    export CPU_ARCH=24kc
    export CPU_TYPE=0

    export FIXED_ARCH_TYPE=mips-le-24kc

    #
    #  set build optons: little-endian
    #
    export ENDIANNESS=-mel

    export KERNEL_DIR=/home/software/Linux/linux-2.6.19-mips.le
    export CROSS_COMPILE=mipsel-linux-gnu-
    export TOOLCHAIN=/home/software/Linux/tools-2.6.27
    export LIB_DIR=$TOOLCHAIN/lib
;;

*)
   echo "ERROR: Unknown $ARCH, or not support so far"
   exit 1
;;

esac;


########################################################
# set special build options valule
# You can modify the build options for different results according your requirement
#
#    option                    value   description                          default value
#    -------------------------------------------------------------------------------------
#    DEBUG                      1      Enable debugging.                               0
#                               0      Disable debugging.
#
#    NO_DMA_COHERENT            1      Disable coherent DMA function.                  0
#                               0      Enable coherent DMA function.
#
#                                      Please set this to 1 if you are not sure what
#                                      it should be.
#
#    ABI                        0      Change application binary interface, default    0
#                                      is 0 which means no setting
#                                      aapcs-linux For example, build driver for Aspenite board
#
#    LINUX_OABI                 1      Enable this if build environment is ARM OABI.   0
#                               0      Normally disable it for ARM EABI or other machines.
#
#    USE_VDK                    1      Eanble this one when the applications           0
#                                      are using the VDK programming interface.
#                               0      Disable this one when the applications
#                                      are NOT using the VDK programming interface.
#
#                                      Don't eanble gcdSTATIC_LINK (see below)
#                                      at the same time since VDK will load some
#                                      libraries dynamically.
#
#    EGL_API_FB                 1      Use the FBDEV as the GUI system for the EGL.    0
#                               0      Use X11 system as the GUI system for the EGL.
#
#    gcdSTATIC_LINK             1      Enable static linking.                          0
#                               0      Disable static linking;
#
#                                      Don't enable this one when you are building
#                                      GFX driver and HAL unit tests since both of
#                                      them need dynamic linking mechanisim.
#                                      And it must NOT be enabled when USE_VDK=1.
#
#    CUSTOM_PIXMAP              1      Use the user defined Pixmap format in EGL.      0
#                               0      Use X11 pixmap format in EGL.
#
#    ENABLE_GPU_CLOCK_BY_DRIVER 1      Set the GPU clock in the driver.                0
#                               0      The GPU clock should be set by BSP.
#
#    USE_FB_DOUBLE_BUFFER       0      Use single buffer for the FBDEV                 0
#                               1      Use double buffer for the FBDEV (NOTE: the FBDEV must support it)
#
#
#    USE_PLATFORM_DRIVER        1      Use platform driver model to build kernel       1
#                                      module on linux while kernel version is 2.6.
#                               0      Use legecy kernel driver model.
#
#    FPGA_BUILD                 1      To workaround a pecical issue on FPGA board;    0
#                               0      build driver for real chip;
#
#
#    USE_PROFILER               0      Disable driver profiler function;               0
#                               1      Enable driver profiler function;
#
#    VIVANTE_ENABLE_VG          1      Built in GC350 HAL support; default value;
#                               0      Do not build in GC350 HAL support;
#
#    USE_355_VG                 1      Build OpenVG driver for GC350,                  1
#                                      VIVANTE_EANBLE_VG should be 1 at the same time;
#                               0      Build OpenVG driver for 3D VG; default value;
#
#    USE_BANK_ALIGNMENT         1      Enable gcdENABLE_BANK_ALIGNMENT, and video memory is allocated bank aligned.
#                                      The vendor can modify _GetSurfaceBankAlignment() and gcoSURF_GetBankOffsetBytes()
#                                      to define how different types of allocations are bank and channel aligned.
#                               0      Disabled (default), no bank alignment is done.
#
#    BANK_BIT_START             0      Use default start bit of the bank defined in gc_hal_options.h
#                    [BANK_BIT_START]  Specifies the start bit of the bank (inclusive).
#                                      This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled;
#
#    BANK_BIT_END               0      Use default end bit of the bank defined in gc_hal_options.h
#                    [BANK_BIT_END]    Specifies the end bit of the bank (inclusive);
#                                      This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled;
#
#    BANK_CHANNEL_BIT           0      Use default channel bit defined in gc_hal_options.h
#                  [BANK_CHANNEL_BIT]  When set to no-zero, video memory when allocated bank aligned is allocated such
#                                      that render and depth buffer addresses alternate on the channel bit specified.
#                                      This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled.
#
#    DIRECTFB_MAJOR_VERSION     1
#    DIRECTFB_MINOR_VERSION     4
#    DIRECTFB_MICRO_VERSION     0      DirectFB version supported by GFX driver.
#                                      Currentlly we support DirectFB-1.4.0.
#

BUILD_OPTION_DEBUG=0
BUILD_OPTION_ABI=0
BUILD_OPTION_LINUX_OABI=0
BUILD_OPTION_NO_DMA_COHERENT=0
BUILD_OPTION_USE_VDK=0
BUILD_OPTION_EGL_API_FB=1
BUILD_OPTION_gcdSTATIC_LINK=0
BUILD_OPTION_CUSTOM_PIXMAP=0
BUILD_OPTION_USE_3D_VG=1
BUILD_OPTION_USE_FB_DOUBLE_BUFFER=0
BUILD_OPTION_USE_PLATFORM_DRIVER=1
BUILD_OPTION_ENABLE_GPU_CLOCK_BY_DRIVER=0
BUILD_OPTION_CONFIG_DOVEXC5_BOARD=0
BUILD_OPTION_FPGA_BUILD=0
BUILD_OPTION_USE_PROFILER=0
BUILD_OPTION_VIVANTE_ENABLE_VG=1
BUILD_OPTION_USE_355_VG=0
BUILD_OPTION_DIRECTFB_MAJOR_VERSION=1
BUILD_OPTION_DIRECTFB_MINOR_VERSION=4
BUILD_OPTION_DIRECTFB_MICRO_VERSION=0

BUILD_OPTIONS="NO_DMA_COHERENT=$BUILD_OPTION_NO_DMA_COHERENT"
BUILD_OPTIONS="$BUILD_OPTIONS USE_VDK=$BUILD_OPTION_USE_VDK"
BUILD_OPTIONS="$BUILD_OPTIONS EGL_API_FB=$BUILD_OPTION_EGL_API_FB"
BUILD_OPTIONS="$BUILD_OPTIONS gcdSTATIC_LINK=$BUILD_OPTION_gcdSTATIC_LINK"
BUILD_OPTIONS="$BUILD_OPTIONS ABI=$BUILD_OPTION_ABI"
BUILD_OPTIONS="$BUILD_OPTIONS LINUX_OABI=$BUILD_OPTION_LINUX_OABI"
BUILD_OPTIONS="$BUILD_OPTIONS DEBUG=$BUILD_OPTION_DEBUG"
BUILD_OPTIONS="$BUILD_OPTIONS CUSTOM_PIXMAP=$BUILD_OPTION_CUSTOM_PIXMAP"
BUILD_OPTIONS="$BUILD_OPTIONS USE_3D_VG=$BUILD_OPTION_USE_3D_VG"
BUILD_OPTIONS="$BUILD_OPTIONS USE_FB_DOUBLE_BUFFER=$BUILD_OPTION_USE_FB_DOUBLE_BUFFER"
BUILD_OPTIONS="$BUILD_OPTIONS USE_PLATFORM_DRIVER=$BUILD_OPTION_USE_PLATFORM_DRIVER"
BUILD_OPTIONS="$BUILD_OPTIONS ENABLE_GPU_CLOCK_BY_DRIVER=$BUILD_OPTION_ENABLE_GPU_CLOCK_BY_DRIVER"
BUILD_OPTIONS="$BUILD_OPTIONS CONFIG_DOVEXC5_BOARD=$BUILD_OPTION_CONFIG_DOVEXC5_BOARD"
BUILD_OPTIONS="$BUILD_OPTIONS FPGA_BUILD=$BUILD_OPTION_FPGA_BUILD"
BUILD_OPTIONS="$BUILD_OPTIONS USE_PROFILER=$BUILD_OPTION_USE_PROFILER"
BUILD_OPTIONS="$BUILD_OPTIONS VIVANTE_ENABLE_VG=$BUILD_OPTION_VIVANTE_ENABLE_VG"
BUILD_OPTIONS="$BUILD_OPTIONS USE_355_VG=$BUILD_OPTION_USE_355_VG"
BUILD_OPTIONS="$BUILD_OPTIONS DIRECTFB_MAJOR_VERSION=$BUILD_OPTION_DIRECTFB_MAJOR_VERSION"
BUILD_OPTIONS="$BUILD_OPTIONS DIRECTFB_MINOR_VERSION=$BUILD_OPTION_DIRECTFB_MINOR_VERSION"
BUILD_OPTIONS="$BUILD_OPTIONS DIRECTFB_MICRO_VERSION=$BUILD_OPTION_DIRECTFB_MICRO_VERSION"

export PATH=$TOOLCHAIN/bin:$PATH

########################################################
# clean/build driver and samples
# build results will save to $SDK_DIR/
#
cd $AQROOT; make -f makefile.linux $BUILD_OPTIONS clean
cd $AQROOT; make -f makefile.linux $BUILD_OPTIONS install 2>&1 | tee $AQROOT/linux_build.log

########################################################
# other build/clean commands to build/clean specified items, eg.
#
# cd $AQROOT; make -f makefile.linux $BUILD_OPTIONS gal_core V_TARGET=clean || exit 1
# cd $AQROOT; make -f makefile.linux $BUILD_OPTIONS gal_core V_TARGET=install || exit 1

