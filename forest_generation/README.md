# V2F forest generation

## Description

This folder contains python code used to analyze the properties of input Satellogic datasets,
and produce a set of V2F coding forests adapted for specific probability distributions.
The `quantize_predict_analyze_makeforests.py` is the main entry point for performing this analysis 
and forest generation experiment. The resulting `.v2fh` files can then be transformed into
the `.v2fc` codec headers used by `v2f_prototype_c`.

A set of prebuilt `.v2fh` coding forests optimized for several configurations using the `boats2020` and `fields2020`
datasets (images not included) can be found in the `prebuilt_forests` folder.

The `v2f.py` module provides the key implementation aspects of of several V2F forest design algorithm.
This includes the `FastYamamotoForest` class, which is the one intended for use in the prototype.

The remaining python modules define utility functions employed by the other aforementioned modules.

## Requirements

- pip3 install enb
- build-essential or the equivalent package in your SO to run `make` and compile a jpegls predictor

## How to train V2F forests for new datasets

1. The `make_link_sets.py` script needs to be adapted so that the `satellogic_all` folder contains
   the images for which V2F forests are to be trained. 
2. Run the `clean.sh` script to delete all previous training results.
3. Run the `quantize_predict_analyze_makeforests.py` script.
4. Copy or link the contents of `prebuilt_forests` into `../v2f_prototype_c/metasrc/prebuilt_forests`.
5. Run the `../v2f_prototype_c/metasrc/generate_test_samples.py` script.

Note that step 3 takes several hours.
