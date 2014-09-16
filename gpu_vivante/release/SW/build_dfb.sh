#!/bin/sh

# enable/disable the multi application core support
multi_app_core=no

# setup cross compiler
export CROSS_COMPILE=arm-none-linux-gnueabi-
export CROSS_COMPILE_HOST=arm-none-linux-gnueabi
export CROSS_COMPILE_TARGET=arm-none-linux-gnueabi
export TOOLCHAIN=/home/software/Linux/arm-none-linux-gnueabi
export PATH=$TOOLCHAIN/bin:$PATH

if [ -z $DIRECTFB_ROOT_DIR ]; then
    DIRECTFB_ROOT_DIR=/home/software/Linux/dfb/arm
fi
# set PREXFIX for DirectFB installation directory
if [ $multi_app_core = "yes" ]; then
export DIRECTFB_DIR=$DIRECTFB_ROOT_DIR/dfb_1.4.15_mp
else
export DIRECTFB_DIR=$DIRECTFB_ROOT_DIR/dfb_1.4.15_sp
fi

export PREFIX=$DIRECTFB_DIR
mkdir -p $PREFIX || exit 1
# Added $PREFIX/bin to your 'PATH' environment before compiling
export PATH=$PREFIX/bin:$PATH

# set some special build flag to resolve build issue
export LIBPNG_CFLAGS="-I$PREFIX/include/libpng12 -I$PREFIX/include"
export CFLAGS="-I$PREFIX/include -I$PREFIX/usr/include"
export LDFLAGS="-L$PREFIX/lib"
export LIBPNG_CONFIG=$PREFIX/bin/libpng-config
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

# build directory
if [ ! -z $dfb_stuff_src_dir ]; then
    dfb_stuff_src_dir=~/projects/dfb_stuff
fi
zlib_src_dir=$dfb_stuff_src_dir/zlib-1.2.3
libpng_src_dir=$dfb_stuff_src_dir/libpng-1.2.29
jpeg_src_dir=$dfb_stuff_src_dir/jpeg-6b
freetype_src_dir=$dfb_stuff_src_dir/freetype-2.3.5
linux_fusion_src_dir=$dfb_stuff_src_dir/linux-fusion-8.7.0
directfb_src_dir=$dfb_stuff_src_dir/DirectFB-1.4.15
directfb_src_samples_dir=$dfb_stuff_src_dir/DirectFB-examples-1.5.3

build_directfb()
{
    builddir=$1
    #
    # enable/disable more configuration at here
    # e.g.
    #   --enable_multi=no    # to diable the multi application core
    #   --enable_multi=yes   # to enable the multi application core
    #   --enable-jpeg        # will enable JPEG image support; remove it if you don't need JPEG image support
    cd $builddir; make distclean
    cd $builddir; ./configure --host=$CROSS_COMPILE_HOST --target=$CROSS_COMPILE_TARGET --prefix=$PREFIX --libdir=$PREFIX/lib --includedir=$PREFIX/include --disable-x11 --disable-voodoo --disable-mmx --disable-see --disable-sdl --enable-jpeg --enable-zlib --enable-png --enable-gif --enable-freetype --enable-video4linux --enable-video4linux2 --with-gfxdrivers=none --enable-fbdev=yes --disable-devmem --enable-multi=$multi_app_core --with-inputdrivers=keyboard,ps2mouse,linuxinput
    cd $builddir; make || exit 1
    cd $builddir; make install || exit 1
}

build_linux_fusion()
{
    local builddir=$1
    # exported more build environment variable for linux fusion driver module
    export LINUX_VERSION=2.6.30.4-dove-3.1.1
    export KERNEL_DIR=/home/software/Linux/linux-2.6.30.4

    export SYSROOT=$PREFIX
    export KERNEL_VERSION=$LINUX_VERSION
    export KERNEL_BUILD=$KERNEL_DIR
    export KERNEL_SOURCE=$KERNEL_DIR
    export KERNELDIR=$KERNEL_DIR
    export INSTALL_MOD_PATH=$PREFIX

    cd $builddir; make ARCH=arm KERNEL_VERSION=$LINUX_VERSION KERNEL_BUILD=$KERNEL_DIR KERNEL_SOURCE=$KERNEL_DIR SYSROOT=$FUSION_ROOT_DIR INSTALL_MOD_PATH=$PREFIX KERNELDIR=$KERNEL_DIR clean || exit 1
    cd $builddir; make ARCH=arm KERNEL_VERSION=$LINUX_VERSION KERNEL_BUILD=$KERNEL_DIR KERNEL_SOURCE=$KERNEL_DIR SYSROOT=$FUSION_ROOT_DIR INSTALL_MOD_PATH=$PREFIX KERNELDIR=$KERNEL_DIR  || exit 1
    cd $builddir; make ARCH=arm KERNEL_VERSION=$LINUX_VERSION KERNEL_BUILD=$KERNEL_DIR KERNEL_SOURCE=$KERNEL_DIR SYSROOT=$FUSION_ROOT_DIR INSTALL_MOD_PATH=$PREFIX KERNELDIR=$KERNEL_DIR  install || exit 1
}

build_zlib()
{
    local builddir=$1
    export CC=${CROSS_COMPILE}gcc
    CFLAGS="-fPIC -O3 $CFLAGS"
    cd $builddir; make distclean
    cd $builddir; ./configure --prefix=$PREFIX --shared
    cd $builddir; make  || exit 1
    cd $builddir; make install || exit 1
    unset CC
}

build_libpng()
{
    local builddir=$1

    cd $builddir; make distclean
    cd $builddir; ./configure --host=$CROSS_COMPILE_HOST --prefix=$PREFIX
    cd $builddir; make  || exit 1
    cd $builddir; make install || exit 1
}

build_jpeg()
{
    local builddir=$1
    export CC=${CROSS_COMPILE}gcc
#    echo ${CC-cc} aaaa && exit 1
    cd $builddir; mkdir -p $PREFIX/man/man1
    cd $builddir; make clean
    cd $builddir; ./configure --host=$CC --prefix=$PREFIX --without-x --enable-shared --without-libjasper
    cd $builddir; make  || exit 1
    cd $builddir; make install || exit 1
    unset CC-cc
    unset CC
}

build_freetype()
{
    local builddir=$1
    cd $builddir; make distclean
    cd $builddir; ./configure --host=$CROSS_COMPILE_HOST --target=$CROSS_COMPILE_TARGET --prefix=$PREFIX --without-x --enable-directfb --disable-xlib --disable-win32
    cd $builddir; make  || exit 1
    cd $builddir; make install || exit 1
}

build_directfb_samples()
{
    local builddir=$1
    cd $builddir; make distclean

    export LIBS=-lz
    cd $builddir; ./configure --host=$CROSS_COMPILE_HOST --prefix=$PREFIX

    cd $builddir; make || exit 1
    cd $builddir; make install || exit 1
}

echo build jpeg library ...
echo =========================
if [ -e $jpeg_src_dir ]; then
    build_jpeg $jpeg_src_dir
fi

echo build zlib library ...
echo =========================
if [ -e $zlib_src_dir ]; then
    build_zlib $zlib_src_dir
fi

echo build libpng library ...
echo =========================
if [ -e $libpng_src_dir ]; then
    build_libpng $libpng_src_dir
fi

echo build jpeg library ...
echo =========================
if [ -e $jpeg_src_dir ]; then
    build_jpeg $jpeg_src_dir
fi

echo build freetype font library ...
echo =========================
if [ -e $freetype_src_dir ]; then
    build_freetype $freetype_src_dir
fi

if [ $multi_app_core = "yes" ]; then
    echo build linux fusion library ...
    echo =========================
    if [ -e $linux_fusion_src_dir ]; then
        build_linux_fusion $linux_fusion_src_dir
    fi
fi

echo build directFB library ...
echo =========================
if [ -e $directfb_src_dir ]; then
    build_directfb $directfb_src_dir
fi

echo build directFB samples ...
echo =========================
if [ -e $directfb_samples_src_dir ]; then
    build_directfb_samples $directfb_src_samples_dir
fi

