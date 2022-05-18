# Retos Colaboración 2020

> # C99 V2F compression prototype

C99 implementation of a full compression pipeline based on V2F code forests.

Author: Universitat Autònoma de Barcelona (UAB): Miguel Hernández-Cabronero et al.

## Compilation instructions

To compile the prototype:

1. Satisfy the requirements described in `DEPENDENCIES.txt`. All of them are
   standard Debian packages and pip packages.
2. Run `make` from `v2f_prototype_c/`.

This will produce:

1. The entry point binaries under `build/bin` (e.g., the compressor and
   decompressor binaries)

2. A set of prebuilt V2F codecs compatible with the aforementioned compressor
   and decompressor binaries. These can be found at `testdata/prebuilt_codecs`.
   These are imported from the V2F forest definition
   in `metasrc/prebuilt_forests`. Note that a basic sanity check is
   automatically applied to all codecs imported this way.

3. All compiled documentation files.
    - A user manual and a requirement traceability matrix In PDF,
      under `build/pdf`
    - A browsable API documentation and design document, e.g.,
      see `build/doxygen/html/index.html`. An offline version of this design
      document is also created at `build/pdf/design_document.pdf`.

## Project distribution

The following folders have been defined to structure the prototype
implementation:

- `src/`: Core library sources
- `bin/`: Entry point binary sources
- `docs/`: Documentation templates and related configuration files
- `test/`: Unit tests sources
- `fuzzing/`: Fuzzing-assisted testing sources.
- `metasrc/`: Internal tool folder for easier compilation and documentation.
  See `metasrc/README.md` for further information.

This project folder should have a sibling one named `forest_generation`. Please
refer to it for how to design and generate V2F forest definitions compatible
with the build script.
