Files layout
============
\VIVANTE_SDK
|   ReadMe.txt
|
+---bin
|       libEGL.dll                      # the driver of EGL
|       libGAL.dll                      # the user driver of HAL
|       libGALCore.dll                  # the kernel driver of HAL
|       libGLESv1_CM.dll                  # OpenGL ES 1.1 driver
|       libGLESv2.dll                  # OpenGL ES 2.0 driver
|       libGLSLC.dll                 # OpenGL ES 2.0 shader compiler
|       libOpenVG.dll                   # OpenVG driver
|		libVDK.dll						# Vivante Development Kit
|       PBUserProjects.reg              # REG file for kernel dirver of HAL on WinCE5.0
|       PBUserProjects.bib              # BIB file for kernel dirver of HAL on WinCE5.0
|
+---inc
|	|	gc_vdk_types.h
|	|	gc_vdk.h
|	|
|   +---HAL
|   |       gc_hal.h
|   |       gc_hal_types.h
|   |       gc_hal_enum.h
|   |       gc_hal_dump.h
|   |       gc_hal_base.h
|   |       gc_hal_raster.h
|   |       gc_hal_options.h
|   |       gc_hal_engine.h
|   |       gc_hal_rename.h
|   |       gc_hal_driver.h
|   |       gc_hal_profiler.h
|   |
|	+---KHR
|	|		khrplatform.h
|   |       khrvivante.h
|	|
|   +---EGL
|   |       egl.h
|   |       eglext.h
|   |       eglplatform.h
|   |       eglvivante.h
|   |       eglrename.h
|   |       eglunname.h
|   |
|   +---GLES
|   |       gl.h
|	|		glext.h
|	|		egl.h
|   |       glplatform.h
|   |       glrename.h
|   |       glunname.h
|   |
|   +---GLES2
|   |       gl2.h
|   |       gl2ext.h
|   |       gl2platform.h
|   |       gl2rename.h
|   |       gl2unname.h
|   |
|   \---VG
|           openvg.h
|           vgu.h
|           vgext.h
|           vgplatform.h
|           vgrename.h
|           vgunname.h
|
\---lib
       libEGL.lib
       libGAL.lib
       libGLSLC.lib
       libGLESv2.lib
       libGLESv1_CM.lib
       libOpenVG.lib
	   libVDK.lib


Running applications on the target machine
==========================================

1. Added the kernel driver of HAL(libGalCore.dll) to WinCE OS image.
   1). Clean and build your WinCE OS project;
   2). Add the content of PBUserProjects.reg under sdk\bin to the %_FLATRELEASEDIR%\PBUserProjects.reg of WinCE OS project;
   3). Add the content of PBUserProjects.bib under sdk\bin to the %_FLATRELEASEDIR%\PBUserProjects.bib of WinCE OS project;
   4). Make run-time image;

2. Copy other VIVANTE driver to the %_FLATRELEASEDIR% directory of WinCE OS project.

3. Copy Samples to the path: %_FLATRELEASEDIR% of WinCE OS project.

4. Load WinCE OS image on your target machine, then run the application
        eg.
        cd \release\Samples\tutorials
        gces11_t1.exe
