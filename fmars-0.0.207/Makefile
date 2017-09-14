#this makefile is broken - (dependencies are wrong)
#use make clean after making changes to .scm files

INDENT = cat
#INDENT = indent


CC = gcc
CFLAGS = -O2 -fomit-frame-pointer -fpic -fno-strict-aliasing #-march=athlon-xp
#DEBUG = -g -fno-omit-frame-pointer -O0

#options for fm_sim.c only:
#no-crossjumping - gcc 3.3 breaks computed gotos, this seems to help a little
OPTS = -fno-cse-follow-jumps -fno-crossjumping
#OPTS += -fno-strict-aliasing

#CFLAGS = $(CFLAGS_OPT) -fverbose-asm -W -Wimplicit -Wparentheses -Wpointer-arith \
	  -Wreturn-type -Wsequence-point -Wstrict-aliasing -Wswitch \
	  -Wuninitialized -Wunused-label -Wunused-value
#CFLAGS += -Wshadow -Wcast-align -Wconversion -Wall -Wno-parentheses

ifndef INSN_FILES
INSN_FILES = fsh.set
endif

.PHONY: clean package

xfmars: exhaust.o fmars.o fm_sim.o fm_asm.o
	$(CC) $(CFLAGS) -o xfmars exhaust.o fmars.o fm_sim.o fm_asm.o $(DEBUG)

fmars.o: fmars.c fm_types.h
	$(CC) $(CFLAGS) -c fmars.c $(DEBUG)


exhaust.o: exhaust.c fm_types.h
	$(CC) $(CFLAGS) -Wno-sign-compare -c exhaust.c $(DEBUG)


fm_sim.o: fm_sim.s
	$(CC) $(CFLAGS) -c fm_sim.s

fm_sim.s: fm_sim.c fm_types.h
	$(CC) $(CFLAGS) $(OPTS) -fverbose-asm -Wuninitialized -S fm_sim.c

fm_sim.c: fm_sim_tmp.c
	guile -l skel.scm -c '(display includes/defines)' | $(INDENT) > fm_sim.c
	gcc -E fm_sim_tmp.c | grep -v "^$$\|^#" | $(INDENT) >> fm_sim.c

fm_sim_tmp.c:
	cat $(INSN_FILES) | guile -s code.scm | $(INDENT) > fm_sim_tmp.c


fm_types.h: header.scm
	guile -s header.scm | $(INDENT) > fm_types.h

clean:
	rm -f *~ *.o core fm_sim* fm_types.h xfmars fmars_wrap.c* *.so *.pyc fmars.py fmars.pm

parser: fm_asm.o

fmars.py: _fmars.so
_fmars.so: fmars.i fm_asm.o fm_types.h fmars.o fm_sim.o
	swig -c++ -python -shadow fmars.i
	g++ $(CFLAGS) -c fmars_wrap.cxx -I/usr/include/python2.3 -I/usr/include/python2.4
	g++ $(CFLAGS) -shared fm_asm.o fmars_wrap.o fmars.o fm_sim.o -o _fmars.so

#fmars.pm: fmars.so
#fmars.so: fmars.i fm_asm.o fm_types.h fmars.o fm_sim.o
#	swig -c++ -perl5 -shadow fmars.i
#	g++ $(CFLAGS) -c fmars_wrap.cxx -I/usr/lib/perl/5.8/CORE
#	g++ $(CFLAGS) -shared fm_asm.o fmars_wrap.o fmars.o fm_sim.o -o fmars.so

fm_asm.o: fm_asm.c
	$(CC) $(CFLAGS) -Wno-parentheses -c fm_asm.c $(DEBUG)
