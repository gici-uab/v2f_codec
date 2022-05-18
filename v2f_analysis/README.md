# V2F Analysis Experiments

This folder contains a number of experiments performed on the Satellogic corpus of images 
to evaluate the performance of V2F codecs testing different types of basic and advanced
compression methods.

The `enb` python library (version >= 0.4.0) is required to run these scripts 
(e.g., with `pip install enb`, see [its github page](https://github.com/miguelinux314/experiment-notebook)).

Satellogic's test dataset is not included; please se `datasets/README.md` for more information
about how to evaluate the V2F codec with your own images.

# Experiment summary

## Basic compression experiments

- `lossless_compression_experiment.py`: This experiment test the lossless compression capabilities
  of the developed V2F codec, comparing its performance to that of the stat of the art including
  JPEG 2000, JPEG-LS and CCSDS 123.0-B-2.
- `lossy_compression_experiment.py`: This experiment test the lossy compression capabilities
  of the developed V2F codec, comparing its performance to that of the stat of the art including
  JPEG 2000, JPEG-LS and CCSDS 123.0-B-2 para diferentes configuraciones de calidad.

Both experiments evaluate the effect of turning on/off the compression of shadow regions.

**Note**: these experiment require the availability of privative codecs that cannot be redistributed here
([kakadu JPEG 2000](kakadusoftware.com/) 
and [lcnl](https://logiciels.cnes.fr/en/license/173/749), i.e., CNES' implementation of the CCSDS 123.0-B-2). 

## Advanced compression experiments

A number of advanced compression experiments have been added to explore
additional ways of improving compression efficiency.

- `displacement_experiment.py`: this experiment evaluates the expected performance lossless using 
  basic prediction with a frame that is displaced a number of pixels in horizontal, vertical or
  diagonal directions.

- `band_component_experiment.py`: this experiment takes the four non-shadow regions of each image and
  stacks them into a new image with 4 spectral bands. The JPEG 2000 and CCSDS 123.0-B-2 standards are
  executed to exploit spectral correlation within those spectral bands.

- `moving_component_experiment.py`: this experiment is similar to `band_component_experiment.py`, with
  the difference that the spectral band of each output image is taken from one consecutive image of
  the original dataset.

- `N_component_experiment.py`: this experiment stacks all images into a single image of N samples
  for the whole dataset, for the `boats` subset, and for the `fields` subset. The resulting images are
  then expoited in a similar fashion as in `moving_component_experiment.py`.

- `vvc_inter_lossless_experiment.py`: this experiment evaluates the performance of the VVC video codec
   in both intra and inter compression regimes. Note that this is an extremely slow experiment if 
   rerun from scratch.


# Project structure

The following directory structure is employed.

- `datasets/`: enb will look for test samples here by default. Samples may be arranged in any desired subfolder
  structure, and symbolic links are followed. Experiments have been run with real data from Satellogic which,
  unfortunately, cannot be redistributed here.
- `prebuilt_codecs`: directory of `*.v2fc` V2F code forest files produced by `../v2f_prototype_c/metasrc/generate_test_samples.py`.
   See `../forest_generation/README.md` for additional information about how to generate these code forests.
- `plugins/`: folder where enb plugins can be installed. The aforementioned experiment scripts will 
  install enb plugins as necessary. Note that you will need to provide your own binaries to the lcnl and kakadu
  plugins, which are no redistributed due to licensing restrictions.
- `plots/`: data analysis outputs the resulting plots in this folder by default. Plots for Satellogic's data are included,
   even though the data itself is not.
- `analysis/`: data analysis outputs tables and other numerical results in this folder by default.
   Tables for Satellogic's corpora are included, even though the data itself is not.
- `persistence`: each script has its dedicated persistence folder in side this directory. Precomputed results
   for the aforementioned Satellogic test corpora.
   
