#! /usr/bin/env python

# Autotest allows easy testing of number of fmars configurations.
# The format of configuration file is:
#
# [configurations]
# (list of configuration files which will be copied over fmars.cfg)
# [benchmark]
# (list of warriors)
# [options]
# (extra options to pass to bench.py)
#
# Autotest will run the reference mars first, then will try to build
# and test them.

import re, sys, shutil, optparse, os
from commands import getstatusoutput


usage       = "%prog [options] test1 test2 ..."
def_checker = "./exmars"
def_res_dir = "atresults"
def_cfg_dir = "atconfigs"


def read_config (test_file):
    configs, options, benchmark = [], [], []
    tgt_list = \
    {
	"[configurations]": configs,
	"[options]": options,
	"[benchmark]": benchmark
    }

    for line in open (test_file, "r"):
	line = line.strip ()
	if not line or line.startswith ("#"):
	    continue
	elif line in tgt_list:
	    curlist = tgt_list[line]
	    continue
	curlist.append (line)

    return configs, options, benchmark


pat = re.compile ("(\d+)\s+(\d+)\s+(\d+)")

def results (output):
    ret = []
    for m in pat.finditer (output):
        if m:
            ret.extend (m.group (1, 2, 3))
    return ret

def config_run (test_file, config, options, benchmark, reference, resdir):
    sconf = os.path.basename (config)
    shutil.copyfile (config, "smars.cfg")
    status, output = getstatusoutput ("make clean")
    
    status, output = getstatusoutput ("make INSN_FILES=\"%s\"" \
				      % " ".join (benchmark))
    if status:
        err_file_name = resdir + "/" + sconf + "--" + test_file + \
                        "--builderror"
        open (err_file_name, "w").write (output)
        print "!! build error - details in " + err_file_name
        return
    else:
        print "build succeded"
        
    status, output = getstatusoutput ("python bench.py %s" \
				      % " ".join (options + benchmark))
    if status or results (output) <> reference:
        err_file_name = resdir + "/" + sconf + "--" + test_file + \
                        "--badoutput"
        open (err_file_name, "w").write (output)
        print "!! bad output - details in " + err_file_name
    else:
        file_name = resdir + "/" + sconf + "--" + test_file + "--ok"
        open (file_name, "w").write (output)
        print "good output - details in " + file_name
        


def main ():
    parser = optparse.OptionParser (usage = usage)
    parser.add_option ("-c", dest = "checker", default = def_checker,
		       metavar = " <simulator>",
		       help = "mars to check against")
    parser.add_option ("-r", dest = "resdir", default = def_res_dir,
		       metavar = " <directory>",
		       help = "directory to hold results of simulations")
    parser.add_option ("-g", dest = "cfgdir", default = def_cfg_dir,
		       metavar = " <directory>",
		       help = "directory where config files are held")

    opt, tests = parser.parse_args ()
    checker = opt.checker
    resdir = opt.resdir
    cfgdir = opt.cfgdir

    for t in tests:
	configs, options, benchmark = read_config (t)
	checker_opts = [opt for opt in options if opt.split ()[0] != "-m"]
	cmdline = "python bench.py -m %s %s" \
		  % (checker, " ".join (checker_opts + benchmark))
        print "reference commandline:"
        print cmdline
	status, output = getstatusoutput (cmdline)
        file_name = resdir + "/" + os.path.basename (t) + "--reference"
        open (file_name, "w").write (output)
        print "reference ouput in " + file_name

	for conf in configs:
            config_run (os.path.basename (t), cfgdir + "/" + conf, options,
                        benchmark, results (output), resdir)


if __name__ == "__main__":
    main ()
