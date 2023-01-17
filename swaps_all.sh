#!/bin/bash
OUTPUT_NAME=${1}

ls ./result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_statistics/* > result_list_swaps_tmp.txt
truncate ./result_eval/${OUTPUT_NAME}/Swaps_${OUTPUT_NAME}.txt --size 0

while read line
do
#echo -n ${line} | xargs -i basename {} | sed -e -z "s/\.champsimtrace\.xz\.statistics//g" | sed -z 's/\n//g; s/$/\n/' >> Swaps_${OUTPUT_NAME}_tmp.txt
echo -n ${line} >> ./result_eval/${OUTPUT_NAME}/Swaps_${OUTPUT_NAME}.txt
echo -n "," >> ./result_eval/${OUTPUT_NAME}/Swaps_${OUTPUT_NAME}.txt
cat ${line} | grep 'swap_request:' | cut -d " " -f 2 >> ./result_eval/${OUTPUT_NAME}/Swaps_${OUTPUT_NAME}.txt
done < ./result_list_swaps_tmp.txt

rm result_list_swaps_tmp.txt