#!/usr/bin/env python3
"""Utility experiments to perform a batch generation of V2F coding forests.
"""

import math
import os
import time
import pprint
import enb
from enb.config import options

import v2f
import sources

seed = 0xbadc0ffee % 2 ** 32


class BuildForestTask(enb.experiment.ExperimentTask):
    """Abstract build generation task.
    """

    def __init__(self, tree_size, tree_count, source, qstep, forest_output_dir=None):
        """
        :param tree_size: number of included elements per tree
        :param tree_count: number of trees in the forest
        :param source: source for which the forest is to be optimized
        :param qstep: quantization step applied to obtain this source
        """
        super().__init__(param_dict=dict(
            tree_size=tree_size,
            tree_count=tree_count,
            source=source,
            qstep=qstep,
            forest_output_dir=forest_output_dir))

    def get_forest(self):
        raise NotImplementedError
    
    @property
    def output_path(self):
        source = self.param_dict["source"]
        source_str = f"optimizedfor-{source.affix + '_' if source.affix else 'customdist_'}" \
                     f"withentropy-{source.entropy:.6f}"
        return os.path.join(
            self.param_dict["forest_output_dir"],
            self.__class__.__name__,
            f"v2f_header-{self.__class__.__name__}_"
            f"qstep-{self.param_dict['qstep']}_"
            f"treesize-{self.param_dict['tree_size']}_"
            f"treecount-{self.param_dict['tree_count']}_"
            f"symbolcount-{len(source.symbols)}_"
            f"{source_str}.v2fh")


class FastYamamotoBuildTask(BuildForestTask):
    """Forest generation based on FastYamamotoForest.
    """

    def get_forest(self):
        # Assume laplacianity and 0, -1, 1, -2, 2, ... coding.
        # Then probabilities are smoothed to be A, B, B, C, C, ...,
        # Where A, B, ... are probabilities that sum 1 and satisfy
        # A >= B >= C >= ...
        # Note that this will not produce very good results for very different distributions

        source = self.param_dict["source"]
        source.symbols = sorted(source.symbols, key=lambda symbol: int(symbol.label))
        assert len(source.symbols) > 1, len(source.symbols)
        probabilities = []
        last_probability = 0
        for i, symbol in enumerate(reversed(source.symbols)):
            if symbol.p > last_probability:
                probabilities.append(symbol.p)
                last_probability = symbol.p
            else:
                probabilities.append(float(last_probability))
            
        probabilities = list(reversed(probabilities))
        assert probabilities == sorted(probabilities, reverse=True), (probabilities[:16], sorted(probabilities[:16]))

        # Make sure that all probabilities are above zero, make sure they sum what they ought to
        probabilities = [max(sources.Source.min_symbol_probability, p)
                         for p in probabilities]
        probability_sum = sum(probabilities)
        assert probabilities == sorted(probabilities, reverse=True), (probabilities[:16], sorted(probabilities[:16]))
        probabilities = [p / probability_sum for p in probabilities]
        assert abs(sum(probabilities) - 1) < 1e-10, sum(probabilities)

        # Compute kl divergence between original and normalized
        kl_divergence_a = 0
        kl_divergence_b = 0
        for symbol, p in zip(source.symbols, probabilities):
            try:
                kl_divergence_a += p * math.log2(p / symbol.p)
            except (ValueError, ZeroDivisionError):
                # Ok, a probability was close to zero, don't even mind
                pass
            try:
                kl_divergence_b += symbol.p * math.log2(symbol.p / p)
            except (ValueError, ZeroDivisionError):
                # Ok, a probability was close to zero, don't even mind
                pass

            symbol.p = p


        enb.logger.debug(f"KL divergence due to smoothing: "
                         f"KL(P || Q) = {kl_divergence_a:.10} bps, "
                         f"KL(Q || P) = {kl_divergence_b:.10} bps.")
        threshold = 0.1
        enb.logger.debug(f"There are {len(probabilities)} defined probabilities used for building the tree at {self.__class__}. "
                         f"These are the ones above {threshold} :")
        enb.logger.debug("\n".join(f"P({s}) = {p}" for s, p in enumerate(probabilities)
                                   if p > threshold))

        last_p = float("inf")
        for s in source.symbols:
            assert s.p <= last_p, f"Error: sources should come in decreasing " \
                                  f"order of probability, but {(s.p, last_p)} appeared."
            last_p = s.p

        enb.logger.verbose("Building  "
                           f'max_included_size={self.param_dict["tree_size"]}, '
                           f'tree_count={self.param_dict["tree_count"]}, source={source}')
        tree = v2f.FastYamamotoForest(
            max_included_size=self.param_dict["tree_size"],
            tree_count=self.param_dict["tree_count"],
            source=source)
        return tree


class ForestBuildExperiment(enb.experiment.Experiment):
    """Experiment to build V2F forests.

    A single column function is defined that calls the tree generation routine,
    dumps it and store some time and size metrics.
    """

    def __init__(self, build_forest_tasks, **kwargs):
        assert all(isinstance(t, BuildForestTask) for t in build_forest_tasks)
        super().__init__(tasks=build_forest_tasks)

    def get_df(self, target_indices=None, target_columns=None,
               fill=True, overwrite=None, chunk_size=None):
        target_indices = ["avg_all"] if target_indices is None else target_indices
        return super().get_df(target_indices=target_indices,
                       target_columns=target_columns,
                       fill=fill, overwrite=overwrite,
                       chunk_size=chunk_size)

    @enb.atable.column_function([
        enb.atable.ColumnProperties("header_size", label="Header size (bytes)"),
        enb.atable.ColumnProperties("build_time", label="Build time (s)"),
        enb.atable.ColumnProperties("dump_time", label="Dump time (s)"),
    ])
    def build_header(self, index, row):
        # Generate forest objects
        file_path, task_name = index
        forest_generation_task = self.tasks_by_name[task_name]

        time_before = time.process_time()
        generated_forest = forest_generation_task.get_forest()
        time_after = time.process_time()
        row["build_time"] = time_after - time_before
        enb.logger.debug(f'Built forest in {row["build_time"]} s')

        # Save forest to a file
        output_path = forest_generation_task.output_path
        os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
        time_before = time.process_time()
        with enb.logger.debug_context("Storing generated forest"):
            enb.logger.verbose(f"Dumping generated forest to {output_path}...")
            generated_forest.dump_v2f_header(output_path)
        enb.logger.verbose(f"Saved v2fh file to {repr(output_path)}")

        time_after = time.process_time()
        row["dump_time"] = time_after - time_before

        row["header_size"] = os.path.getsize(output_path)


if __name__ == '__main__':
    print("Module not callable. Run quantize_predict_analyze_makeforests.py instead.")
