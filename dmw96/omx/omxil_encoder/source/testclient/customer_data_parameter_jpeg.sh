#!/bin/sh -f

RGB_SEQUENCE_HOME=${YUV_SEQUENCE_HOME}/../rgb

# Defined directory path:

in4=${YUV_SEQUENCE_HOME}/vga/kivi_w640h480.yuv
in22=${YUV_SEQUENCE_HOME}/synthetic/kuuba_random_w352h288.uyvy422
in33=${RGB_SEQUENCE_HOME}/1080p/rush_hour_25fps_w1920h1080.rgb565

out=stream.jpg

colorconversion=( 0 0 0 0 0 )
codingmode=( 0 0 0 0 0 )

case "$1" in
    2990|2991|2992|2993|2994) {
        valid=(           0      0      0        -1     -1  )
        input=(           $in33  $in4   $in22   $in22  $in22)
        input_thumbnail=( $in33  $in4   $in22   $in22  $in22)
        output=(          $out   $out   $out    $out   $out )
        write=(           1      1      1       1      1    )
        qtable=(          7      8      3       3      3    )
        width_orig=(      1920   640    352     352    352  )
        height_orig=(     1080   480    288     288    288  )
        width_crop=(      1920   352    352     352    352  )
        height_crop=(     1080   288    288     288    288  )
        nmcu=(            0      6      0       0      0    )
        horoffset=(       0      48     0       0      0    )
        veroffset=(       0      96     0       0      0    )
        mjpeg=(           0      0      0       0      0    )
        frametype=(       4      0      3       3      3    )
        codingtype=(      0      1      0       0      0    )
        markertype=(      0      1      1       1      1    )
        units=(           1      1      1       1      1    )
        xden=(            72     72     72      72     72   )
        yden=(            72     72     72      72     72   )
        rotation=(        0      0      2       2      2    )
        thumb=(           0      0      0       0      0    )
        width_orig_tn=(   0      0      0       0      0    )
        height_orig_tn=(  0      0      0       0      0    )
        width_crop_tn=(   0      0      0       0      0    )
        height_crop_tn=(  0      0      0       0      0    )
        horoffset_tn=(    0      0      0       0      0    )
        veroffset_tn=(    0      0      0       0      0    )
    };;


    * )

    valid=(           -1     -1     -1     -1     -1    );;
    esac

