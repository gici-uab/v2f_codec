#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate show usage strings from user manual.
"""
__author__ = "Ian Blanes <ian.blanes@uab.cat>"
__date__ = "15/02/2018"

import os.path
import sys

if len(sys.argv) < 2:
    raise "Missing output file."

output_file = sys.argv[1]

search_pattern = output_file.split('/')[-1][:-2]

with open('docs/user_manual.md') as f:
    for line in f:
        if line.startswith('## ' + search_pattern):
            break

    for line in f:
        if line.startswith('## '):
            break
        else:
            if line.startswith('### '):
                line = line.upper()
            print(line.rstrip())

print('## AUTHOR INFORMATION')
