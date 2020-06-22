############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
Prints the path to kdynmem based on the operating system type.
"""
import os
import sys

def main():
    '''
    Detect platform and returns path to kdynmem.
    '''
    dynmem_base = os.path.join(os.sep, 'home', 'devtools', 'kdynmem', 'B259829')
    if sys.platform.startswith('linux'):
        # Linux path and executable
        csr_dynmem_path = os.path.join(dynmem_base, 'linux', 'kdynmem')
    elif sys.platform == 'win32':
        # Windows path and Windows executable
        csr_dynmem_path = os.path.join(os.environ['DEVKIT_ROOT'], 'tools', 'kdynmem.exe')
    else:
        print('platform ' + sys.platform + ' is not allowed')
        sys.exit(1)
    return csr_dynmem_path

if __name__ == '__main__':
    print(main())
