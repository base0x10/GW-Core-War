#! /usr/bin/env python

import sys

total = 0
for line in open (sys.argv[1]):
    cols = line.split ()
    if len (cols) == 10:
        total = total + float (cols[8])
print total
