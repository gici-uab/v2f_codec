#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import scipy.misc
import subprocess
import collections
import matplotlib.pyplot as plt
import seaborn as sns
from PIL import Image

import enb
from enb.config import options

import pandas as pd
import numpy as np
from ctypes import *


def map_predicted_sample(sample, prediction, max_sample_value):
    assert prediction <= max_sample_value
    prediction_difference = sample - prediction
    diff_to_max = max_sample_value - prediction
    closer_to_max = diff_to_max < prediction
    theta = (closer_to_max * diff_to_max) + ((not closer_to_max) * prediction)
    abs_value = abs(prediction_difference)
    within_pm_theta = (abs_value <= theta)
    negative = (prediction_difference < 0)
    coded_value = ((within_pm_theta * ((abs_value << 1) - negative)) + ((not within_pm_theta) * (theta + abs_value)))

    return coded_value


class PredictionQuantizationTask(enb.experiment.ExperimentTask):
    def __init__(self, q, param_dict=None):
        super().__init__(param_dict=param_dict)
        self.param_dict["q"] = q


def quantization(img, qstep):
    if qstep != 1:
        img = img // qstep
    return img


def Original(img, qstep):
    img = quantization(img, qstep)
    return img, img


def WPrediction(img, qstep):
    img = quantization(img, qstep)
    prediction = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
    maximum = 2 ** (8 * 2)
    prediction[1:, :, :] = img[:-1, :, :]
    residuals = img - prediction
    residuals_map = np.vectorize(map_predicted_sample)(img, prediction, maximum).astype(np.uint8)

    reconstruction = residuals
    for i in range(1, img.shape[0]):
        reconstruction[i, :, :] = residuals[i, :, :] + reconstruction[i-1, :, :]
    if qstep == 1:
        assert (reconstruction == img).all(), f"WPrediction: Not lossless for qstep {qstep}"
    print("W Lossless")

    return residuals, residuals_map


def IJPrediction(img, qstep):
    img = quantization(img, qstep)
    img_aux = img.astype('int64')
    prediction = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
    maximum = 2 ** (8 * 2)
    prediction[2:, :, :] = (img_aux[:-2, :, :] + img_aux[1:-1, :, :]) >> 1
    residuals = img - prediction
    residuals_map = np.vectorize(map_predicted_sample)(img, prediction, maximum).astype(np.uint8)

    reconstruction = residuals
    for i in range(2, img.shape[0]):
        reconstruction[i, :, :] = residuals[i, :, :] + ((reconstruction[i - 1, :, :] + reconstruction[i-2, :, :]) >> 1)
    if qstep == 1:
        assert (reconstruction == img).all(), f"IJPrediction: Not lossless for qstep {qstep}"

    print("IJ Lossless")

    return residuals, residuals_map


def JLSPrediction(img, qstep):
    img = quantization(img, qstep)
    so_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../forest_generation/jpeg-ls.so")
    if not os.path.exists(so_path):
        invocation = f"make -f {os.path.join(os.path.dirname(os.path.abspath(__file__)), 'Makefile')}"
        status, output = subprocess.getstatusoutput(invocation)
        if status != 0:
            raise Exception("Status = {} != 0.\nInput=[{}].\nOutput=[{}]".format(
                status, invocation, output))

    c_int_p = POINTER(c_int)
    so_file_path = "../forest_generation/jpeg-ls.so"
    jpeg_ls = CDLL(so_file_path)
    jpeg_ls.jpeg_ls.restype = np.ctypeslib.ndpointer(dtype=c_int, shape=(img.shape[0], img.shape[1], img.shape[2]))
    prediction = jpeg_ls.jpeg_ls(np.ascontiguousarray(img, dtype=np.uint32).ctypes.data_as(c_int_p),
                                  img.shape[0], img.shape[1], img.shape[2])
    maximum = 2 ** (8 * 2)
    residuals = img - prediction
    residuals_map = np.vectorize(map_predicted_sample)(img, prediction, maximum).astype(np.uint8)
    return residuals, residuals_map


def NPrediction(img, qstep):
    img = quantization(img, qstep)
    prediction = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
    maximum = 2 ** (8 * 2)
    prediction[:, 1:, :] = img[:, :-1, :]
    residuals = img - prediction
    residuals_map = np.vectorize(map_predicted_sample)(img, prediction, maximum).astype(np.uint8)

    reconstruction = residuals
    for i in range(1, img.shape[1]):
        reconstruction[:, i, :] = residuals[:, i, :] + reconstruction[:, i-1, :]
    if qstep == 1:
        assert (reconstruction == img).all(), f"NPrediction: Not lossless for qstep {qstep}"

    print("N Lossless")
    return residuals, residuals_map


def NWPrediction(img, qstep):
    img = quantization(img, qstep)
    prediction = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
    maximum = 2 ** (8 * 2)
    prediction[1:, 1:, :] = img[:-1, :-1, :]
    residuals = img - prediction
    residuals_map = np.vectorize(map_predicted_sample)(img, prediction, maximum).astype(np.uint8)

    reconstruction = residuals
    for i in range(1, img.shape[0]):
        for j in range(1, img.shape[1]):
            reconstruction[i, j, :] = residuals[i, j, :] + reconstruction[i - 1, j - 1, :]
    if qstep == 1:
        assert (reconstruction == img).all(), f"NWPrediction: Not lossless for qstep {qstep}"

    print("NW Lossless")
    return residuals, residuals_map


def JGPrediction(img, qstep):
    img = quantization(img, qstep)
    img_aux = img.astype('int64')
    prediction = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
    maximum = 2 ** (8 * 2)
    prediction[1:, 1:, :] = (img_aux[1:, :-1] + img_aux[:-1, 1:]) >> 1
    residuals = img - prediction
    residuals_map = np.vectorize(map_predicted_sample)(img, prediction, maximum).astype(np.uint8)

    reconstruction = residuals
    for i in range(1, img.shape[0]):
        for j in range(1, img.shape[1]):
            reconstruction[i, j, :] = residuals[i, j, :] + ((reconstruction[i - 1, j, :] + reconstruction[i, j - 1, :] ) >> 1)

    if qstep == 1:
        assert (reconstruction == img).all(), f"JGPrediction: Not lossless for qstep {qstep}"

    print("JG Lossless")
    return residuals, residuals_map


def EFGIPrediction(img, qstep):
    img = quantization(img, qstep)
    img_aux = img.astype('int64')
    prediction = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
    maximum = 2 ** (8 * 2)
    prediction[2:, 1:, :] = (img_aux[2:, :-1, :]
                              + img_aux[1:-1, :-1, :]
                              + img_aux[:-2, :-1, :]
                              + img_aux[:-2, 1:, :]) >> 2
    residuals = img - prediction
    residuals_map = np.vectorize(map_predicted_sample)(img, prediction, maximum).astype(np.uint8)

    reconstruction = residuals
    for i in range(2, img.shape[0]):
        for j in range(1, img.shape[1]):
            reconstruction[i, j, :] = residuals[i, j, :] + ((reconstruction[i, j - 1, :] + reconstruction[i-1, j - 1, :] +
                                                             reconstruction[i - 2, j - 1, :] + reconstruction[i - 2, j, :]) >> 2)

    if qstep == 1:
        assert (reconstruction == img).all(), f"EFGIPrediction: Not lossless for qstep {qstep}"

    print("EFGI Lossless")
    return residuals, residuals_map


def FGJPrediction(img, qstep):
    img = quantization(img, qstep)
    img_aux = img.astype('int64')
    prediction = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
    maximum = 2 ** (8 * 2)
    prediction[1:, 1:, :] = ((img_aux[:-1, :-1] << 1) + (3 * img_aux[:-1, 1:]) + (3 * img_aux[1:, :-1])) >> 3
    residuals = img - prediction
    residuals_map = np.vectorize(map_predicted_sample)(img, prediction, maximum).astype(np.uint8)

    reconstruction = residuals
    for i in range(1, img.shape[0]):
        for j in range(1, img.shape[1]):
            reconstruction[i, j, :] = residuals[i, j, :] + (
                        (3*reconstruction[i, j - 1, :] + (reconstruction[i - 1, j - 1, :] << 1) +
                         3*reconstruction[i - 1, j, :]) >> 3)

    if qstep == 1:
        assert (reconstruction == img).all(), f"FGJPrediction: Not lossless for qstep {qstep}"

    print("FGJ Lossless")
    return residuals, residuals_map


def FGJIPrediction(img, qstep):
    img = quantization(img, qstep)
    img_aux = img.astype('int64')
    prediction = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
    maximum = 2 ** (8 * 2)
    prediction[2:, 1:, :] = (img_aux[:-2, 1:, :]
                              + img_aux[1:-1, :-1, :]
                              + img_aux[1:-1, 1:, :]
                              + img_aux[2:, :-1, :]) >> 2
    residuals = img - prediction
    residuals_map = np.vectorize(map_predicted_sample)(img, prediction, maximum).astype(np.uint8)

    reconstruction = residuals
    for i in range(2, img.shape[0]):
        for j in range(1, img.shape[1]):
            reconstruction[i, j, :] = residuals[i, j, :] + (
                        (reconstruction[i - 2, j, :] + reconstruction[i - 1, j - 1, :] +
                         reconstruction[i - 1, j, :] + reconstruction[i, j - 1, :]) >> 2)

    if qstep == 1:
        assert (reconstruction == img).all(), f"FGJIPrediction: Not lossless for qstep {qstep}"

    print("FGJI Lossless")
    return residuals, residuals_map


predictions = [
        Original,
        WPrediction,
        IJPrediction,
        JLSPrediction,
        NPrediction,
        NWPrediction,
        JGPrediction,
        EFGIPrediction,
        FGJPrediction,
        FGJIPrediction
        ]


class PredictionQuantizationAnalysis(enb.experiment.Experiment):
    default_file_properties_table_class = enb.isets.ImagePropertiesTable
    dataset_files_extension = "raw"

    def __init__(self, tasks,
                 dataset_paths=None,
                 csv_experiment_path=None,
                 csv_dataset_path=None,
                 dataset_info_table=None,
                 overwrite_file_properties=False,
                 task_families=None):
        super().__init__(tasks=tasks,
                         dataset_paths=dataset_paths,
                         csv_experiment_path=csv_experiment_path,
                         csv_dataset_path=csv_dataset_path,
                         dataset_info_table=dataset_info_table,
                         overwrite_file_properties=overwrite_file_properties,
                         task_families=task_families)

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"{pred.__name__}_symbol_prob_full_image",
                                     label=f"{pred.__name__} symbol probability full image",
                                     plot_min=0,
                                     has_dict_values=True) for pred in predictions]
        + [enb.atable.ColumnProperties(f"{pred.__name__}_entropy",
                                       label=f"{pred.__name__} entropy",
                                       plot_min=0) for pred in predictions]
        + [enb.atable.ColumnProperties(f"{pred.__name__}_unique_values",
                                       label=f"{pred.__name__} unique values",
                                       plot_min=0) for pred in predictions]
        + [enb.atable.ColumnProperties(f"{pred.__name__}_min",
                                       label=f"{pred.__name__} min",
                                       plot_min=0) for pred in predictions]
        + [enb.atable.ColumnProperties(f"{pred.__name__}_max",
                                       label=f"{pred.__name__} max",
                                       plot_min=0) for pred in predictions]
        + [enb.atable.ColumnProperties(f"{pred.__name__}_avg",
                                       label=f"{pred.__name__} average",
                                       plot_min=0) for pred in predictions]
        + [enb.atable.ColumnProperties(f"q", label=f"q", plot_min=0)])
    def set_symbol_probabilities(self, index, row):
        file_path, task_name = index
        task = self.tasks_by_name[task_name]
        image_info_row = self.dataset_table_df.loc[enb.atable.indices_to_internal_loc(file_path)]
        img_original = enb.isets.load_array_bsq(file_or_path=file_path, image_properties_row=image_info_row)

        scale_factor = {1: 25, 3: 15, 5: 8}

        row["q"] = task.param_dict['q']
        for pred in predictions:
            image, image_map = pred(img_original, task.param_dict['q'])  # Residuals
            residual_path = f"./prediction_quantization_analysis/residuals/{file_path.split('/')[-1][:-4]}_{pred.__name__}_{task.param_dict['q']}.raw"
            residual_path_png = residual_path.split(".raw")[0] + ".png"

            # img = mapping(image, task.param_dict['q'], scale_factor)
            #image_map = int(255 / scale_factor[task.param_dict['q']]) * image_map
            #image_map[image_map > 255] = 255
            # enb.isets.dump_array_bsq(image_map, file_or_path=residual_path)
            # image_map = image_map.reshape(image_map.shape[0], image_map.shape[1])
            # im = Image.fromarray(image_map)
            # im.save(residual_path_png)
            # info_row = enb.isets.file_path_to_geometry_dict(residual_path)
            #enb.isets.raw_path_to_png(residual_path, info_row, residual_path_png)

            unique, counts = np.unique(image, return_counts=True)

            row[f"{pred.__name__}_unique_values"] = unique.tolist()
            row[f"{pred.__name__}_min"] = min(unique)
            row[f"{pred.__name__}_max"] = max(unique)
            row[f"{pred.__name__}_avg"] = np.mean(unique)

            # Compute the probability distribution of the whole image
            symbol_prob = dict(zip(unique, counts / image.size))
            '''fig, ax = plt.subplots(figsize=(20, 13), dpi=80)
            if pred.__name__ == 'Original':
                probs = {i: (symbol_prob[i] if i in symbol_prob.keys() else 0) for i in
                         range(0, 255)}
                histogram = sns.barplot(list(probs.keys()), list(probs.values())).get_figure()
                x_axis = [i for i in range(0, 255) if i%5 == 0]
                ax.set_xticks(x_axis)
                ax.set_xticklabels(x_axis, fontsize=11)
            else:
                probs = {i: (symbol_prob[i] if i in symbol_prob.keys() else 0) for i in
                         range(-scale_factor[task.param_dict['q']], scale_factor[task.param_dict['q']]+1)}
                histogram = sns.barplot(list(probs.keys()), list(probs.values())).get_figure()
            ax.set_xlabel('symbol', fontsize=30)
            ax.set_ylabel('frequency', fontsize=30)
            ax.set_title(f"Probability Distribution - {pred.__name__}", fontsize=48)
            histogram.savefig(
                f"./prediction_quantization_analysis/histograms/histrogram_full_image_{file_path.split('/')[-1][:-4]}_{pred.__name__}_{task.param_dict['q']}.png")'''
            row[f"{pred.__name__}_symbol_prob_full_image"] = symbol_prob
            row[f"{pred.__name__}_entropy"] = - np.sum(np.array(list(symbol_prob.values())) *
                                                       np.log2(list(symbol_prob.values())))


class PredictionQuantizationSummaryTable(enb.atable.SummaryTable):
    """Summary table for Prediction and Quantization analysis.
    """

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"{pred.__name__}_symbol_prob_full_image",
                                     label=f"{pred.__name__} symbol prob full image",
                                     plot_min=0) for pred in predictions])
    def column_avg_symbol_to_p(self, index, row):
        """
            #Calculate the average symbol probability of all images.
            #Note that small and large images contribute equally to
            #the average.
        """

        df = self.label_to_df[index]

        scale_factor = {1: 25, 3: 15, 5: 8}

        for pred in predictions:
            symbol_prob = {}
            for img_prob in df[f"{pred.__name__}_symbol_prob_full_image"]:
                symbol_prob = dict(collections.Counter(symbol_prob) + collections.Counter(img_prob))
            q = df["q"][0]
            corpus = df["corpus"][0]
            row[f"{pred.__name__}_symbol_prob_full_image"] = symbol_prob

            fig, ax = plt.subplots(figsize=(20, 13), dpi=80)
            if pred.__name__ == 'Original':
                probs = {i: (symbol_prob[i] if i in symbol_prob.keys() else 0) for i in
                         range(0, 255)}
                histogram = sns.barplot(list(probs.keys()), list(probs.values())).get_figure()
                x_axis = [i for i in range(0, 255) if i%5 == 0]
                ax.set_xticks(x_axis)
                ax.set_xticklabels(x_axis, fontsize=11)
            else:
                probs = {i: (symbol_prob[i] if i in symbol_prob.keys() else 0) for i in
                         range(-scale_factor[q], scale_factor[q]+1)}
                histogram = sns.barplot(list(probs.keys()), list(probs.values())).get_figure()
            ax.set_xlabel('symbol', fontsize=30)
            ax.set_ylabel('frequency', fontsize=30)
            ax.set_title(f"Probability Distribution - {pred.__name__}", fontsize=48)
            histogram.savefig(
                f"./prediction_quantization_analysis/histograms/histrogram_full_image_{corpus}_{pred.__name__}_{q}.png")

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"{pred.__name__}_entropy",
                                     label=f"{pred.__name__} entropy",
                                     plot_min=0) for pred in predictions])
    def column_entropy(self, index, row):
        """Calculate the average kl-divergence of all images for full image and each region.
        """
        df = self.label_to_df[index]

        for pred in predictions:
            row[f"{pred.__name__}_entropy"] = np.mean(df[f"{pred.__name__}_entropy"])

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"{pred.__name__}_unique_values",
                                     label=f"{pred.__name__} unique_values",
                                     plot_min=0) for pred in predictions])
    def column_unique_values(self, index, row):
        """Calculate the average kl-divergence of all images for full image and each region.
        """
        df = self.label_to_df[index]

        for pred in predictions:
            unique_values = []
            for u_values in df[f"{pred.__name__}_unique_values"]:
                unique_values.append(u_values)
            row[f"{pred.__name__}_unique_values"] = len(set(unique_values[0]))

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"{pred.__name__}_min_min",
                                     label=f"{pred.__name__} minimum min",
                                     plot_min=0) for pred in predictions]
        + [enb.atable.ColumnProperties(f"{pred.__name__}_avg_min",
                                       label=f"{pred.__name__} average min",
                                       plot_min=0) for pred in predictions])
    def column_min(self, index, row):
        """Calculate the average kl-divergence of all images for full image and each region.
        """
        df = self.label_to_df[index]

        for pred in predictions:
            row[f"{pred.__name__}_min_min"] = min(df[f"{pred.__name__}_min"])
            row[f"{pred.__name__}_avg_min"] = np.mean(df[f"{pred.__name__}_min"])

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"{pred.__name__}_max_max",
                                     label=f"{pred.__name__} maximum max",
                                     plot_min=0) for pred in predictions]
        + [enb.atable.ColumnProperties(f"{pred.__name__}_avg_max",
                                       label=f"{pred.__name__} average max",
                                       plot_min=0) for pred in predictions])
    def column_max(self, index, row):
        """Calculate the average kl-divergence of all images for full image and each region.
        """
        df = self.label_to_df[index]

        for pred in predictions:
            row[f"{pred.__name__}_max_max"] = max(df[f"{pred.__name__}_max"])
            row[f"{pred.__name__}_avg_max"] = np.mean(df[f"{pred.__name__}_max"])

    @enb.atable.column_function(
        [enb.atable.ColumnProperties(f"{pred.__name__}_avg",
                                     label=f"{pred.__name__} average",
                                     plot_min=0) for pred in predictions]
    )
    def column_average(self, index, row):
        """Calculate the average kl-divergence of all images for full image and each region.
        """
        df = self.label_to_df[index]

        for pred in predictions:
            row[f"{pred.__name__}_avg"] = np.mean(df[f"{pred.__name__}_avg"])


if __name__ == '__main__':
    os.makedirs('./prediction_quantization_analysis', exist_ok=True)
    os.makedirs('./prediction_quantization_analysis/histograms', exist_ok=True)
    os.makedirs('./prediction_quantization_analysis/residuals', exist_ok=True)

    options.persistence_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                           "persistence", "persistence_prediction_quantization_analysis.py")

    tasks = [PredictionQuantizationTask(q=q) for q in [1, 3, 5]]

    exp = PredictionQuantizationAnalysis(tasks)

    df = exp.get_df()

    summary_csv_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"persistence_summary",
                                    f"persistence_summary_prediction.csv")

    region_summary_table = PredictionQuantizationSummaryTable(
        full_df=df,
        column_to_properties=PredictionQuantizationAnalysis.column_to_properties,
        group_by=["corpus", "q"],
        include_all_group=True,
        csv_support_path=summary_csv_path
    )

    df = region_summary_table.get_df()