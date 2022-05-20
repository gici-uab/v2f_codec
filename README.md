# Retos Colaboración - UAB - Compression system

## Introduction

Data compression is an essential tool to provide efficient communication between satellites and ground stations. When used in lossless
regime, original data can be reconstructed with perfect fidelity. However, this is typically at the cost of reduced compression ratios, i.e.,
relatively large compressed data volumes. On the other hand, lossy data compression allows producing significantly smaller data
volumes, at the cost of introducing some distortion between the reconstructed and original samples. 

One important goal of the iquaflow tool is to be able to assess the impact of lossy compression from the perspective of the different
analysis tasks for which the images are captured. Leveraging this information, compression performance can be optimized without
compromising the scientific nor commercial value of the reconstructed data. To achieve this goal, a fully functional codec suited for
space use is developed complementing the iquaflow toolbox. This codec provides both lossless and lossy compression regimes. More specifically,
near-lossless compression with user-defined maximum absolute error is available.

To maximize the relevance of any analysis performed using this codec, the compression and decompression algorithms have been
tailored to the general case of Earth Observation, aiming to jointly improve data compression and computational complexity results.
The remainder of this page provides details on the design decisions and relevant metrics considered during the development of the
aforementioned codec.

This document and the compression system were developed by the Group on Interactive Coding of Images (GICI) of the 
Universitat Autònoma de Barcelona (UAB) as a part of the Retos Colaboración RTC2019-007434-7 project.

## State of the art

A full review of the state of the art has been conducted to establish the best type of compression/decompression pipeline as well as the baseline algorithms for comparison. A detailed summary of this review is available in [this technical report](https://github.com/gici-uab/v2f_codec/raw/master/reports/progress_report_202103.pdf). 
Instead of duplicating contents therein included, only a summary of the most relevant outcomes is described next.

An introduction to satellite constellations from the general perspective of the project is provided first. The need for data compression, and further discussion on lossless versus lossy data compression is also available. The most relevant data compression methods suitable for satellite constellations are compared next.

Experimental have been used to assess the compression performance and complexity trade-offs of each algorithm. Analysis of these results suggests that the family of variable-to-fixed (V2F) codes are the most suitable to provide more beneficial trade-offs that other methods in the state of the art. Note that a quantitative and qualitative description of the test dataset is also available in the technical report.


## Compression pipeline design

The first and more general design decision is the pipeline structure of the codec. Due to the constraint asymmetry between the compressor and the decompressor, this structure is determined entirely by the compressor side of the algorithm.

Based on the review of the state of the art, high-throughput entropy coding is applied after a decorrelation stage. The two main parts of decorrelation are quantization (which introduces distortion except for uniform quantization of step size 1), and prediction. The resulting pipeline is depicted in the following picture. The remainder of this section describes other results and design decisions specific to each of the pipeline stages.

![quantization, prediction, and entropy coding](https://github.com/gici-uab/v2f_codec/raw/master/doc/img/quantization_prediction_entropycoding.png)


## Project Structure

This repository hosts updates to the compression tool.

- `forest_generation/`: Python tools used to analyze and design compression pipelines based on Variable to Fixed (V2F) forests.

- `v2f_prototype_c/`: C implementation of the compressor and decompressor prototypes, with all tools needed for building and testing.

- `v2f_analysis/` : compression experiments to analyze its performance, comparing it to other codecs

- `reports/` : technical reports analyzing different aspects of the codec, advanced compression methods, and their efficiency

- `doc/`: information about the project's structure and the V2F C prototype manual
