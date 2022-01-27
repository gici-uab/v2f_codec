#!/usr/bin/env python
"""Implementations of different prediction modes.
"""

import os
import shutil
import subprocess
import tempfile
import pickle
import ast

import enb
from enb.config import options

import pandas as pd
import numpy as np
from ctypes import *
from collections import Counter
import math

import banded_image_analysis
from banded_image_analysis import SatellogicRegionsTable, ShadowOutRegionVersion

seed = 0xbadc0ffee % 2 ** 32
forest_output_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "prebuilt_forests")

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


class WPredictionVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """West Predictor
    """
    version_name = "W"

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir)

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        maximum = 2 ** (8 * row['bytes_per_sample'])
        predictions[1:, :, :] = img[:-1, :, :]
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class NPredictorVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """North
    """
    version_name = "N"

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir)

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        maximum = 2 ** (8 * row["bytes_per_sample"])
        predictions[:, 1:, :] = img[:, :-1, :]
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class NWPredictorVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """North-West
    """
    version_name = "NW"

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir)

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"])
        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[1:, 1:, :] = img[:-1, :-1, :]
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class JLSPredictorVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """JPEG-LS predictor.
    """
    version_name = "JLS"
    so_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "jpeg-ls.so")

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir)
        if not os.path.exists(self.so_path):
            invocation = f"make -f {os.path.join(os.path.dirname(os.path.abspath(__file__)), 'Makefile')}"
            status, output = subprocess.getstatusoutput(invocation)
            if status != 0:
                raise Exception("Status = {} != 0.\nInput=[{}].\nOutput=[{}]".format(
                    status, invocation, output))

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        c_int_p = POINTER(c_int)
        so_file_path = "./jpeg-ls.so"
        jpeg_ls = CDLL(so_file_path)
        jpeg_ls.jpeg_ls.restype = np.ctypeslib.ndpointer(dtype=c_int, shape=(img.shape[0], img.shape[1], img.shape[2]))
        predictions = jpeg_ls.jpeg_ls(np.ascontiguousarray(img, dtype=np.uint32).ctypes.data_as(c_int_p),
                                      img.shape[0], img.shape[1], img.shape[2])

        maximum = 2 ** (8 * row["bytes_per_sample"])
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class IJPredictorVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """
    A, B,   C, D
    E, F,   G, H
    I, J, [X], -

    (I+J)/2
    """
    version_name = "IJ"

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir)

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[2:, :, :] = (img_aux[:-2, :, :] + img_aux[1:-1, :, :]) >> 1
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)
        enb.isets.dump_array_bsq(residuals, output_path)


class JGPredictorVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """
    A, B, C, D
    E, F, G, H
    I, J, [X]

    (J+G)/2
    """
    version_name = "JG"

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir)

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1
        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[1:, 1:, :] = (img_aux[1:, :-1] + img_aux[:-1, 1:]) >> 1
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)
        enb.isets.dump_array_bsq(residuals, output_path)


class FGHIPredictorVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """
    A, B, C, D
    E, F, G, H
    I, J, [X]

    (F+G+H+I)/4
    """
    version_name = "FGHI"

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir)

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[2:, 1:, :] = (img_aux[2:, :-1, :]
                                  + img_aux[1:-1, :-1, :]
                                  + img_aux[:-2, :-1, :]
                                  + img_aux[:-2, 1:, :]) >> 2

        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)
        enb.isets.dump_array_bsq(residuals, output_path)


class FGJPredictorVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """
    A, B, C, D
    E, F, G, H
    I, J, [X]

    (2*F+3*G+3*J)/8
    """
    version_name = "FGJ"

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir)

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[1:, 1:, :] = ((img_aux[:-1, :-1] << 1) + (3 * img_aux[:-1, 1:]) + (3 * img_aux[1:, :-1])) >> 3
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class FGJIPredictorVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """
    A, B, C, D
    E, F, G, H
    I, J, [X]

    (F+G+J+I)/4
    """
    version_name = "FGJI"

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir)

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[2:, 1:, :] = (img_aux[:-2, 1:, :]
                                  + img_aux[1:-1, :-1, :]
                                  + img_aux[1:-1, 1:, :]
                                  + img_aux[2:, :-1, :]) >> 2
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)
        enb.isets.dump_array_bsq(residuals, output_path)


def get_df(qstep):
    """Get the df for a given qstep.
    """
    enb.ray_cluster.init_ray()
    options.verbose = 0 #4

    persistence_csv_path = os.path.join(options.persistence_dir, f"persistence_prediction_q{qstep}.csv")

    print(f"[watch] qstep={qstep}")

    if not os.path.exists(persistence_csv_path) or options.force:
        with tempfile.TemporaryDirectory(dir=os.path.dirname(os.path.abspath(__file__))) as base_output_dir:
            try:
                original_persistence_dir = options.persistence_dir
                options.persistence_dir = os.path.join(options.persistence_dir, f"qstep{qstep}")

                # Link the original dataset for reference
                print(f"[watch] # Link the original dataset for reference")

                original_dir = os.path.join(base_output_dir, "original")
                os.symlink(options.base_dataset_dir, original_dir)

                # Make version of the dataset that is shadowed out.
                print(f"[watch] # Make version of the dataset that is shadowed out.")
                shadowed_dir = os.path.join(base_output_dir, "shadow_out")
                if not os.path.isdir(shadowed_dir):
                    shutil.rmtree(shadowed_dir, ignore_errors=True)
                    ShadowOutRegionVersion(
                        qstep=qstep,
                        original_base_dir=options.base_dataset_dir,
                        version_base_dir=shadowed_dir).get_df()
                else:
                    print(f"({shadowed_dir} not overwritten)")

                # Make versioned folders using target_classes
                print(f"[watch] # Make versioned folders using {target_classes}")
                for cls in target_classes:
                    print(f"Running {cls.__name__}...")
                    version_base_dir = os.path.join(base_output_dir, cls.version_name)
                    if not os.path.isdir(version_base_dir) or options.force:
                        original_force = options.force
                        try:
                            options.force = True
                            cls(original_base_dir=shadowed_dir,
                                version_base_dir=version_base_dir).get_df()
                        finally:
                            options.force = original_force

                # Obtain statistics for all data
                joint_table = banded_image_analysis.SatellogicRegionsTable(
                    base_dir=base_output_dir,
                    csv_support_path=persistence_csv_path)

                joint_df = joint_table.get_df()

                sum_prob = {}
                for img_prob in joint_df["symbol_prob_full_image"]:
                    sum_prob = dict(Counter(sum_prob) + Counter(img_prob))
                average_prob = {k: v / len(joint_df) for (k, v) in sum_prob.items()}

                joint_df["kl_divergence_full_image"] = joint_df["symbol_prob_full_image"].apply(
                    lambda ip: sum(
                        [ip[symbol_value] * math.log(ip[symbol_value] / average_prob[symbol_value])
                         if symbol_value in ip else 0
                         for symbol_value in average_prob.keys()])
                )



                for r in joint_table.regions:
                    sum_prob = {}
                    for img_prob in joint_df[f"average_symbol_prob_{r.name}"]:
                        sum_prob = dict(Counter(sum_prob) + Counter(img_prob))
                    average_prob = {k: v / len(joint_df) for (k, v) in sum_prob.items()}

                    for j, img_prob in enumerate(joint_df[f"average_symbol_prob_{r.name}"]):
                        joint_df[f"kl_divergence_{r.name}"][j] = sum(
                        [img_prob[i] * math.log(img_prob[i] / average_prob[i])
                         for i in range(0, 255) if i in img_prob.keys() and i in average_prob.keys()])

                    num_symbols = [4, 8, 16, 32, 64, 128]

                    kl_divergence_n_symbols_dict = {}
                    kl_divergence_inverse_n_symbols_dict = {}
                    for n in num_symbols:
                        kl_divergence_n_symbols_dict[n] = joint_df[f"average_symbol_prob_{r.name}"] \
                            .apply(
                            lambda ip: {k: v / sum(dict(Counter(ip).most_common(n)).values()) for (k, v) in
                                        dict(Counter(ip).most_common(n)).items()}) \
                            .apply(lambda ip: sum(
                            [ip[symbol_value] * math.log(ip[symbol_value] / average_prob[symbol_value])
                             if symbol_value in ip else 0
                             for symbol_value in average_prob()]))
                        kl_divergence_inverse_n_symbols_dict[n] = joint_df[f"average_symbol_prob_{r.name}"] \
                            .apply(
                            lambda ip: {k: v / sum(dict(Counter(ip).most_common(n)).values()) for (k, v) in
                                        dict(Counter(ip).most_common(n)).items()}) \
                            .apply(lambda ip: sum(
                            [average_prob[symbol_value] * math.log(average_prob[symbol_value] / ip[symbol_value])
                             if symbol_value in ip else 0
                             for symbol_value in average_prob.keys()]))


                    kl_divergence = []
                    kl_divergence_inverse = []
                    for i in range(joint_df[f"kl_divergence_n_symbols_{r.name}"].shape[0]):
                        kl_divergence.append([kl_divergence_n_symbols_dict[n][i] for n in num_symbols])
                        kl_divergence[i] = dict(zip(num_symbols, kl_divergence[i]))

                    joint_df[f"kl_divergence_n_symbols_{r.name}"] = kl_divergence

                    for i in range(joint_df[f"kl_divergence_inverse_n_symbols_{r.name}"].shape[0]):
                        kl_divergence_inverse.append([kl_divergence_inverse_n_symbols_dict[n][i] for n in num_symbols])
                        kl_divergence_inverse[i] = dict(zip(num_symbols, kl_divergence_inverse[i]))

                    joint_df[f"kl_divergence_inverse_n_symbols_{r.name}"] = kl_divergence_inverse

                kl_divergence_n_symbols_dict = {}
                kl_divergence_inverse_n_symbols_dict = {}
                for n in num_symbols:
                    kl_divergence_n_symbols_dict[n] = joint_df["symbol_prob_full_image"] \
                        .apply(
                        lambda ip: {k: v / sum(dict(Counter(ip).most_common(n)).values()) for (k, v) in
                                    dict(Counter(ip).most_common(n)).items()}) \
                        .apply(
                        lambda ip: sum([ip[symbol_value] * math.log(ip[symbol_value] / average_prob[symbol_value])
                                        if symbol_value in ip else 0
                                        for symbol_value in average_prob.keys()]))

                    kl_divergence_inverse_n_symbols_dict[n] = joint_df["symbol_prob_full_image"] \
                        .apply(
                        lambda ip: {k: v / sum(dict(Counter(ip).most_common(n)).values()) for (k, v) in
                                    dict(Counter(ip).most_common(n)).items()}) \
                        .apply(lambda ip: sum(
                        [average_prob[symbol_value] * math.log(average_prob[symbol_value] / ip[symbol_value])
                         if symbol_value in ip else 0
                         for symbol_value in average_prob.keys()]))

                kl_divergence = []
                kl_divergence_inverse = []
                for i in range(joint_df["kl_divergence_n_symbols_full_image"].shape[0]):
                    kl_divergence.append([kl_divergence_n_symbols_dict[n][i] for n in num_symbols])
                    kl_divergence[i] = dict(zip(num_symbols, kl_divergence[i]))

                joint_df["kl_divergence_n_symbols_full_image"] = kl_divergence

                for i in range(joint_df["kl_divergence_inverse_n_symbols_full_image"].shape[0]):
                    kl_divergence_inverse.append([kl_divergence_inverse_n_symbols_dict[n][i] for n in num_symbols])
                    kl_divergence_inverse[i] = dict(zip(num_symbols, kl_divergence_inverse[i]))

                joint_df["kl_divergence_inverse_n_symbols_full_image"] = kl_divergence_inverse


                kl_divergence = {}
                block_size = [16, 32, 64, 128]
                for n in block_size:
                    sum_prob = {}
                    for img_prob in joint_df["symbol_prob_by_block_size"]:
                        # Compute average distributions
                        sum_prob = dict(Counter(sum_prob) + Counter(img_prob[n]))
                    average_prob = {k: v / len(joint_df) for (k, v) in sum_prob.items()}
                    kl_divergence[n] = {}
                    for j, img_prob in enumerate(joint_df["symbol_prob_by_block_size"]):
                        kl_divergence[n][j] = sum(
                            [img_prob[n][i] * math.log(img_prob[n][i] / average_prob[i])
                             for i in range(0, 255) if i in img_prob[n].keys() and i in average_prob.keys()])

                kl_divergence_list = []
                for i in range(joint_df["kl_divergence_by_block_size"].shape[0]):
                    kl_divergence_list.append([kl_divergence[n][i] for n in block_size])
                    kl_divergence_list[i] = dict(zip(block_size, kl_divergence_list[i]))

                joint_df["kl_divergence_by_block_size"] = kl_divergence_list


            finally:
                options.persistence_dir = original_persistence_dir
    else:
        # CSV existed -- load it from disk
        joint_df = pd.read_csv(persistence_csv_path)

        for c in joint_df.columns:
            try:
                if banded_image_analysis.SatellogicRegionsTable.column_to_properties[c].has_dict_values:
                    joint_df[c] = joint_df[c].apply(ast.literal_eval)
            except KeyError:
                pass

    return joint_df
