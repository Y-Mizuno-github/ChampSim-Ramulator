#!/bin/bash
OUTPUT_NAME=${1}
execution=./bin/champsim_plus_ramulator_O3_CAMEO_ideal_llt    # [./bin/champsim_plus_ramulator][./bin/champsim][./bin/champsim_plus_ramulator_O3_single_memory]
warmup=1000000 # warmup_instructions
simulation=100000000 #simulation_instructions 2000000000/2000000/10000000/100000000
fastmemory=configs/HBM-config.cfg # fast memory
slowmemory=configs/DDR4-config.cfg # slow memory
echo "Run ${execution}"

stats_extension=.HBM_DDR4.stats
ls ./dpc3_traces/* | parallel "${execution} --warmup_instructions ${warmup} --simulation_instructions ${simulation} --stats {/}${stats_extension} ${fastmemory} ${slowmemory} ./dpc3_traces/{/}"
echo "Finish All Benchmarks."

mkdir ./result_eval/${OUTPUT_NAME}
mkdir ./result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_statistics
mkdir ./result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_stats
mv ./*.statistics ./result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_statistics/
mv ./*.stats ./result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_stats/
ls ./result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_statistics/* > result_list_tmp.txt
truncate ./IPC_${OUTPUT_NAME}_tmp.txt --size 0

while read line
do
#echo -n ${line} | xargs -i basename {} | sed -e -z "s/\.champsimtrace\.xz\.statistics//g" | sed -z 's/\n//g; s/$/\n/' >> IPC_${OUTPUT_NAME}_tmp.txt
echo -n ${line} >> IPC_${OUTPUT_NAME}_tmp.txt
echo -n "," >> IPC_${OUTPUT_NAME}_tmp.txt
cat ${line} | grep 'Finished CPU 0 instructions:' | cut -d " " -f 10 >> IPC_${OUTPUT_NAME}_tmp.txt
done < ./result_list_tmp.txt
sort -t'.' -k 1,1 IPC_${OUTPUT_NAME}_tmp.txt > ./result_eval/${OUTPUT_NAME}/IPC_${OUTPUT_NAME}.txt
rm IPC_${OUTPUT_NAME}_tmp.txt
rm result_list_tmp.txt

./swaps_all.sh ${OUTPUT_NAME}
./hitrate_all.sh ${OUTPUT_NAME}