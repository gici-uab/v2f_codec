#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""This module allows analyzing image properties that present distinct horizontal regions
of 100% of the width, stacked along the vertical axis, as used in Satellogic's
4-region dataset for the Retos Colaboración dataset.
"""
__author__ = "Miguel Hernández Cabronero <miguel.hernandez@uab.cat>"
__date__ = "08/02/2021"

import ast
import os
import subprocess
import collections
import numpy as np
import math
import enb
from enb.config import options
from ctypes import *
import pandas as pd
import matplotlib.pyplot as plt

analyzed_block_sizes = [64, 128, 256, 512]
analyzed_symbol_list = list(range(4, 32))

# Set to False to avoid performing a KL analysis.
ADD_KL_ANALYSIS = False


class HorizontalRegion:
    """Abstract representation of a horizontal region spanning the a whole frame.
    """

    def __init__(self, y_min, y_max, name=None):
        """
        :param y_min, y_max: included minimum and maximum row indices in the region.
        :param name: arbitrary label
        """
        self.y_min = y_min
        self.y_max = y_max
        self.name = name

    def __repr__(self):
        return f"{self.__class__.__name__}({','.join(f'{k}={v}' for k, v in self.__dict__.items())})"


class SatellogicRegionsTable(enb.isets.ImagePropertiesTable):
    """Class to analyze image properties that present distinct horizontal regions
    of 100% of the width, stacked along the vertical axis.
    """
    summary_csv_path = os.path.relpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), f"persistence_summary",
                                    f"persistence_summary_prediction.csv"),
                                       enb.config.options.project_root)

    def get_df(self, target_indices=None, target_columns=None,
               fill=True, overwrite=None,
               chunk_size=None):

        joint_df = super().get_df(target_indices=target_indices, target_columns=target_columns,
                                  fill=fill, overwrite=overwrite,
                                  chunk_size=chunk_size)

        region_summary_table = RegionSummaryTable(
            full_df=joint_df,
            column_to_properties=SatellogicRegionsTable.column_to_properties,
            group_by="version_name",
            include_all_group=True,
            csv_support_path=self.summary_csv_path
        )
        rst_df = region_summary_table.get_df()

        # Compute average distributions
        sum_prob = {}
        for img_prob in joint_df["symbol_prob_full_image"]:
            sum_prob = dict(collections.Counter(sum_prob) + collections.Counter(img_prob))
        avg_symbol_to_p = {k: v / len(joint_df) for (k, v) in sum_prob.items()}

        if ADD_KL_ANALYSIS:
            # Compute KL divergence for each row
            joint_df["kl_divergence_full_image"] = joint_df["symbol_prob_full_image"].apply(
                lambda ip: sum(
                    [ip[symbol_value] * math.log2(ip[symbol_value] / avg_symbol_to_p[symbol_value])
                     if symbol_value in ip else 0
                     for symbol_value in avg_symbol_to_p.keys()])
            )

        num_symbols = analyzed_symbol_list
        kl_divergence_n_symbols_dict = {}
        kl_divergence_inverse_n_symbols_dict = {}
        for n in num_symbols:
            kl_divergence_n_symbols_dict[n] = joint_df["symbol_prob_full_image"] \
                .apply(lambda ip: {k: v / sum(dict(collections.Counter(ip).most_common(n)).values()) for (k, v) in
                                   dict(collections.Counter(ip).most_common(n)).items()}) \
                .apply(lambda ip: sum([ip[symbol_value] * math.log2(ip[symbol_value] / avg_symbol_to_p[symbol_value])
                                       if symbol_value in ip else 0
                                       for symbol_value in avg_symbol_to_p.keys()]))
            kl_divergence_inverse_n_symbols_dict[n] = joint_df["symbol_prob_full_image"] \
                .apply(lambda ip: {k: v / sum(dict(collections.Counter(ip).most_common(n)).values()) for (k, v) in
                                   dict(collections.Counter(ip).most_common(n)).items()}) \
                .apply(lambda ip: sum(
                [avg_symbol_to_p[symbol_value] * math.log2(avg_symbol_to_p[symbol_value] / ip[symbol_value])
                 if symbol_value in ip else 0
                 for symbol_value in avg_symbol_to_p.keys()]))

        if ADD_KL_ANALYSIS:
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

        for r in self.regions:
            sum_prob = {}
            for img_prob in joint_df[f"symbol_prob_{r.name}"]:
                sum_prob = collections.Counter(sum_prob) + collections.Counter(img_prob)
            avg_symbol_to_p = {k: v / len(joint_df) for (k, v) in sum_prob.items()}

            if ADD_KL_ANALYSIS:
                # Compute KL divergence for each row
                joint_df[f"kl_divergence_{r.name}"] = joint_df[f"symbol_prob_{r.name}"].apply(
                    lambda ip: sum(
                        [ip[symbol_value] * math.log2(ip[symbol_value] / avg_symbol_to_p[symbol_value])
                         if symbol_value in ip else 0
                         for symbol_value in avg_symbol_to_p.keys()])
                )

                kl_divergence_n_symbols_dict = {}
                kl_divergence_inverse_n_symbols_dict = {}
                for n in num_symbols:
                    kl_divergence_n_symbols_dict[n] = joint_df[f"symbol_prob_{r.name}"] \
                        .apply(
                        lambda ip: {k: v / sum(dict(collections.Counter(ip).most_common(n)).values()) for (k, v) in
                                    dict(collections.Counter(ip).most_common(n)).items()}) \
                        .apply(
                        lambda ip: sum([ip[symbol_value] * math.log2(ip[symbol_value] / avg_symbol_to_p[symbol_value])
                                        if symbol_value in ip else 0
                                        for symbol_value in avg_symbol_to_p.keys()]))
                    kl_divergence_inverse_n_symbols_dict[n] = joint_df[f"symbol_prob_{r.name}"] \
                        .apply(
                        lambda ip: {k: v / sum(dict(collections.Counter(ip).most_common(n)).values()) for (k, v) in
                                    dict(collections.Counter(ip).most_common(n)).items()}) \
                        .apply(lambda ip: sum(
                        [avg_symbol_to_p[symbol_value] * math.log2(avg_symbol_to_p[symbol_value] / ip[symbol_value])
                         if symbol_value in ip else 0
                         for symbol_value in avg_symbol_to_p.keys()]))

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

            # Compute mean
            joint_df[f"mean_{r.name}"] = joint_df[f"symbol_prob_{r.name}"].apply(
                lambda ip: sum(
                    [ip[symbol_value] * symbol_value
                     for symbol_value in ip.keys()]) / len(ip)
            )

            # Compute entropy
            joint_df[f"entropy_{r.name}"] = joint_df[f"symbol_prob_{r.name}"].apply(
                lambda ip: -sum(
                    [ip[symbol_value] * math.log2(ip[symbol_value])
                     for symbol_value in ip.keys()])
            )

            # Compute energy
            joint_df[f"energy_{r.name}"] = joint_df[f"symbol_prob_{r.name}"].apply(
                lambda ip: math.sqrt(sum(
                    [ip[symbol_value] ** 2
                     for symbol_value in ip.keys()]))
            )

        if ADD_KL_ANALYSIS:
            kl_divergence = {}
            for n in analyzed_block_sizes:
                sum_prob = {}
                for img_prob in joint_df["symbol_prob_by_block_size"]:
                    # Compute average distributions
                    sum_prob = dict(collections.Counter(sum_prob) + collections.Counter(img_prob[n]))
                average_prob = {k: v / len(joint_df) for (k, v) in sum_prob.items()}
                kl_divergence[n] = {}
                for j, img_prob in enumerate(joint_df["symbol_prob_by_block_size"]):
                    kl_divergence[n][j] = sum(
                        [img_prob[n][i] * math.log2(img_prob[n][i] / average_prob[i])
                         for i in range(0, 255) if i in img_prob[n].keys() and i in average_prob.keys()])

            kl_divergence_list = []
            for i in range(joint_df["kl_divergence_by_block_size"].shape[0]):
                kl_divergence_list.append([kl_divergence[n][i] for n in analyzed_block_sizes])
                kl_divergence_list[i] = dict(zip(analyzed_block_sizes, kl_divergence_list[i]))

            joint_df["kl_divergence_by_block_size"] = kl_divergence_list

            for n in analyzed_block_sizes:
                img_kl = []
                for group, avg_symbol_to_p in zip(rst_df["group_label"], rst_df["avg_symbol_to_p"]):
                    for index, row in joint_df[joint_df["version_name"] == group].iterrows():
                        image = enb.isets.load_array_bsq(file_or_path=row["file_path"], image_properties_row=row)
                        kl_divergence = []
                        for j in range(0, image.shape[1], n):
                            kl_divergence_2 = []
                            for i in range(0, image.shape[0], n):
                                block = image[i:i + n, j:j + n, :].flatten()
                                if block.size < (n * n) / 2:
                                    continue
                                unique, counts = np.unique(block, return_counts=True)
                                probabilities = dict(zip(unique, counts / block.size))
                                assert abs(sum(probabilities.values()) - 1) < 1e-6, sum(probabilities.values())

                                kl_divergence_2.append(sum(
                                    [probabilities[x] * math.log(probabilities[x] / avg_symbol_to_p[x])
                                     for x in range(0, 255) if
                                     x in probabilities.keys() and x in avg_symbol_to_p.keys()]))

                            kl_divergence.append(kl_divergence_2)
                        img_kl.append(kl_divergence)
                joint_df[f"kl_divergence_matrix_block_size_{n}_full_image_full_dataset"] = img_kl

            for r in self.regions:
                for n in analyzed_block_sizes:
                    img_kl = []
                    for group, avg_symbol_to_p in zip(rst_df["group_label"], rst_df[f"symbol_prob_{r.name}"]):
                        for index, row in joint_df[joint_df["version_name"] == group].iterrows():
                            image = enb.isets.load_array_bsq(file_or_path=row["file_path"], image_properties_row=row)
                            img_region = image[:, r.y_min:r.y_max + 1, :]
                            kl_divergence = []
                            for j in range(0, img_region.shape[1], n):
                                kl_divergence_2 = []
                                for i in range(0, img_region.shape[0], n):
                                    block = image[i:i + n, j:j + n, :].flatten()
                                    if block.size < (n * n) / 2:
                                        continue
                                    unique, counts = np.unique(block, return_counts=True)
                                    probabilities = dict(zip(unique, counts / block.size))
                                    assert abs(sum(probabilities.values()) - 1) < 1e-6, sum(probabilities.values())

                                    kl_divergence_2.append(sum(
                                        [probabilities[x] * math.log(probabilities[x] / avg_symbol_to_p[x])
                                         for x in range(0, 255) if
                                         x in probabilities.keys() and x in avg_symbol_to_p.keys()]))

                                kl_divergence.append(kl_divergence_2)
                            img_kl.append(kl_divergence)
                    joint_df[f"kl_divergence_matrix_block_size_{n}_{r.name}_full_dataset"] = img_kl

        self.write_persistence(joint_df)

        return joint_df

    # For 4-band 5120x5120 images
    regions = [HorizontalRegion(0, 1199, "band0"),
               HorizontalRegion(1200, 1299, "shadow0"),
               HorizontalRegion(1300, 2509, "band1"),
               HorizontalRegion(2510, 2599, "shadow1"),
               HorizontalRegion(2600, 3815, "band2"),
               HorizontalRegion(3816, 3909, "shadow2"),
               HorizontalRegion(3910, 5119, "band3")]

    @enb.atable.column_function(
        [enb.atable.ColumnProperties("area_fraction_content", label="Content area fraction"),
         enb.atable.ColumnProperties("area_fraction_shadow", label="Shadow area fraction")]
        + [enb.atable.ColumnProperties(f"area_fraction_region_{r.name}", label=f"Area fraction of {r.name}")
           for r in regions])
    def set_area_fraction(self, file_path, row):
        self.check_regions_and_row(image_properties_row=row)
        for r in self.regions:
            row[f"area_fraction_region_{r.name}"] = (r.y_max - r.y_min + 1) / row["height"]
        shadow_regions, content_regions = self.get_shadow_content_regions()
        for label, label_regions in zip(("shadow", "content"), (shadow_regions, content_regions)):
            row[f"area_fraction_{label}"] = sum(r.y_max - r.y_min + 1 for r in label_regions) / row["height"]
        assert 1 == sum(row[f"area_fraction_{label}"] for label in ("shadow", "content"))

    def get_shadow_content_regions(self):
        shadow_regions = [r for r in self.regions if r.name.startswith("shadow")]
        content_regions = [r for r in self.regions if not r.name.startswith("shadow")]
        return shadow_regions, content_regions

    @enb.atable.column_function(
        [enb.atable.ColumnProperties("entropy_1B_content",
                                     plot_min=0, plot_max=8,
                                     label="Entropy (bps, content only)"),
         enb.atable.ColumnProperties("entropy_1B_split_content",
                                     plot_min=0, plot_max=8,
                                     label="Entropy (bps, split content)"),
         enb.atable.ColumnProperties("entropy_1B_shadow",
                                     plot_min=0, plot_max=8,
                                     label="Entropy (bps, shadow region)")]
        + [enb.atable.ColumnProperties(f"entropy_1B_region_{r.name}",
                                       plot_min=0, plot_max=8,
                                       label=f"Entropy (bps, region {r.name})")
           for r in regions])
    def set_entropy_1B_per_region(self, file_path, row):
        """Calculates the entropy of each region of the image.
        """
        self.check_regions_and_row(image_properties_row=row)
        image = enb.isets.load_array_bsq(file_or_path=file_path, image_properties_row=row)
        name_to_slice = self.get_name_to_slice(image)
        name_to_entropy = {band: enb.isets.entropy(slice) for band, slice in name_to_slice.items()}
        for name, entropy in name_to_entropy.items():
            row[f"entropy_1B_region_{name}"] = entropy

        # The shadow and content entropies are calculated taking all data within each
        # type as a whole.
        shadow_regions, content_regions = self.get_shadow_content_regions()
        for label, label_regions in (("shadow", shadow_regions), ("content", content_regions)):
            all_data = np.concatenate([name_to_slice[r.name] for r in label_regions], axis=1)
            row[f"entropy_1B_{label}"] = enb.isets.entropy(all_data)

        # For split_content, entropy is calculated separately for each content region
        area_fraction_sum = 0
        weighted_entropy = 0
        for r in content_regions:
            area_fraction_sum += row[f"area_fraction_region_{r.name}"]
            weighted_entropy = sum(row[f"area_fraction_region_{r.name}"]
                                   * row[f"entropy_1B_region_{r.name}"]
                                   for r in content_regions)
        row["entropy_1B_split_content"] = weighted_entropy

    def get_name_to_slice(self, image):
        name_to_slice = {r.name: image[:, r.y_min:r.y_max + 1, :] for r in self.regions}
        return name_to_slice

    def check_regions_and_row(self, image_properties_row):
        """Verifies that regions are well defined for the image_properties_row passed as argument.
        """

        last_y = -1
        for region in self.regions:
            assert region.y_min == last_y + 1
            last_y = region.y_max
        assert last_y + 1 == image_properties_row["height"]

    @enb.atable.column_function(
        [enb.atable.ColumnProperties("symbol_prob_full_image",
                                     label="Average symbol probability",
                                     has_dict_values=True),
         enb.atable.ColumnProperties(f"symbol_prob_by_block_size", has_dict_values=True)]
        + [enb.atable.ColumnProperties(f"symbol_prob_{r.name}",
                                       label=f"Average symbol probability for {r.name}", has_dict_values=True)
           for r in regions]
        + [enb.atable.ColumnProperties(f"symbol_prob_noshadow",
                                       label="Average symbol probability outside the shadow regions",
                                       has_dict_values=True)])
    def set_symbol_probabilities(self, file_path, row):
        image = enb.isets.load_array_bsq(file_or_path=file_path, image_properties_row=row)
        unique, counts = np.unique(image, return_counts=True)
        # Compute the probability distribution of the whole iamge
        row["symbol_prob_full_image"] = dict(zip(unique, counts / image.size))

        # Compute the probability distribution of the image without the shadow parts
        row["symbol_prob_noshadow"] = collections.Counter()
        for r in self.regions:
            img_region = image[:, r.y_min:r.y_max + 1, :]
            unique, count = np.unique(img_region, return_counts=True)
            row["symbol_prob_noshadow"].update({s: c for s, c in zip(unique, count)})

        total_count = sum(row["symbol_prob_noshadow"].values())
        row["symbol_prob_noshadow"] = {symbol: count / total_count for symbol, count in
                                       row["symbol_prob_noshadow"].items()}

        # Compute the probability by regions and blocksblocks
        row["symbol_prob_by_block_size"] = collections.defaultdict(list)
        for r in self.regions:
            row[f"symbol_prob_{r.name}"] = {}
            row[f"symbol_prob_{r.name}"] = {}
            img_region = image[:, r.y_min:r.y_max + 1, :]
            unique_values, unique_count = np.unique(img_region, return_counts=True)
            total_count = sum(unique_count)
            row[f"symbol_prob_{r.name}"] = {v: c / total_count for v, c in zip(unique_values, unique_count)}

            for n in analyzed_block_sizes:
                for i in range(0, img_region.shape[0], n):
                    for j in range(0, img_region.shape[1], n):
                        block = img_region[i:i + n, j:j + n, :].flatten()
                        if block.size < (n * n) / 2:
                            continue
                        unique, counts = np.unique(block, return_counts=True)
                        probabilities = dict(zip(unique, counts / block.size))
                        assert abs(sum(probabilities.values()) - 1) < 1e-6, sum(probabilities.values())
                        row[f"symbol_prob_by_block_size"][n].append(probabilities)

        for n in analyzed_block_sizes:
            probs_this_size = collections.defaultdict(int)
            block_distributions_this_size = row[f"symbol_prob_by_block_size"][n]
            for d in block_distributions_this_size:
                for k, v in d.items():
                    probs_this_size[k] += v
            row[f"symbol_prob_by_block_size"][n] = {
                k: v / len(block_distributions_this_size) for k, v in probs_this_size.items()}

        row["symbol_prob_by_block_size"] = dict(row["symbol_prob_by_block_size"])

    if ADD_KL_ANALYSIS:
        @enb.atable.column_function(
            [enb.atable.ColumnProperties("kl_divergence_full_image", label="kl divergence full image")]
            + [enb.atable.ColumnProperties(f"kl_divergence_{r.name}", label=f"kl_divergence_{r.name}") for r in regions]
            + [enb.atable.ColumnProperties("kl_divergence_by_block_size", label=f"kl divergence by block size",
                                           has_dict_values=True)])
        def set_kl_divergence(self, file_path, row):
            """Set a placeholder value for the KL divergence compared to the global pixel distribution.
            This value is set by the experiment afterwards.
            """
            row[f"kl_divergence_full_image"] = -1
            for r in self.regions:
                row[f"kl_divergence_{r.name}"] = -1
            row[f"kl_divergence_by_block_size"] = {}

        @enb.atable.column_function(
            [enb.atable.ColumnProperties("kl_divergence_n_symbols_full_image",
                                         label="kl_divergence_n_symbols_full_image",
                                         has_dict_values=True)]
            + [enb.atable.ColumnProperties(f"kl_divergence_n_symbols_{r.name}",
                                           label=f"kl_divergence_n_symbols_{r.name}",
                                           has_dict_values=True) for r in regions])
        def set_kl_divergence_n_symbols(self, file_path, row):
            """Set a placeholder value for the KL divergence compared to the global pixel distribution.
            This value is set by the experiment afterwards.
            """
            row[f"kl_divergence_n_symbols_full_image"] = {}
            for r in self.regions:
                row[f"kl_divergence_n_symbols_{r.name}"] = {}

        @enb.atable.column_function(
            [enb.atable.ColumnProperties("kl_divergence_inverse_n_symbols_full_image",
                                         label="kl_divergence_inverse_n_symbols_full_image",
                                         has_dict_values=True)]
            + [enb.atable.ColumnProperties(f"kl_divergence_inverse_n_symbols_{r.name}",
                                           label=f"kl_divergence_inverse_n_symbols_{r.name}",
                                           has_dict_values=True) for r in regions])
        def set_kl_divergence_inverse_n_symbols(self, file_path, row):
            """Set a placeholder value for the KL divergence compared to the global pixel distribution.
            This value is set by the experiment afterwards.
            """
            row[f"kl_divergence_inverse_n_symbols_full_image"] = {}
            for r in self.regions:
                row[f"kl_divergence_inverse_n_symbols_{r.name}"] = {}

        @enb.atable.column_function(
            [enb.atable.ColumnProperties(f"kl_divergence_matrix_block_size_{n}_full_image",
                                         label=f"kl_divergence_matrix_block_size_{n}_full_image",
                                         has_dict_values=False) for n in analyzed_block_sizes]
            + [[enb.atable.ColumnProperties(f"kl_divergence_matrix_block_size_{n}_{r.name}",
                                            label=f"kl_divergence_matrix_block_size_{n}_{r.name}",
                                            has_dict_values=False) for n in analyzed_block_sizes] for r in regions]
            + [enb.atable.ColumnProperties(f"kl_divergence_matrix_block_size_{n}_full_image_full_dataset",
                                           label=f"kl_divergence_matrix_block_size_{n}_full_image_full_dataset",
                                           has_dict_values=False) for n in analyzed_block_sizes]
            + [[enb.atable.ColumnProperties(f"kl_divergence_matrix_block_size_{n}_{r.name}_full_dataset",
                                            label=f"kl_divergence_matrix_block_size_{n}_{r.name}_full_dataset",
                                            has_dict_values=False) for n in analyzed_block_sizes] for r in regions]

            + [enb.atable.ColumnProperties(f"energy_matrix_block_size_{n}_full_image",
                                           label=f"energy_matrix_block_size_{n}_full_image",
                                           has_dict_values=False) for n in analyzed_block_sizes]
            + [enb.atable.ColumnProperties(f"entropy_matrix_block_size_{n}_full_image",
                                           label=f"entropy_matrix_block_size_{n}_full_image",
                                           has_dict_values=False) for n in analyzed_block_sizes]
        )
        def set_symbol_matrix_kl_energy_entropy_by_block(self, file_path, row):

            image = enb.isets.load_array_bsq(file_or_path=file_path, image_properties_row=row)

            unique, counts = np.unique(image, return_counts=True)
            image_prob = dict(zip(unique, counts / image.size))

            for n in analyzed_block_sizes:
                row[f"energy_matrix_block_size_{n}_full_image"] = []
                row[f"entropy_matrix_block_size_{n}_full_image"] = []
                row[f"kl_divergence_matrix_block_size_{n}_full_image"] = []
                row[f"kl_divergence_matrix_block_size_{n}_full_image_full_dataset"] = []

                for i in range(0, image.shape[0], n):
                    energy_2 = []
                    entropy_2 = []
                    kl_divergence_2 = []
                    full_2 = []

                    for j in range(0, image.shape[1], n):
                        block = image[i:i + n, j:j + n, :].flatten()
                        if block.size < (n * n) / 2:
                            continue
                        unique, counts = np.unique(block, return_counts=True)
                        probabilities = dict(zip(unique, counts / block.size))
                        assert abs(sum(probabilities.values()) - 1) < 1e-6, sum(probabilities.values())

                        energy_2.append(sum([probabilities[symbol_value] ** 2
                                             for symbol_value in probabilities.keys()]))

                        entropy_2.append(-sum([probabilities[symbol_value] * math.log2(probabilities[symbol_value])
                                               for symbol_value in probabilities.keys()]))

                        kl_divergence_2.append(sum(
                            [probabilities[i] * math.log2(probabilities[i] / image_prob[i])
                             for i in range(0, 255) if i in probabilities.keys() and i in image_prob.keys()]))

                        full_2.append(0)

                    row[f"energy_matrix_block_size_{n}_full_image"].append(energy_2)
                    row[f"entropy_matrix_block_size_{n}_full_image"].append(entropy_2)
                    row[f"kl_divergence_matrix_block_size_{n}_full_image"].append(kl_divergence_2)
                    row[f"kl_divergence_matrix_block_size_{n}_full_image_full_dataset"].append(full_2)

            for r in self.regions:
                img_region = image[:, r.y_min:r.y_max + 1, :]
                unique_values, unique_count = np.unique(img_region, return_counts=True)
                region_prob = dict(zip(unique_values, unique_count / img_region.size))

                for n in analyzed_block_sizes:
                    row[f"kl_divergence_matrix_block_size_{n}_{r.name}"] = []
                    row[f"kl_divergence_matrix_block_size_{n}_{r.name}_full_dataset"] = []

                    for i in range(0, img_region.shape[0], n):
                        kl_divergence_2 = []
                        full_2 = []

                        for j in range(0, img_region.shape[1], n):
                            block = img_region[i:i + n, j:j + n, :].flatten()
                            if block.size < (n * n) / 2:
                                continue
                            unique, counts = np.unique(block, return_counts=True)
                            probabilities = dict(zip(unique, counts / block.size))
                            assert abs(sum(probabilities.values()) - 1) < 1e-6, sum(probabilities.values())

                            kl_divergence_2.append(sum(
                                [probabilities[i] * math.log2(probabilities[i] / region_prob[i])
                                 for i in range(0, 255) if i in probabilities.keys() and i in region_prob.keys()]))

                            full_2.append(0)

                        row[f"kl_divergence_matrix_block_size_{n}_{r.name}"].append(kl_divergence_2)
                        row[f"kl_divergence_matrix_block_size_{n}_{r.name}_full_dataset"].append(full_2)

    @enb.atable.column_function(
        [enb.atable.ColumnProperties("missing_probabilities_n_symbols_full_image",
                                     label="missing_probabilities_n_symbols_full_image",
                                     has_dict_values=True)]
        + [enb.atable.ColumnProperties(f"missing_probabilities_n_symbols_{r.name}",
                                       label=f"missing_probabilities_n_symbols_{r.name}",
                                       has_dict_values=True) for r in regions])
    def set_missing_probabilities_n_symbols(self, file_path, row):
        """Set a placeholder value for the KL divergence compared to the global pixel distribution.
        This value is set by the experiment afterwards.
        """
        n_symbols = analyzed_symbol_list

        image = enb.isets.load_array_bsq(file_or_path=file_path, image_properties_row=row)
        unique, counts = np.unique(image, return_counts=True)
        # Compute the probability distribution of the whole iamge
        probabilities = dict(zip(unique, counts / image.size))

        p = {}
        for n in n_symbols:
            p[n] = sum([probabilities[i] for i in range(n, 255) if i in probabilities.keys()])

        row["missing_probabilities_n_symbols_full_image"] = p

        for r in self.regions:
            img_region = image[:, r.y_min:r.y_max + 1, :]
            unique_values, unique_count = np.unique(img_region, return_counts=True)
            probabilities = dict(zip(unique_values, unique_count / img_region.size))

            p = {}
            for n in n_symbols:
                p[n] = sum([probabilities[i] for i in range(n, 255) if i in probabilities.keys()])
            row[f"missing_probabilities_n_symbols_{r.name}"] = p

    @enb.atable.redefines_column
    def column_version_name(self, index, row):
        """Guess the name of the versioning class given the input path.
        """
        
        for cls in PredictorVersion.__subclasses__():
            if cls.version_name in index.split(os.sep):
                return cls.version_name
            elif "shadow_out" in index.split(os.sep):
                return "shadow_out"
        else:        
            raise ValueError(f"Cannot determine predictor version for {index}")


class RegionSummaryTable(enb.atable.SummaryTable):
    """Summary table for SatellogicRegionsTable.
    """

    regions = [HorizontalRegion(0, 1199, "band0"),
               HorizontalRegion(1200, 1299, "shadow0"),
               HorizontalRegion(1300, 2509, "band1"),
               HorizontalRegion(2510, 2599, "shadow1"),
               HorizontalRegion(2600, 3815, "band2"),
               HorizontalRegion(3816, 3909, "shadow2"),
               HorizontalRegion(3910, 5119, "band3")]

    def column_avg_symbol_to_p(self, index, row):
        """Calculate the average symbol probability of all images.
        Note that small and large images contribute equally to
        the average.
        """

        df = self.label_to_df[index]
        sum_prob = {}
        for img_prob in df["symbol_prob_full_image"]:
            sum_prob = dict(collections.Counter(sum_prob) + collections.Counter(img_prob))
        return {k: v / len(df) for (k, v) in sum_prob.items()}

    def column_avg_symbol_to_p_noshadow(self, index, row):
        """Calculate the average symbol probability of all images, not considering the shadow regions.
        Note that small and large images contribute equally to
        the average.
        """

        df = self.label_to_df[index]
        sum_prob = {}
        for img_prob in df["symbol_prob_noshadow"]:
            sum_prob = dict(collections.Counter(sum_prob) + collections.Counter(img_prob))
        return {k: v / len(df) for (k, v) in sum_prob.items()}

    if ADD_KL_ANALYSIS:
        @enb.atable.column_function(
            [enb.atable.ColumnProperties("kl_divergence_full_image",
                                         label="kl_divergence_full_image",
                                         has_dict_values=False)]
            + [enb.atable.ColumnProperties(f"kl_divergence_{r.name}",
                                           label=f"kl_divergence_{r.name}",
                                           has_dict_values=False) for r in regions])
        def column_kl_divergence(self, index, row):
            """Calculate the average kl-divergence of all images for full image and each region.
            """
            df = self.label_to_df[index]

            sum_kl = 0
            for kl in df["kl_divergence_full_image"]:
                sum_kl += kl
            row["kl_divergence_full_image"] = sum_kl / len(df)

            for r in self.regions:
                sum_kl = 0
                for kl in df[f"kl_divergence_{r.name}"]:
                    sum_kl += kl
                row[f"kl_divergence_{r.name}"] = sum_kl / len(df)

        @enb.atable.column_function(
            [enb.atable.ColumnProperties("kl_divergence_by_block_size",
                                         label="kl_divergence_by_block_size",
                                         has_dict_values=True)])
        def column_kl_divergence_by_block_size(self, index, row):
            """Calculate the following kl-divergence full image for all images
            """
            df = self.label_to_df[index]

            sum_kl = {}
            for img_prob in df["kl_divergence_by_block_size"]:
                sum_kl = dict(collections.Counter(sum_kl) + collections.Counter(img_prob))
            row["kl_divergence_by_block_size"] = {k: v / len(df) for (k, v) in sum_kl.items()}

        @enb.atable.column_function(
            [enb.atable.ColumnProperties("kl_divergence_n_symbols_full_image",
                                         label="kl_divergence_n_symbols_full_image",
                                         has_dict_values=True)]
            + [enb.atable.ColumnProperties(f"kl_divergence_n_symbols_{r.name}",
                                           label=f"kl_divergence_n_symbols_{r.name}",
                                           has_dict_values=True) for r in regions])
        def column_kl_divergence_n_symbols(self, index, row):
            """Calculate the following kl-divergence full image for all images
            """
            df = self.label_to_df[index]

            sum_kl = {}
            for img_prob in df["kl_divergence_n_symbols_full_image"]:
                sum_kl = dict(collections.Counter(sum_kl) + collections.Counter(img_prob))
            row["kl_divergence_n_symbols_full_image"] = {k: v / len(df) for (k, v) in sum_kl.items()}

            for r in self.regions:
                sum_kl = {}
                for img_prob in df[f"kl_divergence_n_symbols_{r.name}"]:
                    sum_kl = dict(collections.Counter(sum_kl) + collections.Counter(img_prob))
                row[f"kl_divergence_n_symbols_{r.name}"] = {k: v / len(df) for (k, v) in sum_kl.items()}

    @enb.atable.column_function(
        [enb.atable.ColumnProperties("symbol_prob_full_image", label="symbol_prob_full_image",
                                     has_dict_values=True)]
        + [enb.atable.ColumnProperties(f"symbol_prob_{r.name}", label=f"symbol_prob_{r.name}",
                                       has_dict_values=True) for r in regions])
    def column_probability_distribution(self, index, row):
        """Calculate the average of symbol probabilities for all images"""
        df = self.label_to_df[index]

        sum_prob = {}
        for img_prob in df["symbol_prob_full_image"]:
            sum_prob = dict(collections.Counter(sum_prob) + collections.Counter(img_prob))
        row["symbol_prob_full_image"] = {k: v / len(df) for (k, v) in sum_prob.items()}

        for r in self.regions:
            for img_prob in df[f"symbol_prob_{r.name}"]:
                sum_prob = dict(collections.Counter(sum_prob) + collections.Counter(img_prob))
            row[f"symbol_prob_{r.name}"] = {k: v / len(df) for (k, v) in sum_prob.items()}


def map_predicted_sample(sample, prediction, max_sample_value, qstep=None):
    """Python implementation of the reversible, isorange signed-to-unsigned mapping
    applied to all prediction errors before entropy coding in the C99 V2F codec prototype.
    """
    assert prediction <= max_sample_value
    prediction_difference = sample - prediction
    if qstep is not None and qstep != 1:
        prediction_difference = prediction_difference // qstep
    diff_to_max = max_sample_value - prediction
    closer_to_max = diff_to_max < prediction
    theta = (closer_to_max * diff_to_max) + ((not closer_to_max) * prediction)
    abs_value = abs(prediction_difference)
    within_pm_theta = (abs_value <= theta)
    negative = (prediction_difference < 0)
    coded_value = ((within_pm_theta * ((abs_value << 1) - negative))
                   + ((not within_pm_theta) * (theta + abs_value)))
    return coded_value


class PredictorVersion(enb.sets.FileVersionTable, SatellogicRegionsTable):
    """Base class for all versioning tables.

    Input files in original_base_dir must be have `.raw` extension and contain
    a name tag recognizable by enb.isets.ImageGeometryTable, e.g., u8be-3x480x640.
    """

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(version_name=self.version_name,
                         original_base_dir=original_base_dir,
                         original_properties_table=enb.isets.ImageGeometryTable(base_dir=original_base_dir),
                         version_base_dir=version_base_dir)


class WPredictorVersion(PredictorVersion):
    """West Predictor
    """
    version_name = "W"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        if self.qstep != 1:
            img = img // self.qstep

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        maximum = 2 ** (8 * row['bytes_per_sample'])
        predictions[1:, :, :] = img[:-1, :, :]
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class WPredictorVersionPQ(PredictorVersion):
    """West Predictor-Quantization
    """
    version_name = "W_PQ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        maximum = 2 ** (8 * row['bytes_per_sample'])
        predictions[1:, :, :] = img[:-1, :, :]
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum, self.qstep).astype(np.uint8)

        enb.isets.dump_array_bsq(residuals, output_path)


class NPredictorVersion(PredictorVersion):
    """North predictor.
    """
    version_name = "N"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        if self.qstep != 1:
            img = img // self.qstep

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        maximum = 2 ** (8 * row["bytes_per_sample"])
        predictions[:, 1:, :] = img[:, :-1, :]
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class NPredictorVersionPQ(PredictorVersion):
    """North predictor.
    """
    version_name = "N_PQ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        maximum = 2 ** (8 * row["bytes_per_sample"])
        predictions[:, 1:, :] = img[:, :-1, :]
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum, self.qstep).astype(np.uint8)

        enb.isets.dump_array_bsq(residuals, output_path)


class NWPredictorVersion(PredictorVersion):
    """North-West predictor.
    """
    version_name = "NW"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        if self.qstep != 1:
            img = img // self.qstep

        maximum = 2 ** (8 * row["bytes_per_sample"])
        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[1:, 1:, :] = img[:-1, :-1, :]
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class NWPredictorVersionPQ(PredictorVersion):
    """North-West predictor.
    """
    version_name = "NW_PQ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"])
        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[1:, 1:, :] = img[:-1, :-1, :]
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum, self.qstep).astype(np.uint8)

        enb.isets.dump_array_bsq(residuals, output_path)


class JLSPredictorVersion(PredictorVersion):
    """JPEG-LS predictor.
    """
    version_name = "JLS"
    so_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "jpeg-ls.so")

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        # Make sure that the JPEG-LS library is available, or make it on the spot
        if not os.path.exists(self.so_path):
            print("[W]arning: JPEG-LS library not found. Attempting to make...")
            invocation = f"make -f {os.path.join(os.path.dirname(os.path.abspath(__file__)), 'Makefile')}"
            status, output = subprocess.getstatusoutput(invocation)
            if status != 0:
                raise Exception("[E]rror building JPEG-LS library. Status = {} != 0.\nInput=[{}].\nOutput=[{}]".format(
                    status, invocation, output))
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        if self.qstep != 1:
            img = img // self.qstep

        c_int_p = POINTER(c_int)
        jpeg_ls = CDLL(self.so_path)
        jpeg_ls.jpeg_ls.restype = np.ctypeslib.ndpointer(dtype=c_int, shape=(img.shape[0], img.shape[1], img.shape[2]))
        predictions = jpeg_ls.jpeg_ls(np.ascontiguousarray(img, dtype=np.uint32).ctypes.data_as(c_int_p),
                                      img.shape[0], img.shape[1], img.shape[2])

        maximum = 2 ** (8 * row["bytes_per_sample"])
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class JLSPredictorVersionPQ(PredictorVersion):
    """JPEG-LS predictor.
    """
    version_name = "JLS_PQ"
    so_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "jpeg-ls.so")

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        # Make sure that the JPEG-LS library is available, or make it on the spot
        if not os.path.exists(self.so_path):
            print("[W]arning: JPEG-LS library not found. Attempting to make...")
            invocation = f"make -f {os.path.join(os.path.dirname(os.path.abspath(__file__)), 'Makefile')}"
            status, output = subprocess.getstatusoutput(invocation)
            if status != 0:
                raise Exception("[E]rror building JPEG-LS library. Status = {} != 0.\nInput=[{}].\nOutput=[{}]".format(
                    status, invocation, output))
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        c_int_p = POINTER(c_int)
        jpeg_ls = CDLL(self.so_path)
        jpeg_ls.jpeg_ls.restype = np.ctypeslib.ndpointer(dtype=c_int, shape=(img.shape[0], img.shape[1], img.shape[2]))
        predictions = jpeg_ls.jpeg_ls(np.ascontiguousarray(img, dtype=np.uint32).ctypes.data_as(c_int_p),
                                      img.shape[0], img.shape[1], img.shape[2])

        maximum = 2 ** (8 * row["bytes_per_sample"])
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum, self.qstep).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class IJPredictorVersion(PredictorVersion):
    """Predictor using the average of the two neighbors two the left of the one given.

        A, B,   C, D
        E, F,   G, H
        I, J, [X], -

    (I+J)/2
    """
    version_name = "IJ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        if self.qstep != 1:
            img = img // self.qstep

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        # The +1 is to allow graceful rounding
        predictions[2:, :, :] = (img_aux[:-2, :, :] + img_aux[1:-1, :, :] + 1) >> 1
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)
        enb.isets.dump_array_bsq(residuals, output_path)


class IJPredictorVersionPQ(PredictorVersion):
    """Predictor using the average of the two neighbors two the left of the one given.

        A, B,   C, D
        E, F,   G, H
        I, J, [X], -

    (I+J)/2
    """
    version_name = "IJ_PQ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        # The +1 is to allow graceful rounding
        predictions[2:, :, :] = (img_aux[:-2, :, :] + img_aux[1:-1, :, :] + 1) >> 1
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum, self.qstep).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)

        enb.isets.dump_array_bsq(residuals, output_path)


class JGPredictorVersion(PredictorVersion):
    """Predictor using the average of two neighbors:

        A, B, C, D
        E, F, G, H
        I, J, [X]

    (J+G)/2
    """
    version_name = "JG"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        if self.qstep != 1:
            img = img // self.qstep

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1
        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        # The +1 is to allow graceful rounding
        predictions[1:, 1:, :] = (img_aux[1:, :-1] + img_aux[:-1, 1:] + 1) >> 1
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)
        enb.isets.dump_array_bsq(residuals, output_path)


class JGPredictorVersionPQ(PredictorVersion):
    """Predictor using the average of two neighbors:

        A, B, C, D
        E, F, G, H
        I, J, [X]

    (J+G)/2
    """
    version_name = "JG_PQ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1
        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        # The +1 is to allow graceful rounding
        predictions[1:, 1:, :] = (img_aux[1:, :-1] + img_aux[:-1, 1:] + 1) >> 1
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum, self.qstep).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)

        enb.isets.dump_array_bsq(residuals, output_path)


class EFGIPredictorVersion(PredictorVersion):
    """Predictor using the average of four neighbors:

        A, B, C, D
        E, F, G, H
        I, J, [X]

    (F+G+H+I)/4
    """
    version_name = "EFGI"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        if self.qstep != 1:
            img = img // self.qstep

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[2:, 1:, :] = (img_aux[2:, :-1, :]
                                  + img_aux[1:-1, :-1, :]
                                  + img_aux[:-2, :-1, :]
                                  + img_aux[:-2, 1:, :]
                                  + 2) >> 2

        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)
        enb.isets.dump_array_bsq(residuals, output_path)


class EFGIPredictorVersionPQ(PredictorVersion):
    """Predictor using the average of four neighbors:

        A, B, C, D
        E, F, G, H
        I, J, [X]

    (F+G+H+I)/4
    """
    version_name = "EFGI_PQ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

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
                                  + img_aux[:-2, 1:, :]
                                  + 2) >> 2
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum, self.qstep).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)

        enb.isets.dump_array_bsq(residuals, output_path)


class FGJPredictorVersion(PredictorVersion):
    """Predictor using the weighted average of three neighbors:

        A, B, C, D
        E, F, G, H
        I, J, [X]

    (2*F+3*G+3*J)/8
    """
    version_name = "FGJ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        if self.qstep != 1:
            img = img // self.qstep

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[1:, 1:, :] = ((img_aux[:-1, :-1] << 1) + (3 * img_aux[:-1, 1:]) + (3 * img_aux[1:, :-1])
                                  + 4) >> 3
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(np.uint8)
        enb.isets.dump_array_bsq(residuals, output_path)


class FGJPredictorVersionPQ(PredictorVersion):
    """Predictor using the weighted average of three neighbors:

        A, B, C, D
        E, F, G, H
        I, J, [X]

    (2*F+3*G+3*J)/8
    """
    version_name = "FGJ_PQ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[1:, 1:, :] = ((img_aux[:-1, :-1] << 1) + (3 * img_aux[:-1, 1:]) + (3 * img_aux[1:, :-1])
                                  + 4) >> 3
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum, self.qstep).astype(np.uint8)

        enb.isets.dump_array_bsq(residuals, output_path)


class FGJIPredictorVersion(PredictorVersion):
    """Predictor using the average of four neighbors:

    A, B, C, D
    E, F, G, H
    I, J, [X]

    (F+G+J+I)/4
    """
    version_name = "FGJI"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        img = enb.isets.load_array_bsq(file_or_path=input_path, image_properties_row=row)
        if self.qstep != 1:
            img = img // self.qstep

        maximum = 2 ** (8 * row["bytes_per_sample"]) - 1

        assert not row["float"], \
            "Only integer data are supported for now."
        img_aux = img.astype('int64')

        predictions = np.zeros((img.shape[0], img.shape[1], img.shape[2]), dtype=np.int64)
        predictions[2:, 1:, :] = (img_aux[:-2, 1:, :]
                                  + img_aux[1:-1, :-1, :]
                                  + img_aux[1:-1, 1:, :]
                                  + img_aux[2:, :-1, :]
                                  + 2) >> 2
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)
        enb.isets.dump_array_bsq(residuals, output_path)


class FGJIPredictorVersionPQ(PredictorVersion):
    """Predictor using the average of four neighbors:

    A, B, C, D
    E, F, G, H
    I, J, [X]

    (F+G+J+I)/4
    """
    version_name = "FGJI_PQ"

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

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
                                  + img_aux[2:, :-1, :]
                                  + 2) >> 2
        residuals = np.vectorize(map_predicted_sample)(img, predictions, maximum, self.qstep).astype(
            np.uint8 if row["bytes_per_sample"] == 1 else np.uint16)

        enb.isets.dump_array_bsq(residuals, output_path)


class ShadowOutRegionVersion(PredictorVersion):
    """Versioned images are identical to the original except for the following two steps:

    1. All pixels are divided (with floor) by qstep.
    2. Image regions marked as shadow are set to constant zero.
    """
    version_name = "shadow_out"
    regions_table = SatellogicRegionsTable()

    def __init__(self, original_base_dir, version_base_dir, qstep=1):
        assert qstep == int(qstep) and qstep > 0, f"Invalid qstep {qstep}"
        super().__init__(original_base_dir=original_base_dir, version_base_dir=version_base_dir)
        self.qstep = qstep

    def version(self, input_path, output_path, row):
        if options.verbose > 2:
            print(f"[watch] {self.__class__.__name__}::{input_path}->{output_path}={(input_path, output_path)}")

        # Read original image
        img = enb.isets.load_array_bsq(file_or_path=input_path,
                                       image_properties_row=row)

        # Remove the shadow regions
        for shadow_region in self.regions_table.get_shadow_content_regions()[0]:
            img[:, shadow_region.y_min:shadow_region.y_max + 1, :] = 0

        # Apply quantization if configured to do so
        if self.qstep != 1:
            img = img // self.qstep

        # Write the versioned image
        enb.isets.dump_array_bsq(img, file_or_path=output_path)


class SatellogicRegionExperiment(enb.icompression.LossyCompressionExperiment):
    """Extension of a generic lossy compression experiment that also computes
    efficiency looking only at image regions not flagged as shadow.
    """

    default_file_properties_table_class = SatellogicRegionsTable

    @enb.atable.column_function([enb.atable.ColumnProperties(
        name="compression_efficiency_1byte_entropy_content",
        label="Compression efficiency (1B entropy, content only)",
        plot_min=0)])
    def set_content_efficiency(self, index, row):
        """Compute the efficiency compared to the content's entropy.
        """
        row[_column_name] = row.image_info_row["entropy_1B_content"] \
                            * row.image_info_row["size_bytes"] \
                            * row.image_info_row["area_fraction_content"] \
                            / (row["compressed_size_bytes"] * 8)

    @enb.atable.column_function([enb.atable.ColumnProperties(
        name="compression_efficiency_1byte_entropy_split_content",
        label="Compression efficiency (1B entropy, band-split content)",
        plot_min=0)])
    def set_content_efficiency_split(self, index, row):
        """Calculate the compression efficiency compared to the entropy of the concatenation of
        the content regions.
        An efficiency of 1 corresponds to the expected compressed size of using
        one ideal zero-order entropy coder applied to the contents as a whole, and not coding the shadow
        regions.
        """
        _, content_regions = self.dataset_info_table.get_shadow_content_regions()
        weighted_entropy = sum(row.image_info_row[f"area_fraction_region_{r.name}"]
                               * row.image_info_row[f"entropy_1B_region_{r.name}"]
                               for r in content_regions)
        row[_column_name] = weighted_entropy * row.image_info_row["size_bytes"] \
                            / (row["compressed_size_bytes"] * 8)


if __name__ == '__main__':
    print("Module not callable. Run quantize_predict_analyze_makeforests.py instead.")
