#!/bin/bash

DATE=`date --rfc-3339=date`

rm test_tmp.log
rm test_script.txt

cat g1_customer_test_cases.txt >> test_script.txt

./video_decoder &> test_tmp.log

grep cmp test_tmp.log > test_g1_"$DATE".log
grep ^FAILED test_tmp.log >> test_g1_"$DATE".log
grep -A4 Result test_tmp.log >> test_g1_"$DATE".log

