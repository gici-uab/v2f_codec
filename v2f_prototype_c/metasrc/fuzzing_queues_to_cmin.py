#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tools to reduce size in directories of afl test vectors.
"""
__author__ = "Miguel Hernández Cabronero <miguel.hernandez@uab.cat>"
__date__ = "05/12/2020"

import os
import glob
import subprocess
import shutil

project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def clean_fuzzer_queues():
    """Remove duplicates
    """
    print("Removing queue duplicates...")
    
    queue_dirs = [d for d in glob.glob(os.path.join(project_root, "testdata", "fuzzing_samples", "queues", "*"))
                  if os.path.basename(d) != "command_line_fuzzer"]
    queue_dirs.extend(glob.glob(os.path.join(
        project_root, "testdata", "fuzzing_samples", "queues", "command_line_fuzzer", "*")))

    for d in queue_dirs:
        if not os.path.isdir(d):
            continue
        invocation = f"fdupes -r -N -d -I {d}"
        status, output = subprocess.getstatusoutput(invocation)
        if status != 0:
            raise Exception("Status = {} != 0.\nInput=[{}].\nOutput=[{}]".format(
                status, invocation, output))

def queue_to_fuzzer_cmin():
    """Minimize the fuzzing sample queues into cmin using afl-cmin
    """
    for d in glob.glob(os.path.join(project_root, "testdata", "fuzzing_samples", "queues", "*")):
        print(f"[A]pplying afl-cmin to {os.path.basename(d)}...")
        
        if os.path.basename(d) == "command_line_fuzzer":
            # Handled separately
            continue

        fuzzer_name = os.path.basename(d)
        fuzzer_bin_path = os.path.join(project_root, "build", "fuzzers", fuzzer_name)
        assert os.path.isfile(fuzzer_bin_path), f"{fuzzer_bin_path} not found. Please run `make` before this script."
        cmin_path = os.path.join(project_root, "testdata", "fuzzing_samples", "cmin", os.path.basename(d))
        shutil.rmtree(cmin_path, ignore_errors=True)
        os.makedirs(os.path.dirname(cmin_path), exist_ok=True)
        invocation = f"afl-cmin -i {d} -o {cmin_path} -- {fuzzer_bin_path}"
        status, output = subprocess.getstatusoutput(invocation)
        if status != 0:
            raise Exception("Status = {} != 0.\nInput=[{}].\nOutput=[{}]".format(
                status, invocation, output))

    for d in glob.glob(os.path.join(
            project_root, "testdata", "fuzzing_samples", "queues", "command_line_fuzzer", "*")):
        fuzzer_name = os.path.basename("command_line_fuzzer")
        fuzzer_bin_path = os.path.join(project_root, "build", "fuzzers", fuzzer_name)
        assert os.path.isfile(fuzzer_bin_path), \
            f"{fuzzer_bin_path} not found. Please run `make` before this script."
        cmin_path = os.path.join(
            project_root, "testdata", "fuzzing_samples", "cmin", "command_line_fuzzer", os.path.basename(d))
        shutil.rmtree(cmin_path, ignore_errors=True)
        os.makedirs(os.path.dirname(cmin_path), exist_ok=True)
        invocation = f"afl-cmin -i {d} -o {cmin_path} -- {fuzzer_bin_path} {os.path.basename(d)}"
        status, output = subprocess.getstatusoutput(invocation)
        if status != 0:
            raise Exception("Status = {} != 0.\nInput=[{}].\nOutput=[{}]".format(
                status, invocation, output))
        
if __name__ == '__main__':
    clean_fuzzer_queues()
    queue_to_fuzzer_cmin()