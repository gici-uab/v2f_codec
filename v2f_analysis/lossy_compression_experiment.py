#!/usr/bin/env python3
"""Lossless compression experiment using enb, the Electronic NoteBook library.

See https://github.com/miguelinux314/experiment-notebook for additional documentation.
"""
__author__ = "Miguel Hernández-Cabronero"

import os
import re
import glob
import enb
import pandas as pd
import time
from enb.config import options

for plugin in ["jpeg", "kakadu", "v2f", "lcnl"]:
    enb.plugins.install(plugin)
try:
    from plugins import jpeg, kakadu, v2f, lcnl
except ImportError as ex:
    raise RuntimeError(f"The v2f, jpeg and kakadu plugins need to be installed in plugin, but an error occurred while "
                       f"importing them: {repr(ex)}") from ex


class LossyExperiment(enb.icompression.LossyCompressionExperiment):
    """Custom data columns can be defined in this class.
    """

    def column_qstep(self, index, row):
        """Quantization step used."""
        path, codec = self.index_to_path_task(index)

        try:
            qstep = codec.param_dict["qstep"]
            return int(qstep)
        except:
            assert not isinstance(codec, v2f.V2FCodec)
            return 0

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
            return f"{codec.label}: N/A"

    def column_tree_count(self, index, row):
        """Number of trees in the V2F forest.
        """
        path, codec = self.index_to_path_task(index)
        try:
            return f"V2F Tree Count: " + str(re.search(r'_treecount-(\d+)_',
                                                       os.path.basename(codec.v2fc_header_path)).group(1))
        except AttributeError:
            return f"{codec.label}: N/A"

    def column_symbol_count(self, index, row):
        """Number of symbols in the source.
        """
        path, codec = self.index_to_path_task(index)
        try:
            return int(re.search(r"_symbolcount-(\d+)_", os.path.basename(codec.v2fc_header_path)).group(1))
        except AttributeError:
            return f"{codec.label}: N/A"

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
            # ValueError: could not convert string to float: '1.123456_avg_all'
            return float(
                re.search(r"_withentropy-(.*)\._avg_all.v2fc", os.path.basename(codec.v2fc_header_path)).group(1))
        except AttributeError:
            return f"{codec.label}: N/A"

    @enb.atable.column_function("block_coding_time_seconds", label="Block coding time (s)", plot_min=0)
    def set_block_coding_time_seconds(self, index, row):
        """Calculate the total coding time without considering initialization,
        """
        path, codec = self.index_to_path_task(index)
        if not isinstance(codec, v2f.V2FCodec):
            row[_column_name] = row["compression_time_seconds"]
        else:
            while os.path.getsize(codec.get_time_path(path)) == 0:
                print(">>>> Waiting...")
                time.sleep(1)
            time_df = pd.read_csv(codec.get_time_path(path))
            row[_column_name] = float(time_df[time_df["name"] == "v2f_compressor_compress_block"]["total_cpu_seconds"])


if __name__ == '__main__':
    # If uncommented, it slows down computation but prevents ram hoarding and
    # out of memory problems with large images:
    options.persistence_dir = os.path.join("persistence", "persistence_lossy_compression_experiment")
    options.chunk_size = 2048 if options.chunk_size is None else options.chunk_size
    options.plot_dir = os.path.join("plots", "lossy_compression")

    shadow_regions = [(1200, 1300), (2510, 2600), (3816, 3910)]

    codecs = []
    task_families = []

    jpeg_ls_family = enb.experiment.TaskFamily(label=f"JPEG-LS lossy")
    for m in range(1, 6):
        c = jpeg.JPEG_LS(max_error=m)
        codecs.append(c)
        jpeg_ls_family.add_task(c.name, c.label)
    task_families.append(jpeg_ls_family)

    kakadu_family = enb.experiment.TaskFamily(label=f"Kakadu JPEG 2000 lossy")
    for b in range(1, 6):
        c = kakadu.Kakadu2D(bit_rate=b, lossless=False)
        codecs.append(c)
        kakadu_family.add_task(c.name, c.label)
    task_families.append(kakadu_family)

    ccsds_family = enb.experiment.TaskFamily(label=f"CCSDS 123.0-B-2 Hybrid")
    for e in range(1, 6):
        c = lcnl.CCSDS_LCNL_GreenBook(absolute_error_limit=e,
                                      entropy_coder_type=lcnl.CCSDS_LCNL.ENTROPY_HYBRID)
        codecs.append(c)
        ccsds_family.add_task(c.name, f"{c.label} Absolute Error Limit {c.param_dict['a_vector']}")
    task_families.append(ccsds_family)

    qsteps = [2, 3, 4, 5, 6, 7, 8]
    v2fc_dir = "prebuilt_codecs"
    v2fc_path_list = []

    for q in qsteps:
        v2fc_path_list += list(glob.glob(os.path.join(v2fc_dir, f"q{q}", "**", "*treecount-1*.v2fc"), recursive=True))
        # v2fc_path_list += list(glob.glob(os.path.join(v2fc_dir, f"q{q}", "**", "*treecount-2*.v2fc"), recursive=True))
        # v2fc_path_list += list(glob.glob(os.path.join(v2fc_dir, f"q{q}", "**", "*treecount-3*.v2fc"), recursive=True))
        # v2fc_path_list += list(glob.glob(os.path.join(v2fc_dir, f"q{q}", "**", "*treecount-4*.v2fc"), recursive=True))
        # v2fc_path_list += list(glob.glob(os.path.join(v2fc_dir, f"q{q}", "**", "*treecount-8*.v2fc"), recursive=True))
    v2fc_path_list = [os.path.relpath(p, enb.config.options.project_root) for p in v2fc_path_list]

    v2f_families_by_name = dict()

    # Combine all QP-trained forests with their corresponding prediction modes,
    # while ignoring the PQ-trained forests.
    # Each combination is considered without coding the shadow regions so as not to distort the PSNR.
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
            enb.logger.info(f"Not using forest {repr(os.path.abspath(v2fc_path))}")
            continue

        c = v2f.v2f_codecs.V2FCodec(
            v2fc_header_path=v2fc_path,
            qstep=qstep, decorrelator_mode=decorrelator_mode,
            quantizer_mode=v2f.V2F_C_QUANTIZER_MODE_UNIFORM,
            time_results_dir=os.path.join(options.persistence_dir, "time_results"),
            verify_on_initialization=False)
        codecs.append(c)
        family_name = f"V2F {prediction_label}, {treecount} tree{'s' if treecount > 1 else ''}"
        codec_label = f"{c.label} Qstep {c.param_dict['qstep']}" \
                      f" Decorrelator Mode {c.param_dict['decorrelator_mode']}" \
                      f" Tree Count {treecount}"
        try:
            v2f_families_by_name[family_name].add_task(c.name, codec_label)
        except KeyError:
            v2f_families_by_name[family_name] = enb.experiment.TaskFamily(label=family_name)
            v2f_families_by_name[family_name].add_task(c.name, codec_label)

    for name, family in v2f_families_by_name.items():
        task_families.append(family)

    # Create experiment
    exp = LossyExperiment(codecs=codecs, task_families=task_families)

    # Generate pandas dataframe with results. Persistence is automatically added
    df = exp.get_df()

    column_pairs = [
        ("bpppc", "pae"), ("bpppc", "psnr_dr"), ("qstep", "bpppc"), ("qstep", "psnr_dr"), ("qstep", "pae"),
        ("qstep", "compression_ratio_dr"),
        ("bpppc", "block_coding_time_seconds"),
        ("pae", "block_coding_time_seconds"),
        ("pae", "bpppc"),
    ]

    exp.column_to_properties["pae"].plot_max = 10

    twoscalar_analyzer = enb.aanalysis.TwoNumericAnalyzer(
        csv_support_path=os.path.join(
            options.analysis_dir, "analysis_lossy/", "lossy_compression_analysis_twocolumn.csv"))
    twoscalar_analyzer.show_individual_samples = False
    twoscalar_analyzer.show_global = False
    twoscalar_analyzer.get_df(
        full_df=df,
        target_columns=column_pairs,
        column_to_properties=exp.joined_column_to_properties,
        group_by=task_families,
        selected_render_modes={"line"},
        show_grid=True)
