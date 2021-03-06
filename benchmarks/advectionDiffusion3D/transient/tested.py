
import os
from nose.tools import istest

from estest import ESPRESOTest

def setup():
    ESPRESOTest.path = os.path.dirname(__file__)
    ESPRESOTest.args = [ "etype", 1, 4, 1, 2, 2, 2, 4, 6, 4 ]

def teardown():
    ESPRESOTest.clean()

@istest
def by():
    for etype in [ "HEXA8", "TETRA4", "PRISMA6", "PYRAMID5" ]:
        yield run, etype

def run(etype):
    ESPRESOTest.args[0] = etype
    ESPRESOTest.run()
    ESPRESOTest.compare(".".join([etype, "emr"]))
    ESPRESOTest.report("espreso.time.xml")
