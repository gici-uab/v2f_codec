#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate the requirements traceability matrix.
"""
__author__ = "Ian Blanes <ian.blanes@uab.cat>"
__date__ = "09/07/2018"

#######################################################################################################################
#
# How to use this file...
#
# 1) Make sure all your requirement ids are in actual_requirements (i.e., double check these lists).
#
# 2) Whenever a test case covers a requirement id, add "@req id" to the doxygen comment of that test case.
#
# Notes:
#  - If two or more requirements are covered by the same test, use "@req id1, id2, id3...".
#  - A prefix id in a test case covers all sub-ids (e.g., @req 3, would cover 3.1, 3.2, 3-a, etc).
#  - The actual requirement name is prefixed by a namespace id in "actual_requirements = ...".
#
#######################################################################################################################

main_requirements = [
    # ("1", "Lossless compression regime"),

    ("1.1", "Decorrelation"),
    ("1.1.1", "Bypass decorrelation"),
    ("1.1.2", "Left neighbor prediction"),

    ("1.2", "Entropy coding and decoding"),
    ("1.2.1", "Entropy coding"),
    ("1.2.2", "Entropy decoding"),

    ("1.3", "Lossless compressor"),
    ("1.4", "Lossless decompressor"),


    # ("2", "Lossy compression regime"),

    ("2.1", "Quantization and dequantization"),
    ("2.1.1", "Quantization"),
    ("2.1.2", "Dequantization"),

    # ("3", "Testing"),
    ("3.1", "Unitary tests"),
]

#############################################################################

actual_requirements = \
    ["V2F-{}".format(r[0])
     if isinstance(r, tuple) else "V2F-{}".format(r)
     for r in main_requirements]
    # + ["V2F-sd-{}".format(r[0])
    #    if isinstance(r, tuple) else "V2F-sd-{}".format(r)
    #    for r in specification_document_requirements]

import os
import re
import sys
import datetime

# requirements tested in .c files

_, _, filenames1 = next(os.walk('test'), (None, None, []))
_, _, filenames2 = next(os.walk('fuzzing'), (None, None, []))

filenames = \
    [os.path.join('test', file) for file in filenames1] \
    + [os.path.join('fuzzing', file) for file in filenames2]

doxygen_comment = '(' + re.escape('/**') + '(?:[^*]|[*][^/])* ' + re.escape(
    '*/') + ')[ \t\n\r]*?(?:[a-zA-Z0-9_]+[ \t\r\n]+)*([a-zA-Z0-9_]+)[ \t\r\n]*\('

req_dict = {}

for file in filenames:
    if not re.fullmatch('.*\.c', file):
        continue

    with open(file) as f:
        content = f.read()

        for match in re.finditer(doxygen_comment, content, re.DOTALL):
            if '#' in match.group(0):
                continue

            comment = match.group(1)
            function = match.group(2)

            match2 = re.search('@req\s(.*)$', comment, re.MULTILINE)

            if match2 is not None:
                reqs = match2.group(1).replace(' ', '').split(',')

                for r in reqs:
                    try:
                        req_dict[r].append((file, function))
                    except KeyError:
                        req_dict[r] = [(file, function)]

# requirements tested in .sh files

_, _, filenames3 = next(os.walk('crossvalidation'), (None, None, []))

filenames = [os.path.join('crossvalidation', file) for file in filenames3]

for file in filenames:
    if not any(
            re.fullmatch(".*\.{}".format(ext), file) for ext in ["sh", "py"]):
        continue

    with open(file) as f:
        content = f.readlines()

        for number, line in enumerate(content):
            match = re.match('^.*#[^#]*@req\s(.*)$', line)

            if match is not None:
                reqs = match.group(1).replace(' ', '').split(',')

                for r in reqs:
                    try:
                        req_dict[r].append((file, number + 1))
                    except KeyError:
                        req_dict[r] = [(file, number + 1)]

unaddressed_reqs = []

print("""---
sourcefile: build_rtm.py
title: Requirements Traceability Matrix
revisions: # older versions first
\t- version: 1.0
      date: """ + datetime.date.today().strftime('%d %b %Y') + """
      changes: (initial version)
authors:
\t- (automatically generated)
---

# OVERVIEW

This document contains a Requirements Traceability Matrix associating 
each requirement with a test case.

# REQUIREMENTS TRACEABILITY MATRIX

| Requirement | Test Case(s) |
|------------|--------------------------------------------------------|""")

for oreq in sorted(actual_requirements):
    # try simplifying requirement
    req = oreq

    while req not in req_dict and ('-' in req or '.' in req):
        req = req[0:max(req.rfind('-'), req.rfind('.'))]

    if req not in req_dict:
        unaddressed_reqs.append(oreq)
        continue

    # found
    fmt = "{}:{}"

    if req != oreq:
        fmt += ' (tests ' + req + ')'

    print('|' + oreq + '|'
          + ", ".join(fmt.format(*t) for t in req_dict[req])
          + '|')

for req in unaddressed_reqs:
    print("Warning: unaddressed requirement " + req, file=sys.stderr)

for k in req_dict.keys():
    if k not in actual_requirements:
        print("Warning: unused requirement name {} in {}".format(
            k,
            ", ".join("{}:{}".format(*t) for t in req_dict[k])),
            file=sys.stderr)
