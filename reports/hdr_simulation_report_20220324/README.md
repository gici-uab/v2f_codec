# HDR report 2022/03/24

The following plots have been discussed during the regular meeting on March 24, 2022.

## New HDR dataset

The following plots apply to the updated red HDR dataset

### H/L ratio

Plots: 

- `ScalarNumericAnalyzer-empirical_hl_factor-histogram.pdf`

Notes:

- Approximately the same H/L factors are obtained, close to 2:1, as observed in the previous dataset version.

### New HDR dataset distribution (average all images)

Plots: 

- `DictNumericAnalyzer-original_h_distribution-line.pdf`
- `DictNumericAnalyzer-original_l_distribution-line.pdf`

Notes:

- The maximum values is 254, not 255
- The most common value is 253, not 254


### Entropy gain

Plots:

- `ScalarNumericAnalyzer-entropy_gain_h_ref_bps-histogram.pdf`
- `ScalarNumericAnalyzer-entropy_gain_l_ref_bps-histogram.pdf`


Notes:

- Approximately the same amount of gain is produced when the L frame is produced using the H frame as referencem, and vice-versa. This was not the case for the simulated HDR dataset.

- The following plots, obtained for the simulated HDR dataset are provided as well for ease of reference (H reference):

    * `sigma0_TwoNumericAnalyzer-columns_offset_euclidean__entropy_gain_high_ref-line-groupby__family_label.pdf`
    * `sigma1_TwoNumericAnalyzer-columns_offset_euclidean__entropy_gain_high_ref-line-groupby__family_label.pdf`
    * `sigma2_TwoNumericAnalyzer-columns_offset_euclidean__entropy_gain_high_ref-line-groupby__family_label.pdf`

