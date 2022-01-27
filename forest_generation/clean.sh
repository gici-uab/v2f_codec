#!/bin/bash

# Clean all intermediate and persistence data folders, similar to a make clean in spirit.

# WARNING: This tool does not remove anything that cannot be re-created given the same input samples,
# but it might take some time to re-run the complete tests.

rm -rf satellogic_* tmp* persistence* prebuilt_* plots_* analysis debug_storage __pycache__
