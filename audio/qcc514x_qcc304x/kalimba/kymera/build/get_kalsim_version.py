############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2013 - 2018 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
Prints the path to the relevant Kalsim binary based on the operating system type
and requested target chip path.
"""

import sys
import os

SUPPORTED_CHIPS = ["gordon", "crescendo_audio", "aura_audio", "streplus_audio"]
SUPPORTED_PLATFORMS = ["win32", "linux2"]

if __name__ == '__main__':

    chipname = sys.argv[1]
    if chipname not in SUPPORTED_CHIPS:
        sys.stderr.write("Unknown chipname: {0}{1}".format(chipname, os.linesep))
        sys.exit(1)

    if sys.platform not in SUPPORTED_PLATFORMS:
        sys.stderr.write("Unknown platform: {0}{1}".format(sys.platform, os.linesep))
        sys.exit(1)

    if chipname == "gordon":
        if sys.platform == "win32":
            kalsim_path = "r:/home/dspsw_tools/kalsim/kalsim_win32/kalsim_gordon.exe"
        else:
            kalsim_path = "/home/devtools/kalsim/external/2016a/bin/kalsim_csr8670"
    else:
        if sys.platform == "win32":
            kalsim_path = "r:/home/devtools/kalsim/21e/win32/kalsim_"+sys.argv[1]+".exe"
        else:
            kalsim_path = "/home/devtools/kalsim/21e/posix/kalsim_"+sys.argv[1]

    print(kalsim_path)
