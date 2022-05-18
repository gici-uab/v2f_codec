#!/usr/bin/env python3
"""Summarize the results produced by the different frame/region stacking experiments
"""

import os
import enb
import numpy as np
import pandas as pd

from enb.config import options

from lossless_compression_experiment import LosslessExperiment


class CompressionExperimentSummaryTable(enb.atable.SummaryTable):
    """Summary table for the compression performance of different codecs in a given set of images"
    """

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"compression_ratio",
                                     label=f"Compression Ratio")])
    def compression_ratio(self, index, row):
        df = self.label_to_df[index]
        row["compression_ratio"] = np.mean(df["compression_ratio"])

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"compression_time_seconds",
                                     label=f"Compression Time Seconds")])
    def compression_time(self, index, row):
        df = self.label_to_df[index]
        row["compression_time_seconds"] = np.mean(df["compression_time_seconds"])

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"decompression_time_seconds",
                                     label=f"Decompression Time Seconds")])
    def decompression_time(self, index, row):
        df = self.label_to_df[index]
        row["decompression_time_seconds"] = np.mean(df["decompression_time_seconds"])

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"bpppc",
                                     label=f"bpppc")])
    def bpppc(self, index, row):
        df = self.label_to_df[index]
        row["bpppc"] = np.mean(df["bpppc"])

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"compression_efficiency_1byte_entropy",
                                     label=f"compression_efficiency_1byte_entropy")])
    def efficiency(self, index, row):
        df = self.label_to_df[index]
        row["compression_efficiency_1byte_entropy"] = np.mean(df["compression_efficiency_1byte_entropy"])


if __name__ == '__main__':
    os.makedirs('persistence/persistence_stacked_summary/band_component', exist_ok=True)
    os.makedirs('persistence/persistence_stacked_summary/moving_component', exist_ok=True)
    os.makedirs('persistence/persistence_stacked_summary/N_component', exist_ok=True)
    os.makedirs('persistence/persistence_stacked_summary/original', exist_ok=True)

    options.persistence_dir = os.path.join("persistence", "persistence_band_component_experiment.py")

    df_list = ["persistence/persistence_band_component_experiment_experiment.py/LosslessExperiment_persistence.csv",
               "persistence/persistence_moving_component_experiment_experiment.py/LosslessExperiment_persistence.csv",
               "persistence/persistence_n_component_experiment.py/LosslessExperiment_persistence.csv",
               "persistence/persistence_lossless_compression_experiment.py/LosslessExperiment_persistence.csv"]

    summary_path_list = [os.path.join("persistence", "persistence_stacked_summary", "band_component",
                                      "persistence_lossless_band_component.csv"),
                         os.path.join("persistence", "persistence_stacked_summary", "moving_component",
                                      "persistence_lossless_moving_component.csv"),
                         os.path.join("persistence", "persistence_stacked_summary", "N_component",
                                      "persistence_lossless_N_component.csv"),
                         os.path.join("persistence", "persistence_stacked_summary", "original",
                                      "persistence_lossless_original.csv")]
    
    # Force regeneration of the summary even if it has been computed before
    for p in summary_path_list:
        if os.path.exists(p):
            os.remove(p)

    for d, s in zip(df_list, summary_path_list):
        df = pd.read_csv(d)

        df["corpus"] = ["fields2020" if "fields" in c else "boats2020" if "boats" in c else "Other"
                        for c in df["file_path"]]

        summary_table = CompressionExperimentSummaryTable(
            full_df=df,
            column_to_properties=LosslessExperiment.column_to_properties,
            group_by=["task_name", "corpus"],
            include_all_group=True,
            csv_support_path=s
        )

        summary_table.get_df()
