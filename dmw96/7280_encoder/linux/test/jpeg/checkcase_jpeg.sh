#!/bin/sh
    
test_data_home=/export/work/testdata/7280_testdata
out_dir=.
 
let "case_nro=$1"

# Test data dir
case_dir=$test_data_home/case_$case_nro

# Do nothing if test data doesn't exist
if [ ! -e $case_dir ]
then
    echo "Directory $case_dir doesn't exist!"
else
(
    # Compare stream to reference
    if [ -e ${case_dir}/stream.jpg ]
    then
    (
        echo -e -n "JPEG Case $case_nro\t"
        if (cmp -s ${out_dir}/case_$case_nro.jpg ${case_dir}/stream.jpg)
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
        echo "FAILED(reference stream.jpg missing)"
	exit -1
    )
    fi
)
fi
 
