#! /usr/bin/env python

import sys

filename1 = sys.argv[1]
filename2 = sys.argv[2]
file1 = open (filename1, "r")
file2 = open (filename2, "r")
namelen = max (len (filename1), len (filename2))
linenum = 0

while True:
    linenum = linenum + 1
    line1 = file1.readline ()
    line2 = file2.readline ()

    if line1 != line2:
	print "line %s:" % linenum
	print "%*s  --  %s" % (namelen, filename1, line1.rstrip ())
	print "%*s  --  %s" % (namelen, filename2, line2.rstrip ())
	break
    elif line1 == line2 == "":
	break
