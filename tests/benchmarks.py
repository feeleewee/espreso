
from utils import *
import unittest
import glob
import string

ESPRESO_TESTS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(ESPRESO_TESTS)

class ESPRESOBenchmarks(unittest.TestCase):

    espreso = Espreso()

    def benchmark(self, path, file):
        config = { "ENV::TESTING_LEVEL": 3, "ENV::VERBOSE_LEVEL": 0, "ENV::MEASURE_LEVEL": 0, "OUTPUT::RESULT": 0 }
        for line in [ line.rstrip('\n') for line in open(os.path.join(path, file)) ]:
            param, value = line.split("=")
            if param.strip() == "PROCS":
                procs = int(value)
            elif param.strip() == "ARGS":
                args = value.split()
            else:
                config["RESULTS::" + param.strip()] = value.strip()

        self.espreso.run(procs, path, config, args)


if __name__ == '__main__':

    path = os.path.join(ROOT, "benchmarks")

    benchmarks = []
    for root, subFolders, files in os.walk(path):
        for file in files:
            if file.endswith(".test"):
                benchmarks.append(( root, file[0:-5]))

    benchmarks.sort()
    for benchmark in benchmarks:
        name = os.path.relpath(benchmark[0], path).replace('/', '_') + "_" + benchmark[1]
        TestCaseCreator.create_test(ESPRESOBenchmarks, ESPRESOBenchmarks.benchmark, name, benchmark[0], benchmark[1] + ".test")

    unittest.main()
