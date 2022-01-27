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

    def __init__(self, tree_size, tree_count, source, qstep):
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
            qstep=qstep))

    def get_forest(self):
        raise NotImplementedError


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
        for i, symbol in enumerate(source.symbols):
            if i == 0:
                probabilities.append(symbol.p)
                previous_p = symbol.p
            elif (i < len(source.symbols) - 1) and (i % 2 == 1):
                # There is a pair here - smooth and check
                next_symbol = source.symbols[i + 1]
                avg_p = (symbol.p + next_symbol.p) / 2
                if options.verbose > 1 and avg_p > previous_p and avg_p - previous_p > 5e-5:
                    enb.logger.warn(f"Warning: @symbol#{i}: avg_p > previous_p: "
                                    f"avg_p={avg_p}, previous_p={previous_p}, "
                                    f"avg_p - previous_p={avg_p - previous_p}. Will normalize to previous_p.")
                avg_p = min(avg_p, previous_p)

                # Inform of variability
                symbol_rel_err = (symbol.p - avg_p) / symbol.p \
                    if symbol.p > 0 else (0 if symbol.p == avg_p else float('inf'))
                next_symbol_rel_err = (next_symbol.p - avg_p) / next_symbol.p \
                    if next_symbol.p > 0 else (0 if next_symbol.p == avg_p else float('inf'))
                if options.verbose and avg_p > 0.001:
                    enb.logger.debug(f"Relative::absolute error for +-{(i + 1) / 2}: "
                                     f"{max(symbol_rel_err, next_symbol_rel_err):.7f}"
                                     f"::{abs(symbol.p - avg_p)}")

                # Save smoothed probabilities
                probabilities.extend((avg_p, avg_p))
                previous_p = avg_p
            elif i == len(source.symbols) - 1:
                probabilities.append(min(symbol.p, previous_p))
        assert len(probabilities) == len(source.symbols)

        # Make sure that all probabilities are above zero, make sure they sum what they ought to
        probabilities = [max(2 * sources.Source.min_symbol_probability, p)
                         for p in probabilities]
        probability_sum = sum(probabilities)
        assert probability_sum < 2
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
        enb.logger.debug(f"There are {len(probabilities)} defined probabilities used for building the tree at {self}. "
                         f"These are the ones above {threshold} :")
        enb.logger.debug("\n".join(f"P({s}) = {p}" for s, p in enumerate(probabilities)
                                   if p > threshold))

        last_p = float("inf")
        for s in source.symbols:
            assert s.p <= last_p, f"Error: sources should come in decreasing " \
                                  f"order of probability, but {(s.p, last_p)} appeared."
            last_p = s.p

        return v2f.FastYamamotoForest(
            max_included_size=self.param_dict["tree_size"],
            tree_count=self.param_dict["tree_count"],
            source=source)


class ForestBuildExperiment(enb.experiment.Experiment):
    """Experiment to build V2F forests.

    A single column function is defined that calls the tree generation routine,
    dumps it and store some time and size metrics.
    """

    def __init__(self, output_dir, build_forest_tasks, **kwargs):
        assert all(isinstance(t, BuildForestTask) for t in build_forest_tasks)
        super().__init__(tasks=build_forest_tasks)
        self.output_dir = output_dir
        os.makedirs(self.output_dir, exist_ok=True)
        assert os.path.isdir(self.output_dir), \
            f"Could not create output dir {self.output_dir} to store the generated forests."

    def get_df(self, target_indices=None, target_columns=None,
               fill=True, overwrite=None, chunk_size=None):
        target_indices = ["avg_all"] if target_indices is None else target_indices
        super().get_df(target_indices=target_indices,
                       target_columns=target_columns,
                       fill=fill, overwrite=overwrite,
                       chunk_size=chunk_size)

    @enb.atable.column_function([
        enb.atable.ColumnProperties("header_size", label="Header size (bytes)"),
        enb.atable.ColumnProperties("build_time", label="Build time (s)"),
        enb.atable.ColumnProperties("dump_time", label="Dump time (s)"),
    ])
    def build_header(self, index, row):
        tree_size = row["param_dict"]["tree_size"]
        tree_count = row["param_dict"]["tree_count"]
        source = row["param_dict"]["source"]
        qstep = row["param_dict"]["qstep"]

        # Generate forest objects
        file_path, task_name = index
        forest_generation_task = self.tasks_by_name[task_name]

        time_before = time.process_time()
        generated_forest = forest_generation_task.get_forest()
        time_after = time.process_time()
        row["build_time"] = time_after - time_before

        # Save forest to a file
        source_str = f"optimizedfor-{source.affix + '_' if source.affix else 'customdist_'}" \
                     f"withentropy-{source.entropy:.6f}"
        output_path = os.path.join(
            self.output_dir,
            forest_generation_task.__class__.__name__,
            f"v2f_header-{forest_generation_task.__class__.__name__}_"
            f"qstep-{qstep}_"
            f"treesize-{tree_size}_"
            f"treecount-{tree_count}_"
            f"symbolcount-{len(source.symbols)}_"
            f"{source_str}_"
            f"{os.path.basename(file_path)}.v2fh")
        os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
        time_before = time.process_time()
        generated_forest.dump_v2f_header(output_path)
        enb.logger.verbose(f"Saved v2fh file to {repr(output_path)}")

        time_after = time.process_time()
        row["dump_time"] = time_after - time_before

        row["header_size"] = os.path.getsize(output_path)


if __name__ == '__main__':
    print("Module not callable. Run quantize_predict_analyze_makeforests.py instead.")
