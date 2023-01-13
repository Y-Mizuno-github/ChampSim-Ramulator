#!/bin/bash
OUTPUT_NAME=${1}
mkdir ${OUTPUT_NAME}_statistics
mkdir ${OUTPUT_NAME}_stats
mv ./*.statistics ./${OUTPUT_NAME}_statistics/
mv ./*.stats ./${OUTPUT_NAME}_stats/
ls ./${OUTPUT_NAME}_statistics/* > result_list_tmp.txt
truncate ./IPC_${OUTPUT_NAME}_tmp.txt --size 0

while read line
do
#echo -n ${line} | xargs -i basename {} | sed -e -z "s/\.champsimtrace\.xz\.statistics//g" | sed -z 's/\n//g; s/$/\n/' >> IPC_${OUTPUT_NAME}_tmp.txt
echo -n ${line} >> IPC_${OUTPUT_NAME}_tmp.txt
echo -n "," >> IPC_${OUTPUT_NAME}_tmp.txt
cat ${line} | grep 'Finished CPU 0 instructions:' | cut -d " " -f 10 >> IPC_${OUTPUT_NAME}_tmp.txt
echo -n "," >> IPC_${OUTPUT_NAME}_tmp.txt
cat ${line} | grep 'swap request:' | cut -d " " -f 3 >> IPC_${OUTPUT_NAME}_tmp.txt
done < ./result_list_tmp.txt
sort -t'.' -k 1,1 IPC_${OUTPUT_NAME}_tmp.txt > IPC_${OUTPUT_NAME}.txt
rm IPC_${OUTPUT_NAME}_tmp.txt
rm result_list_tmp.txt