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

csvfile=$csv_path/integrationreport_mpeg4_${hwtag}_${swtag}_${reporter}_$reporttime

# parse argument list
while [ $# -ge 1 ]; do
        case $1 in
        -csv*)  CSV_REPORT=1 ;;
        -*)     echo -e $Usage; exit 1 ;;
        *)      ;;
        esac

        shift
done

if [ $CSV_REPORT -eq 1 ]
then
    echo "TestCase;TestPhase;Category;TestedPictures;Language;StatusDate;ExecutionTime;Status;HWTag;EncSystemTag;SWTag;TestPerson;Comments;Performance;KernelVersion;RootfsVersion;CompilerVersion" >> $csvfile.csv
fi

first_case=0
last_case=999
    
for ((case_nr=$first_case; case_nr<=$last_case; case_nr++))
do
    set_nro=${case_nr}-${case_nr}/5*5
    
    . $test_case_list_dir/test_data_parameter_mpeg4.sh "$case_nr"
    
    getExecutionTime "$case_nr"
  
    if [ ${valid[${set_nro}]} -eq 0 ]
    then
    
        ttt=`echo | sh checkcase_mpeg4.sh $case_nr`
        res=$?

        if [ $CSV_REPORT -eq 1 ]
        then
            echo -n "case_$case_nr;Integration;MPEG4Case;;$language;$timeform1 $timeform2;$execution_time;" >> $csvfile.csv
	    echo -n "case_$case_nr;" >> $resultcsv
            case $res in
            0)
                echo "OK;$hwtag;$systag;$swtag;$reporter;$comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv
                echo "OK;" >> $resultcsv
		;;
            1)
                extra_comment=${comment}"Stream differences but YUV data OK."
                echo "FAILED;$hwtag;$systag;$swtag;$reporter;$extra_comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv
                echo "FAILED;" >> $resultcsv
		;;
	    2)
                extra_comment=${comment}" $ttt."
                echo "NOT FAILED;$hwtag;$systag;$swtag;$reporter;$extra_comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv
                echo "NOT FAILED;" >> $resultcsv
	        ;;
	    3)
                extra_comment=${comment}" $ttt."
                echo "NOT FAILED;$hwtag;$systag;$swtag;$reporter;$extra_comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv
                echo "NOT FAILED;" >> $resultcsv
		;;		
            *)
                extra_comment=${comment}" $ttt."
                echo "FAILED;$hwtag;$systag;$swtag;$reporter;$extra_comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv
                echo "FAILED;" >> $resultcsv
		;;
            esac
        fi
        
        echo $ttt   
        
     fi                      
done
