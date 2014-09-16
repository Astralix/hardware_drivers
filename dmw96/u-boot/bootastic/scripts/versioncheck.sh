#!/bin/bash

bversion=$(strings bootastic.bin | grep "Bootastic " | cut -d" " -f2 | cut -d- -f1)
uversion=$(strings u-boot.bin| grep "U-Boot dmw" | cut -d" " -f3 | cut -d- -f1)

echo "bversion: $bversion"
echo "uversion: $uversion"

[ "$bversion" == "$uversion" ] && exit 0
exit 1
