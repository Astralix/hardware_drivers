#!/bin/bash

# This script runs a single test case or all test cases

#Imports

.  ./f_run.sh

# import commonconfig
if [ ! -e masterscripts/commonconfig.sh ]
then
    echo "<commonconfig.sh> missing"
    exit 1
else
.  ./masterscripts/commonconfig.sh
fi

mount_point=/export # this is for versatile
if [ ! -e $mount_point ]; then
    mount_point=/mnt/export # this is for integrator
fi

#SET THIS
# HW burst size 0..63
burst=16


# Output file when running all cases
# .log and .txt are created
resultfile=results_vs

# Current date
#date=`date +"%d.%m.%y %k:%M"`

# Check parameter
if [ $# -eq 0 ]; then
    echo "Usage: $0 <test_case_number>"
    echo "   or: $0 <all>"
    echo "   or: $0 <first_case> <last_case>"
    exit -1
fi

vs_bin=./videostabtest

if [ ! -e $vs_bin ]; then
    echo "Executable $vs_bin not found. Compile with 'make'"
    exit -1
fi

if [ ! -e $test_case_list_dir/test_data_parameter_h264.sh ]; then
    echo "<test_data_parameter_h264.sh> not found"
    echo "$test_case_list_dir/test_data_parameter_h264.sh" 
    exit -1
fi

process() { 

    # run only stab cases */    
    if [ ${stabilization[${set_nro}]} -eq 0 ]; then
        return        
    fi
    
    ${vs_bin} \
    -i${input[${set_nro}]} \
    -a${firstVop[${set_nro}]} \
    -b${lastVop[${set_nro}]} \
    -w${lumWidthSrc[${set_nro}]} \
    -h${lumHeightSrc[${set_nro}]} \
    -W${width[${set_nro}]} \
    -H${height[${set_nro}]} \
    -l${inputFormat[${set_nro}]} \
    -T \
    -N${burst} \
    -P$trigger
    
    #-f${outputRateNumer[${set_nro}]} \
    #-F${outputRateDenom[${set_nro}]} \
    #-j${inputRateNumer[${set_nro}]} \
    #-J${inputRateDenom[${set_nro}]} \    
    #-r${rotation[${set_nro}]}
}    

test_set() {
    # Test data dir
    let "set_nro=${case_nro}-${case_nro}/5*5"
    case_dir=$test_data_home/case_$case_nro

    # Do nothing if test case is not valid
    if [ ${valid[${set_nro}]} -eq -1 ]; then
        echo "case_$case_nro is not valid."
    else
        # Do nothing if test data doesn't exists
        #if [ ! -e $case_dir ]; then
        #    echo "Directory $case_dir doesn't exist!"
        #    echo "You must generate test data first using system model."
        #else
            # Invoke new shell
            (
            echo "========================================================="
            echo "Run case $case_nro"

            # Run stabilization
            
    	    startTimeCounter "$case_nro"
	    
	    process > vs.log
            
    	    endTimeCounter "$case_nro"
	    
	    cat vs.log
            #mv video_stab_result.log vs_case_$case_nro.trc
	    
	    if [ -e random_run ]
	    then
	        mv video_stab_result.log random_cases/vs_case_$case_nro.trc
                touch random_cases/run_${case_nro}_stab_done
		mv case_${case_nro}.time random_cases/case_${case_nro}.time
	    else
                mv video_stab_result.log vs_case_$case_nro.trc
		touch run_${case_nro}_stab_done
            fi


            # Make script run.sh for this testcase
            vs_bin="echo $vs_bin "
            process > run_vs.sh


            )
        #fi
    fi
}

run_cases() {    
    echo "Running test cases $first_case..$last_case"
    echo "This will take several minutes"
    echo "Output is stored in $resultfile.log"
    rm -f $resultfile.log
    for ((case_nro=$first_case; case_nro<=$last_case; case_nro++))
    do
        . $test_case_list_dir/test_data_parameter_h264.sh "$case_nro"
        echo -en "\rCase $case_nro\t"
        test_set >> $resultfile.log
	
    done
    echo -e "\nAll done!"
}

run_case() {
    . $test_case_list_dir/test_data_parameter_h264.sh "$case_nro"
    test_set
}

# vs test set.
trigger=-1 # NO Logic analyzer trigger by default

case "$1" in
    all)
    first_case=1750;
    last_case=1800;
    run_cases
    ;;
    *)
    if [ $# -eq 3 ]; then trigger=$3; fi
    if [ $# -eq 1 ]; then
        case_nro=$1;
        run_case;
    else
        first_case=$1;
        last_case=$2;
        run_cases;
    fi
    ;;
esac


