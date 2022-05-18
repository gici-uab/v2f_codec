#!/usr/bin/env python3
"""Experiment where the 4-horizontal-region bands from consecutive images are considered to have 4 bands,
each spanning one of the regions
"""
import os
import numpy as np
import enb
from enb.config import options
import re
import glob
from band_component_experiment import HorizontalRegion

try:
    from plugins import jpeg, kakadu, v2f, lcnl
except ImportError as ex:
    raise RuntimeError(f"The v2f, jpeg and kakadu plugins need to be installed in plugin, but an error occurred while "
                       f"importing them: {repr(ex)}") from ex

from lossless_compression_experiment import LosslessExperiment

moving_component_image_output_dir = "datasets_stacking/moving_component_experiment"


class MovingBandVersionTable(enb.sets.FileVersionTable):
    """
    Generate Moving bands Dataset. Stack bands from consecutive images
    """
    dataset_files_extension = "raw"

    regions = [HorizontalRegion(0, 1215, "band0"),
               HorizontalRegion(1216, 1299, "shadow0"),
               HorizontalRegion(1300, 2515, "band1"),
               HorizontalRegion(2516, 2599, "shadow1"),
               HorizontalRegion(2600, 3815, "band2"),
               HorizontalRegion(3816, 3903, "shadow2"),
               HorizontalRegion(3904, 5119, "band3")]
    band_regions = [band for band in regions if band.name.startswith("band")]
    band_height = 1216

    def __init__(self, original_base_dir, version_base_dir):
        super().__init__(original_base_dir=original_base_dir,
                         version_base_dir=version_base_dir,
                         check_generated_files=False)
        self.boats_dir = os.path.join(self.original_base_dir, "boats2020")
        self.fields_dir = os.path.join(self.original_base_dir, "fields2020")
        self.boats_image_names = [os.path.basename(p) for p in glob.glob(os.path.join(self.boats_dir, "*.raw"))]
        self.fields_image_names = [os.path.basename(p) for p in glob.glob(os.path.join(self.fields_dir, "*.raw"))]
        assert self.boats_image_names and self.fields_image_names, \
            (len(self.boats_image_names), len(self.fields_image_names))

    def version(self, input_path, output_path, row):
        input_bn = os.path.basename(input_path)
        try:
            index = self.boats_image_names.index(input_bn)
            target_list = self.boats_image_names
            input_dir = self.boats_dir
        except ValueError:
            try:
                index = self.fields_image_names.index(input_bn)
                target_list = self.fields_image_names
                input_dir = self.fields_dir
            except ValueError:
                raise ValueError(f"input_bn={input_bn} not in self.boats_image_names nor self.fields_image_names")

        if index >= len(target_list) - 4:
            # Not enough following images
            return

        output_name = re.sub(r"(\d+)x(\d+)x(\d+)", f"4x{self.band_height}x5120", os.path.basename(output_path))
        output_path = os.path.join(os.path.dirname(output_path), output_name)
        with enb.logger.info_context(f"Outputting {output_path}"):
            images = [enb.isets.load_array_bsq(os.path.join(input_dir, p)) for p in target_list[index:index + 4]]
            chunks = tuple(image[:, region.y_min:region.y_max + 1, 0]
                           for image, region in zip(images, self.band_regions))
            output_properties_row = enb.isets.file_path_to_geometry_dict(input_path)
            output_properties_row["height"] = self.band_height
            output_properties_row["component_count"] = 4
            enb.isets.dump_array_bsq(np.dstack(chunks), output_path)


if __name__ == '__main__':
    options.persistence_dir = os.path.join("persistence", "persistence_moving_component_experiment_experiment.py")

    if not os.path.isdir(moving_component_image_output_dir):
        print("Generating moving datasets")
        MovingBandVersionTable(original_base_dir="datasets",
                               version_base_dir=moving_component_image_output_dir).get_df()

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
            prediction_label = r"(W1+W2) / 2"
            decorrelator_mode = v2f.V2F_C_DECORRELATOR_MODE_2_LEFT
        elif "prediction_JLS" in os.path.abspath(v2fc_path):
            prediction_label = "JPEGLS"
            decorrelator_mode = v2f.V2F_C_DECORRELATOR_MODE_JPEGLS
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
    options.base_dataset_dir = moving_component_image_output_dir
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
            options.analysis_dir, "analysis_lossless", "moving_components_compression_analysis_scalar.csv"))
    scalar_analyzer.show_x_std = True
    scalar_analyzer.bar_width_fraction = 0
    scalar_analyzer.sort_by_average = True

    twoscalar_analyzer = enb.aanalysis.TwoNumericAnalyzer(
        csv_support_path=os.path.join(
            options.analysis_dir, "analysis_lossless", "moving_components_compression_analysis_twocolumn.csv"))

    plot_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "plots", "moving_components_plots_lossless")
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
