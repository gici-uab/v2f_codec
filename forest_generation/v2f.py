#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Python prototype of the V2F codes based on the paper:

 Manuel Martínez Torres, Miguel Hernández-Cabronero, Ian Blanes and Joan Serra-Sagristà,
  “High-throughput variable-to-fixed entropy codec using selective, stochastic code forests,” 
  IEEE Access, vol. 8, no. 1, pp. 81283-81297, 2020. DOI: 10.1109/ACCESS.2020.2991314.
"""
__author__ = "Miguel Hernández Cabronero <miguel.hernandez@uab.cat>"
__date__ = "04/07/2019"

import os
import sys
import heapq
import numpy as np
import math
import itertools
import pickle
import shutil
import collections
import time
import datetime

import bitstream
from sources import Source, Symbol

import enb
from enb.config import get_options

options = get_options()

# Implementation of the internal V2F codec interface.
# See below for enb compatible interface.

class AbstractV2FCodec:
    """Abstract V2F codec, internal to this module.
    See the MarlinCodec class for a fully enb-compatible interface.
    """

    def __init__(self, is_raw=False, build=True):
        self.is_raw = is_raw
        if build:
            self.build()

    def build(self):
        """Run after __init__ if build is True, normally to initialize
        coding structures based on the initialization parameters.
        """
        pass

    def code(self, input, **kwargs):
        """Code an iterable of symbols.

        :return a CodingResults instance
        """
        if self.is_raw:
            return self.code_raw(input=input)
        raise NotImplementedError()

    def decode(self, coded_output, **kwargs):
        """Decode the coded_output of a call to self.code
        :return a list symbols (should be equal to the
          original input for lossless codecs)
        """
        if self.is_raw:
            return self.decode_raw(coded_output=coded_output)
        raise NotImplementedError()

    @property
    def params(self):
        """Get a dictionary with all needed initialization information
        and a '_cls' field with the class whose __init__ is to be invoked.
        """
        d = {"_cls": self.__class__, "is_raw": self.is_raw}
        d.update(self.get_initialization_params())
        return d

    def get_initialization_params(self):
        """:return: a dictionary with the initialization parameters needed to
          produce this codec. This dictionary is passed as argument to self.finish_loading
          after a build=False initialization.
        """
        raise NotImplementedError()

    def code_raw(self, input):
        counter = collections.Counter(input)
        word_length = math.ceil(math.log2(len(counter.items())))
        return {"coded_data_nodes": input, "coded_lenght_bits": len(input) * word_length}

    def decode_raw(self, coded_output):
        return coded_output["coded_data_nodes"]


class CodingResults:
    def __init__(self, data, bits):
        """
        :param data: data needed to reconstruct the input
        :param bits: bits needed to represent the input
        """
        self.data = data
        self.bits = bits


class TreeNode:
    index = None

    def __init__(self, symbol, parent=None):
        """
        :param symbol: symbol of the edge connecting with the parent
        :param parent: parent node, or None for the root node
        """
        self.symbol = symbol
        self.parent = parent
        self.symbol_to_node = {}

    def add_child(self, symbol):
        """Add a new child and return the created node
        """
        assert symbol not in self.symbol_to_node, \
            f"{self.__repr__()} already contained a child for symbol {symbol}"
        node = TreeNode(symbol=symbol, parent=self)
        self.symbol_to_node[symbol] = node
        return node

    def show(self, level=0, sep="    "):
        print(f"{self if self.parent is not None else '(root)'} [p={self.word_probability:.03f}]")
        for symbol, node in self.symbol_to_node.items():
            sys.stdout.write(sep * (level))
            sys.stdout.write("+"
                             + "-" * (len(sep) // 2)
                             + f"({repr(symbol)})"
                             + "-" * (len(sep) // 2)
                             + "> ")
            node.show(level=level + 1, sep=sep)

    @property
    def word(self):
        """Return a tuple of symbols corresponding to this node.
        """
        nodes = [self]
        while nodes[-1].parent is not None:
            nodes.append(nodes[-1].parent)
        return tuple(reversed([node.symbol for node in nodes if node is not None and node.parent is not None]))

    @property
    def word_probability(self):
        """Probability of a node's word, subtracting children probability
        (Marlin's)
        """
        # Probability of the symbols occurring
        p = 1
        for symbol in self.word:
            p *= symbol.p

        # Probability of a longer child being matched
        p *= (1 - sum(symbol.p for symbol in self.symbol_to_node.keys()))

        return round(p, 12)

    @property
    def raw_word_probability(self):
        """Probability of a node's word, WITHOUT subtracting children probability
        (Yamamotos's)
        """
        # Probability of the symbols occurring
        p = 1
        for symbol in self.word:
            p *= symbol.p
        return round(p, 12)

    @property
    def all_children(self):
        nodes = [self] if self.parent is not None else []
        pending_nodes = list(self.symbol_to_node.values())
        
        while pending_nodes:
            next_node = pending_nodes.pop()
            nodes.append(next_node)
            pending_nodes.extend(next_node.symbol_to_node.values())

        return nodes

    def __repr__(self):
        return f"TreeNode(symbol={self.symbol}, parent={self.parent}, #ch={len(self.symbol_to_node)}, word={self.word})"

    def __str__(self):
        return ''.join(str(s) for s in self.word)

    def __gt__(self, other):
        return id(self) > id(other)

    def __lt__(self, other):
        return id(self) < id(other)

    def __hash__(self):
        return id(self)


class TreeCodec(AbstractV2FCodec):
    def __init__(self, size: int, source: Source, build=True, **kwargs):
        """Build a code dictionary for the given source

        :param size: number of nodes to be had in the tree's dictionary
        """
        self.source = Source(symbols=list(source.symbols), seed=None)
        assert size >= len(self.source.symbols), (size, len(self.source.symbols))
        self.size = size
        self.root = TreeNode(symbol=Symbol(label="root", p=0))

        super().__init__(build=False, **kwargs)

        if build:
            self.build()
            assert len(self.included_nodes) <= self.size
            if len(self.included_nodes) != self.size:
                print(f"WARNING: {self.__class__.__name__} produced {len(self.included_nodes)} "
                      f"<= size {self.size} included nodes")

    def deep_copy(self):
        """:return a Tree with new nodes having the same structure and associated symbols
        """
        node_to_new_node = {None: None}
        pending_nodes = [self.root]
        while pending_nodes:
            next_node = pending_nodes.pop()
            new_parent = node_to_new_node[next_node.parent]
            new_node = new_parent.add_child(next_node.symbol) \
                if new_parent is not None else TreeNode(symbol=next_node.symbol, parent=None)
            node_to_new_node[next_node] = new_node
            pending_nodes.extend(next_node.symbol_to_node.values())

        new_tree = TreeCodec(size=self.size, source=self.source, build=False)
        new_tree.root = node_to_new_node[self.root]
        return new_tree, node_to_new_node

    def dump_pickle(self, output_path):
        return TreeWrapper(self).dump_pickle(output_path)

    def finish_loading(self, loaded_params):
        return self

    def build(self):
        raise NotImplementedError()

    def code_one_word(self, input):
        """:return node, remaining_input,
          where node is the node with the longest word in self.included_nodes that prefixes input
        """
        current_index = 0
        current_node = self.root
        try:
            while True:
                current_symbol = input[current_index]
                current_node = current_node.symbol_to_node[current_symbol]
                current_index = current_index + 1
        except (KeyError, IndexError) as ex:
            assert current_index > 0 and current_node is not self.root, \
                "Non complete dictionary?"
            return current_node, input[current_index:]
        except IndexError as ex:
            raise ex

    def code(self, input):
        """:return coded_output, output_bits
        """
        if self.is_raw:
            return self.code_raw(input)

        emitted_nodes = []
        while len(input) > 0:
            node, new_input = self.code_one_word(input)
            node_word = node.word
            assert len(node_word) > 0
            assert all(n == i for n, i in zip(node_word, input))
            emitted_nodes.append(node)
            input = input[len(node.word):]

        coded_dict = {
            "coded_data_nodes": emitted_nodes,
            "coded_lenght_bits": len(emitted_nodes) * self.word_length_bits
        }
        return coded_dict

    def decode(self, coded_dict):
        if self.is_raw:
            return self.decode_raw(coded_output=coded_dict)

        return list(itertools.chain(*(node.word for node in coded_dict["coded_data_nodes"])))

    @property
    def included_nodes(self):
        return [c for c in self.root.all_children
                if len(c.symbol_to_node) < len(self.source.symbols)]

    @property
    def average_length(self):
        p_sum = 0
        avg_len = 0
        for node in self.included_nodes:
            node_p, word_length = node.word_probability, len(node.word)
            p_sum += node_p
            avg_len += node_p * word_length
        assert round(p_sum, 7) == 1, p_sum
        # return round(avg_len, 12)
        return avg_len

    @property
    def word_length_bits(self):
        return max(1, math.ceil(math.log2(len(self.included_nodes))))

    def get_initialization_params(self):
        return {"size": self.size, "source": self.source}

    @property
    def name(self):
        ignored_params = ["_cls"]
        return f"{self.__class__.__name__}" \
               + ("_" if any(k not in ignored_params for k in self.params.keys()) else "") \
               + "_".join(f"{k}-{v}" if k != "source" else f"{k}-{v.name}"
                          for k, v in sorted(self.params.items())
                          if k not in ignored_params)

    def __repr__(self):
        return f"[{self.__class__.__name__} size={self.size} source={self.source.name}]"


class TrivialTree(TreeCodec):
    def __init__(self, size: int, source: Source, build=True, **kwargs):
        super().__init__(size=size, source=source, build=build, **kwargs)

    def build(self):
        # Basic initialization
        for symbol in self.source.symbols:
            self.root.add_child(symbol)


class TunstallTree(TreeCodec):
    """Build a V2F coding tree using Tunstall's algorithm.
    """

    def __init__(self, size: int, source: Source, build=True, **kwargs):
        super().__init__(size=size, source=source, build=build, **kwargs)

    def build(self):
        node_list = [self.root.add_child(symbol)
                     for symbol in self.source.symbols]

        while len(node_list) + len(self.source.symbols) <= self.size:
            node_list = sorted(node_list, key=lambda n: n.word_probability)
            next_node = node_list[-1]
            node_list.extend((next_node.add_child(symbol)
                              for symbol in self.source.symbols))

        included_nodes = sorted(
            [n for n in node_list if len(n.symbol_to_node) < len(self.source.symbols)],
            key=lambda n: n.word)
        for i, node in enumerate(included_nodes):
            node.index = i


class MarlinBaseTree(TreeCodec):
    """GetTree function - no Markov optimization
    """

    def __init__(self, size: int, source: Source, build=True, **kwargs):
        super().__init__(size=size, source=source, build=build, **kwargs)

    def node_probability(self, node):
        return node.word_probability

    def build(self):
        # Basic initialization
        p_node_heap = [(-self.node_probability(node), node)
                       for symbol in self.source.symbols
                       for node in [self.root.add_child(symbol=symbol)]]
        heapq.heapify(p_node_heap)

        while len(p_node_heap) < self.size:
            # Add new node
            p, expanded_node = heapq.heappop(p_node_heap)
            next_symbol = self.source.symbols[len(expanded_node.symbol_to_node)]
            new_node = expanded_node.add_child(next_symbol)

            # Update heap
            if len(expanded_node.symbol_to_node) < len(self.source) - 1:
                # Still can be further expanded
                heapq.heappush(p_node_heap,
                               (-self.node_probability(expanded_node),
                                expanded_node))
            elif len(self.source) > 1:
                additional_node = expanded_node.add_child(self.source.symbols[-1])
                heapq.heappush(p_node_heap,
                               (-self.node_probability(additional_node),
                                additional_node))

            heapq.heappush(p_node_heap,
                           (-self.node_probability(new_node),
                            new_node))
            # p_node_heap is incremented in exactly one element after each loop


class MarlinTreeMarkov(MarlinBaseTree):
    """Marlin's code tree, optimized iteratively for stationarity
    """

    def __init__(self, size: int, source: Source, build=True, **kwargs):
        self.state_probabilities = [1] + [0] * (len(source.symbols) - 2)
        # For conditional probability calculation
        self.state_probability_divisors = [1] + [sum(s.p for s in source.symbols[i:])
                                                 for i in range(1, len(source.symbols) - 1)]
        super().__init__(size=size, source=source, build=build, **kwargs)

    def node_conditional_probability(self, node, state_index, state_tree):
        word_probability = node.word_probability
        state_probability = self.state_probabilities[state_index] if self is state_tree else 0
        state_divisor = self.state_probability_divisors[state_index]
        first_symbol_index = self.source.symbols.index(node.word[0])
        if state_probability > 0:
            cp = (state_probability * word_probability / state_divisor) \
                if first_symbol_index >= state_index and state_divisor > 0 else 0
        else:
            cp = 0

        return cp

    def node_probability(self, node):
        return sum(self.node_conditional_probability(node=node, state_index=i, state_tree=self)
                   for i in range(len(self.state_probabilities)))

    def calculate_state_probabilities(self):
        self.state_probabilities = [1] + [0] * (len(self.state_probabilities) - 1)

        # Some sanity checks
        p_sum_by_i = {}
        for i in range(len(self.state_probabilities)):
            p_sum_by_i[i] = 0
            for node in self.included_nodes:
                p_sum_by_i[i] += self.node_conditional_probability(node=node, state_index=i, state_tree=self)
        assert abs(sum(p_sum_by_i.values()) - 1) < 1e-8, sum(p_sum_by_i.values())
        #
        total_s = 0
        for i in range(len(self.state_probabilities)):
            s = 0
            for node in self.included_nodes:
                s += self.node_conditional_probability(node=node, state_index=i, state_tree=self)
            total_s += s
        assert abs(total_s - 1) < 1e-8, math.log10(total_s)

        previous_state_probabilities = self.state_probabilities

        try:
            transition_matrix = np.zeros((len(self.state_probabilities),
                                          len(self.state_probabilities)))
            for i in range(len(self.state_probabilities)):
                for node in self.included_nodes:
                    j = len(node.symbol_to_node)
                    transition_matrix[i, j] += \
                        self.node_conditional_probability(node=node, state_index=i, state_tree=self) / \
                        self.state_probabilities[i] \
                            if self.state_probabilities[i] else 0
                if (transition_matrix[i, :] == 0).all():
                    transition_matrix[i, :] = 1
                    transition_matrix[i, :] /= np.sum(transition_matrix[i, :])

            iteration_count = 0
            for i in range(250):
                iteration_count += 1
                new_state_probabilities = [0] * len(self.state_probabilities)
                for j in range(len(self.state_probabilities)):
                    for i in range(len(self.state_probabilities)):
                        new_state_probabilities[j] += self.state_probabilities[i] * transition_matrix[i, j]

                delta = sum(abs(old - new) for old, new in zip(
                    self.state_probabilities, new_state_probabilities))
                # assert abs(sum(new_state_probabilities) - 1) < 1e-8, sum(new_state_probabilities)
                tolerance = 1e4
                if delta < tolerance:
                    break
                self.state_probabilities = new_state_probabilities
            else:
                raise Exception(f"Couldn't converge calculating state probabilities {delta}")

            return self.state_probabilities

        finally:
            self.state_probilities = previous_state_probabilities

    def build(self, update_state_probabilities=True):
        prev_state_probabilities = self.state_probabilities
        super().build()

        previous_words = set()
        for iteration_count in itertools.count(0):
            if update_state_probabilities:
                self.state_probabilities = self.calculate_state_probabilities()
            self.root = TreeNode(symbol=self.root.symbol, parent=None)
            super().build()

            new_words = ",".join(sorted((str(node.word) for node in self.included_nodes)))
            if new_words in previous_words:
                break
            else:
                previous_words.add(new_words)

            tolerance = 1e-10 * ((iteration_count + 1) / 10)
            delta = sum(abs(old - new) for old, new in zip(
                prev_state_probabilities, self.state_probabilities))
            if delta <= tolerance:
                break

            prev_state_probabilities = self.state_probabilities


class YamamotoTree(TreeCodec):
    """[1] YAMAMOTO AND YOKOO:
    AVERAGE-SENSE OPTIMALITY AND COMPETITIVE OPTIMALITY FOR VF CODES
    """

    def __init__(self, size: int, source: Source, build=True,
                 first_allowed_symbol_index=0, **kwargs):
        self.first_allowed_symbol_index = first_allowed_symbol_index
        assert 0 <= self.first_allowed_symbol_index <= len(source.symbols) - 2
        super().__init__(size=size, source=source, build=build, **kwargs)

    def next_child_raw_probability(self, node):
        return node.raw_word_probability * self.source.symbols[
            len(node.symbol_to_node)].p

    def build(self):
        # Basic initialization
        node_list = [self.root.add_child(symbol)
                     for symbol in self.source.symbols[self.first_allowed_symbol_index:]]
        # hat_n : based on raw word probability and maximum degree
        hat_n_heap = [(-node.raw_word_probability, -len(node.symbol_to_node), node)
                      for node in node_list]
        heapq.heapify(hat_n_heap)
        # tilde_n : based on the probability of the next available child
        tilde_n_heap = [(-self.next_child_raw_probability(node), node)
                        for node in node_list]
        heapq.heapify(tilde_n_heap)
        node_count = len(node_list)
        del node_list

        while node_count < self.size:
            _, _, hat_n = hat_n_heap[0]
            expansion_count = max(0, len(self.source.symbols) - len(hat_n.symbol_to_node) - 1)
            assert expansion_count > 0

            S1, S2 = 0, 0  # Gains as defined in [1]

            if node_count + expansion_count <= self.size:
                # S1 - Calculate gain of expanding the most probable node
                S1 = hat_n.raw_word_probability * sum(
                    symbol.p for symbol in self.source.symbols[-expansion_count - 1:])

                # S2 - Calculate gain of expanding the expansion_count best nodes
                # but do it in a copy in case S2 is not accepted
                # s2_root = copy.deepcopy(self.root, memo)
                s2, memo = self.deep_copy()
                s2_root = s2.root
                s2_hat_heap = [(t[0], t[1], memo[t[2]]) for t in hat_n_heap]
                s2_tilde_heap = [(t[0], memo[t[1]]) for t in tilde_n_heap]
            else:
                # Not enough free words to expand - S2 happening for sure
                expansion_count = self.size - node_count
                # no need to deepcopy
                s2_root = self.root
                s2_hat_heap = hat_n_heap
                s2_tilde_heap = tilde_n_heap

            for i in range(expansion_count):
                # Add child node
                ## Remove expanded from queues
                _, tilde_n = heapq.heappop(s2_tilde_heap)
                s2_hat_heap = list(filter(lambda t: t[2] != tilde_n, s2_hat_heap))

                ## Add node to expanded
                new_node = tilde_n.add_child(self.source.symbols[
                                                 len(tilde_n.symbol_to_node)])
                S2 += new_node.raw_word_probability
                # Automatically expand last child if possible
                if len(tilde_n.symbol_to_node) == len(self.source.symbols) - 1:
                    tilde_n = tilde_n.add_child(self.source.symbols[-1])
                    S2 += tilde_n.raw_word_probability

                # Update heaps
                ## Expanded node (or automatic expansion)
                heapq.heappush(s2_hat_heap,
                               (-tilde_n.raw_word_probability, len(tilde_n.symbol_to_node), tilde_n))
                heapq.heappush(s2_tilde_heap,
                               (-self.next_child_raw_probability(tilde_n), tilde_n))
                ## New node
                heapq.heappush(s2_hat_heap,
                               (-new_node.raw_word_probability, len(new_node.symbol_to_node), new_node))
                heapq.heappush(s2_tilde_heap,
                               (-self.next_child_raw_probability(new_node), new_node))

            if S1 >= S2:
                assert hat_n == heapq.heappop(hat_n_heap)[2]
                len_before = len(tilde_n_heap)
                tilde_n_heap = [t for t in tilde_n_heap if t[1] != hat_n]
                assert len(tilde_n_heap) == len_before - 1

                for symbol in self.source.symbols[-expansion_count - 1:]:
                    if len(self.included_nodes) == self.size:
                        break
                    new_node = hat_n.add_child(symbol)
                    heapq.heappush(hat_n_heap,
                                   (-new_node.raw_word_probability, 0, new_node))
                    heapq.heappush(tilde_n_heap,
                                   (-self.next_child_raw_probability(new_node), new_node))
                else:
                    assert len(hat_n.symbol_to_node) == len(self.source.symbols), \
                        (len(hat_n.symbol_to_node), len(self.source.symbols))
            else:
                self.root = s2_root
                hat_n_heap = s2_hat_heap
                tilde_n_heap = s2_tilde_heap

            node_count += expansion_count


class FastYamamotoTree(YamamotoTree):
    """Fast version of the Yamamoto algorithm, in which the S1 vs S2 comparison is not made.

    This means that the next expanded node is decided in a slightly suboptimal ways in some cases.
    However, experimental results indicate that this difference is small, particularly for
    larger tree sizes. In exchange, this version is able to generate trees significantly
    faster than its fully optimal counterpart, and requires less memory.
    """

    def build(self):
        # Basic initialization: nodes for all defined symbols are added to the root.
        # However, only symbols from the i-th onwards are put in the list of optimizable nodes.
        # This prevents expansion of the first i branches of the root, hence optimizing
        # for the stat in which the tree is going to be used.
        # Note that strictly speaking, the root node does not need have children associated
        # to any of the first i symbols. By adding them, trees are more regular and it allows
        # for easier and less bug-prone development of the associated C prototype. This is trade off
        # for slightly suboptimal performance, because these nodes could have been employed
        # to expand branches of interest. However, the expected loss is neglibible for large enough
        # tree sizes, e.g., 65535.
        node_list = [self.root.add_child(symbol)
                     for symbol in self.source.symbols][self.first_allowed_symbol_index:]

        # hat_n : based on raw word probability and degree (larger number of children first)
        hat_n_heap = [(-node.word_probability, -len(node.symbol_to_node), node)
                      for node in node_list]
        heapq.heapify(hat_n_heap)

        # In UAB's modification of Yamamoto, expansion of nodes is always on that of the highest
        # probability, and among them the one with more children already present.
        # The case where expanding the previous-to-last node and the "free" last node addition
        # is not compared to it because benefits are negligible for large tree sizes (e.g., 65535).
        # # tilde_n : based on the probability of the next available child
        # tilde_n_heap = [(-self.next_child_raw_probability(node), node)
        #                 for node in node_list]
        # heapq.heapify(tilde_n_heap)

        # The original list is deleted to avoid unintentional accesses from here on
        del node_list

        # In UAB's modification of Yamamoto, a single node is always added at a time.
        # Therefore, the exact number of iterations is known in advance
        for node_count in range(self.size - len(self.source.symbols)):
            if node_count % 5000 == 0:
                enb.logger.info(f"Forest generation {self.source} :: processed {node_count} nodes")

            # The most probable node is expanded next
            _, _, hat_n = heapq.heappop(hat_n_heap)

            # Add the next node and include it in the heap of expandable nodes (the child always has 0 children itself)
            probability_before = hat_n.word_probability
            next_node = hat_n.add_child(self.source.symbols[len(hat_n.symbol_to_node)])
            assert hat_n.word_probability < probability_before
            heapq.heappush(hat_n_heap, (-next_node.word_probability, 0, next_node))

            if len(hat_n.symbol_to_node) < len(self.source.symbols) - 1:
                # Regular node that did not cause expansion: it is added back to the heap
                # with its updated probability.
                heapq.heappush(hat_n_heap, (-hat_n.word_probability, len(hat_n.symbol_to_node), hat_n))
            else:
                # hat_n has |A|-1 symbols, it is free to add the last symbol and remove hat_n from the
                # heap of expandable
                assert len(hat_n.symbol_to_node) == len(self.source.symbols) - 1
                next_node = hat_n.add_child(self.source.symbols[-1])
                heapq.heappush(hat_n_heap, (-next_node.word_probability, 0, next_node))
                assert next_node.word_probability == 0

        enb.logger.info(f"Forest generation {self.source} :: processed all nodes")


class Forest(AbstractV2FCodec):

    def __init__(self, tree_sizes, source: Source, build=True, **kwargs):
        """For each value in tree_sizes, a tree is created with at most that size
        """
        if tree_sizes is None and source is None:
            # Empty tree
            return

        self.is_raw = (len(source) == 1)

        assert all(s > 0 for s in tree_sizes)
        self.tree_sizes = list(tree_sizes)
        self.trees = []
        self.current_tree = None
        self.node_to_next_tree = {}
        self.source = source

        super().__init__(build=build)

    def build(self):
        raise NotImplementedError()

    def finish_loading(self, loaded_params):
        """Re-build any needed structures after loading parameters in Forest.load()
        """
        return self

    def code_one_word(self, input):
        node, new_input = self.current_tree.code_one_word(input)
        try:
            self.current_tree = self.node_to_next_tree[node]
        except KeyError as ex:
            if len(new_input) == 0:
                self.current_tree = None
            else:
                raise ex
        return node, new_input

    def code(self, input):
        """Code all symbols in input"""
        if self.is_raw:
            return self.code_raw(input=input)

        emitted_nodes = []
        root_nodes = set(tree.root for tree in self.trees)
        i = 0
        while len(input) > 0 and self.current_tree not in root_nodes:
            new_node, input = self.code_one_word(input)
            emitted_nodes.append(new_node)
            if options.verbose > 1:
                i += 1
                if i % ((101 + len(input)) // 100) == 0:
                    print(f"[watch] len(input)={len(input)}")

        coded_dict = {
            "coded_data_nodes": emitted_nodes,
            "coded_lenght_bits": len(emitted_nodes) * self.word_length_bits
        }
        return coded_dict

    def decode(self, coded_output):
        if self.is_raw:
            return self.decode_raw(coded_output=coded_output)
        return list(itertools.chain(*(node.word for node in coded_output["coded_data_nodes"])))

    @property
    def included_nodes(self):
        return list(itertools.chain(*(tree.included_nodes for tree in self.trees)))

    def dump_v2f_header(self, output_path):
        """Represent this codec in a file with format as described in v2f_file.h.
        """
        with bitstream.OutputBitStream(output_path) as obs:
            bytes_per_index = 4

            # Total entry count
            total_entry_count = sum(len(tree.root.all_children) for tree in self.trees)
            assert total_entry_count <= 2 ** 32, total_entry_count
            obs.put_unsigned_value(total_entry_count, 8 * 4)

            # Bytes per word
            max_included_size = max(len(tree.included_nodes) for tree in self.trees)
            bytes_per_word = math.ceil(math.log2(max_included_size) / 8)
            obs.put_unsigned_value(bytes_per_word, 8 * 1)

            # Bytes per sample
            bytes_per_sample = math.ceil(math.log2(len(self.source.symbols)) / 8)
            assert 1 <= bytes_per_sample <= 2, bytes_per_sample
            obs.put_unsigned_value(bytes_per_sample, 8 * 1)

            # Maximum expected value
            max_expected_value = len(self.source.symbols) - 1
            assert 1 <= max_expected_value <= 2 ** 16 - 1, max_expected_value
            obs.put_unsigned_value(max_expected_value, 8 * 2)

            # Number of defined roots
            root_count = len(self.trees)
            assert 1 <= root_count <= 2 ** 16
            obs.put_unsigned_value(root_count - 1, 8 * 2)

            # Root entries
            for i, tree in enumerate(self.trees):
                enb.logger.info(f"Dumping {i}-th tree")

                root_entries = sorted(tree.root.all_children,
                                      # A good tree should produce homogeneously distributed words
                                      key=lambda node: node.raw_word_probability,
                                      reverse=True)
                root_included_entries = [n for n in root_entries
                                         if len(n.symbol_to_node) < len(self.source.symbols)]

                # Total number of entries in this tree
                root_entry_count = len(root_entries)
                obs.put_unsigned_value(root_entry_count, 8 * 4)

                # Total number of included in this tree
                root_included_count = len(root_included_entries)
                obs.put_unsigned_value(root_included_count, 8 * 4)

                # Tree entries (sorted by probability of having to access the node
                with enb.logger.info_context(f"Dumping nodes of {i}-th tree"):
                    for index, node in enumerate(root_entries):
                        if index % 5000 == 0:
                            enb.logger.info(f"Dumping node #{index}")

                        # Index
                        obs.put_unsigned_value(index, 8 * bytes_per_index)

                        # Number of children
                        obs.put_unsigned_value(len(node.symbol_to_node), 8 * 4)

                        # children indices
                        for symbol, child_node in sorted(node.symbol_to_node.items()):
                            obs.put_unsigned_value(root_entries.index(child_node), 8 * bytes_per_index)

                        # fields for included nodes
                        if len(node.symbol_to_node) < len(self.source.symbols):
                            # Sample count
                            word = node.word
                            obs.put_unsigned_value(len(word), 8 * 2)

                            # Sample bytes
                            for symbol in word:
                                obs.put_unsigned_value(int(symbol.label), 8 * bytes_per_sample)

                            # word
                            obs.put_unsigned_value(root_included_entries.index(node), 8 * bytes_per_word)

                # Root children count and indices
                with enb.logger.info_context("Dumping root node"):
                    obs.put_unsigned_value(len(tree.root.symbol_to_node), 8 * 4)

                    for i, (symbol, node) in enumerate(sorted(tree.root.symbol_to_node.items(),
                                                              key=lambda t: t[0].label)):
                        obs.put_unsigned_value(root_entries.index(node), 8 * bytes_per_index)
                        obs.put_unsigned_value(symbol.label, 8 * bytes_per_sample)

    def dump_pickle(self, output_path):
        """Dump a pickle of this forest.
        """
        assert output_path.split(".")[-1] == "zip"
        output_dir = f"tmp_dump_dir_pid{os.getpid()}"
        os.makedirs(output_dir)
        try:
            tree_node_list = [tree.included_nodes for tree in self.trees]
            next_tree_by_word_index = [self.trees[self.trees.index(self.node_to_next_tree[node])]
                                       for node in tree_node_list]

            pickle.dump(self.source, open(os.path.join(output_dir, "source.pickle"), "wb"))
            pickle.dump(tree_word_list, open(os.path.join(output_dir, "tree_word_list.pickle"), "wb"))
            pickle.dump(next_tree_by_word_index, open(os.path.join(output_dir, "next_tree_by_word_index.pickle"), "wb"))
            pickle.dump(self.params, open(os.path.join(output_dir, "params.pickle"), "wb"))
            shutil.make_archive(base_name=output_path.replace(".zip", ""), root_dir=output_dir, format="zip")
        finally:
            shutil.rmtree(output_dir)

    @staticmethod
    def load(input_path):
        input_dir = f"tmp_load_dir_PID{os.getpid()}"
        shutil.unpack_archive(filename=input_path, extract_dir=input_dir, format="zip")
        try:
            source = pickle.load(open(os.path.join(input_dir, "source.pickle"), "rb"))
            tree_word_list = pickle.load(open(os.path.join(input_dir, "tree_word_list.pickle"), "rb"))
            next_tree_by_word_index = pickle.load(open(os.path.join(input_dir, "next_tree_by_word_index.pickle"), "rb"))
            params = pickle.load(open(os.path.join(input_dir, "params.pickle"), "rb"))
            flat_word_list = list(itertools.chain(*tree_word_list))

            cls = params.pop("_cls")
            params["build"] = False
            forest = cls(**params)
            del params["build"]
            params["_cls"] = cls
            # forest.params = params
            forest.source = source
            forest.trees = []
            for tree_words in tree_word_list:
                forest.trees.append(TreeCodec(size=len(tree_words), source=source, build=False))
                forest.trees[-1].root = TreeNode(symbol=Symbol(label="root", p=0))
                for word in tree_words:
                    current_node = forest.trees[-1].root
                    for symbol in word:
                        try:
                            current_node = current_node.symbol_to_node[symbol]
                        except KeyError:
                            current_node = current_node.add_child(symbol)
                stw = sorted(tree_words, key=lambda word: str(word))
                siw_nodes = sorted(forest.trees[-1].included_nodes, key=lambda node: str(node.word))
                siw_words = [node.word for node in siw_nodes]

                assert len(stw) == len(siw_nodes), (len(stw), len(siw_nodes))
                for i, (t, w) in enumerate(zip(stw, siw_words)):
                    assert t == w, (i, t, w)
            forest.tree_sizes = [len(nodes) for nodes in tree_word_list]
            forest.current_tree = forest.trees[0]

            assert len(flat_word_list) == len(next_tree_by_word_index)
            forest.word_to_next_tree = {}
            for word, next_tree in zip(flat_word_list, next_tree_by_word_index):
                forest.word_to_next_tree[word] = next_tree

            # Give a chance to codecs to rebuild any needed internal structures
            returned_forest = forest.finish_loading(loaded_params=params)

            assert abs(sum(forest.source.probabilities) - 1) < 1e-10

            return returned_forest
        finally:
            shutil.rmtree(input_dir)

    @property
    def word_length_bits(self):
        return max(tree.word_length_bits for tree in self.trees)

    @property
    def params(self):
        try:
            return self._params
        except AttributeError:
            return {"tree_sizes": self.tree_sizes,
                    "is_raw": self.is_raw,
                    "source": self.source,
                    "_cls": self.__class__}

    @params.setter
    def params(self, new_params):
        self._params = new_params
        for k, v in new_params.items():
            self.__setattr__(k, v)

    @property
    def name(self):
        ignored_params = ["_cls"]
        param_parts = []
        for param_name, param_value in sorted(self.params.items()):
            if param_name == "source":
                param_parts.append(f"{param_name}-{param_value.name}")
            elif param_name in ignored_params:
                continue
            else:
                param_parts.append(f"{param_name}-{param_value}")
        return f"{self.__class__.__name__}" \
               + ("_" if any(k not in ignored_params for k in self.params.keys()) else "") \
               + "_".join(param_parts)

    def __repr__(self):
        return f"[{self.__class__.__name__} " \
               f"|F|={len(self.trees)} " \
               f"|T|={max(tree.size for tree in self.trees)}" \
               f" source={self.source}" \
               f"]"


class TreeWrapper(Forest):
    def __init__(self, tree, build=True, **kwargs):
        if build:
            self.is_raw = tree.is_raw
            self.source = tree.source
            self.trees = [tree]
            self.tree_sizes = [tree.size]
            self.word_to_next_tree = {node.word: tree for node in tree.included_nodes}

    @property
    def params(self):
        d = super().params
        d["tree"] = self.trees[0]
        return d

    def finish_loading(self, loaded_params):
        tree_params = dict(loaded_params["tree"].params)
        cls = tree_params.pop("_cls")
        tree = cls(build=False, **(tree_params))
        tree.root = self.trees[0].root
        return tree.finish_loading(loaded_params=loaded_params)


class TrivialForest(Forest):
    def build(self):
        for size in self.tree_sizes:
            self.trees.append(TrivialTree(size=size, source=self.source))

        for tree in self.trees:
            for i, node in enumerate(tree.included_nodes):
                self.node_to_next_tree[node] = self.trees[i % len(self.trees)]

        self.current_tree = self.trees[0]


class TunstallForest(Forest):
    def __init__(self, max_tree_size, tree_count, source, build=True, **kwargs):
        assert len(source.symbols) > 1
        assert max_tree_size >= len(source.symbols)
        assert tree_count >= 1
        super().__init__(tree_sizes=[max_tree_size] * tree_count, source=source, build=build, **kwargs)

    def build(self):
        for size in self.tree_sizes:
            self.trees.append(TunstallTree(size=size, source=self.source))

        for tree in self.trees:
            for i, node in enumerate(tree.included_nodes):
                self.node_to_next_tree[node] = self.trees[min(len(node.symbol_to_node), len(self.trees) - 1)]

        self.current_tree = self.trees[0]


class YamamotoForest(Forest):
    """A Yamamoto forest as described in their paper - with exactly |A|-1 trees"""

    def __init__(self, max_included_size, source: Source, tree_count=None,
                 yamamoto_tree_class=YamamotoTree,
                 **kwargs):
        """
        Create a Yamamoto forest with at most max_included_size nodes.

        :param: max_included_size: maximum number of included nodes in each tree.
          Determines the output codeword size. The minimum number of symbols needed
          to create a valid forest for the input source is always created.
        :param: source data source model to use to design the trees.
        :param: tree_count number of trees that will be created.
          The i-th tree is optimized so as not to expect the i-th input symbol.
          If None, the maximum number of trees is created, i.e., one per symbol
          in the source.
          Else, it must be a positive integer smaller than the number of symbols
          in the source.
        """
        tree_count = len(source.symbols) - 1 if tree_count is None else tree_count
        assert tree_count >= 1
        if tree_count > len(source.symbols) - 1:
            raise ValueError(f"Requesting {tree_count} trees for a source with {len(source.symbols)} symbols. "
                             f"At most {len(source.symbols) - 1} trees can be requested.")
        tree_count = min(tree_count, len(source.symbols) - 1)

        tree_included_sizes = [max_included_size] * tree_count
        self.size = max_included_size
        self.yamamoto_tree_class = yamamoto_tree_class
        super().__init__(tree_sizes=tree_included_sizes, source=source, **kwargs)

    def build(self):
        for i, size in enumerate(self.tree_sizes):
            with enb.logger.info_context(f"Building yamamoto tree #{i} "
                                         f"with {len(self.source.symbols)} symbols "
                                         f"and size {size} "
                                         f"@ {datetime.datetime.now()}"):
                self.trees.append(self.yamamoto_tree_class(size=size, source=self.source, first_allowed_symbol_index=i))

        for node in self.included_nodes:
            self.node_to_next_tree[node] = self.trees[min(len(self.trees) - 1, len(node.symbol_to_node))]
        self.current_tree = self.trees[0]

    @property
    def params(self):
        try:
            return self._params
        except AttributeError:
            return {"size": self.size,
                    "source": self.source,
                    "is_raw": self.is_raw,
                    "_cls": self.__class__}

    @params.setter
    def params(self, new_params):
        self._params = new_params
        for k, v in new_params.items():
            self.__setattr__(k, v)


class FastYamamotoForest(YamamotoForest):

    def __init__(self, max_included_size, source: Source, tree_count=None, **kwargs):
        super().__init__(max_included_size=max_included_size,
                         source=source,
                         tree_count=tree_count,
                         yamamoto_tree_class=FastYamamotoTree,
                         **kwargs)


class MarlinForestMarkov(Forest):
    """Proposed Markov forest"""

    def __init__(self, K, O, S, source, symbol_p_threshold=0,
                 original_source=None, build=True, is_raw=False,
                 **kwargs):
        """
        :param K: trees have size 2**K
        :param O: 2**O trees are created
        :param S: shift parameter
        :param source: source model for which the dictionary is to be optimized
        :param symbol_p_threshold: less probably symbols are dropped until the total probability of dropped symbols
          reaches but does not exceed symbol_p_threshold. Dropped symbols are output raw separately. symbol_p_threshold=0
          means that no symbol is coded raw
        """

        self.K = K
        assert 2 ** self.K >= len(source.symbols)
        self.O = O
        assert self.O >= 0
        self.S = S
        assert S >= 0
        assert self.K > self.O  # To meet the required number of transitions between trees

        self.is_raw = is_raw
        self.original_source = Source(symbols=source.symbols) if original_source is None \
            else Source(symbols=list(original_source.symbols))
        self.symbol_p_threshold = symbol_p_threshold
        assert 0 <= self.symbol_p_threshold <= 1

        full_meta_source, \
        self.metasymbol_to_symbols, \
        self.symbol_to_metasymbol_remainder = \
            Source.get_meta(source=source, symbols_per_meta=2 ** self.S)
        meta_symbols = full_meta_source.symbols

        assert abs(sum(s.p for s in meta_symbols) + - 1) < 1e-10, sum(s.p for s in meta_symbols)

        # Meta-symbols are filtered based on the probability threshold
        selected_symbols = []
        selected_p_sum = 0
        while selected_p_sum < (1 - symbol_p_threshold) \
                and len(selected_symbols) < len(meta_symbols):
            selected_symbols = meta_symbols[:len(selected_symbols) + 1]
            selected_p_sum = sum(ss.p for ss in selected_symbols)
        source = Source([Symbol(label=ss.label, p=ss.p / selected_p_sum)
                         for ss in selected_symbols])

        selected_symbol_ratio = len(source.symbols) / len(meta_symbols)
        auxiliary_root = TreeNode(symbol=None, parent=None)
        self.symbol_to_auxiliary_node = {
            non_selected_symbol: auxiliary_root.add_child(symbol=non_selected_symbol)
            for non_selected_symbol in meta_symbols[len(selected_symbols):]}
        super().__init__(tree_sizes=[2 ** K] * (2 ** O), source=source, build=False, **kwargs)

        if build:
            self.build()

        assert abs(sum(self.source.probabilities) - 1) < 1e-10

    def finish_loading(self, loaded_params):
        self.is_raw = (len(self.source) == 1)
        assert 0 <= self.symbol_p_threshold <= 1
        assert self.S >= 0

        meta_source, \
        self.metasymbol_to_symbols, \
        self.symbol_to_metasymbol_remainder = \
            Source.get_meta(source=self.original_source,
                            symbols_per_meta=2 ** self.S)
        meta_symbols = meta_source.symbols

        assert len(meta_symbols) == math.ceil(len(self.original_source) / 2 ** self.S)

        selected_symbols = []
        selected_p_sum = 0
        while selected_p_sum < (1 - self.symbol_p_threshold) \
                and len(selected_symbols) < len(meta_symbols):
            selected_symbols = meta_symbols[:len(selected_symbols) + 1]
            selected_p_sum = sum(ss.p for ss in selected_symbols)

        auxiliary_root = TreeNode(symbol=None, parent=None)
        self.symbol_to_auxiliary_node = {
            non_selected_symbol: auxiliary_root.add_child(symbol=non_selected_symbol)
            for non_selected_symbol in meta_symbols[len(selected_symbols):]}

        for symbol in self.original_source.symbols:
            meta_symbol, remainder = self.symbol_to_metasymbol_remainder[symbol]
            recovered_symbol = self.metasymbol_to_symbols[meta_symbol][remainder]
            assert recovered_symbol is symbol

        self.source = meta_source
        self.source.symbols = self.source.symbols[:len(selected_symbols)]
        source_psum = sum(self.source.probabilities)
        for s in self.source.symbols:
            s.p /= source_psum
        #
        return self

    def tree_index_to_tmatrix_index(self, tree, index):
        assert tree in self.trees
        assert 0 <= index < (len(self.source.symbols) - 1), (index, len(self.source.symbols))
        # states_per_tree * tree_index + index_in_tree
        return (len(self.source.symbols) - 1) * self.trees.index(tree) + index

    def tmatrix_index_to_tree_index(self, tmatrix_index):
        tree = self.trees[tmatrix_index // (len(self.source.symbols) - 1)]
        state_index = tmatrix_index % (len(self.source.symbols) - 1)
        return tree, state_index

    def build(self, max_iterations=50):
        # Initial tree building and linking
        for size in self.tree_sizes:
            if options.verbose > 2:
                sys.stdout.write(",")
                sys.stdout.flush()
            self.trees.append(MarlinTreeMarkov(size=size if len(self.source) > 1 else 1,
                                               source=self.source))
        if len(self.source) == 1:
            self.is_raw = True
            self.trees = self.trees[:1]
            self.node_to_next_tree = {node: self.trees[0] for node in self.trees[0].included_nodes}
            self.current_tree = self.trees[0]
        else:
            self.node_to_next_tree = self.get_node_to_next_tree()
            states_per_tree = len(self.source.symbols) - 1
            state_count = states_per_tree * 2 ** self.O
            state_probabilities = np.zeros(state_count)
            state_probabilities[:] = 1 / state_count
            produced_word_lists = []

            assert len(self.source) > 1
            min_delta = float("inf")
            for iteration_index in range(max_iterations):
                # Transition matrix
                tmatrix = np.zeros((state_count, state_count))
                if options.verbose > 2:
                    sys.stdout.write(".")
                    sys.stdout.flush()
                for from_tree in self.trees:
                    for from_index in range(states_per_tree):
                        row = self.tree_index_to_tmatrix_index(tree=from_tree, index=from_index)
                        for node in from_tree.included_nodes:
                            to_tree = self.node_to_next_tree[node]
                            to_index = min(state_count - 1, len(node.symbol_to_node))
                            col = self.tree_index_to_tmatrix_index(tree=to_tree, index=to_index)

                            tmatrix[row, col] += from_tree.node_conditional_probability(
                                node=node, state_index=from_index, state_tree=from_tree)
                for row in range(state_count):
                    s = tmatrix[row, :].sum()
                    if s > 0:
                        tmatrix[row, :] /= s
                    else:
                        tmatrix[row, :] = 1 / tmatrix.shape[1]

                # State probability finish_loading
                old_state_probabilities = np.array(state_probabilities)
                while True:
                    for j in range(tmatrix.shape[1]):
                        state_probabilities[j] = 0
                        for transition_p, state_p in zip(tmatrix[:, j], old_state_probabilities):
                            state_probabilities[
                                j] += state_p * transition_p  # P(transition) = sum_state P(transition|state)
                    delta = sum(abs(old - new) for old, new in zip(old_state_probabilities, state_probabilities))
                    min_delta = min(delta, min_delta)
                    if delta < 5e-7:
                        break
                    old_state_probabilities = np.array(state_probabilities)
                mse = 0
                for i, state_p in enumerate(state_probabilities):
                    tree, state_index = self.tmatrix_index_to_tree_index(i)
                    mse += np.abs(np.array(state_p)
                                  - np.array(tree.state_probabilities[state_index])).sum() ** 2
                    tree.state_probabilities[state_index] = state_p
                mse /= state_count

                # Update tree
                all_equal = True
                for tree in self.trees:
                    previous_words = sorted((node.word for node in tree.included_nodes),
                                            key=lambda w: str(w))
                    tree.root = TreeNode(Symbol("root", 0), parent=None)
                    if options.verbose > 2:
                        sys.stdout.write("-")
                        sys.stdout.flush()
                    tree.build(update_state_probabilities=False)
                    new_words = sorted((node.word for node in tree.included_nodes),
                                       key=lambda w: str(w))
                    if previous_words != new_words:
                        all_equal = False

                self.node_to_next_tree = self.get_node_to_next_tree()

                new_words = sorted((node.word for node in self.included_nodes),
                                   key=lambda word: str(word))

                tolerance = 1e-6
                if all_equal \
                        or new_words in produced_word_lists \
                        or mse < tolerance:
                    break
                else:
                    produced_word_lists.append(new_words)

        self.current_tree = self.trees[0]

    def get_node_to_next_tree(self):
        """Implementation of the DefineTransitions routine described in the paper.
        Determines which words produce transitions to what tree.
        """
        node_to_next_tree = {}
        for tree in self.trees:
            nodes_by_metastate_index = {i: [] for i in range(max(1, len(self.source.symbols) - 1))}
            for node in tree.included_nodes:
                nodes_by_metastate_index[max(0, min(len(self.source.symbols) - 2, len(node.symbol_to_node)))].append(
                    node)
            nodes_by_metastate_index = {i: sorted(nodes, key=lambda node: -tree.node_probability(node))
                                        for i, nodes in nodes_by_metastate_index.items()}

            sorted_nodes = list(
                itertools.chain(*(nodes_by_metastate_index[i] for i in range(len(nodes_by_metastate_index)))))
            assert len(self.trees) == 2 ** self.O
            for i, tree in enumerate(self.trees):
                assert len(tree.included_nodes) == 2 ** self.K
                for node in sorted_nodes[i * 2 ** (self.K - self.O):(i + 1) * 2 ** (self.K - self.O)]:
                    node_to_next_tree[node] = tree

        for auxiliary_node in self.symbol_to_auxiliary_node.values():
            node_to_next_tree[auxiliary_node] = self.trees[0]

        return node_to_next_tree

    def code(self, input):
        if self.is_raw:
            return self.code_raw(input=input)

        if len(self.source) > 1:
            print(f"[watch] A  time.process_time()={time.process_time()}")

            input, remainders = zip(*(self.symbol_to_metasymbol_remainder[symbol] for symbol in input))

            print(f"[watch] B  time.process_time()={time.process_time()}")

            excluded_index_symbol = [(index, symbol) for index, symbol in enumerate(input)
                                     if symbol not in self.source.symbols]

            print(f"[watch] C  time.process_time()={time.process_time()}")

            excluded_indices = set(index for index, symbol in excluded_index_symbol)

            filtered_input = [symbol for i, symbol in enumerate(input)
                              if i not in excluded_indices]

            print(f"[watch] D  time.process_time()={time.process_time()}")

            coded_dict = super().code(filtered_input)

            print(f"[watch] E time.process_time()={time.process_time()}")

            coded_nodes = coded_dict["coded_data_nodes"]

            proper_word_length = len(coded_nodes) * self.word_length_bits
            excluded_word_length = len(excluded_index_symbol) \
                                   * (self.word_length_bits + math.ceil(math.log2(len(input))))
            raw_data_length = len(remainders) * self.S

            coded_dict = {
                "coded_data_nodes": (coded_nodes, excluded_index_symbol, remainders),
                "coded_lenght_bits": proper_word_length + excluded_word_length + raw_data_length
            }
        else:
            assert self.is_raw
            coded_dict = {
                "coded_data_nodes": ([self.trees[0].root.symbol_to_node[self.source.symbols[0]]] * len(input),
                                     [],
                                     [0] * len(input),),
                "coded_lenght_bits": len(input) * math.ceil(math.log2(len(self.original_source)))
            }
        return coded_dict

    def decode(self, coded_dict):
        if self.is_raw:
            return self.decode_raw(coded_output=coded_dict)

        coded_nodes, excluded_index_symbol, remainders = coded_dict["coded_data_nodes"]
        decoded_symbols = list(itertools.chain(*(node.word for node in coded_nodes)))

        for index, symbol in excluded_index_symbol:
            decoded_symbols.insert(index, symbol)

        assert len(decoded_symbols) >= len(remainders), (len(decoded_symbols), len(remainders))
        decoded_symbols = [self.metasymbol_to_symbols[metasymbol][remainder]
                           for metasymbol, remainder in zip(decoded_symbols, remainders)]

        return decoded_symbols

    @property
    def params(self):
        try:
            return self._params
        except AttributeError:
            return {"K": self.K,
                    "O": self.O,
                    "S": self.S,
                    "symbol_p_threshold": self.symbol_p_threshold,
                    "source": self.source,
                    "original_source": self.original_source,
                    "is_raw": self.is_raw,
                    "_cls": self.__class__}

    @params.setter
    def params(self, new_params):
        for k, v in new_params.items():
            self.__setattr__(k, v)

    @property
    def name(self):
        param_parts = []
        for param_name, param_value in sorted(self.params.items()):
            if param_name in ["source", "original_source"]:
                param_parts.append(f"{param_name}-{param_value.name}")
            elif param_name in ["metasymbol_to_symbols",
                                "_cls",
                                "symbol_to_auxiliary_node",
                                "symbol_to_metasymbol_remainder"]:
                continue
            else:
                param_parts.append(f"{param_name}-{param_value}")
        return f"{self.__class__.__name__}" \
               + ("_" if self.params else "") \
               + "_".join(param_parts)

    def __str__(self):
        return f"[MarlinForestMarkov(K={self.K}, O={self.O}, S={self.S}, source={self.source}, symbol_p_trheshold={self.symbol_p_threshold}]"


if __name__ == '__main__':
    print("Non executable module")
