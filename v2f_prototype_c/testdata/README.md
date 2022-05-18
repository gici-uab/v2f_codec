# Retos Colaboración 2020

> # The `testdata/` folder

This folder contains different types of test vectors needed for the unit tests
and the fuzzers.

Important notes:

- Running `make` at the project folder automatically populates this folder, by
  invoking `metasrc/generate_test_samples.py`.

- Data manually added to `testdata/prebuilt_codecs` is overwritten with every
  call to `metasrc/generate_test_samples.py`. You can change the contents
  of `metasrc/prebuilt_forests`
  to configure the forest definitions that are imported into the project.

## `unittest_samples/`: Unit test data

Data employed to exercise the unitary tests defined in `test/`.

## `fuzzing_samples/`: Fuzzing test data

Fuzzing vectors that can be used in combination with the `afl` toolchain.

Fuzzing data is divided in two parts, each stored in a different folder within
`fuzzing_samples/`. Within each part, there is one subfolder per fuzzer, with
the same name as the binary. The two parts are:

- `seed/`: Initial test vectors for each of the fuzzers, produced by
  `metasrc/generate_test_samples.py`, to be fed to `afl`. The
  `metasrc/fuzzer_runner_*.sh` scripts perform this in parallel with the help
  of `metasrc/fuzz_parallel.sh`. These are only needed to produce comprehensive
  testing vectors from scratch.

- `queues/`: All unique test vectors explored by `afl`.

- `cmin/`: A minimal set of `queues/` as produced by `afl-cmin` for the
  appropriate fuzzers.
 
