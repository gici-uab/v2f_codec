#!/usr/bin/env python3
"""Find the best alignment of consecutive frames.

Adapted from https://learnopencv.com/image-alignment-feature-based-using-opencv-c-python/
"""
__author__ = "Miguel Hernández-Cabronero"
__since__ = "2022/03/30"

import os
import glob
import cv2
import numpy as np
import time
import enb
import plugins.kakadu.kakadu_codec
import plugins.lcnl.lcnl_codecs


def get_aligned_image(target_img, reference_img,
                      max_features=500, good_match_percent=0.15,
                      matches_img_path=None):
    """Given two images (target and reference) indexed by [x,y,z], align the target to the reference.
    
    :param target_img: image to be aligned.
    :param reference_img: reference image against which the alignment is to be performed.
    :param shift_only: if True, a shifting is performed instead of a full homography.
    :param max_features: maximum number of features obtained for alignment.
    :param good_match_percent: fraction of the best features found used for alignment.
    :param matches_img_path: if not None, it must be a '*.jpg' or '*.png' path into which
      the matching image displaying the features aligned in target_img and reference_img is displayed.
      If None, nothing is stored.
    
    :return: aligned_img, homography; where aligned_img is a version of target_img that is best aligned with reference_img,
      and homography is the 3x3 matrix that represents the homography. 
      Unknown pixel values after the alignment are set to zero in aligned_img.
    """
    # Convert images to grayscale if needed
    im1Gray = cv2.cvtColor(target_img, cv2.COLOR_BGR2GRAY) if target_img.shape[2] > 1 else target_img
    im2Gray = cv2.cvtColor(reference_img, cv2.COLOR_BGR2GRAY) if reference_img.shape[2] > 1 else reference_img

    # Detect ORB features and compute descriptors.
    orb = cv2.ORB_create(max_features)
    keypoints1, descriptors1 = orb.detectAndCompute(im1Gray, None)
    keypoints2, descriptors2 = orb.detectAndCompute(im2Gray, None)

    # Match features.
    matcher = cv2.DescriptorMatcher_create(cv2.DESCRIPTOR_MATCHER_BRUTEFORCE_HAMMING)
    matches = matcher.match(descriptors1, descriptors2, None)

    # Sort matches by score, keep the specified fraction
    matches = list(sorted(matches, key=lambda x: x.distance, reverse=False))
    numGoodMatches = int(len(matches) * good_match_percent)
    matches = matches[:numGoodMatches]

    # Draw top matches
    if matches_img_path is not None:
        imMatches = cv2.drawMatches(target_img, keypoints1, reference_img, keypoints2, matches, None)
        cv2.imwrite(matches_img_path, imMatches)

    # Extract location of good matches
    points1 = np.zeros((len(matches), 2), dtype=np.float32)
    points2 = np.zeros((len(matches), 2), dtype=np.float32)

    for i, match in enumerate(matches):
        points1[i, :] = keypoints1[match.queryIdx].pt
        points2[i, :] = keypoints2[match.trainIdx].pt

    # Find homography
    h, mask = cv2.findHomography(points1, points2, cv2.RANSAC)

    # Use homography
    height, width, channels = reference_img.shape
    im1Reg = cv2.warpPerspective(target_img, h, (width, height))
    im1Reg = im1Reg[:, :, np.newaxis]

    return im1Reg, h


class AlignmentTask(enb.experiment.ExperimentTask):
    def __init__(self, image_list):
        super().__init__(param_dict=dict(image_list=image_list))


class AlignmentExperiment(enb.experiment.Experiment):
    def __init__(self, tasks, output_dir):
        super().__init__(tasks=tasks, dataset_paths=[__file__])
        self.output_dir = output_dir

    def column_alignment_time(self, index, row):
        """Perform alignment, generate stacked image and store into the output_dir.
        """
        _, task = self.index_to_path_task(index)
        task_index = self.tasks.index(task)
        group_images = [enb.isets.load_array_bsq(p) for p in task.param_dict["image_list"]]
        ref_img = group_images[0]
        time_before = time.time()
        aligned_images = [get_aligned_image(
            target_img=g_img, reference_img=ref_img,
            matches_img_path=os.path.join(self.output_dir, f"matches_group{task_index}_target{g_index + 1}.png"))[0]
                          for g_index, g_img in enumerate(group_images[1:])]
        alignment_time = time.time() - time_before
        output_images = [img[160:, :2560, :].squeeze() for img in [ref_img] + aligned_images]
        width, height = output_images[0].shape[:2]
        stacked_aligned_img = np.dstack(output_images)
        output_path = os.path.join(self.output_dir,
                                   f"aligned_img{task_index}_u8be-{len(output_images)}x{height}x{width}.raw")
        enb.isets.dump_array_bsq(array=stacked_aligned_img, file_or_path=output_path)
        if len(output_images) == 3:
            enb.isets.render_array_png(img=stacked_aligned_img,
                                       png_path=output_path[:-3] + "png")
        return alignment_time


if __name__ == '__main__':
    # Generate aligned dataset
    enb.config.options.persistence_dir = os.path.join("persistence", "persistence_automatic_alignment_experiment.py")
    image_list = list(sorted(glob.glob(os.path.join("datasets", "fields2020", "*.raw"))))
    output_dir = "datasets_aligned"
    group_size = 3
    tasks = []
    for i in range(len(image_list) - (group_size - 1)):
        tasks.append(AlignmentTask(image_list=image_list[i:i + group_size]))
    alignment_exp = AlignmentExperiment(tasks=tasks, output_dir=output_dir)
    df = alignment_exp.get_df()

    # Compress with and without MCT
    enb.config.options.base_dataset_dir = output_dir
    enb.config.options.plot_dir = os.path.join("plots", "aligned_images")
    enb.config.options.analysis_dir = os.path.join("analysis", "aligned_images")
    enb.config.options.base_tmp_dir = "tmp"

    task_families = []
    codecs = []
    
    for large_p in [0, 1, 2]:
        codec = plugins.lcnl.lcnl_codecs.CCSDS_LCNL_GreenBook()
        codec.param_dict["large_p"] = large_p
        codecs.append(codec)
        task_families.append(enb.aanalysis.TaskFamily(label=f"{codec.label} $P = {large_p}$"))
        task_families[-1].add_task(task_name=codec.name, task_label=f"{codec.label} $P = {large_p}$")

    compression_exp = enb.icompression.LosslessCompressionExperiment(
        codecs=codecs, task_families=task_families)
    df = compression_exp.get_df()

    ctp = compression_exp.joined_column_to_properties
    ctp["bpppc"].plot_min, ctp["bpppc"].plot_max = 2.5, 2.8
    ctp["compression_efficiency_1byte_entropy"].plot_min = 2.0
    enb.aanalysis.ScalarNumericAnalyzer().get_df(
        full_df=df,
        target_columns=["bpppc", "compression_efficiency_1byte_entropy"],
        column_to_properties=ctp,
        group_by=task_families,
        show_grid=True, show_subgrid=True)
