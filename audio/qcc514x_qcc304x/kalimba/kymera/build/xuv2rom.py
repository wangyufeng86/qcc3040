#!/usr/bin/env python
############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2015 - 2018 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
Converts 16-bit xuv format into 8-bit rom format.
"""
import sys
import re

def print_help():
    """
    Print script usage.
    """
    print('''
    xuv2rom - Converts 16-bit xuv format into 8-bit rom format.
              Called from makerules_src.mkf.
               
    takes as command line arguments:
        (1) input xuv files
    ''')

if __name__ == '__main__':

    # Sanity checks
    if len(sys.argv) != 2:
        print_help()
        sys.exit(1)

    # search for the following pattern: @[address in hexadecimal] [data in hexadecimal (16 bit)]
    PATTERN = re.compile("^@([0-9a-fA-F]+) ([0-9a-fA-F]+)")
    try:
        with open(sys.argv[1], "r") as file:
            for line in file:
                match = re.search(PATTERN, line)
                if match != None:
                    addr = int(match.group(1), 16)
                    data = int(match.group(2), 16)
                    # print the data byte by byte
                    print("@{0:06X} {1:02X}".format(2*addr, data&0xff))
                    print("@{0:06X} {1:02X}".format(2*addr+1, data//256))
    except IOError:
        print("Input file {0} cannot be opened".format(sys.argv[1]))
        print_help()
        sys.exit(1)
