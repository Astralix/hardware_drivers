#!/bin/sh

#Imports
.  ./f_check.sh

# import commonconfig
if [ ! -e masterscripts/commonconfig.sh ]
then
    echo "<commonconfig.sh> missing"
    exit 1
else
.  ./masterscripts/commonconfig.sh
fi

Usage="\n$0 [-csv]\n\t -csv generate csv file report\n"

resultcsv=result.csv

CSV_REPORT=0

csvfile=$csv_path/integrationreport_vs_${hwtag}_${swtag}_${reporter}_$reporttime

# parse argument list
while [ $# -ge 1 ]; do
        case $1 in
        -csv*)
              CSV_REPORT=1
              echo -e "CSV report file generated to:\n$csvfile" ;;
        -*)   echo -e $Usage; exit 1 ;;
        *)    ;;
        esac

        shift
done

if [ ! -e $test_case_list_dir/test_data_parameter_h264.sh ]
then
    echo "<test_data_parameter_h264.sh> doesn't exist!"
    exit -1
fi

if [ $CSV_REPORT -eq 1 ]
then
    echo "TestCase;TestPhase;Category;TestedPictures;Language;StatusDate;ExecutionTimeSec;Status;HWTag;EncSystemTag;SWTag;TestPerson;Comments;Performance;KernelVersion;RootfsVersion;CompilerVersion;TestDeviceIP;TestDeviceVersion" >> $csvfile.csv
fi

first_case=1750
last_case=1800
    
for ((case_nr=$first_case; case_nr<=$last_case; case_nr++))
do
    let "set_nro=${case_nr}-${case_nr}/5*5"
     
    . $test_case_list_dir/test_data_parameter_h264.sh "$case_nr"
    
    getExecutionTime "$case_nr"
  
    if [ ${valid[${set_nro}]} -eq 0 ] && [ ${stabilization[${set_nro}]} -eq 1 ]
    then
        
        ttt=`echo | sh checkcase_vs.sh $case_nr`
        res=$?
               
        if [ $CSV_REPORT -eq 1 ]
        then
            echo -n "case_$case_nr;Integration;Standalone stabilization;;$language;$timeform1 $timeform2;$execution_time;" >> $csvfile.csv 
            echo -n "vs_case_$case_nr;" >> $resultcsv
            case $res in
            0)
                echo "OK;$hwtag;$systag;$swtag;$reporter;$comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv 
		echo "OK;" >> $resultcsv
		;;     
            *)
                echo "FAILED;$hwtag;$systag;$swtag;$reporter;$comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv 
		echo "FAILED;" >> $resultcsv
		;; 
            esac
        fi
        
        echo $ttt        
     fi                      
done
