#!/usr/bin/env python3
"""Analyze the quantized entropy and reconstructed fidelity using uniform scalar quantization.
"""
__author__ = "Miguel Hernández-Cabronero"
__since__ = "2022/03/28"

import collections

import matplotlib
# matplotlib.use("tk")
import matplotlib.pyplot as plt
import os
import math
import numpy as np
import enb
import glob


class DequantizationTask(enb.experiment.ExperimentTask):
    def __init__(self, qstep):
        assert qstep == int(qstep) and qstep >= 1, f"Invalid qstep {qstep}"
        super().__init__(param_dict=dict(qstep=qstep))


class DequantizationAnalysisExperiment(enb.experiment.Experiment):
    dataset_files_extension = "raw"

    @enb.atable.column_function([
        enb.atable.ColumnProperties("quantized_entropy", label="Quantized entropy (bps)"),
        enb.atable.ColumnProperties("mse", label="MSE"),
        enb.atable.ColumnProperties("psnr", label="PSNR (dB)"),
        enb.atable.ColumnProperties("qstep", label="Quantization step"),
    ])
    def analyze(self, index, row):
        path, task = self.index_to_path_task(index)
        qstep = task.param_dict["qstep"]
        img = enb.isets.load_array_bsq(path).flatten()
        quantized_img = img // qstep
        rec_img = np.clip((quantized_img.astype("i8") * qstep) + (qstep // 2),
                          np.iinfo(img.dtype).min, np.iinfo(img.dtype).max)
        diff_img = img.astype("i8") - rec_img
        row["qstep"] = qstep
        row["quantized_entropy"] = enb.isets.entropy(quantized_img)
        row["mse"] = (diff_img * diff_img).sum() / diff_img.size
        row["psnr"] = 10 * math.log10((np.iinfo(img.dtype).max * np.iinfo(img.dtype).max) / row["mse"])


def run_dequantization_analysis():
    enb.config.options.base_dataset_dir = "datasets_js"
    enb.config.options.persistence_dir = os.path.join("persistence", f"persistence_{os.path.basename(__file__)}")
    enb.config.options.plot_dir = os.path.join("plots", "dequantization_analysis")

    # Task definition 
    tasks = [DequantizationTask(qstep=q) for q in range(2, 128)]
    task_families = [enb.aanalysis.TaskFamily(label="Scalar quantization",
                                              task_names=[t.name for t in tasks],
                                              name_to_label={t.name: t.label for t in tasks})]

    # Experiment execution and analysis
    exp = DequantizationAnalysisExperiment(tasks=tasks, task_families=task_families)
    df = exp.get_df()
    tna = enb.aanalysis.TwoNumericAnalyzer()
    tna.get_df(full_df=df,
               target_columns=[("quantized_entropy", "psnr"),
                               ("qstep", "psnr"),
                               ("qstep", "quantized_entropy")],
               group_by=task_families,
               column_to_properties=exp.joined_column_to_properties)


def analyze_reconstruction_points():
    plot_dir = os.path.join("plots", "dequantization_analysis", "manual")
    os.makedirs(plot_dir, exist_ok=True)
    input_path = glob.glob("datasets_js/**/*.raw", recursive=True)[0]
    img = enb.isets.load_array_bsq(input_path)
    values, counts = np.unique(img, return_counts=True)
    probabilities = collections.defaultdict(int)
    probabilities.update({v: c / sum(counts) for v, c in zip(values, counts)})
    assert (abs(sum(probabilities.values()) - 1) < 1e-10)

    # Generate sample histogram
    plt.figure()
    plt.plot(values, [probabilities[v] for v in values])
    plt.xlabel("Pixel value")
    plt.ylabel("Relative frequency")
    plt.xlim(0, 255)
    plt.savefig(os.path.join(plot_dir, f"histogram_{os.path.basename(input_path)}.pdf"),
                bbox_inches="tight")
    plt.close()

    qsteps = []
    reconstruction_counts = []
    reconstruction_prob_sum = []
    for qstep in range(2, 127):
        reconstruction_points = list(range(qstep // 2, 256, qstep))
        qsteps.append(qstep)
        reconstruction_counts.append(len(reconstruction_points))
        reconstruction_prob_sum.append(sum(probabilities[r] for r in reconstruction_points))

    plt.figure()
    plt.plot(qsteps, reconstruction_counts)
    plt.xlabel("Qstep")
    plt.ylabel("Possible reconstruction points")
    plt.ylim(0, 10)
    plt.savefig(os.path.join(plot_dir, f"reconstruction_counts.pdf"))
    plt.close()

    plt.figure()
    plt.plot(qsteps, reconstruction_prob_sum)
    plt.xlabel("Qstep")
    plt.ylabel("Reconstruction probability sum")
    plt.ylim(0, 0.1)
    plt.savefig(os.path.join(plot_dir, f"reconstruction_probability_sum.pdf"))
    plt.close()


if __name__ == "__main__":
    # run_dequantization_analysis()
    analyze_reconstruction_points()
