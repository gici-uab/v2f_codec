#!/bin/bash

# Script that invokes as many coordinater afl fuzzers
# as CPUs are reported by nproc --all,
# with any parameters passed as argument.

trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM EXIT

for i in $(seq -f "%03g" 2 $(nproc --all)); do
    AFL_SKIP_CPUFREQ=1 afl-fuzz -t 2000 -S fuzzer$i "$@" > /dev/null &
#     AFL_SKIP_CPUFREQ=1 afl-fuzz -S fuzzer$i "$@" &
    sleep 0.5
done

AFL_SKIP_CPUFREQ=1 afl-fuzz -t 2000 -M fuzzer001 "$@"  > /dev/null
# AFL_SKIP_CPUFREQ=1 afl-fuzz -t 2000 -M fuzzer01 "$@"

kill -- -$$
