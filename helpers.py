import re
import os

def natural_sort(l):
    """
    Sorts the list of strings into human-expected order
    """
    convert = lambda text: int(text) if text.isdigit() else text.lower()
    alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key) ]
    return sorted(l, key = alphanum_key)

BENCHMARK_DIR = os.path.join(os.getcwd(), 'benchmarks')

def get_last_problem_instances(domain, last):
    """
    domain is the directory name in the benchmarks folder.
    last is the number of instances to get (the hardest instances)
    """
    instances = os.listdir(os.path.join(BENCHMARK_DIR, domain))
    instances = filter(lambda a: 'domain' not in a, instances)
    return natural_sort(instances)[-last:]
