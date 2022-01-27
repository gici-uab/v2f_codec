#!/bin/bash

# Execute the command line fuzzers
#
# @req sd-4.2.3, sd-4.4.2

cd $(dirname $0)
cd ..

./metasrc/fuzzer_runner_cli_one_10cores.sh 0
./metasrc/fuzzer_runner_cli_one_10cores.sh 1
./metasrc/fuzzer_runner_cli_one_10cores.sh 2

sleep 1000000h
