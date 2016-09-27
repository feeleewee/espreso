
from utils import *
import unittest

class ESPRESOTests(unittest.TestCase):

    espreso = Espreso()

    def regular_cube(self, procs, config, args):
        config["ITERATIONS"] = 300
        config["EPSILON"] = 1e-4
        config["INPUT"] = "GENERATOR"
        config["PATH"] = "regular_fixed_bottom.txt"
        info = self.espreso.info(procs, "tests/examples/linearElasticity/cube", config, args + [2, 2, 2, 2, 2, 2])
        if info.precision > config["EPSILON"]:
            raise Exception("Example not convergated to a requested precision (" + str(info.precision) + " > " + str(config["EPSILON"]) + ")")
        if config["PRECONDITIONER"] == "DIRICHLET":
            if info.max_oscilation > 2:
                raise Exception("Oscilation " + str(info.max_oscilation) + " is higher than 2")
        else:
            if info.max_oscilation > 3:
                raise Exception("Oscilation " + str(info.max_oscilation) + " is higher than 3")

    def metis_cube(self, procs, config, args):
        config["ITERATIONS"] = 500
        config["EPSILON"] = 1e-4
        config["INPUT"] = "GENERATOR"
        config["PATH"] = "metis_fixed_bottom.txt"
        info = self.espreso.info(procs, "tests/examples/linearElasticity/cube", config, args + [2, 2, 2, 5, 5, 5])
        if info.precision > config["EPSILON"]:
            raise Exception("Example not convergated to a requested precision (" + str(info.precision) + " > " + str(config["EPSILON"]) + ")")
        if config["PRECONDITIONER"] == "DIRICHLET":
            if info.max_oscilation > 2:
                raise Exception("Oscilation " + str(info.max_oscilation) + " is higher than 2")
        else:
            if info.max_oscilation > 3:
                raise Exception("Oscilation " + str(info.max_oscilation) + " is higher than 3")

    def metis_cube_with_cyclic_edge(self, procs, config, args):
        config["ITERATIONS"] = 300
        config["EPSILON"] = 1e-4
        config["INPUT"] = "GENERATOR"
        config["PATH"] = "metis_fixed_bottom.txt"
        config["ITERATIONS"] = 20
        info = self.espreso.info(procs, "tests/examples/linearElasticity/cube", config, args + [2, 1, 1, 2, 4, 4])
        if config["PRECONDITIONER"] == "DIRICHLET":
            if info.max_oscilation > 2:
                raise Exception("Oscilation " + str(info.max_oscilation) + " is higher than 2")
        else:
            if info.max_oscilation > 3:
                raise Exception("Oscilation " + str(info.max_oscilation) + " is higher than 3")

if __name__ == '__main__':

    def create_instance(config, example):
            if config["FETI_METHOD"] == "TOTAL_FETI" and config["B0_TYPE"] == "KERNELS":
                return
            procs = reduce(lambda x, y: x * y, example["CLUSTERS"])
            args = [example["ETYPE"]] + example["CLUSTERS"]
            name = "_".join(str(x) for x in args + config.values())
            config["VERBOSE_LEVEL"] = 1
            config["TESTING_LEVEL"] = 1
            TestCaseCreator.create_test(ESPRESOTests, ESPRESOTests.regular_cube, name, procs, config, args)
            TestCaseCreator.create_test(ESPRESOTests, ESPRESOTests.metis_cube, name + "_METIS", procs, config, args)
            TestCaseCreator.create_test(ESPRESOTests, ESPRESOTests.metis_cube_with_cyclic_edge, name + "_METIS_TWO_SUBDOMAINS", procs, config, args)

    config = {
      "FETI_METHOD": [ "TOTAL_FETI", "HYBRID_FETI" ],
      "PRECONDITIONER": [ "NONE", "LUMPED", "WEIGHT_FUNCTION", "DIRICHLET" ],
      "REGULARIZATION": [ "FIX_POINTS", "NULL_PIVOTS" ],
      "B0_TYPE": [ "CORNERS", "KERNELS" ]
    }

    example = {
      "ETYPE":  [ "HEXA8", "TETRA4", "PRISMA6", "PYRAMID5", "HEXA20", "TETRA10", "PRISMA15", "PYRAMID13" ],
      "CLUSTERS": [ [1, 1, 1], [1, 1, 4], [2, 2, 2] ]
    }

    TestCaseCreator.iterate(create_instance, config, example)
    unittest.main()
