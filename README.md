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

### Quantization

Three main aspects need to be determined regarding the quantization stage. Its order respect to the prediction stage, the type of quantization performed, and the quantization steps to be used. Design decisions on these aspects are justified next.

#### Order of quantization and prediction

It is well known that applying quantization before prediction typically incurs in a compression performance penalty, as compared to applying quantization after prediction. Notwithstanding, this approach requires the compressor to also apply the decoder's side of the pipeline, in order for both to performed identical predictions. 

This entails a complexity increment at the encoder side, specially for software-based implementations. Furthermore, compression performance differences are often small, specially at high bitrates close to lossless regimes. A comparison of the resulting entropy with the quantization-then-prediction (QP) and prediction-then-quantization (PQ) approaches can be found in [this figure](https://github.com/gici-uab/v2f_codec/raw/master/doc/img/entropy_comparison_qp_pq_order.png).

Taking all this into account, applying quantization after prediction is discarded in favor of quantization followed by prediction.

#### Quantization types

Two important aspects of quantization with direct impact on compression performance and computational complexity are (a) the number of symbols that are quantized together; and (b) the size of each quantization bin, both relative to the dynamic range of the data, and to other quantization bins.

When one symbol is quantized at a time, it is typically referred to as scalar quantization. For the proposed compressor, scalar quantization is used due to is very low complexity, and its competitive compression performance compared to non-scalar methods in the literature. It should be highlighted that this decision is consistent with those made for the most successful codecs analyzed in the state of the art.

The size of each quantization bin determines the maximum error introduced for pixels that fall into that bin, and contributes to the final compressed bit rate. It is well known that, even though optimal quantizers typically have bins of different sizes, using uniformly sized bins often yields similar rate-distortion results except at medium and high maximum pixel errors. Based on this, and also to allow for less complex implementations, uniform scalar quantization is considered hereafter.


### Prediction

The prediction stage produces educated guesses for each sample value based on previously seen samples. Prediction errors (also known as prediction residuals) are then passed to the entropy codec after mapping zero, negative and positive integers to non-negative, integer values. 

When prediction uses only samples pertaining to the image being currently compressed, it is usually referred to as intra prediction. When other images or frames are employed, it is often called inter prediction. For this project, both intra and inter prediction will be considered.

#### Intra prediction

Intra prediction uses only previously seen samples from the image being compressed. Furthermore, to reduce the buffer requirements and compression delay, and to maximize the available redundancy to be exploited, only neighboring samples are considered. In the following table, **X** refers to the sample being predicted, and letters **A**-**J** are neighboring samples.

<br/>

| **A**   | **B**   | **C**   | **D**   |
| --- | --- | --- | --- |
| **E**   | **F**   | **G**   | **H**   |
| **I**   | **J**   |**X**|     |

<br/>

In the following figure, the entropy distribution of the residuals for different prediction methods is shown. Here, **N**, **W** and **NW** are employed as an aliases for **G**, **J** and **F**, respectively. In the figure, the average entropy for the complete test dataset is depicted with a dot. 
It should be highlighted that when more than one pixel is considered, their average value is computed for the prediction.

![Entropy distribution of different prediction methods](https://github.com/gici-uab/v2f_codec/raw/master/doc/img/prediction_distribution_entropy_1B_bps.png)

Several observations can be made based on the obtained results:

- Intra prediction using neighboring pixels is highly accurate and significantly reduces output entropy.

- Using 1 or 2 neighbors typically yields results close to those of more complex predictors that use more neighbors.

- Edge detection and high-frequency detection is not more efficient than exploiting only the low-frequency redundancy.

Based on these observations, the **W** and **JG** yield the best complexity-entropy trade-off for this scenario, and have been included in the codec prototype. 
The more complex but potentially better performing **FGJI** and **JPEG-LS** predictors have also been included in the prototype.

#### Inter prediction 

The exploitation of redundancy among consecutive frames produced onboard the satellites has been researched for this project. 

Unfortunately, none of the tested approaches managed to improve coding efficiency in a significant way. A [full report](https://github.com/gici-uab/v2f_codec/raw/master/reports/stacking_experiment_report_20220322.pdf) analyzing the results of several such approaches is made available. Complementing this report, the following figure illustrates the small gains attained by the highly complex VVC/H.266 codec when the inter mode is activated:

![VVC inter and intra performance](https://github.com/gici-uab/v2f_codec/raw/master/v2f_analysis/plots/vvc_stacking/ScalarNumericAnalyzer-bpppc-histogram-groupby__task_label.png)


### Entropy coding

The entropy coding stage transforms input samples (in this case, prediction residuals) into a compact representation. Among the many existing types of entropy coders, the focus was set on those of low complexity. Therefore, range coders such as arithmetic coders and adaptive methods with complex contexts remain out of the scope of this project. Among the remaining methods, variable-to-fixed (V2F) codes were highlighted in the review of the state of the art. 

These methods work by defining a coding tree adapted to the input probability distribution. When coding, one starts at the root of the tree and advances through edges corresponding to the next input symbol. When no such edge exists (e.g., for leafs), the fixed-width word is emitted and one returns to the root node.

Some of the main advantages of V2F, decisive for using them in this project are as follows:

- These methods transform multiple input samples into a single output word of fixed length. This enables high compression efficiency can be obtained with reasonably sized dictionaries. 

- Codes can be easily tailored for any input distribution. This feature helps V2F codes be competitive and sometimes superior to other entropy coding methods in the literature.

- Producing compressed codewords of fixed length allows very fast software implementations, in particular when that length is an integer number of bytes. This feature has a particularly positive impact on the complexity-compression trade-offs.


## Performance analysis

After completing the implementation of the compression pipeline described above, its coding efficiency and execution time performance was evaluated.
A [full report](https://github.com/gici-uab/v2f_codec/raw/master/reports/final_report_20220511.pdf) describing the obtained results and the employed test corpora is available.


## Project Structure

This repository hosts the compression tool as well as the utilities and performance evaluation scripts developed during this project. The following structure is employed:

- `v2f_prototype_c/`: C implementation of the compressor and decompressor prototypes, with all tools needed for building and testing. Run `make` in this folder to 
   generate the codec binaries and the documentation.

- `forest_generation/`: Python tools used to analyze and design compression pipelines based on Variable to Fixed (V2F) forests.

- `v2f_analysis/` : Compression experiments to analyze its performance, comparing it to other codecs.

- `reports/` : Technical reports analyzing different aspects of the codec, advanced compression methods, and their efficiency.

- `doc/`: Information about the project's structure, the V2F C prototype manual, and several figures shown in this page.

Note that individual `README.md` files can be found in the three first folders with additional information
about their contents.

The following diagram depicts this project's structure.

![Project structure and workflow](https://github.com/gici-uab/v2f_codec/raw/master/doc/img/project_workflow.png)
