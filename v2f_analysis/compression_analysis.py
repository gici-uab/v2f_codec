#!/usr/bin/env python3
"""Compression experiment for V2F codecs vs the state of the art with the same predictions
"""

import os
import re
import glob
import shutil
import tempfile
import enb
import pandas as pd
from enb.config import options
from lossless_compression_experiment import LosslessExperiment

import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "../forest_generation/"))
import banded_image_analysis

try:
    from plugins import jpeg, kakadu, lcnl, v2f_mod, fse, zip, zstd
except ImportError as ex:
    raise RuntimeError(f"The v2f, jpeg and kakadu plugins need to be installed in plugin, but an error occurred while "
                       f"importing them: {repr(ex)}") from ex

if __name__ == '__main__':
    options.chunk_size = 100
    options.base_tmp_dir = os.path.join("/tmp", "compression_tmp")
    os.makedirs(options.base_tmp_dir, exist_ok=True)

    options.base_dataset_dir = os.path.join("/tmp", "satellogic_dataset")
    os.makedirs(options.base_dataset_dir, exist_ok=True)

    options.persistence_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                           "persistence", "persistence_compression_analysis")
    os.makedirs(options.persistence_dir, exist_ok=True)

    quantization_steps = [1, 2, 3, 4, 5, 6, 7, 8]

    predictor_list = [
        # Single-reference prediction
        banded_image_analysis.NPredictorVersion,
        banded_image_analysis.WPredictorVersion,
        banded_image_analysis.NWPredictorVersion,
        # Multiple pixel prediction
        ## 2 pixels
        banded_image_analysis.IJPredictorVersion,
        banded_image_analysis.JGPredictorVersion,
        ## 4 Pixels
        banded_image_analysis.JLSPredictorVersion,
        banded_image_analysis.EFGIPredictorVersion,
        banded_image_analysis.FGJPredictorVersion,
        banded_image_analysis.FGJIPredictorVersion
    ]

    codecs = []
    task_families = []

    huffman_family = enb.experiment.TaskFamily(label="Huffman")
    c = fse.Huffman()
    codecs.append(c)
    huffman_family.add_task(c.name, c.label)
    task_families.append(huffman_family)

    zip_family = enb.experiment.TaskFamily(label="Zip")
    for c in [zip.LZMA(), zip.LZ77Huffman(), zip.BZIP2()]:
        codecs.append(c)
        zip_family.add_task(c.name, c.label)
    task_families.append(zip_family)

    zstd_family = enb.experiment.TaskFamily(label="Zstandard")
    c = zstd.Zstandard()
    codecs.append(c)
    zstd_family.add_task(c.name, c.label)
    task_families.append(zstd_family)

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

    v2f_family = enb.experiment.TaskFamily(label=f"V2F")
    c = v2f_mod.v2f_codecs.V2FCodec(
        qstep=1,
        decorrelator_mode=v2f_mod.V2F_C_DECORRELATOR_MODE_NONE,
        quantizer_mode=v2f_mod.V2F_C_QUANTIZER_MODE_UNIFORM,
        time_results_dir=os.path.join(options.persistence_dir, "time_results"))
    codecs.append(c)
    v2f_family.add_task(c.name, c.label)
    task_families.append(v2f_family)

    original_persistence_dir = options.persistence_dir

    qstep_to_df = dict()
    for qstep in quantization_steps:
        with tempfile.TemporaryDirectory(dir=os.path.dirname(os.path.abspath(__file__))) as base_output_dir:
            os.makedirs(base_output_dir, exist_ok=True)
            options.persistence_dir = os.path.join(original_persistence_dir, str(qstep))
            os.makedirs(options.persistence_dir, exist_ok=True)

            # Make version of the dataset that is shadowed out and quantized
            print(f"[M]aking shadowed-out version of the dataset for q={qstep}...")
            shadowed_dir = os.path.join(base_output_dir, "original")
            if not os.path.isdir(shadowed_dir):
                shutil.rmtree(shadowed_dir, ignore_errors=True)
                banded_image_analysis.ShadowOutRegionVersion(
                    qstep=1,
                    original_base_dir=options.base_dataset_dir,
                    version_base_dir=shadowed_dir).get_df()
            else:
                print(f"({shadowed_dir} not overwritten)")

            # Make versioned folders for each of the target_classes
            for cls in predictor_list:
                print(f"[M]aking predictions with {cls.__name__}...")
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

            options.persistence_dir = original_persistence_dir
            options.base_dataset_dir = base_output_dir

            for codec in codecs:
                exp = LosslessExperiment(codecs=[codec])
                exp.get_df()
                del exp
