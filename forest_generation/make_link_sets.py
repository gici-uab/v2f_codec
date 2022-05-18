#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Create subsets of the main dataset creating symlinks
"""
__author__ = "Miguel Hernández Cabronero <miguel.hernandez@uab.cat>"
__date__ = "29/04/2021"

import os
import glob
import shutil
import math
from enb.config import options


def link_datasets():
    base_dataset = "/data/research-materials/satellogic"
    if not os.path.isdir(base_dataset):
        raise ValueError(f"Please set base_dataset to an existing directory containing"
                         f" the fields2020 and boats2020 datasets")

    for number in [1, 12, 50, -1]:
        for dir_path in [os.path.join(base_dataset, "fields2020"), os.path.join(base_dataset, "boats2020")]:
            input_paths = sorted(glob.glob(os.path.join(dir_path, "*.raw")))
            if not os.path.isdir(dir_path) or not input_paths:
                print(f"Skipping dir {os.path.basename(dir_path)}")

            output_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                      f"satellogic_{number if number > 0 else 'all'}",
                                      os.path.basename(dir_path))
            shutil.rmtree(output_dir, ignore_errors=True)
            os.makedirs(output_dir)

            if options.verbose:
                print(f"[L]inking dir_path={dir_path} into {output_dir}...")
            for p in input_paths[::math.ceil(len(input_paths) / number) if number > 0 else 1]:
                os.symlink(p, os.path.join(output_dir, os.path.basename(p)))


if __name__ == '__main__':
    link_datasets()
