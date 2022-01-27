#!/bin/bash

# Runs the tests and the fuzzers with a selection of input samples,
# with and without fault injection, so as to exercise all possible
# code branches.

# TO BE CALLED FROM PROJECT ROOT!

set -e

tmpfile1=$(mktemp /tmp/extract_fuzzing.XXXXXXXXXX)
tmpfile2=$(mktemp /tmp/extract_fuzzing.XXXXXXXXXX)

# Run coverage for the afl-cmin'ed set sample
for fuzzer_test_dir in testdata/fuzzing_samples/cmin/*; do
    if [ $(basename $fuzzer_test_dir) == "command_line_fuzzer" ]; then
        for tool_dir in $fuzzer_test_dir/*; do
            tool_number=$(basename $tool_dir)
            echo "Testing CLI tool ${tool_number}..."

            fuzzer_name="command_line_fuzzer_coverage"
            for a in ${tool_dir}/*; do
              echo "Testing $fuzzer_name $tool_number $a..."
              ./build/fuzzers/${fuzzer_name} ${tool_number} < $a >& /dev/null
              echo "... $fuzzer_name $tool_number $a tested."
            done

        done
    else
        fuzzer_name="$(basename $fuzzer_test_dir)_coverage"
        echo "Testing ${fuzzer_name} on $fuzzer_test_dir..."
        for a in $fuzzer_test_dir/*; do
            echo "... $a"
            ./build/fuzzers/${fuzzer_name} < $a 2>&1 > /dev/null
            # These fuzzers may admit 1 or 2 arguments
            # (to store derived fuzzing test vectors) - invoke them to test all the code
            ./build/fuzzers/${fuzzer_name} $tmpfile1 < $a 2>&1 > /dev/null
            rm -rf $tmpfile1
            rm -rf $tmpfile2
            ./build/fuzzers/${fuzzer_name} $tmpfile1 $tmpfile2 < $a 2>&1 > /dev/null
            rm -rf $tmpfile1
            rm -rf $tmpfile2
            echo "... done"
        done
    fi
done

rm -rf $tmpfile1
rm -rf $tmpfile2

OUTPUT_FILE="$1"


