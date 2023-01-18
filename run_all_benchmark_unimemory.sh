#!/bin/bash
OUTPUT_NAME=${1}
MEMORY_NAME=${2}
trace_dir=/home/ChampSim-Ramulator/
execution=/home/ChampSim-Ramulator/bin/champsim_plus_ramulator_O3_unimemory    # [./bin/champsim_plus_ramulator][./bin/champsim][./bin/champsim_plus_ramulator_O3_single_memory]
warmup=1000000 # warmup_instructions
simulation=100000000 #simulation_instructions 2000000000/2000000/10000000/100000000
fastmemory=/home/ChampSim-Ramulator/configs/${MEMORY_NAME}-config.cfg # fast memory
echo "Run ${execution}"

stats_extension=.${MEMORY_NAME}.stats
ls ${trace_dir}dpc3_traces/* | parallel "${execution} --warmup_instructions ${warmup} --simulation_instructions ${simulation} --stats {/}${stats_extension} ${fastmemory} ${trace_dir}dpc3_traces/{/}"
echo "Finish All Benchmarks."

mkdir /home/ChampSim-Ramulator/result_eval/${OUTPUT_NAME}
mkdir /home/ChampSim-Ramulator/result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_statistics
mkdir /home/ChampSim-Ramulator/result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_stats
mv ./*.statistics /home/ChampSim-Ramulator/result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_statistics/
mv ./*.stats /home/ChampSim-Ramulator/result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_stats/
ls /home/ChampSim-Ramulator/result_eval/${OUTPUT_NAME}/${OUTPUT_NAME}_statistics/* > result_list_tmp.txt
truncate ./IPC_${OUTPUT_NAME}_tmp.txt --size 0
truncate ./LLC_miss_Latency_${OUTPUT_NAME}_tmp.txt --size 0

while read line
do
#echo -n ${line} | xargs -i basename {} | sed -e -z "s/\.champsimtrace\.xz\.statistics//g" | sed -z 's/\n//g; s/$/\n/' >> IPC_${OUTPUT_NAME}_tmp.txt
echo -n ${line} >> IPC_${OUTPUT_NAME}_tmp.txt
echo -n "," >> IPC_${OUTPUT_NAME}_tmp.txt
cat ${line} | grep 'Finished CPU 0 instructions:' | cut -d " " -f 10 >> IPC_${OUTPUT_NAME}_tmp.txt
echo -n ${line} >> LLC_miss_Latency_${OUTPUT_NAME}_tmp.txt
echo -n "," >> LLC_miss_Latency_${OUTPUT_NAME}_tmp.txt
cat ${line} | grep 'LLC AVERAGE MISS LATENCY:' | cut -d " " -f 5 >> LLC_miss_Latency_${OUTPUT_NAME}_tmp.txt
done < ./result_list_tmp.txt

sort -t'.' -k 1,1 IPC_${OUTPUT_NAME}_tmp.txt > /home/ChampSim-Ramulator/result_eval/${OUTPUT_NAME}/IPC_${OUTPUT_NAME}.txt
rm IPC_${OUTPUT_NAME}_tmp.txt
sort -t'.' -k 1,1 LLC_miss_Latency_${OUTPUT_NAME}_tmp.txt > /home/ChampSim-Ramulator/result_eval/${OUTPUT_NAME}/LLC_miss_Latency_${OUTPUT_NAME}.txt
rm LLC_miss_Latency_${OUTPUT_NAME}_tmp.txt

rm result_list_tmp.txt

/home/ChampSim-Ramulator/swaps_all.sh ${OUTPUT_NAME}
/home/ChampSim-Ramulator/hitrate_all.sh ${OUTPUT_NAME}