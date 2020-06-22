############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2012 - 2019 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
Print the path to Kalimba tools based on the operating system type.
"""

import os
import sys
import subprocess

class kcc_version:
    """
    Class that encapsulates the logic to find the correct path to the kcc compiler.
    """
    kcc_release = 67

    def get_kcc_path(self, ostype):
        """
        Returns the path to kcc
        """
        if "linux" in ostype:
            # Internal KCC releases live here
            path = "/home/devtools/kcc/kcc-" + str(self.kcc_release) + "-linux"
        elif "Windows" in ostype:
            path = os.path.join(os.environ["DEVKIT_ROOT"], 'tools', 'kcc')
        elif "win32" in ostype:
            path = os.path.join(os.environ["DEVKIT_ROOT"], 'tools', 'kcc')
        else:
            sys.stderr.write("Error, Invalid OSTYPE: " + ostype + "\n")
            sys.exit(1)
        return path

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.stderr.write(sys.argv[0] + ": Error, OSTYPE was not provided.\n")
        sys.exit(1)
    OSTYPE = sys.argv[1]
    version = kcc_version()
    print(version.get_kcc_path(OSTYPE))
