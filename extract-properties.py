import mmap
import os
import re
import sys

import subprocess

def extract_number(line):
    ''' extract first number-sequence
    '''
    val = re.search(r'\d+', line).group()
    return val


def main(directory):
  output_log_files = []
  OUTPUT_FILENAME = 'output.log'
  #for root, dirnames, filenames in os.walk('./data/thesis3'):
  for root, dirnames, filenames in os.walk(directory):
      print("root = ", root)
      print("dirnames = ", dirnames)
      print("filenames = ", filenames)
      for filename in filenames:
          if filename.lower() == OUTPUT_FILENAME.lower():
              output_log_files.append(os.path.abspath(os.path.join(root, filename)))
  
  
  bool_regexes = {
          'Coverage':     r'Solution found\!',
  }
  capture_regexes = {
          'Nickname':     r'nickname: ([a-zA-Z0-9\.\_]+)',
          'Plan length':  r'Plan length: (\d+) step\(s\)',
          'Plan Cost':    r'Plan Cost\: (\d+)',
          'Cost':         r'Plan Cost\: (\d+)',
          'Expansions':   r'Expanded (\d+) state\(s\)',
          'Reopened':     r'Reopened (\d+) state\(s\)',
          'Evaluated':    r'Evaluated (\d+) state\(s\)',
          'Evaluations':  r'Evaluations\: (\d+)',
          'Generated':    r'Generated (\d+) state\(s\)',
          'Dead ends':    r'Dead ends\: (\d+) state\(s\)',
          'domain':       r'INFO.*\/benchmarks\/(.+)\/domain\.pddl',  # exclude .pddl
          'problem':      r'INFO.*\/benchmarks\/.+\/(.*\d+.*\.pddl)', # include .pddl
          'search time':  r'Actual search time: ([0-9\.]+)s',
  }
  
  count = 0
  for output_log_file in output_log_files:
      count += 1
  
      if count % 100 == 0:
          print "on %s" % output_log_file
  
      results = {
          'coverage':     0,
          'nickname':     '?',
          'problem':      '?',
          'domain':       '?',
          'plan length':  0,
          'plan cost':    0,
          'cost':         0,    # duplicate of plan cost just for backward compatibility
          'expansions':   0,
          'reopened':     0,
          'evaluated':    0,
          'evaluations':  0,
          'generated':    0,
          'dead ends':    0,
          'search time':  0,
      }
  
      skipped_files = []
      # Do pre-processing on file
      # Remove all lines containing the patterns
      was_empty = os.stat(output_log_file).st_size == 0  
      subprocess.call(["sed", "-i", "", "/new\ restart\_length/d", output_log_file])
      subprocess.call(["sed", "-i", "", "/restart\ length/d", output_log_file])
      with open(output_log_file) as log_file:
          # memory-map the file, size 0 means whole file
          is_empty = os.stat(output_log_file).st_size == 0
	  print("output_log_file = ", output_log_file, "is_empty=", is_empty)
          if is_empty:
              print("skipping", output_log_file)
              skipped_files.append((output_log_file, was_empty, is_empty))
              continue
          log_string = mmap.mmap(log_file.fileno(), 0, access=mmap.ACCESS_READ)
          for line in iter(log_string.readline, ''):
  
              for attribute, regex in bool_regexes.iteritems():
                  match = re.search(regex, line, re.I)
                  if match:
                      # Override default value
                      results[attribute.lower()] = 1
  
              for attribute, regex in capture_regexes.iteritems():
                  match = re.search(regex, line, re.I)
                  if match:
                      val = match.group(1)
                      results[attribute.lower()] = val
                      if 'HFF' in results['nickname']:
                          print output_log_file
  
  
      import json
      output_file = os.path.dirname(output_log_file) + '/properties'
  
      with open(output_file, 'w+') as outfile:
          json.dump(results, outfile)

  print(len(skipped_files), "skipped files")
  print(skipped_files)

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print('Incorrect Number of args, expected 1 directory')
  else:
    main(sys.argv[1])
