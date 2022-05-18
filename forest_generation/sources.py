#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Plugin-internal Symbol and Source classes common to Huffman and V2F codes.
"""
__author__ = "Miguel Hernández Cabronero <miguel.hernandez@uab.cat>"
__date__ = "04/11/2020"

import ast
import math
import itertools
import collections
import numpy as np


class Symbol:
    def __init__(self, label, p):
        self.label = label
        self.p = p

    def __repr__(self):
        return str(self.label)

    def __hash__(self):
        return hash(self.label)

    def __eq__(self, other):
        try:
            return self.label == other.label
        except AttributeError:
            return self.label == other

    def __lt__(self, other):
        return self.label < other.label

    def __gt__(self, other):
        return self.label > other.label


class Source:
    min_symbol_probability = 1e-10

    def __init__(self, symbols, seed=None, affix=""):
        """Generate a soruce given a list of symbols.

        :param symbols: the list of symbols to include in the source
        :param seed: seed used to generate this source, if any
        :param affix: optional string describing the type of source this is. It is reflected in self.name
        """
        if symbols:
            assert round(sum(s.p for s in symbols), 10) == 1, \
                f"The probability sum of the input symbols was not close enough to 1 ({sum(s.p for s in symbols)})."
            assert all(s.p >= 0 for s in symbols), f"A negative probability was found among the input symbols."

        for i in range(len(symbols), -1):
            assert symbols[i].p >= symbols[i + 1].p, f"Symbols should be in order of probability"

        self.symbols = symbols
        self.affix = affix
        self.seed = seed

    @property
    def name(self):
        name = f"{self.__class__.__name__}"
        if self.affix:
            name += f"_{self.affix}"
        name += f"_{len(self.symbols)}symbols_entropy{self.entropy:.7f}"
        if self.seed is not None:
            name += f"_seed{hex(self.seed)}"
        return name

    @staticmethod
    def get_meta(source, symbols_per_meta):
        """Build a meta-source from source.
        :param source: source from which the meta-source is to be created
        :param symbols_per_meta: meta_symbols represent at most symbols_per_meta
          symbols
        :return: (
          # the created meta source
          meta_source,
          # dictionary indexed by metasymbol, entries are list of contained symbols
          metasymbol_to_symbols,
          # dictionary indexed by (a source's) symbol, entries are indices
          # of symbol in metasymbol_to_symbol[symbol]
          symbol_to_metasymbol_remainder:
          )
        """
        assert symbols_per_meta > 0
        meta_symbols = []
        metasymbol_to_symbols = {}
        symbol_to_metasymbol_remainder = {}
        for i in range(math.ceil(len(source.symbols) / symbols_per_meta)):
            symbols_in_meta = source.symbols[i * (symbols_per_meta):(i + 1) * (symbols_per_meta)]
            meta_symbols.append(Symbol(label=f"Q{i}", p=sum(s.p for s in symbols_in_meta)))
            symbol_to_metasymbol_remainder.update({symbol: (meta_symbols[-1], remainder)
                                                   for remainder, symbol in enumerate(symbols_in_meta)})
            metasymbol_to_symbols[meta_symbols[-1]] = list(symbols_in_meta)

        # Sanity check
        for symbol in source.symbols:
            meta_symbol, remainder = symbol_to_metasymbol_remainder[symbol]
            assert metasymbol_to_symbols[meta_symbol][remainder] is symbol

        meta_source = Source(symbols=meta_symbols, affix=f"meta-{source.name}")

        if abs(sum(source.probabilities) - 1) > 1e-10:
            print(f"[WARNING:Source.get_meta()] input source has psum = {sum(source.probabilities):12} !!= 1")
        assert abs(sum(meta_source.probabilities) - sum(source.probabilities)) < 1e-10

        return meta_source, metasymbol_to_symbols, symbol_to_metasymbol_remainder

    @staticmethod
    def from_data(array):
        """Produce a source with values contained in a numpy array.
        The resulting code is guaranteed to be able code the array samples
        (read from numpy).
        """
        abs_freq_dict = collections.Counter(array.flatten())
        total_element_count = array.size
        symbols = [Symbol(label=k, p=v / total_element_count) for k, v in abs_freq_dict.items()]
        return Source(symbols=symbols)

    @staticmethod
    def from_average_distributions(*distribution_dicts, symbol_count=None, **kwargs):
        """Take an iterable of dicts representing probability distributions (distribution_dicts)
        and return a `source.Source` instance with the average probability of all distributions passed.

        :param distribution_dicts: one or more probability distributions in the form of dict-like objects.
        :param symbol_count: if not None, distribution_dicts must be in the [0, symbol_count-1] range and
          new symbols with 0 probability will be created so that exactly symbol_count symbols are defined
          in the returned source.
        :param kwargs: passed directly to the Source initializer
        """
        distribution_dicts = [d if isinstance(d, dict) else ast.literal_eval(str(d))
                              for d in distribution_dicts]
        assert len(distribution_dicts) >= 1, f"At least one distribution must be provided"

        # Compute the average distribution.
        sum_prob = collections.Counter()
        for img_prob in distribution_dicts:
            sum_prob.update(img_prob)
        average_prob = {k: v / len(distribution_dicts) for (k, v) in sum_prob.items()}
        assert all(i == int(i) for i in average_prob.keys()), \
            f"Only integer symbols are allowed at this time. " \
            f"{repr(sorted(average_prob.keys()))}"
        assert min(average_prob.keys()) >= 0, f"Only positive symbols are supported at this time."

        # If symbol_cunt is fixed, some zeros might need to be added to the dictionary
        if symbol_count is not None:
            max_found_symbol = max(average_prob.keys())
            assert max_found_symbol <= symbol_count, \
                f"symbol_count={symbol_count}, but found symbol {max_found_symbol} greater than that."
            for i in range(0, symbol_count):
                if i not in average_prob.keys():
                    average_prob[i] = 0

        return Source([Symbol(label=i, p=p) for (i, p) in average_prob.items()], seed=None, **kwargs)

    @staticmethod
    def get_laplacian(symbol_count, max_entropy_fraction, seed):
        """
        :param max_entropy_fraction: fraction of the maximum entropy (ie, the entropy
          obtained for a uniform distribution) for the returned Source
        :return a Laplacian source with symbol_count symbols
        """
        assert 0 <= max_entropy_fraction <= 1
        assert symbol_count > 0
        max_entropy = math.log2(symbol_count)
        entropy_fraction = -1

        min_b = 1e-20
        max_b = 1e10
        while abs(max_entropy_fraction - entropy_fraction) > 1e-10:
            mid_b = 0.5 * (min_b + max_b)
            R = 100
            pdf = [0] * symbol_count
            for i in range(R * symbol_count):
                pdf[i % symbol_count] += max(0, math.exp(-(i) / mid_b))
            # pdf = [max(Source.min_symbol_probability, p) for p in pdf]
            pdf = [v if v > Source.min_symbol_probability else 0 for v in pdf]
            pdf = [p / sum(pdf) for p in pdf]
            entropy_fraction = sum(-p * math.log2(p) for p in pdf if p > 0) / max_entropy
            if entropy_fraction > 1 - 1e-6:
                pdf = [1 / len(pdf)] * len(pdf)
                entropy_fraction = 1
            if entropy_fraction > max_entropy_fraction:
                max_b = mid_b
            else:
                min_b = mid_b
            if abs(min_b - max_b) < 1e-10:
                print("[watch] entropy_fraction = {}".format(entropy_fraction))
                print("[watch] min_b = {}".format(min_b))
                print("[watch] max_b = {}".format(max_b))
                print("[watch] min(pdf) - max(pdf) = {}".format(min(pdf) - max(pdf)))
                raise RuntimeError(f"Source probabilitites didn't converge :: "
                                   f"symbol_count={symbol_count} "
                                   f"max_entropy_fraction={max_entropy_fraction}")

        source = Source([Symbol(label=i, p=p) for i, p in enumerate(pdf)],
                        seed=seed)

        source.affix = "Laplacian"
        assert (max_entropy_fraction - source.entropy / max_entropy) <= 1e-10, (
            max_entropy_fraction, source.entropy / max_entropy)
        return source

    @property
    def entropy(self):
        """:return: the binary source entropy given the symbol distribution
        """
        assert abs(sum(self.probabilities) - 1) < 1e-5, abs(1 - sum(self.probabilities))
        return sum(-math.log2(s.p) * s.p if s.p else 0 for s in self.symbols)

    def generate_symbols(self, count):
        """Get a list of Symbols using the given probability distribution
        """
        ideal_symbols = list(itertools.chain(*([symbol] * math.ceil(symbol.p * count)
                                               for symbol in self.symbols)))
        np.random.seed(self.seed)
        np.random.shuffle(ideal_symbols)
        assert abs(len(ideal_symbols) - count) < len(self.symbols), (len(ideal_symbols), count)
        if count > 10 ** 4:
            assert abs(self.entropy - input_entropy(ideal_symbols)) < 1e-2, \
                (self.entropy, input_entropy(ideal_symbols), self.entropy - input_entropy(ideal_symbols))
        return ideal_symbols

    @property
    def probabilities(self):
        return [s.p for s in self.symbols]

    @property
    def labels(self):
        return [s.label for s in self.symbols]

    def __len__(self):
        return len(self.symbols)

    def __repr__(self):
        # return f"[Source({self.symbols})]"
        return self.name


def input_entropy(input):
    counter = collections.Counter(input)
    assert len(input) == sum(counter.values())
    symbols = sorted(counter.keys(), reverse=True, key=lambda symbol: symbol.p)
    pdf = [counter[symbol] / len(input) for symbol in symbols]
    return sum(-p * math.log2(p) if p > 0 else 0 for p in pdf)


if __name__ == '__main__':
    print("Non executable module")
