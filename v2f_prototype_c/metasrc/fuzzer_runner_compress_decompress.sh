#!/bin/bash

# Execute the fuzzer that accepts an input sample and a header
# path and performs compression. Only the samples are fuzzed.
#
# @req sd-4.2.3, sd-4.4.2

cd $(dirname $0)
cd ..

FUZZER="./build/fuzzers/fuzzer_compress_decompress"

input_dir="./testdata/fuzzing_samples/cmin/fuzzer_compress_decompress"

output_dir="/dev/shm/v2f_fuzzer/$(basename $FUZZER)"
rm -rf $output_dir
mkdir -p $output_dir

export AFL_SKIP_CPUFREQ=1

 echo -e "About to run. To monitor progress, run\n\n\twatch -n1 afl-whatsup -s $output_dir\n\nTo terminate, run\n\n\tkillall afl-fuzz\n\n[Ctrl+Z to set on BG]\n"


./metasrc/fuzz_parallel.sh -m none -i $input_dir -o $output_dir $FUZZER
