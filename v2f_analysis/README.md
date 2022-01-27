# V2F Analysis Experiments

This folder contains a number of experiments performed on the Satellogic corpus of images 
to evaluate the performance of V2F codecs testing different types of basic and advanced
compression methods.

The latest version of the `enb` python library is required to run this scripts (`pip install enb`).

# Experiment summary

## Basic compression experiments

- `lossless_compression_experiment.py`: This experiment test the lossless compression capabilities
  of the developed V2F codec, comparing its performance to that of the stat of the art including
  JPEG 2000, JPEG-LS and CCSDS 123.0-B-2.
- `lossy_compression_experiment.py`: This experiment test the lossy compression capabilities
  of the developed V2F codec, comparing its performance to that of the stat of the art including
  JPEG 2000, JPEG-LS and CCSDS 123.0-B-2 para diferentes configuraciones de calidad.

## Advanced compression experiments

- `displacement_experiment.py`: this experiment evaluates the expected performance lossless using 
  basic prediction with a frame that is displace a number of pixels in horizontal, vertical or
  diagonal directions.

- `band_component_experiment.py`: this experiment takes the four non-shadow regions of each image and
  stacks them into a new image with 4 spectral bands. The JPEG 2000 and CCSDS 123.0-B-2 standards are
  executed to exploit spectral correlation within those spectral bands.

- `moving_component_experiment.py`: this experiment is similar to `band_component_experiment.py`, with
  the difference that the spectral band of each output image is taken from one consecutive image of
  the original dataset.

- `N_component_experiment.py`: this experiment stacks all images into a single image of N samples
  for the whole dataset, for the `boats` subset, and for the `fields` subset. The resulting images are
  then expoited in a similar fashion as in `moving_component_experiment.py`


# Project structure

The following directory structure is employed.

- `datasets/`: enb will look for test samples here by default. Samples may be arranged in any desired subfolder
  structure, and symbolic links are followed.
- `plugins/`: folder where enb plugins can be installed. It contains an `__init__.py` file so that plugins can be
  imported from the code. Run `enb plugin list` to obtain a list of available plugins.
  A plugin for the V2F prototype is include as well.
- `plots/`: data analysis outputs the resulting plots in this folder by default.
- `analysis/`: data analysis outputs tables and other numerical results in this folder by default.
- `persistence`: each script has its dedicated persistence folder in side this directory.
