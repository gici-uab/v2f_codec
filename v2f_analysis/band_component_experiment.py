#!/usr/bin/env python3
"""Experiment where the 4-horizontal-region images are considered to have 4 bands, each spanning one of the regions.
"""

import os
import numpy as np
import enb
from enb.config import options
import re
import glob

try:
    from plugins import jpeg, kakadu, v2f, lcnl
except ImportError as ex:
    raise RuntimeError(f"The v2f, jpeg and kakadu plugins need to be installed in plugin, but an error occurred while "
                       f"importing them: {repr(ex)}") from ex

from lossless_compression_experiment import LosslessExperiment


# Base dir where the modified dataset is to be stored 
banded_image_output_dir = "datasets_stacking/band_component_experiment"

class HorizontalRegion:
    """Abstract representation of a horizontal region spanning the whole frame.
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


class BandRedundancyExperiment(enb.experiment.Experiment):
    """
    Generate Band Dataset. Stack 4 bands of an image
    """
    regions = [HorizontalRegion(0, 1215, "band0"),
               HorizontalRegion(1216, 1299, "shadow0"),
               HorizontalRegion(1300, 2515, "band1"),
               HorizontalRegion(2516, 2599, "shadow1"),
               HorizontalRegion(2600, 3815, "band2"),
               HorizontalRegion(3816, 3903, "shadow2"),
               HorizontalRegion(3904, 5119, "band3")]

    dataset_files_extension = "raw"

    default_file_properties_table_class = enb.isets.ImagePropertiesTable

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

    def column_path_to_4band_image(self, index, row):

        file_path, task_name = index
        image_info_row = self.dataset_table_df.loc[enb.atable.indices_to_internal_loc(file_path)]

        img = enb.isets.load_array_bsq(file_or_path=file_path, image_properties_row=image_info_row)

        folder_path = file_path.split("/")[-2]
        output_dir_path = f"{banded_image_output_dir}/{folder_path}"
        os.makedirs(output_dir_path, exist_ok=True)

        aux = file_path.split("/")[-1].split("x")
        path = os.path.join(output_dir_path, f"{aux[0][:-1]}4x1280x{aux[1]}{aux[2][4:]}")
        enb.isets.dump_array_bsq(img, file_or_path=path)

        return path


class BandRedundancyTask(enb.experiment.ExperimentTask):
    def __init__(self, param_dict=None):
        super().__init__(param_dict=param_dict)


if __name__ == '__main__':
    options.persistence_dir = os.path.join("persistence", "persistence_band_component_experiment_experiment.py")
    tasks = [BandRedundancyTask()]
    experiment = BandRedundancyExperiment(tasks=tasks)

    df = experiment.get_df()

    options.base_dataset_dir = banded_image_output_dir
    options.base_tmp_dir = "tmp"
    options.chunk_size = 256

    codecs = []
    families = []

    kakadu_family = enb.experiment.TaskFamily(label="Kakadu JPEG 2000")
    c = kakadu.KakaduMCT(lossless=True)
    codecs.append(c)
    kakadu_family.add_task(c.name, c.label)
    families.append(kakadu_family)

    jpeg_ls_family = enb.experiment.TaskFamily(label="JPEG-LS")
    c = jpeg.JPEG_LS(max_error=0)
    codecs.append(c)
    jpeg_ls_family.add_task(c.name, c.label)
    families.append(jpeg_ls_family)

    ccsds_family = enb.experiment.TaskFamily(label="CCSDS LCNL")
    c = lcnl.CCSDS_LCNL_GreenBook(
        absolute_error_limit=0,
        relative_error_limit=0,
        entropy_coder_type=lcnl.CCSDS_LCNL.ENTROPY_HYBRID)
    codecs.append(c)
    ccsds_family.add_task(c.name, c.label)
    families.append(ccsds_family)

    # Add lossless (q1) V2F codecs
    v2fc_dir = "prebuilt_codecs"
    v2fc_path_list = list(glob.glob(os.path.join(v2fc_dir, "q1", "**", "*treecount-*.v2fc"), recursive=True))
    for v2fc_path in v2fc_path_list:
        qstep = int(re.search(r"_qstep-(\d+)", os.path.basename(v2fc_path)).group(1))
        treecount = int(re.search(r"_treecount-(\d+)", os.path.basename(v2fc_path)).group(1))
        if "prediction_W" in os.path.abspath(v2fc_path):
            prediction_label = "W"
            decorrelator_mode = v2f.V2F_C_DECORRELATOR_MODE_LEFT
        elif "prediction_IJ" in os.path.abspath(v2fc_path):
            decorrelator_mode = v2f.V2F_C_DECORRELATOR_MODE_2_LEFT
            prediction_label = r"(W1+W2) / 2"
        elif "prediction_JLS" in os.path.abspath(v2fc_path):
            decorrelator_mode = v2f.V2F_C_DECORRELATOR_MODE_JPEGLS
            prediction_label = "JPEGLS"
        else:
            print(f"Unsupported decorrelator mode for {repr(os.path.abspath(v2fc_path))}")
            continue
        c = v2f.v2f_codecs.V2FCodec(
            v2fc_header_path=v2fc_path,
            qstep=qstep, decorrelator_mode=decorrelator_mode,
            quantizer_mode=v2f.V2F_C_QUANTIZER_MODE_UNIFORM,
            time_results_dir=os.path.join(options.persistence_dir, "time_results"))
        codecs.append(c)
        families.append(enb.experiment.TaskFamily(label=f"V2F {treecount} trees, {prediction_label}"))
        families[-1].add_task(c.name, f"{c.label} Qstep {c.param_dict['qstep']}"
                                      f" Decorrelator Mode {c.param_dict['decorrelator_mode']}"
                                      f" Tree Count {treecount}")
    # Create experiment
    exp = LosslessExperiment(codecs=codecs, task_families=families)
    # Generate pandas dataframe with results. Persistence is automatically added
    df = exp.get_df()
    df.loc[:, "tree_count"] = df["tree_count"].apply(int)
    df.loc[:, "symbol_count"] = df["symbol_count"].apply(int)
    df.loc[:, "ideal_source_entropy"] = df["ideal_source_entropy"].apply(float)
    v2f_df = df[df["task_name"].str.contains("V2FCodec")].copy()

    # Plot some relevant results
    scalar_columns = ["compression_ratio_dr", "bpppc", "compression_efficiency_1byte_entropy",
                      "block_coding_time_seconds"]
    column_pairs = [("tree_count", "bpppc")]

    # Scalar column analysis
    scalar_analyzer = enb.aanalysis.ScalarNumericAnalyzer(
        csv_support_path=os.path.join(
            options.analysis_dir, "analysis_lossless", "4_components_compression_analysis_scalar.csv"))
    scalar_analyzer.show_x_std = True
    scalar_analyzer.bar_width_fraction = 0
    scalar_analyzer.sort_by_average = True

    twoscalar_analyzer = enb.aanalysis.TwoNumericAnalyzer(
        csv_support_path=os.path.join(
            options.analysis_dir, "analysis_lossless", "4_components_compression_analysis_twocolumn.csv"))

    plot_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "plots", "4_components_plots_lossless")
    for group_by in ["family_label"]:
        scalar_analyzer.csv_support_path = os.path.join(options.analysis_dir, f"groupby_{group_by}",
                                                        os.path.basename(scalar_analyzer.csv_support_path))
        scalar_analyzer.get_df(
            full_df=df,
            target_columns=scalar_columns,
            column_to_properties=exp.joined_column_to_properties,
            group_by=group_by,
            selected_render_modes={"histogram"},
            output_plot_dir=plot_dir,
            show_global=False,
            fig_height=5,
        )

    # Split by corpus for a selection of codecs
    # df = exp.get_df()
    reduced_df = df[df["family_label"].str.contains("JPEG") |
                    df["family_label"].str.contains("CCSDS")].copy()
    reduced_df = reduced_df[reduced_df["tree_count"] <= 1]
    group_by = ["corpus", "family_label"]
    scalar_analyzer.get_df(
        full_df=reduced_df,
        target_columns=scalar_columns,
        column_to_properties=exp.joined_column_to_properties,
        group_by=group_by,
        selected_render_modes={"histogram"},
        output_plot_dir=os.path.join(plot_dir, "corpus_split"),
        plot_title=f"Grouped by {', '.join(s.replace('_', ' ') for s in group_by)}",
        show_global=False,
    )
    for corpus_name in reduced_df["corpus"].unique():
        scalar_analyzer.get_df(
            full_df=reduced_df[reduced_df["corpus"] == corpus_name],
            target_columns=scalar_columns,
            column_to_properties=exp.joined_column_to_properties,
            group_by=group_by,
            selected_render_modes={"histogram"},
            output_plot_dir=os.path.join(plot_dir, "corpus_split", corpus_name),
            plot_title=f"Grouped by {', '.join(s.replace('_', ' ') for s in group_by)}",
            show_global=False)
