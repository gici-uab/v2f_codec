#!/bin/bash

# Execute the fuzzer that accepts a parameter file, followed by packet data,
# and then executes compression, decompression and losslessness check.
#
# @req sd-4.2.2, sd-4.4.2

cd $(dirname $0)
cd ..

FUZZER="./build/fuzzers/v2f_encoder_custom_fuzzer"

input_dir="./testdata/fuzzing_samples/cmin/v2f_encoder_custom_fuzzer"

output_dir="/dev/shm/v2f_fuzzer/$(basename $FUZZER)"
rm -rf $output_dir
mkdir -p $output_dir

export AFL_SKIP_CPUFREQ=1

echo -e "About to run. To monitor progress, run\n\n\twatch -n1 afl-whatsup -s $output_dir\n\nTo terminate, run\n\n\tkillall afl-fuzz\n\n[Ctrl+Z to set on BG]\n"

./metasrc/fuzz_parallel.sh -m none -i $input_dir -o $output_dir $FUZZER
