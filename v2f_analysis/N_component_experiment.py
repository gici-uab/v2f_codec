#!/usr/bin/env python3
"""Test compression after stacking the boats2020 and fields2020 frames as two multicomponent images.
"""

import os
import shutil
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
n_component_image_output_dir = "datasets_stacking/n_component_experiment"


def generate_stacked_sets(original_dataset, versioned_dataset):
    """
    Generate Dataset with images from each corpus and from all corpus stacked
    """
    output_dir = versioned_dataset
    boats_dir_path = f"{original_dataset}/boats2020"
    fields_dir_path = f"{original_dataset}/fields2020"

    boat_paths = sorted(p for p in glob.glob(os.path.join(boats_dir_path, "**", "*.raw"), recursive=True)
                        if os.path.isfile(p) and not os.path.islink(p))
    field_paths = sorted(p for p in glob.glob(os.path.join(fields_dir_path, "**", "*.raw"), recursive=True)
                         if os.path.isfile(p) and not os.path.islink(p))
    all_paths = sorted(boat_paths + field_paths)

    common_image_properties = enb.isets.file_path_to_geometry_dict(boat_paths[0])

    for label, path_list in [("all", all_paths), ("boats", boat_paths), ("fields", field_paths)]:
        with open(os.path.join(output_dir, f"stacked_{label}-u8be_{len(path_list)}"
                                           f"x{common_image_properties['height']}"
                                           f"x{common_image_properties['width']}.raw"), "wb") as output_file:
            for input_path in path_list:
                with open(input_path, "rb") as input_file:
                    output_file.write(input_file.read())


if __name__ == '__main__':
    options.base_tmp_dir = "tmp"
    options.chunk_size = 256
    options.persistence_dir = os.path.join("persistence", "persistence_n_component_experiment.py")

    # options.ray_cpu_limit = 1
    os.makedirs(n_component_image_output_dir, exist_ok=True)
    options.base_dataset_dir = n_component_image_output_dir
    os.makedirs(options.base_dataset_dir, exist_ok=True)

    generate_stacked_sets(original_dataset="datasets",
                          versioned_dataset=options.base_dataset_dir)

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
            # raise ValueError(f"Unsupported decorrelator mode for {repr(os.path.abspath(v2fc_path))}")
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
            options.analysis_dir, "analysis_lossless", "n_components_compression_analysis_scalar.csv"))
    scalar_analyzer.show_x_std = True
    scalar_analyzer.bar_width_fraction = 0
    scalar_analyzer.sort_by_average = True

    twoscalar_analyzer = enb.aanalysis.TwoNumericAnalyzer(
        csv_support_path=os.path.join(
            options.analysis_dir, "analysis_lossless", "n_components_compression_analysis_twocolumn.csv"))

    plot_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "plots", "n_components_plots_lossless")
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
            show_global=False,
        )
