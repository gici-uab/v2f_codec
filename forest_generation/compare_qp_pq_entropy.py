#!/usr/bin/env python3
"""Analyze the entropy differences between the 
quantization-prediction and prediction-quantization coding approaches.
"""
__author__ = "Miguel Hernández-Cabronero"
__since__ = "2022/05/04"

import os
import pandas as pd
import ast
import math
import re
import enb

if __name__ == "__main__":
    enb.config.options.analysis_dir = os.path.join("analysis", "pq_qp_comparison")
    enb.config.options.plot_dir = os.path.join("plots_comparison_qp_pq_entropies")

    qp_template = os.path.join("persistence_QP", "persistence_q{qstep}", "persistence_summary_prediction_q{qstep}.csv")
    pq_template = os.path.join("persistence_PQ", "persistence_q{qstep}", "persistence_summary_prediction_q{qstep}.csv")

    combined_df = None
    for qstep in range(1, 9):
        path_order_pairs = [(qp_template.format(qstep=qstep), "QP"),
                            (pq_template.format(qstep=qstep), "PQ")]
        
        for path, order in path_order_pairs:
            df = pd.read_csv(path)
            df = df[~df["group_label"].isin(["shadow_out", "All"])]
            df.loc[:, "qstep"] = qstep
            df.loc[:, "order"] = order
            df.loc[:, "avg_symbol_to_p_noshadow"] = df.loc[:, "avg_symbol_to_p_noshadow"].apply(ast.literal_eval)
            df.loc[:, "entropy"] = df.loc[:, "avg_symbol_to_p_noshadow"].apply(
                lambda d: -sum(x * math.log2(x) if x > 0 else 0 for x in d.values()))

            combined_df = df.copy() if combined_df is None else pd.concat((combined_df, df))

    enb.aanalysis.ScalarNumericAnalyzer().get_df(
        full_df=combined_df,
        target_columns=["entropy"],
        selected_render_modes={"histogram", "boxplot"},
        group_by=["order", "qstep"],
        color_by_group_name={str(label): f"C{label[1]-1}" for label, _ in combined_df.groupby(by=["order", "qstep"])},
        y_labels_by_group_name={str(label): f"Order {label[0]}, Qstep {label[1]}" for label, _ in combined_df.groupby(by=["order", "qstep"])},
        fig_height=6)
