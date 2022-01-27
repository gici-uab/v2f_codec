#!/bin/bash

# Execute the command line fuzzers
#
# @req sd-4.2.3, sd-4.4.2

cd $(dirname $0)
cd ..

export AFL_SKIP_CPUFREQ=1
FUZZER="./build/fuzzers/command_line_fuzzer"

tool_index=$1
input_dir="./testdata/fuzzing_samples/cmin/command_line_fuzzer/${tool_index}"
output_dir="/dev/shm/v2f_fuzzer/$(basename $FUZZER)_${tool_index}"
rm -rf $output_dir
mkdir -p $output_dir
./metasrc/fuzz_parallel_10cores.sh -m none -i $input_dir -o $output_dir $FUZZER ${tool_index} &
