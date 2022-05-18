#!/usr/bin/env python3
"""Experiment that evaluates the basic properties of the datasets employed
for the evaluation of the V2F codec prototype.
"""
__author__ = "Miguel Hernández-Cabronero"
__since__ = "2022/05/11"

import os
import numpy as np
import enb

class DatasetAnalysisTable(enb.isets.ImagePropertiesTable, enb.isets.SampleDistributionTable):
    pass

if __name__ == "__main__":
    enb.config.options.plot_dir = os.path.join("plots", "dataset_analysis")
    enb.config.options.analysis_dir = os.path.join("analysis", "dataset_analysis")
    enb.config.options.persistence_dir = os.path.join("persistence", "dataset_analysis")
    
    # Compute basic properties of all files
    dat = DatasetAnalysisTable()
    df = dat.get_df()
    
    # Analyze and plot the most relevant properties
    ## Scalar values
    sna = enb.aanalysis.ScalarNumericAnalyzer()
    dat.column_to_properties["sample_min"].plot_max = 2.5
    dat.column_to_properties["sample_max"].plot_min = 240
    dat.column_to_properties["entropy_1B_bps"].plot_min = 5
    dat.column_to_properties["entropy_1B_bps"].plot_max = 7
    sna.get_df(
        full_df=df,
        target_columns=["sample_min", "sample_max", "dynamic_range_bits", "entropy_1B_bps"],
        column_to_properties=dat.column_to_properties,
        group_by="corpus",
        show_global=True)
    ## Distribution analyss
    dna = enb.aanalysis.DictNumericAnalyzer()
    dna.main_marker_size = 1
    dna.show_grid = True
    dna.get_df(
        full_df=df,
        target_columns=["sample_distribution"],
        column_to_properties=dat.column_to_properties,
        group_by="corpus",
        show_global=True,
        x_tick_list=[int(x) for x in np.linspace(df["sample_min"].min(), df["sample_max"].max(), 10)],
        y_tick_list=[0, 0.01, 0.02, 0.03, 0.04],
        y_min=0, y_max=0.04)

