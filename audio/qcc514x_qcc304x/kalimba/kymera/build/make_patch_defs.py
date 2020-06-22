#!/usr/bin/env python
############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
Given a directory or a list of files as arguments, extract the patchpoint
identifiers and store them in a specified output file.
"""
import sys
import os
import re
import argparse

### Helper functions
def read_file(filename, split_to_lines=True):
    """ Reads the content of a file. For gaining some performance all the content is
    read in one go. Additionally the content is split into lines."""
    try:
        with open(filename) as file:
            content = file.read()
        if split_to_lines:
            return content.split("\n")
        else:
            return content
    except IOError:
        return None

def update_file(file_to_write, content):
    """ This function updates a file. If the files doesn't exist
    it will be created. Only writes to the file if the content is different."""
    if read_file(file_to_write, split_to_lines=False) != content:
        with open(file_to_write, "w") as targetfile:
            targetfile.write(content)

parser = argparse.ArgumentParser(description="Goes through supplied list of source files and "+
                                 "generates patchpoint ids in dest_file")
parser.add_argument("-o",
                    action="store", type=str, dest="dest_file", required=True,
                    help="The destination file")
parser.add_argument("-c", "--collate",
                    action="store_true", dest="collate", default=False,
                    help="This signals that the temporary files for each component " +
                    "should be collated into a single large patch id table")
parser.add_argument("args",
                    help="When '-c' is not specified, a list of input files to process, " +
                    "otherwise the source directory followed by the number of bits per words.",
                    type=str, nargs='+')
options = parser.parse_args()

# Sanity check.
if options.collate and len(options.args) != 2:
    parser.print_help()
    sys.exit(1)

dest_file_content = ""

if options.collate:
    patch_num = 1
    root = options.args[0]
    bits_in_word = (int)(options.args[1])
    # check if dir exist.
    if os.path.exists(root):
        for file_name in sorted(os.listdir(root)):
            full_file_path = os.path.join(root, file_name)
            for line in read_file(full_file_path):
                if line.strip() != "":
                    # Turn the line into a patch point
                    dest_file_content += ".CONST " + line + " " + str(patch_num) +";\n"
                    patch_num += 1
    dest_file_content += "\n.CONST $patch.SLOW_TABLE_SIZE " + \
                         str(int((patch_num+bits_in_word-1)/bits_in_word)) + ";\n"
    update_file(options.dest_file, dest_file_content)
else:
    pattern = re.compile(r"[^\(]+\(([^,\)]+)")
    for file_name in options.args:
        for line in  read_file(file_name):
            # Remove commentS from the line
            line = line.strip().split(r"//")[0]
            if "SLOW_SW_ROM_PATCH_POINT" in line:
                # Extract the first parameter (from left-bracket to comma)
                match = re.search(pattern, line)
                if match != None:
                    dest_file_content += match.group(1)+ "\n"
    update_file(options.dest_file, dest_file_content)
