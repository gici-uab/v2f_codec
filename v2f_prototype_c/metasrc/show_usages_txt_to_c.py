#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate show usage strings from user manual.
"""
__author__ = "Ian Blanes <ian.blanes@uab.cat>"
__date__ = "15/02/2018"

import sys

sys.stdout.write('static char const * const show_usage_string = ')

for line in sys.stdin:
    if line.startswith('-   '):
        line = '  - ' + line[4:]

    sys.stdout.write('\n   "{}\\n"'.format(line.rstrip()))

sys.stdout.write(';\n\n')
