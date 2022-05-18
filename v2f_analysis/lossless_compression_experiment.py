#!/usr/bin/env python3
"""Lossless compression experiment for V2F codecs vs the state of the art
"""
__author__ = "Miguel Hernández-Cabronero et al."

import os
import re
import glob
import enb
import pandas as pd
from enb.config import options

for plugin in ["jpeg", "kakadu", "v2f", "lcnl"]:
    enb.plugins.install(plugin)
try:
    from plugins import jpeg, kakadu, v2f, lcnl
except ImportError as ex:
    raise RuntimeError(f"The v2f, jpeg and kakadu plugins need to be installed in plugin, but an error occurred while "
                       f"importing them: {repr(ex)}") from ex


class LosslessExperiment(enb.icompression.LosslessCompressionExperiment):
    """Custom data columns can be defined in this class.
    """

    def column_decorrelator_mode(self, index, row):
        """Name of the decorrelation method used by this current codec.
        """
        path, codec = self.index_to_path_task(index)

        try:
            decorrelator_mode = codec.param_dict["decorrelator_mode"]
            if decorrelator_mode == v2f.V2F_C_DECORRELATOR_MODE_NONE:
                return "No decorrelation"
            elif decorrelator_mode == v2f.V2F_C_DECORRELATOR_MODE_LEFT:
                return "Left neighbor prediction"
            elif decorrelator_mode == v2f.V2F_C_DECORRELATOR_MODE_2_LEFT:
                return "Two left neighbor prediction"
            elif decorrelator_mode == v2f.V2F_C_DECORRELATOR_MODE_JPEGLS:
                return "JPEG-LS prediction"
            elif decorrelator_mode == v2f.V2F_C_DECORRELATOR_MODE_FGIJ:
                return "Four neighbor prediction"
            else:
                raise KeyError(decorrelator_mode)
        except KeyError:
            assert not isinstance(codec, v2f.V2FCodec)
            return f"{codec.label}'s decorrelation"

    def column_tree_size(self, index, row):
        """Number of included nodes in the trees of the V2F forest.
        """
        path, codec = self.index_to_path_task(index)
        try:
            return int(re.search(r"_treesize-(\d+)_", os.path.basename(codec.v2fc_header_path)).group(1))
        except AttributeError:
            return -1

    def column_tree_count(self, index, row):
        """Number of trees in the V2F forest.
        """
        path, codec = self.index_to_path_task(index)
        try:
            return int(re.search(r"_treecount-(\d+)_", os.path.basename(codec.v2fc_header_path)).group(1))
        except AttributeError:
            return -1

    def column_symbol_count(self, index, row):
        """Number of symbols in the source.
        """
        path, codec = self.index_to_path_task(index)
        try:
            return int(re.search(r"_symbolcount-(\d+)_", os.path.basename(codec.v2fc_header_path)).group(1))
        except AttributeError:
            return -1

    def column_optimization_column(self, index, row):
        """Name of the analysis column used to obtain the ideal source's
        symbol distribution.
        """
        path, codec = self.index_to_path_task(index)
        try:
            return str(re.search(r"_optimizedfor-(.*)_withentropy-", os.path.basename(codec.v2fc_header_path)).group(1))
        except AttributeError:
            return f"{codec.label}: N/A"

    def column_ideal_source_entropy(self, index, row):
        """Entropy of the ideal source for which the forest is optimized.
        """
        path, codec = self.index_to_path_task(index)
        try:
            return float(
                re.search(r"_withentropy-(\d+\.\d+)_avg_all.v2fc", os.path.basename(codec.v2fc_header_path)).group(1))
        except AttributeError:
            return -1

    @enb.atable.column_function("block_coding_time_seconds", label="Block coding time (s)", plot_min=0)
    def set_block_coding_time_seconds(self, index, row):
        """Calculate the total coding time without considering initialization,
        """
        path, codec = self.index_to_path_task(index)
        if not isinstance(codec, v2f.V2FCodec):
            row[_column_name] = row["compression_time_seconds"]
        else:
            time_df = pd.read_csv(codec.get_time_path(path))
            row[_column_name] = float(time_df[time_df["name"] == "v2f_compressor_compress_block"]["total_cpu_seconds"])


if __name__ == '__main__':
    # If uncommented, it slows down computation but prevents ram hoarding and
    # out of memory problems with large images:
    options.persistence_dir = os.path.join("persistence", "persistence_lossless_compression_experiment")
    options.analysis_dir = os.path.join("analysis", "lossless_compression_experiment")
    options.plot_dir = os.path.join("plots", "lossless_compression")
    options.chunk_size = 1024 if options.chunk_size is None else options.chunk_size

    shadow_regions = [(1200, 1300), (2510, 2600), (3816, 3910)]

    codecs = []
    task_families = []

    jpeg_ls_family = enb.experiment.TaskFamily(label="JPEG-LS")
    c = jpeg.JPEG_LS(max_error=0)
    codecs.append(c)
    jpeg_ls_family.add_task(c.name, c.label)
    task_families.append(jpeg_ls_family)

    kakadu_family = enb.experiment.TaskFamily(label=f"Kakadu JPEG 2000 lossless")
    c = kakadu.Kakadu2D(lossless=True)
    codecs.append(c)
    kakadu_family.add_task(c.name, c.label)
    task_families.append(kakadu_family)

    ccsds_family = enb.experiment.TaskFamily(label=f"CCSDS 123.0-B-2 Hybrid")
    c = lcnl.CCSDS_LCNL_GreenBook(
        absolute_error_limit=0,
        relative_error_limit=0,
        entropy_coder_type=lcnl.CCSDS_LCNL.ENTROPY_HYBRID)
    codecs.append(c)
    ccsds_family.add_task(c.name, c.label)
    task_families.append(ccsds_family)

    # Add lossless (q1) V2F codecs
    v2fc_dir = "prebuilt_codecs"
    v2fc_path_list = list(p for p in glob.glob(os.path.join(v2fc_dir, "q1", "**", "*treecount-*.v2fc"), recursive=True)
                          if any(s in p for s in [f"treecount-{c}" for c in (1, 2, 3, 4, 8)]))
    v2fc_path_list = [os.path.relpath(p, enb.config.options.project_root) for p in v2fc_path_list]

    # Combine all QP-trained forests with their corresponding prediction modes,
    # while ignoring the PQ-trained forests.
    # Each combination is considered with and without coding the shadow regions
    for shadow_pairs in [shadow_regions, []]:
        for v2fc_path in v2fc_path_list:
            qstep = int(re.search(r"_qstep-(\d+)", os.path.basename(v2fc_path)).group(1))
            treecount = int(re.search(r"_treecount-(\d+)", os.path.basename(v2fc_path)).group(1))
            if "prediction_W" in os.path.abspath(v2fc_path).split(os.sep):
                prediction_label = "W"
                decorrelator_mode = v2f.V2F_C_DECORRELATOR_MODE_LEFT
            elif "prediction_IJ" in os.path.abspath(v2fc_path).split(os.sep):
                decorrelator_mode = v2f.V2F_C_DECORRELATOR_MODE_2_LEFT
                prediction_label = r"(W1+W2) / 2"
            elif "prediction_JLS" in os.path.abspath(v2fc_path).split(os.sep):
                decorrelator_mode = v2f.V2F_C_DECORRELATOR_MODE_JPEGLS
                prediction_label = "JPEGLS"
            elif "prediction_FGJI" in os.path.abspath(v2fc_path).split(os.sep):
                decorrelator_mode = v2f.V2F_C_DECORRELATOR_MODE_FGIJ
                prediction_label = "FGIJ"
            else:
                enb.logger.info(f"Not including forest {repr(os.path.abspath(v2fc_path))}")
                continue
            c = v2f.v2f_codecs.V2FCodec(
                v2fc_header_path=v2fc_path,
                qstep=qstep, decorrelator_mode=decorrelator_mode,
                quantizer_mode=v2f.V2F_C_QUANTIZER_MODE_UNIFORM,
                time_results_dir=os.path.join(options.persistence_dir, "time_results"),
                shadow_position_pairs=shadow_pairs,
                verify_on_initialization=False)
            codecs.append(c)
            task_families.append(enb.experiment.TaskFamily(
                label=f"V2F {treecount} trees, {prediction_label}"
                      f"{' noshadow' if shadow_pairs else ''}"))
            task_families[-1].add_task(c.name, f"{c.label} Qstep {c.param_dict['qstep']}"
                                               f" Decorrelator Mode {c.param_dict['decorrelator_mode']}"
                                               f" Tree Count {treecount}"
                                               f"{' noshadow' if shadow_pairs else ''}")


    # Create experiment
    exp = v2f.ShadowLossyExperiment(codecs=codecs, task_families=task_families, shadow_position_pairs=shadow_regions)

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
            options.analysis_dir, "analysis_lossless/", "lossless_compression_analysis_scalar.csv"))
    scalar_analyzer.show_x_std = True
    scalar_analyzer.bar_width_fraction = 0
    scalar_analyzer.sort_by_average = True

    twoscalar_analyzer = enb.aanalysis.TwoNumericAnalyzer(
        csv_support_path=os.path.join(
            options.analysis_dir, "analysis_lossless/", "lossless_compression_analysis_twocolumn.csv"))

    for group_by in ["family_label"]:
        scalar_analyzer.csv_support_path = os.path.join(options.analysis_dir, f"groupby_{group_by}",
                                                        os.path.basename(scalar_analyzer.csv_support_path))
        scalar_analyzer.get_df(
            full_df=df,
            target_columns=scalar_columns,
            column_to_properties=exp.joined_column_to_properties,
            group_by=group_by,
            selected_render_modes={"histogram"},
            show_global=False,
            fig_height=7,
        )

    # Show decomposed by tree count
    scalar_analyzer.csv_support_path = os.path.join(options.analysis_dir, "v2f_only",
                                                    os.path.basename(scalar_analyzer.csv_support_path))
    group_by = ["tree_count", "decorrelator_mode"]
    scalar_analyzer.get_df(
        full_df=v2f_df,
        target_columns=scalar_columns,
        column_to_properties=exp.joined_column_to_properties,
        group_by=group_by,
        selected_render_modes={"histogram"},
        output_plot_dir=os.path.join(options.plot_dir, "v2f_only"),
        plot_title=f"Grouped by {', '.join(s.replace('_', ' ') for s in group_by)}",
        show_global=False,
    )

    # Split by corpus for a selection of codecs
    # df = exp.get_df()
    reduced_df = df[df["family_label"].str.contains("V2F") |
                    df["family_label"].str.contains("JPEG") |
                    df["family_label"].str.contains("CCSDS")].copy()
    reduced_df = reduced_df[reduced_df["tree_count"] <= 1]
    group_by = ["corpus", "family_label"]
    scalar_analyzer.get_df(
        full_df=reduced_df,
        target_columns=scalar_columns,
        column_to_properties=exp.joined_column_to_properties,
        group_by=task_families,
        selected_render_modes={"histogram"},
        output_plot_dir=os.path.join(options.plot_dir, "corpus_split"),
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
            output_plot_dir=os.path.join(options.plot_dir, "corpus_split", corpus_name),
            plot_title=f"Grouped by {', '.join(s.replace('_', ' ') for s in group_by)}",
            show_global=False,
        )
