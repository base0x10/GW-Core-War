#! /usr/bin/env python


######

import fmars, sys

# optional arguments/default values:
# Mars (coresize = 8000, processes = 8000, cycles = 80000, maxlength = 100)
mars = fmars.Mars ()

# read warriors to strings
w1code = open (sys.argv[1], "r").read ()
w2code = open (sys.argv[2], "r").read ()

w1 = fmars.Warrior (mars, w1code)
w2 = fmars.Warrior (mars, w2code)

w1wins, w2wins, ties = mars.fightn (w1, w2, 2000, 4000) # rounds = 2000, fnumber = 4000
print w1wins, ties
print w2wins, ties

sys.exit ()

#### or using custom positioning algorithm:

avail = 8000 + 1 - 2 * 100
seed = 4000 - 100
def rng (seed):
   temp = seed
   temp = 16807 * (temp % 127773) - 2836 * (temp / 127773)
   if temp > 2147483648:
      temp = temp - 2147483648
   elif temp < 0:
      temp = temp + 2147483647
   return temp

w1wins = w2wins = ties = 0
for round in xrange (2000):
   pos2 = 100 + seed % avail
   seed = rng (seed)
   # position of warrior2 = pos2
   # the warrior who starts = round % 2
   # (0 --> w1 starts, 1 --> w2 starts)
   res = mars.fight (w1, w2, pos2, round % 2)
   if res == 0:
      ties += 1
   elif res == 1: # w1 won
      w1wins += 1 
   else:
      w2wins += 1

print w1wins, ties
print w2wins, ties

