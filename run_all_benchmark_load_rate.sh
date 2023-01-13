#!/bin/bash
OUTPUT_NAME=${1}
execution=./bin/champsim_plus_ramulator_O3_load_rate    # [./bin/champsim_plus_ramulator][./bin/champsim][./bin/champsim_plus_ramulator_O3_single_memory]
warmup=1000000 # warmup_instructions
simulation=1999000000 #simulation_instructions 2000000000/2000000/10000000/100000000
fastmemory=configs/DDR4-config.cfg # slow memory
echo "Run ${execution}"

stats_extension=.HBM_DDR4.stats
ls ./dpc3_traces/* | parallel "${execution} --warmup_instructions ${warmup} --simulation_instructions ${simulation} --stats {/}${stats_extension} ${fastmemory} ./dpc3_traces/{/}"
echo "Finish All Benchmarks."

mkdir ${OUTPUT_NAME}_trace
mv ./*.trace ./${OUTPUT_NAME}_trace/
rm ./*.stats