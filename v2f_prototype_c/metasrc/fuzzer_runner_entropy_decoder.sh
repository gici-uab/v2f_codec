#!/bin/bash

# Execute the fuzzer that accepts a (possibly invalid) compressed bitstream as an argument
#
# @req sd-4.2.3, sd-4.4.2

cd $(dirname $0)
cd ..

FUZZER="./build/fuzzers/fuzzer_entropy_decoder"

input_dir="./testdata/fuzzing_samples/cmin/fuzzer_entropy_decoder"

echo "FUZZER=$FUZZER"
echo "input_dir=$input_dir"

output_dir="/dev/shm/v2f_fuzzer/$(basename $FUZZER)"
rm -rf $output_dir
mkdir -p $output_dir

export AFL_SKIP_CPUFREQ=1

echo -e "About to run. To monitor progress, run\n\n\twatch -n1 afl-whatsup -s $output_dir\n\nTo terminate, run\n\n\tkillall afl-fuzz\n\n[Ctrl+Z to set on BG]\n"


./metasrc/fuzz_parallel.sh -m none -i $input_dir -o $output_dir $FUZZER
