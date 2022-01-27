#!/usr/bin/env python3
"""Displacement experiment studying prediction using a given image for predicting itself
after different shiftings.
"""

import os
import numpy as np
import enb
import math
from enb.config import options


class DisplacementExperiment(enb.experiment.Experiment):
    default_file_properties_table_class = enb.isets.ImagePropertiesTable
    dataset_files_extension = "raw"

    def __init__(self, tasks,
                 dataset_paths=None,
                 csv_experiment_path=None,
                 csv_dataset_path=None,
                 dataset_info_table=None,
                 overwrite_file_properties=False,
                 task_families=None):
        super().__init__(tasks=tasks,
                         dataset_paths=dataset_paths,
                         csv_experiment_path=csv_experiment_path,
                         csv_dataset_path=csv_dataset_path,
                         dataset_info_table=dataset_info_table,
                         overwrite_file_properties=overwrite_file_properties,
                         task_families=task_families)

    @enb.atable.column_function([enb.atable.ColumnProperties("entropy", label="Residual entropy", plot_min=0),
                                 enb.atable.ColumnProperties("mse", label="Residual MSE", plot_min=0)])
    def set_displacement_results(self, index, row):
        """Generates the displaced image, calculates the mapped residuals between the original image and displaced, and
        sets the entropy and energy columns."""

        file_path, task_name = index
        task = self.tasks_by_name[task_name]
        image_info_row = self.dataset_table_df.loc[enb.atable.indices_to_internal_loc(file_path)]

        img = enb.isets.load_array_bsq(file_or_path=file_path, image_properties_row=image_info_row)

        x_displacement = task.param_dict["x_displacement"]
        y_displacement = task.param_dict["y_displacement"]

        img_displaced = np.zeros(np.shape(img), dtype=np.int64)
        if x_displacement == 0 and y_displacement == 0:
            img_displaced = img
        elif x_displacement == 0:
            if y_displacement > 0:
                img_displaced[:, y_displacement:, :] = img[:, :-y_displacement, :]
            else:
                img_displaced[:, :y_displacement, :] = img[:, -y_displacement:, :]
        elif y_displacement == 0:
            if x_displacement > 0:
                img_displaced[x_displacement:, :, :] = img[:-x_displacement, :, :]
            else:
                img_displaced[:x_displacement, :, :] = img[-x_displacement:, :, :]
        else:
            if x_displacement > 0 and y_displacement > 0:
                img_displaced[x_displacement:, y_displacement:, :] = img[:-x_displacement, :-y_displacement, :]
            elif x_displacement < 0 and y_displacement < 0:
                img_displaced[:x_displacement, :y_displacement, :] = img[-x_displacement:, -y_displacement:, :]
            elif x_displacement < 0:
                img_displaced[:x_displacement, y_displacement:, :] = img[-x_displacement:, :-y_displacement, :]
            else:
                img_displaced[x_displacement:, :y_displacement, :] = img[:-x_displacement, -y_displacement:, :]

        residuals = img - img_displaced
        symbols, counts = np.unique(residuals, return_counts=True)
        prob = counts / np.size(residuals)

        entropy = - np.sum(prob * np.log2(prob))
        mse = np.sum(residuals.flatten() ** 2) / image_info_row["samples"]

        row["entropy"] = entropy
        row["mse"] = mse

    def column_x_displacement(self, index, row):
        path, task = self.index_to_path_task(index)
        return task.param_dict["x_displacement"]

    def column_y_displacement(self, index, row):
        path, task = self.index_to_path_task(index)
        return task.param_dict["y_displacement"]

    @enb.atable.column_function("total_displacement", label="Total displacement X + Y (px)")
    def set_total_displacement(self, index, row):
        path, task = self.index_to_path_task(index)
        row[_column_name] = task.param_dict["x_displacement"] + task.param_dict["y_displacement"]

    @enb.atable.column_function("total_displacement_euclidean", label=r"Total displacement $\sqrt{X^2 + Y^2}$ (px)")
    def set_total_displacement_euclidean(self, index, row):
        path, task = self.index_to_path_task(index)
        row[_column_name] = math.sqrt((task.param_dict["x_displacement"] * task.param_dict["x_displacement"])
                                      + (task.param_dict["y_displacement"] * task.param_dict["y_displacement"]))

    def column_axis_of_displacement(self, index, row):
        path, task = self.index_to_path_task(index)
        if task.param_dict["x_displacement"] > 0 and task.param_dict["y_displacement"] == 0:
            return "x"
        elif task.param_dict["y_displacement"] > 0 and task.param_dict["x_displacement"] == 0:
            return "y"
        else:
            return "x and y"


class DisplacementTask(enb.experiment.ExperimentTask):

    def __init__(self, x_displacement=0, y_displacement=0, param_dict=None):
        super().__init__(param_dict=param_dict)
        self.param_dict["x_displacement"] = x_displacement
        self.param_dict["y_displacement"] = y_displacement


if __name__ == '__main__':
    options.plot_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "plots", "plots_displacement")
    options.persistence_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                            "persistence", "persistence_displacement_experiment.py")
    enb.config.options.chunk_size = 1024
    x_displacement_family = enb.aanalysis.TaskFamily("X-axis displacement")
    y_displacement_family = enb.aanalysis.TaskFamily("Y-axis displacement")
    xy_displacement_family = enb.aanalysis.TaskFamily("X+Y-axis displacement")
    task_families = [x_displacement_family, y_displacement_family, xy_displacement_family]

    tasks = []
    for displacement in list(range(-20, 0)) + list(range(1, 21)):
        tasks.append(DisplacementTask(x_displacement=displacement))
        x_displacement_family.add_task(tasks[-1].name)

        tasks.append(DisplacementTask(y_displacement=displacement))
        y_displacement_family.add_task(tasks[-1].name)

    for displacement in list(range(-10, 0)) + list(range(1, 11)):
        tasks.append(DisplacementTask(x_displacement=displacement, y_displacement=displacement))
        xy_displacement_family.add_task(tasks[-1].name)

    # Create experiment
    exp = DisplacementExperiment(tasks=tasks, task_families=task_families)

    # Generate pandas dataframe with results. Persistence is automatically added
    df = exp.get_df()

    column_pairs = [("total_displacement", "entropy"), ("total_displacement", "mse"),
                    ("total_displacement_euclidean", "entropy"), ("total_displacement_euclidean", "mse"), ]

    exp.column_to_properties["entropy"].plot_min = 3
    exp.column_to_properties["entropy"].plot_max = 4.6
    exp.column_to_properties["mse"].plot_max = 150

    twoscalar_analyzer = enb.aanalysis.TwoNumericAnalyzer(
        csv_support_path=os.path.join(
            options.analysis_dir, "analysis_displacement/", "displacement_analysis_twocolumn.csv"))

    # twoscalar_analyzer.show_y_std = True
    twoscalar_analyzer.get_df(
        full_df=df,
        target_columns=column_pairs,
        group_by=task_families,
        column_to_properties=exp.joined_column_to_properties,
        selected_render_modes={"line"},
        show_grid=True,
    )

    # TODO: Conversion to png is falling, ask Miguel on Wednesday
    # with enb.logger.message_context("Saving plots to PNG"):
    #     enb.aanalysis.pdf_to_png("plots/plots_displacement", "plots/png_plots_displacement")
