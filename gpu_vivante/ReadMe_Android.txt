Android-addon Build for Vivante's Graphics Drivers

Contents

1. Files layout
2. Build instruction
3. Build options

1. Files layout
===============
<AQROOT>/
    |
    +---ReadMe_Android.txt           : ReadMe file for Android-addon Build
    |
    +---Android.mk                   : Make file for Android
    |
    +---Android.mk.def               : Make DEF file for Android
    |
    +---hal
    |   |
    |   +---kernel                   : Make file for HAL kernel driver
    |   |
    |   \---user                     : Make file for HAL user driver
    |
    \---driver
        |
        +---openGL
        |   |
        |   +---egl                  : Make file for EGL driver
        |   |
        |   +---libGLESv11           : Make file for OES11 driver
        |   |
        |   \---libGLESv2x
        |       |
        |       +---driver           : Make file for OES20 driver
        |       |
        |       \---compiler
        |           |
        |           \---libGLESv2SC  : Make file for OES20 compiler driver
        |
        +---coypbit                  : Copybit driver for Android
        |
        \---gralloc                  : gralloc driver for Android


1. Build instruction
====================

  1) Make sure you have install Android source tree
     cd <ANDROID_BUILD_TOP>

  2) Install Vivante driver: VIVANTE_GAL_Unified_Src_drv_<version>.tgz.
     Install Vivante driver in one of directory like <PROJECTS_DIR>
     under Android sdk directory, e.g.

	   mkdir <ANDROID_BUILD_TOP>/<PROJECTS_DIR>
       cd <ANDROID_SDK_TOP_DIR>
       tar zxvf VIVANTE_GAL_Unified_Src_drv_<version>.tgz -C <PROJECTS_DIR>

  3) Install Vivante Android-addon: VIVANTE_GAL_Unified_Src_drv_android-addon_<version>.tgz.

       cd <ANDROID_SDK_TOP_DIR>
       tar zxvf VIVANTE_GAL_Unified_Src_drv_android-addon_<version>.tgz -C <PROJECTS_DIR>

  4) Modify build options in <PROJECTS_DIR>/Android.mk.def for build environment.

        ARCH_TYPE                   ?= arm                                   # CPU arch type. Could be 'arm' or 'mips'

        RO_HARDWARE                 ?=                                       # <ro.hardware> property, which comes from
                                                                             # 'Hardware' field of 'cpuinfo' of the device.
                                                                             # This affect <property> field of copybit/gralloc

        FIXED_ARCH_TYPE             ?= arm-android                           # To select ES20 pre-built complier binary files
                                                                             # Could be 'arm-android' or 'mips-android'

        GPU_TYPE                    ?= XAQ2                                  # To set AQARCH

        export KERNEL_DIR           ?= $(ANDROID_BUILD_TOP)/kernel/kernel    # Kernel directory

        export CROSS_COMPILE        ?= arm-eabi-                             # Cross compiler for building kernel module

        # driver build options, please refer to build options introduce in section 2.
        NO_DMA_COHERENT             ?= 1
        ENABLE_GPU_CLOCK_BY_DRIVER  ?= 0
        USE_PLATFORM_DRIVER         ?= 1
        USE_PROFILER                ?= 0
        USE_COPYBIT_MODE_FOR_ANDROID?= 1
        USE_LINUX_MODE_FOR_ANDROID  ?= 0
        ENABLE_CACHED_VIDEO_MEMORY  ?= 1

  5) Set up Android build environment, typicaly,

        . build/envsetup.sh
        chooseprodcut <productname>

  6) Build Vivante driver.

        cd <PROJECTS_DIR>
        mm                          # use "mm -B" do clean build

  7) Check out build result.

        $OUT/system/lib/modules/galcore.ko
        $OUT/system/lib/libGAL.so
        $OUT/system/lib/egl/libEGL_VIVANTE.so
        $OUT/system/lib/egl/libGLESv1_CM_VIVANTE.so
        $OUT/system/lib/egl/libGLESv2_VIVANTE.so
        $OUT/system/lib/libGLSLC.so
        $OUT/system/lib/hw/gralloc.<property>.so
        $OUT/system/lib/hw/copybit.<property>.so

2. Build options
================

   There are a lot of OPTIONS to control how to build the driver.
   To enable/disable an option, set <OPTION>=<value> in the command line.
    option                 value      description
    -------------------------------------------------
    DEBUG                        0     Disable debugging;default value;
                                 1     Enable debugging;

    NO_DMA_COHERENT              0     Enable coherent DMA;default value;
                                 1     Disable coherent DMA;

    USE_PLATFORM_DRIVER          1     Set USE_PLATFORM_DRIVER to 1 if Linux kernel version is 2.6.x;default value;
                                 0     Set USE_PLATFORM_DRIVER to 0 for some old Linux kernel.

    ENABLE_GPU_CLOCK_BY_DRIVER   1     Enable GPU clock in driver layer for some special boards;
                                 0     Do not care GPU clock setting in driver;

    USE_PROFILER                 0     Disable driver profiler function;default value;
                                 1     Enable driver profiler function;

    gcdFPGA_BUILD                1     To workaround a FPGA special issue;
                                 0     Don't care it for real chip; default value;

    USE_COPYBIT_MODE_FOR_ANDROID 1     Gralloc is built to work with vivante 2D copybit driver for composition; default value;
                                 0     Gralloc is built to work with s/w opengles for composition;

    USE_LINUX_MODE_FOR_ANDROID   0     Build as vanilla Linux in Android environment with VDK backend; default value;
                                 1     Enable VDK backend support

    ENABLE_CACHED_VIDEO_MEMORY   0     Disable caching for the video memory
                                 1     Enable caching for the video memory; default value;

    DEFER_RESOLVES               0     Disable deferred resolves, used for composition bypass;used for HWC.
                                 1     Enable deferred resolves, used for composition bypass;used for HWC.

    BANK_BIT_START               0     Use default start bit of the bank defined in gc_hal_options.h
                  [BANK_BIT_START]     Specifies the start bit of the bank (inclusive).
                                       This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled;

    BANK_BIT_END                 0     Use default end bit of the bank defined in gc_hal_options.h
                    [BANK_BIT_END]     Specifies the end bit of the bank (inclusive);
                                       This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled;

    BANK_CHANNEL_BIT             0     Use default channel bit defined in gc_hal_options.h
                [BANK_CHANNEL_BIT]     When set to no-zero, video memory when allocated bank aligned is allocated such
                                       that render and depth buffer addresses alternate on the channel bit specified.
                                       This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled.
