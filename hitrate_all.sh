#!/bin/bash
OUTPUT_NAME=${1}

ls ./result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_statistics/* > result_list_hitrate_tmp.txt
truncate ./result_eval/${OUTPUT_NAME}/hitrate_${OUTPUT_NAME}.txt --size 0

while read line
do
#echo -n ${line} | xargs -i basename {} | sed -e -z "s/\.champsimtrace\.xz\.statistics//g" | sed -z 's/\n//g; s/$/\n/' >> hitrate_${OUTPUT_NAME}.txt
echo -n ${line} >> ./result_eval/${OUTPUT_NAME}/hitrate_${OUTPUT_NAME}.txt
echo -n "," >> ./result_eval/${OUTPUT_NAME}/hitrate_${OUTPUT_NAME}.txt
cat ${line} | grep 'hit rate:' | cut -d " " -f 3 | sed -e 's/\.$//g' >> ./result_eval/${OUTPUT_NAME}/hitrate_${OUTPUT_NAME}.txt
done < ./result_list_hitrate_tmp.txt

rm result_list_hitrate_tmp.txt