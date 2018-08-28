from multiprocessing import Pool
import multiprocessing
import time
import subprocess
import pipes



def f(x, y):
  print x + y
  #time.sleep(1)
  return_code = subprocess.call(["sleep", str(x)])
  print "ret code for %d,%d = %d" %  (x,y, return_code)

def run_async(pool):
  results = []
  for i in range(1,13):
    results.append(pool.apply_async(f, tuple([i, i+1])))

  return results

def run_fd(args):
  args = list(args)  # Convert tuple to list
  log_file = args.pop()
  print "1"
  args.insert(0, "./run_fd.sh")
  print "2"
  #with open(log_file, "w") as file_log:
  with open("./out.log" + log_file[-1], "w") as file_log:
    #return_code = subprocess.call(" ".join(args), shell=True, stdout=file_log)
    # " ".join(args) does not work in pool process (no idea why)
    #return_code = subprocess.call(["sleep", str(3)])
    #args = [pipes.quote(x) for x in args]
    print "running with args = ", args
    return_code = subprocess.call(args, stdout=file_log)
  print "3"
  print "==return code for %s = %s" % (log_file[-5:], return_code)

def run_fd_async(pool):
  print "running fd_async"
  args = ['RRW_EXP_HFF', '--search-time-limit', '10m', '--search-memory-limit', '2G', '/ais/krs0/loyzer/workspace/fd/benchmarks/grid/prob01.pddl', '--heuristic', 'hff=ff()', '--search', 'pure_rrw_scaled(h=hff, restart=exp)', './data/thesis/rrw_exp_hff/grid/run0/output.log']

  import copy
  args_temp = copy.deepcopy(args)
  results = []
  for i in range(4):
    # run_fd takes one parameter which is a tuple, so convert array to tuple and place it as first element in a tuple
    results.append(pool.apply_async(run_fd, (tuple(args_temp),)))
    args_temp[-1] = args[-1] + str(i)

  return results


def main():
  print multiprocessing.cpu_count()
  print ""
  pool = Pool(processes=4)

  results = run_fd_async(pool)

  count = 1
  for result in results:
    result.wait()
    # NOTE: as this is run, the workers (in Pool) will be running processes,
    #  so while the current result may not be done, subsequent results may be
    #  and will be reflected by quick, successive prints once the current result is done
    print("Process {} finished with success = {}".format(str(count), str(result.successful())))
    count += 1

  pool.close()
  pool.join()  # Unncessary if doing the above for-loop (printing successful or not)


if __name__ == '__main__':
  main()
