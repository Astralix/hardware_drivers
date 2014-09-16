#!/bin/bash

set -e

pagesize=$1
oobsize=$2
infile=$3
outfile=$4

ecc_mode=4 # 1 = hamming, 4 = bch4
n1=4
n3=2
n2=$(( ($oobsize / (pagesize / 512)) - $n1 ))

make bin2pages
make BCH_Implementation
make vpjoin

# remove old data files
rm -rf BCH_ref${oobsize}_${ecc_mode}_${n1}_${n3}_${n2}_0_*

./bin2pages $pagesize $oobsize $infile
./BCH_Implementation $ecc_mode $n1 $n3 $n2 0 ref$oobsize -i bin2pages.out

# remove unneeded generated files
rm -rf BCH_ref${oobsize}*{Corrected_Data,ECC_CALC,Errors_loc,Read_Data,Setup}.txt
./vpjoin $pagesize $ecc_mode $outfile BCH_ref${oobsize}_${ecc_mode}_${n1}_${n3}_${n2}_0_*
