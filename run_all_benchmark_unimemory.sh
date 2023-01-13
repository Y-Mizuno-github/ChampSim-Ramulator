#!/bin/bash
OUTPUT_NAME=${1}
execution=./bin/champsim_plus_ramulator_O3_single_memory    # [./bin/champsim_plus_ramulator][./bin/champsim][./bin/champsim_plus_ramulator_O3_single_memory]
warmup=1000000 # warmup_instructions
simulation=100000000 #simulation_instructions 2000000000/2000000/10000000/100000000
fastmemory=configs/DDR4-config.cfg # slow memory
echo "Run ${execution}"

stats_extension=.HBM_DDR4.stats
ls ./dpc3_traces/* | parallel "${execution} --warmup_instructions ${warmup} --simulation_instructions ${simulation} --stats {/}${stats_extension} ${fastmemory} ./dpc3_traces/{/}"
echo "Finish All Benchmarks."

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
done < ./result_list_tmp.txt
sort -t'.' -k 1,1 IPC_${OUTPUT_NAME}_tmp.txt > IPC_${OUTPUT_NAME}.txt
rm IPC_${OUTPUT_NAME}_tmp.txt
rm result_list_tmp.txt

./swaps_all.sh ${OUTPUT_NAME}