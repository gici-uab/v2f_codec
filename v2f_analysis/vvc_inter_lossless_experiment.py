#!/usr/bin/env python3
"""Lossless compression of stacked frames using VVC inter lossless
"""
__author__ = "Miguel Hernández-Cabronero et al."

import os
import glob
import shutil

import enb
from enb.config import options

enb.plugins.install("vvc")
from plugins.vvc.vvc_codec import VVCLosslessInter, VVCLosslessIntra


def generate_small_stacks(original_dataset=os.path.join("datasets", "fields2020"),
                          versioned_dataset=os.path.join("datasets_stacking", "small_stacks"),
                          stack_size=8):
    """Generate small stack of images for vvc, by default 8 consecutive frames from the fields dataset.
    """
    if os.path.isdir(versioned_dataset) and not options.force:
        return

    all_paths = sorted(glob.glob(os.path.join(original_dataset, "**", "*.raw"), recursive=True))
    img_properties = enb.isets.file_path_to_geometry_dict(all_paths[0])
    assert img_properties["bytes_per_sample"] == 1
    assert not img_properties["signed"]

    assert len(all_paths) >= stack_size
    shutil.rmtree(versioned_dataset, ignore_errors=True)
    os.makedirs(versioned_dataset)
    for index, i in enumerate(range(0, len(all_paths), stack_size)):
        with enb.logger.verbose_context(f"Generating stack #{index}"):
            if i >= len(all_paths) - stack_size:
                continue
            output_path = os.path.join(
                versioned_dataset,
                f"small_stack_{index}-u8be-{stack_size}x{img_properties['height']}x{img_properties['width']}.raw")
            with open(output_path, "wb") as output_file:
                for input_path in all_paths[i:i + stack_size]:
                    img = enb.isets.load_array_bsq(input_path)
                    enb.isets.dump_array_bsq(array=img, file_or_path=output_file)

            assert os.path.getsize(output_path) == stack_size * os.path.getsize(all_paths[0])


if __name__ == '__main__':
    generate_small_stacks()

    options.base_tmp_dir = "tmp"
    options.persistence_dir = os.path.join("persistence", "persistence_vvc_inter_lossless_experiment.py")
    options.base_dataset_dir = os.path.join("datasets_stacking", "small_stacks")
    options.plot_dir = os.path.join("plots", "vvc_stacking")
    options.analysis_dir = os.path.join("analysis", "vvc_stacking")

    codecs = [VVCLosslessInter(), VVCLosslessIntra()]

    exp = enb.icompression.LosslessCompressionExperiment(codecs=codecs)
    
    df = exp.get_df()
    enb.aanalysis.ScalarNumericAnalyzer().get_df(
        full_df=df,
        target_columns=["bpppc", "compression_time_seconds"],
        column_to_properties=exp.joined_column_to_properties,
        group_by="task_label",
        x_min=2.8, x_max=3.41)
