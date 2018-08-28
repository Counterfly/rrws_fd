#! /usr/bin/env python2.7

"""Solve some tasks with A* and the LM-Cut heuristic."""

import sys
import os
import os.path
import platform
import pipes
import subprocess
from subprocess import Popen, PIPE
from multiprocessing import cpu_count, Pool, Process


import shlex

ATTRIBUTES = ['coverage', 'expansions']
#ATTRIBUTES = PlanningReport.ATTRIBUTES.keys()
ATTRIBUTES.append('cost')
ATTRIBUTES = ATTRIBUTES.sort()

REPO = os.getcwd()
BENCHMARKS_DIR = os.path.join(REPO , 'benchmarks')
SUITE = ['blocks', 'grid', 'gripper', 'pipesworld-tankage', 'rovers', 'schedule']
#SUITE = ['grid', 'blocks']

file_name = os.path.splitext(os.path.basename(__file__))[0]
#STORE_PATH = "./data/" +SUITE[0] + '/' + file_name + "4"
STORE_PATH = './data/thesis'
if os.path.exists(STORE_PATH):
  print "Directory " + STORE_PATH + " already exists"
  #sys.exit()


def run_fd(args):
  args = list(args)
  log_file = args.pop()
  args.insert(0, "./run_fd.sh")
  out, err = (None, None)
  with open(log_file, "w") as file_log:
    #return_code = subprocess.call(" ".join(program), shell=True, stdout=file_log)
    p = subprocess.Popen(args, stdout=file_log)
    out, err = p.communicate()
  #return_code = subprocess.call(" ".join(args), shell=True, stdout=file_log)
  print("return code (if '(None, None)' then successful!!: ", out, err)
  assert False, "STOP HERE"
  return out, err

#def run_fd(args):
#  args = list(args)  # Convert tuple to list
#  log_file = args.pop()
#  args.insert(0, "./run_fd.sh")
#  with open(os.devnull, 'w') as devnull:
#    with open(log_file, "w") as file_log:
#      #return_code = subprocess.call(["sleep", str(3)])
#      #args = [pipes.quote(x) for x in args]  # Only if passing string into subprocess.call
#      return_code = subprocess.call(args, stdout=file_log, stderr=devnull)
#  #print "==return code for %s = %s" % (log_file[-20:], return_code)

class FastDownwardExperiment(object):

  fast_downward_exec_file = "fast_downward.py"
  DEFAULT_SEARCH_TIME_LIMIT = '10m'
  DEFAULT_SEARCH_MEMORY_LIMIT = '2G'

  def __init__(self):
    self._suite = None
    self._algorithms = []
    #self._pool = Pool()	# Use all CPU cores
    self._pool = Pool(processes=8)	# Use 2 processes!! not cores.
    self._num_executions = 10

  def add_suite(self, benchmarks_path, benchmarks):
    # Should be arr
    self._suite = (benchmarks_path, benchmarks)

  def add_algorithm(self, name, path_to_fd, repo, args):
    self._algorithms.append((name, path_to_fd, args))

  def run_steps(self, store_path):
    processes = []
    process_results = []
    algorithm_names = [x[0].lower().strip() for x in self._algorithms]
    for algorithm in self._algorithms:
      algorithm_name = algorithm[0].lower().strip()
      print "Starting %s" % algorithm_name
      # Run all suite/benchmarks on algorithm
      print("run alg with ", algorithm, self._suite, store_path)
      process_results.extend(self.run_algorithm(algorithm, self._suite, store_path))
      print "Finished running %s" % algorithm_name
  
    count = 1
    for result in process_results:
      result.wait()
      # NOTE: as this is run, the workers (in Pool) will be running processes,
      #  so while the current result may not be done, subsequent results may be
      #  and will be reflected by quick, successive prints once the current result is done
      print("Process {}/{} finished with success = {}".format(str(count), str(len(process_results)), str(result.successful())))
      count += 1

    self._pool.close()
    self._pool.join()  # Unncessary if doing the above for-loop (printing successful or not)
    print "\n\n======\nFinished, Joined Pool"

  def run_algorithm(self, algorithm, suite, store_path):
    '''Runs the specified algorithm on the entire suite, and stores the logs under store_path root'''
    algorithm_name = algorithm[0].lower().strip()
    alg_store_path = store_path + '/' + algorithm_name
    if not os.path.exists(alg_store_path):
      os.makedirs(alg_store_path)
    return self.run_benchmarks_single_algorithm(suite, algorithm, alg_store_path)
    
  def run_benchmarks_single_algorithm(self, suite, algorithm, store_path):
    results = []
    for benchmark in suite[1]:
      benchmark_store_path = store_path + '/' + benchmark

      if os.path.exists(benchmark_store_path):
        #print("Directory " + benchmark_store_path + " already exists. NOT running %s on %s" % (algorithm, benchmark))
        #return []
	# continue to next benchmark
	#continue  # This is commented out in favour of checking each execution number (see line "if os.path.isfile(logfile)")
	pass

      benchmark_path = suite[0] + "/" + benchmark
      benchmarks = os.listdir(benchmark_path)
      benchmarks.sort()
      benchmark_instances = filter(lambda a: 'domain' not in a, benchmarks)
      for benchmark_instance in benchmark_instances:
        benchmark_instance_store_path = benchmark_store_path + "/" + benchmark_instance.replace('.pddl', '')
        #assert not os.path.exists(benchmark_instance_store_path), "Directory " + benchmark_instance_store_path + " already exists, exiting (should never reach here bc checking alg folder existence)"
	if not os.path.exists(benchmark_instance_store_path):
	  os.makedirs(benchmark_instance_store_path)

        benchmark_file = benchmark_path + "/" + benchmark_instance
        # Get all problem instances
        name = algorithm[0]
        exe = algorithm[1] + "/" + self.fast_downward_exec_file
        args = list(algorithm[2])

        for run in range(self._num_executions):
          log_file = benchmark_instance_store_path + "/run" + str(run) + "/output.log" 
	  if os.path.isfile(log_file) and os.stat(log_file).st_size != 0:
	    # Execution already run and log file is not empty
            print("Already ran " + str(benchmark) + ".execution#" + str(run) + " on " + str(algorithm))
	    continue

	  if not os.path.exists(os.path.dirname(log_file)):
            os.makedirs(os.path.dirname(log_file))
          #print "log_file = ", log_file

          program = [name]
          program.extend(['--search-time-limit', self.DEFAULT_SEARCH_TIME_LIMIT])
          program.extend(['--search-memory-limit', self.DEFAULT_SEARCH_MEMORY_LIMIT])
          program.append(benchmark_file)
          program.extend(args)
          #program.extend([">", log_file])
          program.append(log_file)
          #program = [pipes.quote(x) for x in program]  # Needed if pass a string to subprocess.call
          # run_fd takes one parameter which is a tuple, so convert array to tuple and place it as first element in a tuple
          results.append(self._pool.apply_async(run_fd, (tuple(program),)))
      
    return results


  def run_benchmarks(self, suite, algorithms, store_path):
    run = 0
    for benchmark in suite[1]:
      benchmark_path = suite[0] + "/" + benchmark
      benchmarks = os.listdir(benchmark_path)
      benchmarks.sort()
      benchmarks = filter(lambda a: 'domain' not in a, benchmarks)
      benchmarks = [benchmark_path + "/" + x for x in benchmarks]
      for benchmark_file in benchmarks:
        # Get all problem instances
        for algorithm in algorithms:
          name = algorithm[0]
          exe = algorithm[1] + "/" + self.fast_downward_exec_file
          args = list(algorithm[2])
          log_file = store_path + "/run" + str(run) + "/output.log" 
          os.makedirs(os.path.dirname(log_file))
          run += 1

          program = ["./run_fd.sh", name]
          program.extend(['--search-time-limit', self.DEFAULT_SEARCH_TIME_LIMIT])
          program.extend(['--search-memory-limit', self.DEFAULT_SEARCH_MEMORY_LIMIT])
          program.append(benchmark_file)
          program.extend(args)
          #program.extend([">", log_file])
          program = [pipes.quote(x) for x in program]
          print "program = %s" % ' '.join(program)
          with open(log_file, "w") as file_log:
	    #proc = Popen(program, shell=True, stdin=None, stdout=file_log, stderr=None, close_fds=True)
	    #proc.wait()
            return_code = subprocess.call(" ".join(program), shell=True, stdout=file_log)
          print "return code = ", return_code
          #subprocess.call(program)

exp = FastDownwardExperiment()

exp.DEFAULT_SEARCH_TIME_LIMIT = '10m'
exp.DEFAULT_SEARCH_MEMORY_LIMIT = '2G'

exp.add_suite(BENCHMARKS_DIR, SUITE)

REV = 'default'  # tag of commit in 'hg log', tip should be the most recent commit
# Thesis experiments
#exp.add_algorithm('EHC_BFS', REPO, REV, ['--search', 'ehc_dd(h=ff())'])
#exp.add_algorithm('EHC_BFS_DD', REPO, REV, ['--search', 'ehc(h=ff())'])
#
#exp.add_algorithm('EHC_LUBY', REPO, REV, ['--search', 'ehc_rw(h=ff(), restart=luby)'])
#
#exp.add_algorithm('EHC_EXP', REPO, REV, ['--search', 'ehc_rw(h=ff(), restart=exp)'])
#
#exp.add_algorithm('EHC_FP0.01', REPO, REV, ['--search', 'ehc_rw_fixed_prob(h=ff(), prob=0.01)'])
#
#exp.add_algorithm('EHC_FP0.05', REPO, REV, ['--search', 'ehc_rw_fixed_prob(h=ff(), prob=0.05)'])
#
#exp.add_algorithm('EHC_FP0.1', REPO, REV, ['--search', 'ehc_rw_fixed_prob(h=ff(), prob=0.1)'])
#
#exp.add_algorithm('EHC_FP0.2', REPO, REV, ['--search', 'ehc_rw_fixed_prob(h=ff(), prob=0.2)'])
#
#

## Conference experiments
#exp.add_algorithm('EHC_BFS_DD_PREF_PRUNE', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_dd(hff, preferred=[hff], preferred_usage=PRUNE_BY_PREFERRED)'])
#
#exp.add_algorithm('EHC_BFS_DD_PREF_RANK', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_dd(hff, preferred=[hff], preferred_usage=RANKY_PREFERRED_FIRST)'])
#
#exp.add_algorithm('EHC_LUBY_PREF0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=0)'])
#
#exp.add_algorithm('EHC_LUBY_PREF0.5', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=0.5)'])
#
#exp.add_algorithm('EHC_LUBY_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=0.9)'])
#
#exp.add_algorithm('EHC_LUBY_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=1.0)'])
#
#exp.add_algorithm('EHC_EXP_PREF0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=exp, preferred=[hff], pref_prob=0)'])
#
#exp.add_algorithm('EHC_EXP_PREF0.5', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=exp, preferred=[hff], pref_prob=0.5)'])
#
#exp.add_algorithm('EHC_EXP_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=exp, preferred=[hff], pref_prob=0.9)'])
#
#exp.add_algorithm('EHC_EXP_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=exp, preferred=[hff], pref_prob=1.0)'])
#
#exp.add_algorithm('EHC_FP0.01_PREF0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.01, preferred=[hff], pref_prob=0)'])
#
#exp.add_algorithm('EHC_FP0.01_PREF0.5', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.01, preferred=[hff], pref_prob=0.5)'])
#
#exp.add_algorithm('EHC_FP0.01_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.01, preferred=[hff], pref_prob=0.9)'])
#
#exp.add_algorithm('EHC_FP0.01_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.01, preferred=[hff], pref_prob=1.0)'])
#
#
#exp.add_algorithm('EHC_FP0.1_PREF0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.1, preferred=[hff], pref_prob=0)'])
#
#exp.add_algorithm('EHC_FP0.1_PREF0.5', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.1, preferred=[hff], pref_prob=0.5)'])
#
#exp.add_algorithm('EHC_FP0.1_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.1, preferred=[hff], pref_prob=0.9)'])
#
#exp.add_algorithm('EHC_FP0.1_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.1, preferred=[hff], pref_prob=1.0)'])
#
#
#exp.add_algorithm('EHC_FP0.2_PREF0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.2, preferred=[hff], pref_prob=0)'])
#
#exp.add_algorithm('EHC_FP0.2_PREF0.5', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.2, preferred=[hff], pref_prob=0.5)'])
#
#exp.add_algorithm('EHC_FP0.2_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.2, preferred=[hff], pref_prob=0.9)'])
#
#exp.add_algorithm('EHC_FP0.2_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.2, preferred=[hff], pref_prob=1.0)'])

##############
# conference experiments
##############

# December 2, do the other EHC methods first
# Pure RRW (no EHC)
#exp.add_algorithm('SingleRW', REPO, REV, ['--search', 'pure_rw()'])
#exp.add_algorithm('RRW_Luby', REPO, REV, ['--search', 'pure_rrw_scaled(restart=luby)'])
#exp.add_algorithm('RRW_EXP', REPO, REV, ['--search', 'pure_rrw_scaled(restart=exp)'])
#exp.add_algorithm('RRW_Luby_HFF', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'pure_rrw_scaled(h=hff, restart=luby)'])
#exp.add_algorithm('RRW_EXP_HFF', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'pure_rrw_scaled(h=hff, restart=exp)'])
#exp.add_algorithm('RRW_FP0.01', REPO, REV, ['--search', 'pure_rrw_fixed_prob(prob=0.01)'])
#exp.add_algorithm('RRW_FP0.05', REPO, REV, ['--search', 'pure_rrw_fixed_prob(prob=0.05)'])
#exp.add_algorithm('RRW_FP0.1', REPO, REV, ['--search', 'pure_rrw_fixed_prob(prob=0.1)'])

## These are still pure RRWs but the algorithm (FP) will not restart at a depth less than the scaling factor
#exp.add_algorithm('RRW_FP0.01_HFF', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'pure_rrw_fixed_probability(scaling_heuristic=hff, prob=0.01)'])
#exp.add_algorithm('RRW_FP0.05_HFF', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'pure_rrw_fixed_probability(scaling_heuristic=hff, prob=0.05)'])
#exp.add_algorithm('RRW_FP0.1_HFF', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'pure_rrw_fixed_probability(scaling_heuristic=hff, prob=0.1)'])


# EHC methods
exp.add_algorithm('EHC_BFS_DD', REPO, REV, ['--search', 'ehc(h=ff())'])
exp.add_algorithm('EHC_LUBY', REPO, REV, ['--search', 'ehc_rw(h=ff(), restart=luby)'])
exp.add_algorithm('EHC_EXP', REPO, REV, ['--search', 'ehc_rw(h=ff(), restart=exp)'])
exp.add_algorithm('EHC_FP0.01', REPO, REV, ['--search', 'ehc_rw_fixed_prob(h=ff(), prob=0.01)'])
exp.add_algorithm('EHC_FP0.05', REPO, REV, ['--search', 'ehc_rw_fixed_prob(h=ff(), prob=0.05)'])
exp.add_algorithm('EHC_FP0.1', REPO, REV, ['--search', 'ehc_rw_fixed_prob(h=ff(), prob=0.1)'])
# Do the below extra 2 just for EHC
exp.add_algorithm('EHC_FP0.5', REPO, REV, ['--search', 'ehc_rw_fixed_prob(h=ff(), prob=0.5)'])
exp.add_algorithm('EHC_FP0.00001', REPO, REV, ['--search', 'ehc_rw_fixed_prob(h=ff(), prob=0.00001)'])

# HA Pruning
exp.add_algorithm('EHC_BFS_DD_PREF_PRUNE', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc(hff, preferred=[hff], preferred_usage=PRUNE_BY_PREFERRED)'])
exp.add_algorithm('EHC_LUBY_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=1.0)'])
exp.add_algorithm('EHC_EXP_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=exp, preferred=[hff], pref_prob=1.0)'])
exp.add_algorithm('EHC_FP0.01_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.01, preferred=[hff], pref_prob=1.0)'])
exp.add_algorithm('EHC_FP0.05_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.05, preferred=[hff], pref_prob=1.0)'])
exp.add_algorithm('EHC_FP0.1_PREF1.0', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.1, preferred=[hff], pref_prob=1.0)'])


# HA Bias 0.9
exp.add_algorithm('EHC_LUBY_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=0.9)'])
exp.add_algorithm('EHC_EXP_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=exp, preferred=[hff], pref_prob=0.9)'])
exp.add_algorithm('EHC_FP0.01_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.01, preferred=[hff], pref_prob=0.9)'])
exp.add_algorithm('EHC_FP0.05_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.05, preferred=[hff], pref_prob=0.9)'])
exp.add_algorithm('EHC_FP0.1_PREF0.9', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw_fixed_prob(h=hff, prob=0.1, preferred=[hff], pref_prob=0.9)'])


#exp.add_algorithm('EHC_LUBY_PREF0.1', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=0.1)'])
#exp.add_algorithm('EHC_LUBY_PREF0.25', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=0.25)'])
#exp.add_algorithm('EHC_LUBY_PREF0.5', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=0.5)'])
#exp.add_algorithm('EHC_LUBY_PREF0.95', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=0.95)'])
#exp.add_algorithm('EHC_LUBY_PREF0.99', REPO, REV, ['--heuristic', 'hff=ff()', '--search', 'ehc_rw(h=hff, restart=luby, preferred=[hff], pref_prob=0.99)'])


# Make a report (AbsoluteReport is the standard report).
'''
exp.add_report(
    AbsoluteReport(attributes=ATTRIBUTES), outfile='report.html')

exp.add_report(
    AbsoluteReport(
        attributes=ATTRIBUTES),
    outfile='report.csv')
'''
# Parse the commandline and show or run experiment steps.
exp.run_steps(STORE_PATH)
