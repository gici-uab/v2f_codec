# V2F python analysis 

This folder contains python code used to analyze the properties of input Satellogic datasets,
and produce a set of pre-built V2F coding forests adapted for specific probability distributions.
The `quantize_predict_analyze_makeforests.py` is the main entry point for performing this analysis 
and forest generation experiment.

The `v2f.py` module provides the key implementation aspects of of several V2F forest design algorithm.
This includes the `FastYamamotoForest` class, which is the one intended for use in the prototype.

The remaining python modules define utility functions employed by the other aforementioned modules.

## Requirements

- pip3 install enb
- build-essential or equivalent for making the jpegls .so 
