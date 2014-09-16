#!/bin/sh    

#Imports

# Import commonconfig
if [ ! -e masterscripts/commonconfig.sh ]
then
    echo "<commonconfig.sh> missing"
    exit 1
else
.  ./masterscripts/commonconfig.sh
fi


supportedPicWidth=$MAX_WIDTH
supportedPicHeight=$MAX_HEIGHT
roi=0

let "case_no=$1"

SWDEC=./mx170dec_pclinux

# Test data dir
case_dir=$test_data_home/case_$case_no
 
# If test data doesn't exist, generate it
if [ ! -e $case_dir ]
then
    cd $systag/7280_encoder/system
    ./test_data/test_data.sh $case_no >> /dev/null 2>&1
    cd ../../../

#    echo "Directory $case_dir doesn't exist!"
#    exit -1
fi

if [ "$test_case_list_dir" == "" ]
then
    echo "Check test_case_list_dir missing from <commonconfig.sh>."
    exit -1
fi

if [ ! -e $test_case_list_dir/test_data_parameter_mpeg4.sh ]
then
    echo "<test_data_parameter_mpeg4.sh> doesn't exist!"
    exit -1
fi

. $test_case_list_dir/test_data_parameter_mpeg4.sh "$case_no"
set_nro=$((case_no-case_no/5*5))          
picWidth=$((width[set_nro]))
picHeight=$((height[set_nro]))
casetestId=$((testId[set_nro]))

if [ $picWidth -gt $supportedPicWidth ] || [ $picHeight -gt $supportedPicHeight ]
then
    echo "MPEG4 Case $case_no Unsupported picture width or height!"
    exit 2
fi

if ( [ "$INTERNAL_TEST" == "n" ] )
then

    if [ $casetestId != "0" ] 
    then
        echo "Internal test, does not work with customer releases!"
        exit 2
    fi
fi


# Find the failing picture
findFailingPicture()
{
 
    if ! [ -e $1 ] || ! [ -e $2  ]
    then
        echo -n "Missing decoded YUV file! "
        failing_picture=0
        export failing_picture
        return 1
    fi
 
    # Find the failing picture
    # Calculate the failing picture using the picture width and height
    error_pixel_number=`cmp $1 $2 | awk '{print $5}' | awk 'BEGIN {FS=","} {print $1}'`

    if [ "$error_pixel_number" != "" ] 
    then
	# The multiplier should be 1,5 (multiply the values by 10)
	pixel_in_pic=$((picWidth*picHeight*3/2))
	failing_picture=$((error_pixel_number/pixel_in_pic))
	export failing_picture
    return 1
    else
    return 0
    fi
}

# Compare stream to reference
if [ -e ${case_dir}/stream.mpeg4 ]
then
(
    echo -e -n "MPEG4 Case $case_no\t"
    
    while [ ! -e ${TESTDIR}/run_${case_no}_done ]
    do
        sleep 1    
    done
    
    # Compare output to reference
    if (cmp -s ${TESTDIR}/case_$case_no.mpeg4 ${case_dir}/stream.mpeg4)
    then
        echo "OK"
        exit 0
    else
        ${SWDEC} -O${TESTDIR}/case_$case_no.yuv ${TESTDIR}/case_$case_no.mpeg4 &> dec_mpeg4.log	
	if [ -e ${TESTDIR}/case_${case_no}.yuv ]
        then
            tmp=`ls -l ${TESTDIR}/case_${case_no}.yuv | awk '{print $5}'`
	    if [ $tmp -eq 0 ]
	    then
	        echo "case_$case_no.yuv size 0!"
		exit -1
            fi
	else
	    echo "case_$case_no.yuv doesn't exist!"
            exit -1    
        fi
	
        ${SWDEC} -O${TESTDIR}/ref_case_$case_no.yuv ${case_dir}/stream.mpeg4 &> dec_mpeg4.log
	if [ -e ${TESTDIR}/ref_case_${case_no}.yuv ]
        then
            tmp=`ls -l ${TESTDIR}/ref_case_${case_no}.yuv | awk '{print $5}'`
	    if [ $tmp -eq 0 ]
	    then
	        echo "ref_case_$case_no.yuv size 0!"
		exit -1
            fi
	else
	    echo "ref_case_$case_no.yuv doesn't exist!"
            exit -1    
        fi
                
        # Compare output to reference
        findFailingPicture ${TESTDIR}/case_$case_no.yuv ${TESTDIR}/ref_case_$case_no.yuv
               
        if [ $? == 0 ]
        then
            echo "Stream differences but YUV data OK"
            rm -f ${TESTDIR}/case_$case_no.yuv ${TESTDIR}/ref_case_$case_no.yuv
            exit 1
        else
            #findFailingPicture case_$case_no.yuv ref_case_$case_no.yuv
            echo "FAILED in picture $failing_picture"       
            exit -1
        fi
    fi
)
else
(
    echo "MPEG4 Case $case_no FAILED (reference 'stream.mpeg4' missing)"
    exit 3
)
fi
