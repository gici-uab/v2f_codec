# Retos Colaboración 2020

> # The `metasrc/` folder

This directory contains different scripts to aid with the compilation,
documentation, and basic sanity checks of the project.

## Build scripts

There should be no need to invoke anything from this folder; the `Makefile`
provided at `v2f_prototype_c/` automatically performs execution of all needed
scripts.

## Prebuilt forests

The `metasrc/prebuilt_forests` directory determines which V2F forest definition
files are imported as V2F codecs.

All `*.v2fh` files in this directory (V2F forest definition) are transformed
into `*.v2fc` files with a full V2F codec definition, and stored at
`testdata/prebuilt_codecs`.

Please see the `forest_generation/` module on how to add new forest definitions
here optimized for your data.

