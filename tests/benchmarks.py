
import os
import sys
import shutil

ESPRESO_TESTS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(ESPRESO_TESTS)
sys.path.append(os.path.join(ESPRESO_TESTS, "utils"))

from testing import *
from snailwatch import *
import unittest

class ESPRESOBenchmarks(unittest.TestCase):

    espreso = Espreso()
    snailwatch = SnailWatch()

    def benchmark(self, name, path, file, args, report):
        arguments = args.values()
        for key in args:
            arguments[int(key[3:])] = args[key]
        self.espreso.output(
            self.espreso.get_processes(os.path.join(path, file)),
            path,
            { "config": os.path.join(path, file), "ENV::VERBOSE_LEVEL": 1, "ENV::MEASURE_LEVEL": 1, "OUTPUT::RESULTS_STORE_FREQUENCY": "NEVER" },
            arguments
        )

        self.snailwatch.push(name, os.path.join(path, "results", "last", file.replace(".ecf", ".log")))

        self.espreso.compare_monitors(
            os.path.join(path, report),
            os.path.join(path, "results", "last", file.replace(".ecf", ".emr"))
        )

        shutil.rmtree(os.path.join(path, "results"))

if __name__ == '__main__':

    print "the benchmarks script is not working - run benchmarks using: nosetests"

#     benchmarks = TestCaseCreator.select(os.path.join(ROOT, "benchmarks"))
# 
#     def run(vars, range, name, path, file):
#         def call(args):
#             report = ".".join([ args[var] for var in vars ])
#             if len(report) == 0:
#                 report = "espreso"
#             TestCaseCreator.create_test(ESPRESOBenchmarks, ESPRESOBenchmarks.benchmark, name + "." + report, name, path, file, args, report + ".emr")
# 
#         TestCaseCreator.iterate(call, range)
# 
#     for subdirectory in benchmarks:
#         for name, path, file in TestCaseCreator.gather(subdirectory, ".ecf"):
#             vars, range = Espreso.get_instances(os.path.join(path, file))
#             run(vars, range, name, path, file)
# 
#     unittest.main()
