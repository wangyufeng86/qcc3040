############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2011 - 2017 Qualcomm Technologies International, Ltd.
#
############################################################################
'''
Script to capture and collate build warnings and errors from the
compiler and assembler, run as part of the audio build system.
'''

import os
import sys
import subprocess
import re
import platform

# Regular expressions for compiler/linker warnings and errors
c_Warning = re.compile(r"\sWarning:\s")
c_warning = re.compile(r"\swarning:\s")
c_Error = re.compile(r"\sError:\s")
c_error = re.compile(r"\serror[:]?\s")
h_error = re.compile(r"\sline \d+:\s")

# REs for error prefix/suffix lines
pref_error = re.compile(r"\sfrom [\S]+:[\d]?")
pref2_error = re.compile(r"In function [\S]+")
suf_error = re.compile(r"/[\S]+:\s")


def process_warnings(err_file):
    """
    Counts the warnings and errors encountered in the build and reports their
    totals along with a summary of all the errors/warnings that were encountered
    """
    warn_cnt = 0
    err_cnt = 0
    print_to_screen = False

    print("\nBuild Warnings & Errors\n")

    file = open(err_file)
    for line in file:
        # test the output for the different possible warnings and outputs
        if c_warning.findall(line) or \
           c_Warning.findall(line):
            warn_cnt += 1
            print_to_screen = True
        elif c_Error.findall(line) or \
             c_error.findall(line) or \
             h_error.findall(line):
            err_cnt += 1
            print_to_screen = True
        elif pref_error.findall(line) or \
             pref2_error.findall(line):
            # error(s) will be counted soon
            print_to_screen = True
        elif suf_error.findall(line):
            # error already counted
            print_to_screen = True
        elif line != "":
            # could be an unknown error so output, but still surpress blank lines
            print_to_screen = True

        if print_to_screen:
            # line endings are already included in the file so remove trailing white space
            print(line.rstrip())
            print_to_screen = False

    # if we have one or more warnings/errors output the count
    if warn_cnt + err_cnt:
        print("{0} warnings and {1} errors occurred".format(warn_cnt, err_cnt))
    else:
        print("Build completed without errors")

# If the function is called with only 2 arguments the build process has finished.
# The second argument is the file the errors were logged to
if len(sys.argv) == 2:
    # Output the results
    process_warnings(sys.argv[1])

else:
    # Correct any relative paths in the parameters
    # Limit to Windows build as linux doesn't care
    av = []
    last_path = ""
    for path_string in sys.argv:
        if platform.system() == "Windows":
            # Don't adjust the target of the generated .d file. Make likes it to match.
            if last_path != "-MT":
                path_string = os.path.normpath(path_string)
        av.append(path_string)
        last_path = path_string

    # execute the compile/assemble command
    # Divert the output to errors_file
    try:
        compiler = subprocess.Popen(av[2:], stderr=subprocess.PIPE)
    except (OSError, ValueError):
        print("Failed to run command " + av[2:])
        sys.exit(2)

    # call communicate so we can access the return code when we have finished
    (_, errstr) = compiler.communicate()

    # capture any errors and output to the screen before writing to the temporary
    # log file
    errstr = errstr.decode("ascii")
    if errstr != "":
        # Open a file to log the warnings and errors.
        # The file will be temporarily kept in the build directory.
        # The location differs depending on gcc/kcc as the call in
        # makerules_src.mkf is different
        st = open(sys.argv[1], 'a')

        print(errstr)

        st.write(str(errstr))
        st.close()

    sys.exit(compiler.returncode)
