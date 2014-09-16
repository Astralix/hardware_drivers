#!/bin/bash
#-------------------------------------------------------------------------------
#-                                                                            --
#-       This software is confidential and proprietary and may be used        --
#-        only as expressly authorized by a licensing agreement from          --
#-                                                                            --
#-                            Hantro Products Oy.                             --
#-                                                                            --
#-                   (C) COPYRIGHT 2009 HANTRO PRODUCTS OY                    --
#-                            ALL RIGHTS RESERVED                             --
#-                                                                            --
#-                 The entire notice above must be reproduced                 --
#-                  on all copies and should not be removed.                  --
#-                                                                            --
#-------------------------------------------------------------------------------
#-
#--  Abstract : Test script
#--
#-------------------------------------------------------------------------------
#--
#--  Version control information, please leave untouched.
#--
#--  $RCSfile: customer_data_parameter_mpeg4.sh,v $
#--  $Date: 2009-11-30 14:14:23 $
#--  $Revision: 1.1 $
#--
#-------------------------------------------------------------------------------

RGB_SEQUENCE_HOME=${YUV_SEQUENCE_HOME}/../rgb


# Input YUV files
in18=${YUV_SEQUENCE_HOME}/qcif/metro2_25fps_qcif.yuv
in22=${YUV_SEQUENCE_HOME}/synthetic/kuuba_random_w352h288.uyvy422
in84=${YUV_SEQUENCE_HOME}/synthetic/rangetest_w352h288.yuv
in100=${YUV_SEQUENCE_HOME}/cif/metro_25fps_cif.yuv

# User data
ud1=${USER_DATA_HOME}/userDataVos.txt
ud2=${USER_DATA_HOME}/userDataVisObj.txt
ud3=${USER_DATA_HOME}/userDataVol.txt
ud4=${USER_DATA_HOME}/userDataGov.txt
ud5=${USER_DATA_HOME}/null.txt

# Mpeg4 stream, do not change
out=stream.mpeg4

# Default parameters
colorConversion=( 0 0 0 0 0 )

case "$1" in
                        990|    991|    992|    993|    994) {
	valid=(			    0	    0	    0	    0	    0	)
	input=(			    $in100	$in18	$in84	$in22	$in100	)
	output=(		    $out    $out	$out	$out	$out	)
	firstVop=(		    0	    0	    0	    0	    0	)
	lastVop=(		    1	    30	    100	    1	    5	)
	lumWidthSrc=(	    352	    176	    352	    352	    352	)
	lumHeightSrc=(	    288	    144	    288	    288	    288	)
	width=(			    112	    176	    336	    112	    96	)
	height=(		    96	    144	    272	    96	    96	)
	horOffsetSrc=(		0	    0	    8	    0	    100	)
	verOffsetSrc=(		1	    0	    8	    0	    100	)
	outputRateNumer=(	30	    30	    30	    30	    30000)
	outputRateDenom=(	1	    1	    1	    1		1001)
	inputRateNumer=(	30	    30	    30	    30	    30000)
	inputRateDenom=(	1		1	    1	    1	    1001)
	scheme=(		    0		0	    0		0	    3	)
	profile=(		    2		3	    2		2	    1007)
	videoRange=(		1		0	    0	    1	    1	)
	intraDcVlcThr=(		0		0	    0	    0	    0	)
	acPred=(		    0		0	    0		0	    0	)
	intraVopRate=(		0		30	    0   	0	    2	)
	goVopRate=(		    0		0	    0		0	    0	)
	cir=(			    0		0	    0		0	    0	)
	vpSize=(		    0		0	    0		0	    0	)
	dataPart=(		    0		0	    0		0	    0	)
	rvlc=(			    0		0	    0		0	    0	)
	hec=(			    0		0	    0		0	    0	)
	gobPlace=(		    0		0	    0		0	    3	)
	bitPerSecond=(      0		100000  0	    0		100000	)
	vopRc=(			    0		1	    0		0	    1	)
	mbRc=(			    0		0	    0		0	    1	)
	videoBufferSize=(	0		0	    0	    0	    10	)
	vopSkip=(		    0		1	    0	    0	    0	)
	qpHdr=(			    1		10	    1	    1	    31	)
	qpMin=(			    1		1	    1	    1	    1	)
	qpMax=(			    31	    31	    31	    31	    31	)
	inputFormat=(		0	    0	    0	    3	    0	)
	rotation=(		    1	    0	    0	    0	    0	)
	stabilization=(		0	    0	    1	    0	    0	)
	userDataVos=(		$ud1	$ud1	$ud1	$ud1	$ud1	)
	userDataVisObj=(	$ud2	$ud2	$ud2	$ud2	$ud2	)
	userDataVol=(		$ud3	$ud3	$ud3	$ud3	$ud3	)
	userDataGov=(		$ud4	$ud4	$ud4	$ud4	$ud4	)
	testId=(		0	0	0	0	0	)
        category=("mpeg4_cropping"
                  "mpeg4_rate_control"
                  "mpeg4 stabilization"
                  "mpeg4 Input uyvy422 (interleaved)"
                  "H263")
	desc=("Test cropping. Offset 1x0. Frame size 112x96. IF 0, rotation +90."
		"Test rate control frame skip with frame RC"
		"Test stabilization. Frame size 336x272. IF 0, rot. 0."
		"Test cropping. Offset 0x0. Frame size 112x96. IF 3, rot. 0."
		"Test rate control")
	};;

	* )
	valid=(			-1	-1	-1	-1	-1	);;
esac


