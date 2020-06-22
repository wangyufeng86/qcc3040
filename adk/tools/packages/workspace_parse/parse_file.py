"""
%%fullcopyright(2018)
"""

# Python 2 and 3
from __future__ import print_function

import os
import sys

import xml.etree.ElementTree as ET

class ParseFile:
    def _parse_tree(self, filename):
        try:
            return ET.parse(filename)
        except IOError as e:
            if (self._is_file_not_found_error(e.errno)):
                print("File does not exist : %s" % filename)
                sys.exit(1)
            else:
                raise e
