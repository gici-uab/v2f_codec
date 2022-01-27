#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Checks line length of all .c and .h source files.
"""

import os
import re
import sys

_, _, filenames1 = next(os.walk('bin'), (None, None, []))
_, _, filenames2 = next(os.walk('fuzzing'), (None, None, []))
_, _, filenames3 = next(os.walk('src'), (None, None, []))
_, _, filenames4 = next(os.walk('test'), (None, None, []))

filenames = [os.path.join('bin', file) for file in filenames1] \
    + [os.path.join('fuzzing', file) for file in filenames2] \
    + [os.path.join('src', file) for file in filenames3] \
    + [os.path.join('test', file) for file in filenames4]

for file in filenames:
    if not re.fullmatch('.*\.c', file) and not re.fullmatch('.*\.h', file)\
            and not re.fullmatch('.*\.md', file):
        continue

    if file == "test/test_images.c":
        continue

    with open(file) as f:
        for i, line in enumerate(f.readlines()):
            if len(line.replace("\t", "xXxX")) > 120:  # including \r
                print("{}:{}: excessive line length".format(file, i + 1), file=sys.stderr)

            if len(line) > 0 and line[-1] == " ":
                print("{}:{}: line ends in space".format(file, i + 1), file=sys.stderr)
