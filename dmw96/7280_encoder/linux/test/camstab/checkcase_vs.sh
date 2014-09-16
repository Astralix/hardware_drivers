#!/bin/sh    

#Imports

# import commonconfig
if [ ! -e masterscripts/commonconfig.sh ]
then
    echo "<commonconfig.sh> missing"
    exit 1
else
.  ./masterscripts/commonconfig.sh
fi

let "case_no=$1"

# Test data dir
case_dir=$test_data_home/case_$case_no

if [ ! -e ${case_dir}/video_stab_result.trc ]
then

    cd $systag/7280_encoder/system
    ./test_data/test_data.sh $case_no >> /dev/null 2>&1
    cd ../../../
    #echo "Directory $case_dir doesn't exist!"
    #exit -1
fi

if [ "$test_case_list_dir" == "" ]
then
    echo "Check test_case_list_dir missing <commonconfig.sh>."
    exit -1
fi

if [ ! -e $test_case_list_dir/test_data_parameter_h264.sh ]
then
    echo "<test_data_parameter_h264.sh> doesn't exist!"
    exit -1
fi

. $test_case_list_dir/test_data_parameter_h264.sh "$case_no"
set_nro=$((case_no-case_no/5*5))          
picWidth=$((width[set_nro]))
picHeight=$((height[set_nro]))
casetestId=$((testId[set_nro]))

# run only stab cases */    
if [ ${stabilization[${set_nro}]} -eq 0 ]; then
    echo -e "NOT valid case!"
    exit -1        
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
        echo "Missing decoded YUV file(s) to cmp!"
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

while [ ! -e ${TESTDIR}/run_${case_no}_stab_done ]
do
    sleep 1    
    
done

if [ -e ${case_dir}/video_stab_result.trc ]
then
(

    echo -e -n "Stabilization Case $case_no\t"
           
    # Compare output to reference
    if (cmp -s ${TESTDIR}/vs_case_$case_no.trc ${case_dir}/video_stab_result.trc)
    then
        echo "OK"
        exit 0
    else
        echo "FAILED"
        exit -1
    fi
)
else
(
    echo "FAILED (reference <video_stab_result.trc> missing)"
    exit -1
)
fi
 
