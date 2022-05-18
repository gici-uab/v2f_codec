#!/usr/bin/env python
"""Implementations of different prediction modes.
"""

import os
import shutil
import copy
import tempfile
import collections
import ast
import pandas as pd
import numpy as np
import functools
import matplotlib.pyplot as plt
import enb
from enb.config import options
import v2f_forest_generation
import sources
import banded_image_analysis
import make_link_sets

if banded_image_analysis.ADD_KL_ANALYSIS:
    import seaborn as sns


def quantize_predict_analyze_makeforests(target_qsteps, target_classes,
                                         debug_storage_dir=None):
    """
    :param debug_storage_dir: if not None, a copy of the internal working data used by the experiments
      is made here. If None, it is ignored.
    """
    # Optionally invoke with -q for a faster test.
    if options.quick:
        target_qsteps = target_qsteps[:min(options.quick, 2)]
        target_classes = target_classes[:min(options.quick, 2)]

    if options.verbose:
        print(f"[Q]uick mode = {options.quick} "
              f"-- {len(target_qsteps)} qsteps "
              f"-- {len(target_classes)} classes.\n"
              f"[NOTE]: Remember to run ./clean.sh before generating "
              f"a new set of data after using the quick mode.")

    # For each target qstep, the analysis experiment is run and all forests/codec files are produced.
    qstep_to_df = dict()
    for q in target_qsteps:
        try:
            original_persistence_dir = options.persistence_dir

            # Generate predictions and analyze the resulting data
            with enb.logger.info_context(f"Running quantization, prediction and analysis for qstep={q}..."):
                options.persistence_dir = os.path.join(original_persistence_dir, f"persistence_q{q}")

                qstep_to_df[q] = quantize_and_predict(
                    qstep=q, predictor_classes=target_classes,
                    internal_copy_dir=os.path.join(debug_storage_dir, f"qstep{q}")
                    if debug_storage_dir is not None else None)

            # Generate optimized V2F forests for the selected probability distributions for each predictor class
            if not options.no_new_results:
                with enb.logger.info_context(f"Generating optimized trees for qstep={q}..."):
                    options.persistence_dir = os.path.join(original_persistence_dir,
                                                           f"persistence_q{q}")
                    run_forest_building(q, target_classes)
        finally:
            if original_persistence_dir is not None:
                options.persistence_dir = original_persistence_dir

    # Generate the plots
    analyze_prediction_results(qstep_to_df)


def quantize_and_predict(qstep, predictor_classes, internal_copy_dir=None):
    """Apply quantization and then compute the prediction residual for all elements in predictor_classes.
    Return a pandas.DataFrame that contains detailed analysis information
    about all transformed data (e.g., the prediction residuals).

    :param qstep: quantization step (integer division with floor) to be applied to the original data
    :param predictor_classes: list of FileVersionTables , e.g., the banded_image_analysis
    :param internal_copy_dir: if not None, a copy of the internal data used to produce is saved here.
      Note that this will not be the case if the experiment is loaded from persistence instead
      of run anew or with -f.
    """
    persistence_csv_path = os.path.join(options.persistence_dir, f"persistence_prediction_q{qstep}.csv")

    if not os.path.exists(persistence_csv_path) or options.force:
        if options.verbose:
            print(f"Quantizing and Predicting for qstep={qstep}. Target: {persistence_csv_path}")

        with tempfile.TemporaryDirectory(dir=os.path.dirname(os.path.abspath(__file__))) as base_output_dir:
            try:

                # Link the original dataset for reference
                print(f"Linking the original dataset for reference...")
                original_dir = os.path.join(base_output_dir, "original")
                os.symlink(options.base_dataset_dir, original_dir)

                # Make version of the dataset that is shadowed out and quantized
                print(f"Making shadowed-out version of the dataset for q=1...")
                shadowed_dir = os.path.join(base_output_dir, "shadow_out")
                shutil.rmtree(shadowed_dir, ignore_errors=True)
                # The shadowed-out dir is not quantized because the prediction classes
                # apply their quantization (before or after prediction, depending on the case)
                banded_image_analysis.ShadowOutRegionVersion(
                    qstep=1,
                    original_base_dir=options.base_dataset_dir,
                    version_base_dir=shadowed_dir).get_df()

                # Make versioned folders for each of the target_classes
                for cls in predictor_classes:
                    print(f"Making predictions with {cls.__name__}...")
                    version_base_dir = os.path.join(base_output_dir, cls.version_name)
                    if not os.path.isdir(version_base_dir) or options.force:
                        original_force = options.force
                        try:
                            options.force = True
                            cls(original_base_dir=shadowed_dir,
                                version_base_dir=version_base_dir,
                                qstep=qstep).get_df()
                        finally:
                            options.force = original_force

                # Obtain statistics for all data
                joint_table = banded_image_analysis.SatellogicRegionsTable(
                    base_dir=base_output_dir,
                    csv_support_path=persistence_csv_path)
                joint_table.summary_csv_path = os.path.join(
                    options.persistence_dir, f"persistence_summary_prediction_q{qstep}.csv")
                joint_df = joint_table.get_df()

            finally:
                if internal_copy_dir is not None:
                    if options.verbose:
                        print(f"Saving internal persistence state to {internal_copy_dir}")
                    shutil.rmtree(internal_copy_dir, ignore_errors=True)
                    os.makedirs(os.path.dirname(internal_copy_dir), exist_ok=True)
                    shutil.copytree(base_output_dir, internal_copy_dir)
    else:
        print(f"Loading pre-existing data from {persistence_csv_path}")
        # CSV existed -- load it from disk
        joint_df = pd.read_csv(persistence_csv_path)

        for c in joint_df.columns:
            try:
                if banded_image_analysis.SatellogicRegionsTable.column_to_properties[c].has_dict_values:
                    joint_df[c] = joint_df[c].apply(ast.literal_eval)
            except KeyError:
                pass

    return joint_df


def analyze_prediction_results(qstep_to_df):
    # Define columns of interest and plot configuration
    scalar_columns = ["entropy_1B_bps",
                      "entropy_1B_content",
                      "entropy_1B_split_content",
                      "entropy_1B_region_band0",
                      "entropy_1B_region_band1",
                      "entropy_1B_region_band2",
                      "entropy_1B_region_band3"]
    if banded_image_analysis.ADD_KL_ANALYSIS:
        scalar_columns.append("kl_divergence_full_image")
        scalar_columns += [f"kl_divergence_{r.name}" for r in
                           banded_image_analysis.SatellogicRegionsTable.regions]
    for c in scalar_columns:
        banded_image_analysis.SatellogicRegionsTable.column_to_properties[c].plot_min = 0
        banded_image_analysis.SatellogicRegionsTable.column_to_properties[c].plot_max = 7

    # For each retrieved df, perform the plotting
    for q, full_df in qstep_to_df.items():
        options.analysis_dir = os.path.join(
            "analysis", os.path.basename(options.persistence_dir), f"analysis_q{q}")

        enb.logger.verbose(f"Plotting for qstep={q}")

        summary_csv_path = os.path.join(options.persistence_dir, f"persistence_q{q}",
                                        f"persistence_summary_prediction_q{q}.csv")

        banded_image_analysis.RegionSummaryTable(
            full_df=full_df,
            column_to_properties=banded_image_analysis.SatellogicRegionsTable.column_to_properties,
            group_by="version_name",
            include_all_group=True,
            csv_support_path=summary_csv_path
        ).get_df()

        options.plot_dir = os.path.join(options.persistence_dir, f"plots_q{q}")

        # Scalar data plotting
        sna = enb.aanalysis.ScalarNumericAnalyzer()
        y_labels_by_group_name = {name: name for name in np.unique(full_df["version_name"])}
        # y_labels_by_group_name["shadow_out"] = f"Removed shadow, qstep = {q}"

        # # A little curation before returning the generated data
        replacement_dict = {"satellogic": "(Original)",
                            "shadow_out": "(Quantized, zeroed-out shadows)",
                            "N": r"N prediction",
                            "W": r"W prediction",
                            "NW": r"NW prediction",
                            "IJ": r"W1,W2 prediction",
                            "JG": r"N,W prediction",
                            "JLS": r"JPEG-LS prediction",
                            "FGJ": r"N,NW,W prediction",
                            "EFGI": r"NE,N,NW,W prediction",
                            "FGJI": r"N,NW,W1,W2 prediction"}
        for k, v in replacement_dict.items():
            if k in y_labels_by_group_name:
                y_labels_by_group_name[k] = v

        sna.get_df(
            full_df=full_df,
            target_columns=scalar_columns,
            column_to_properties=banded_image_analysis.SatellogicRegionsTable.column_to_properties,
            y_labels_by_group_name=y_labels_by_group_name,
            group_by="version_name",
            show_global=False,
            show_count=True)

        # Vectorial data plotting of probabilities (up to the 16th symbol)
        vector_columns = ["symbol_prob_noshadow"]

        def keep_n_symbols(d, n=64):
            new_d = collections.defaultdict(int)
            new_d.update({k: v for k, v in d.items() if k <= n})
            return {k: new_d[k] for k in range(n + 1)}

        dna = enb.aanalysis.DictNumericAnalyzer(combine_keys_callable=functools.partial(keep_n_symbols, n=16))
        dna.show_x_std = False
        dna.show_global = False
        dna.show_individual_samples = False
        dna.sort_by_average = True

        dna.get_df(
            full_df=full_df,
            target_columns=vector_columns,
            column_to_properties=banded_image_analysis.SatellogicRegionsTable.column_to_properties,
            group_by="version_name",
            key_to_x={i: i for i in range(65)},
            x_tick_label_angle=90,
            fig_width=8,
            combine_groups=True)

        # Vectorial data plotting
        vector_columns = []

        if banded_image_analysis.ADD_KL_ANALYSIS:
            vector_columns += ["kl_divergence_n_symbols_full_image", "kl_divergence_by_block_size",
                               "kl_divergence_inverse_n_symbols_full_image"]
            vector_columns += [f"kl_divergence_n_symbols_{r.name}" for r in
                               banded_image_analysis.SatellogicRegionsTable.regions]

            for semilog_y in [False]:
                for c in vector_columns:
                    banded_image_analysis.SatellogicRegionsTable.column_to_properties[c].semilog_y = semilog_y
                dna.get_df(
                    full_df=full_df,
                    target_columns=vector_columns,
                    column_to_properties=banded_image_analysis.SatellogicRegionsTable.column_to_properties,
                    group_by="version_name",
                    key_to_x={s: i for i, s in enumerate(banded_image_analysis.analyzed_symbol_list)},
                    fig_width=10)

        vector_columns = []
        if banded_image_analysis.ADD_KL_ANALYSIS:
            vector_columns += [f"kl_divergence_inverse_n_symbols_{r.name}" for r in
                               banded_image_analysis.SatellogicRegionsTable.regions]
        vector_columns += ["missing_probabilities_n_symbols_full_image"]
        for c in vector_columns:
            banded_image_analysis.SatellogicRegionsTable.column_to_properties[c].semilog_y = False
            banded_image_analysis.SatellogicRegionsTable.column_to_properties[c].plot_min = None
        dna.get_df(
            full_df=full_df,
            target_columns=vector_columns,
            column_to_properties=banded_image_analysis.SatellogicRegionsTable.column_to_properties,
            group_by="version_name",
            fig_width=10,
            x_tick_label_angle=90,
            combine_groups=True)

        if banded_image_analysis.ADD_KL_ANALYSIS:
            if not os.path.exists(f"./matrix_plots_{q}"):
                os.mkdir(f"./matrix_plots_{q}")

            vector_columns = [f"kl_divergence_matrix_block_size_{n}_full_image" for n in
                              banded_image_analysis.analyzed_block_sizes]

            for col in vector_columns:
                img_id = 0
                for kl, version in zip(full_df[col], full_df["version_name"]):
                    img_id += 1
                    plt.clf()
                    try:
                        kl = ast.literal_eval(kl)
                    except ValueError:
                        pass
                    kl_heatmap = sns.heatmap(np.array(kl).transpose()).get_figure()
                    kl_heatmap.savefig(f"./matrix_plots_{q}/kl_heatmap__{col}__{version}__{img_id}.png")

            vector_columns = [f"energy_matrix_block_size_{n}_full_image" for n in
                              banded_image_analysis.analyzed_block_sizes]
            for col in vector_columns:
                img_id = 0
                for kl, version in zip(full_df[col], full_df["version_name"]):
                    img_id += 1
                    plt.clf()
                    try:
                        kl = ast.literal_eval(kl)
                    except ValueError:
                        pass
                    kl_heatmap = sns.heatmap(np.array(kl).transpose()).get_figure()
                    kl_heatmap.savefig(f"./matrix_plots_{q}/energy_heatmap_{col}_{version}_{img_id}.png")

            vector_columns = [f"entropy_matrix_block_size_{n}_full_image" for n in
                              banded_image_analysis.analyzed_block_sizes]
            for col in vector_columns:
                img_id = 0
                for kl, version in zip(full_df[col], full_df["version_name"]):
                    img_id += 1
                    plt.clf()
                    try:
                        kl = ast.literal_eval(kl)
                    except ValueError:
                        pass
                    kl_heatmap = sns.heatmap(np.array(kl).transpose()).get_figure()
                    kl_heatmap.savefig(f"./matrix_plots_{q}/entropy_heatmap_{col}_{version}_{img_id}.png")


def run_forest_building(q, predictor_classes):
    """Generate V2F forests optimized for the given predictor classes and qstep q.
    """
    prob_columns = ["avg_symbol_to_p_noshadow"]

    forest_generation_tasks = []

    persistence_csv_path = os.path.join(
        options.persistence_dir, f"persistence_summary_prediction_q{q}.csv")

    df = pd.read_csv(persistence_csv_path)

    for predictor in predictor_classes:
        # df = df[df["version_name"] == predictor.version_name]

        for column in prob_columns:
            # For each column containing measured probability distributions,
            # a single Experiment is run to generate all forests in parallel.
            forest_output_dir = os.path.join(
                output_forest_dir,
                f"q{q}",
                f"prediction_{predictor.version_name}",
                f"column_{column}")
            # shutil.rmtree(forest_output_dir, ignore_errors=True)
            os.makedirs(forest_output_dir, exist_ok=True)

            # Define the list of Forest generation tasks to use in the experiment
            for tree_size in global_target_tree_sizes:
                for tree_count in global_target_tree_counts:
                    distributions = list(df[df["group_label"] == predictor.version_name][column])
                    source = sources.Source.from_average_distributions(
                        *distributions,
                        symbol_count=256, affix=column)
                    forest_generation_tasks.append(v2f_forest_generation.FastYamamotoBuildTask(
                        tree_size=tree_size, tree_count=tree_count,
                        source=copy.copy(source),
                        qstep=q,
                        forest_output_dir=forest_output_dir))

    if options.verbose:
        print(f"Generating {len(forest_generation_tasks)} forests unless present. "
              f"Size distribution: " +
              repr(collections.Counter((task.param_dict['tree_size'], task.param_dict['tree_count'])
                                       for task in forest_generation_tasks)) +
              f". (this might take a while...)")

    v2f_forest_generation.ForestBuildExperiment(build_forest_tasks=forest_generation_tasks).get_df()


if __name__ == '__main__':
    # Path where the generated forests are to be stored.
    output_forest_dir = "prebuilt_forests"
    # shutil.rmtree(output_forest_dir, ignore_errors=True)
    os.makedirs(output_forest_dir, exist_ok=True)

    # Make sure the input data dirs are correctly set
    make_link_sets.link_datasets()
    # Select the folder where statistics are to be gathered from
    # (it does not include the newsat2020 dataset for training)
    options.base_dataset_dir = "satellogic_all"

    # Avoid RAM temporary storage to avoid RAM hoarding (it may make a big difference!)
    options.base_tmp_dir = "tmp"
    # If not None, a copy of the intermediate data is stored under internal_storage_dir
    # debug_storage_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "debug_storage")
    debug_storage_dir = None

    # Number of included nodes per tree and number of trees per forest
    global_target_tree_sizes = [2**16]
    global_target_tree_counts = [1, 2, 3, 4, 8]
    global_target_qsteps = list(range(1, 9))

    # Run the pipeline, quantization before prediction
    options.persistence_dir = "persistence_QP"
    global_target_classes = [
        banded_image_analysis.WPredictorVersion,
        banded_image_analysis.IJPredictorVersion,
        banded_image_analysis.JLSPredictorVersion,
        banded_image_analysis.FGJIPredictorVersion
    ]
    quantize_predict_analyze_makeforests(
        target_qsteps=global_target_qsteps,
        target_classes=global_target_classes,
        debug_storage_dir=debug_storage_dir)

    # Run the pipeline, prediction before quantization
    options.persistence_dir = "persistence_PQ"
    global_target_classes = [
        banded_image_analysis.WPredictorVersionPQ,
        banded_image_analysis.IJPredictorVersionPQ,
        banded_image_analysis.JLSPredictorVersionPQ,
        banded_image_analysis.FGJIPredictorVersionPQ
    ]
    quantize_predict_analyze_makeforests(
        target_qsteps=global_target_qsteps,
        target_classes=global_target_classes,
        debug_storage_dir=debug_storage_dir)
