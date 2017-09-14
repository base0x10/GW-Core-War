#! /usr/bin/env python

import commands, math, optparse, os, sys


usage      = "%prog [options] warrior1 warrior2 ..."
def_rounds = 2000
def_mars   = "./xfmars"
def_fnum   = 4000


def fight (war1, war2, rounds, mars, fnumber):
    cmd = "time -p %s %s %s -r %d -bkF %d" % (mars, war1, war2, rounds, fnumber)
    out = commands.getoutput (cmd)
    try:
        output = [line.split () for line in out.split ("\n")]
        w1wins = int (output[0][0])
        w2wins = int (output[1][0])
        ties = int (output[0][1])
        realtime = float (output[3][1])
    except:
        print "output:", out
        raise
    return w1wins, w2wins, ties, realtime


def main ():
    parser = optparse.OptionParser (usage = usage)
    parser.add_option ("-r", type = "int", dest = "rounds",
		       default = def_rounds, metavar = " <rounds>",
		       help = "number of rounds to fight")
    parser.add_option ("-m", dest = "mars", default = def_mars,
		       metavar = " <simulator>",
		       help = "use specified simulator")
    parser.add_option ("-F", type = "int", dest = "fnumber",
		       default = def_fnum, metavar = " <position>",
		       help = "fixed position of second warrior")

    options, warriors = parser.parse_args ()
    rounds = options.rounds
    mars = options.mars
    fnumber = options.fnumber
	
    if not warriors:
	parser.error ("at least one warrior required")

    reslen = int (math.log10 (rounds)) + 1
    maxlen = max ([len (name) for name in warriors])
    totaltime = 0.0
    separator = (2 * maxlen + 3 * reslen + 28) * "-"

    print "using %s, %d battle(s), %d rounds each" % (os.path.basename (mars),
				    len (warriors) * (len (warriors) + 1) / 2,
				    rounds)
    print separator

    while warriors:
	war1 = warriors[0]
	for war2 in warriors:
	    print "%-*s vs. %*s  -- " % (maxlen, war1, maxlen, war2),
	    sys.stdout.flush ()
	    w1wins, w2wins, ties, realtime = fight (war1, war2, rounds,
						    mars, fnumber)
	    print "%*d %*d %*d  --  %5.2f sec" % (reslen, w1wins, reslen,
						  w2wins, reslen, ties,
						  realtime)
	    totaltime = totaltime + realtime
	del warriors[0]

    print separator
    print "total time  --  %.2f sec" % totaltime


if __name__ == "__main__":
    main ()
