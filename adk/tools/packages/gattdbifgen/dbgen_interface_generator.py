##############################################################################
# Copyright (c) 2020 Qualcomm Technologies International, Ltd.
#
# FILE NAME
#   dbgen_interface_generator.py
#
# DESCRIPTION
#   An application needs to provide an implementation of a function with the
#   following prototype:
#       uint16 getGattAttributeValue(gatt_attribute_t id, uint16 n);
# 
# USAGE
#   dbgen_interface_generator.py --header output.h *.db
#   dbgen_interface_generator.py --source output.c gatt_handler_db.h
#
##############################################################################
from __future__ import print_function
import sys
import os
import re
import argparse
import pprint
from string import Template

here = __file__

# The template source file is in the same location as this script, with a .c extension
def getSourceTemplate():
    (stub,_) = os.path.splitext(here)
    with open(stub + ".c", "r") as f:
        lines = f.readlines()
        template = Template("".join(lines))
    return template

# The template header file is in the same location as this script, with a .h extension
def getHeaderTemplate():
    (stub,_) = os.path.splitext(here)
    with open(stub + ".h", "r") as f:
        lines = f.readlines()
        template = Template("".join(lines))
    return template

# Build a list of #define names which match the naming scheme of gattdbgen.
def getDefines(path,defines):
    with open(path) as f:
        for line in f:
            line = line.rstrip('\r\n')
            m = re.match('#define\s+(HANDLE_\w+)\s+(\S+)',line)
            if m:
                defines.append(m.group(1))

# Output a header file, based on the extracted list of gattdbgen symbolic names.
def generateHeader(defines,outfile):
    bname = os.path.basename(outfile)
    dbname= re.sub("_db_if\.[ch]$","",bname)

    count = 0
    enum_list = []
    for name in defines:
        # The enum names are just the lowercased representation of the
        # gattdbgen generated symbolic names.
        enum_list.append("    {},".format(name.lower()))
        count += 1
        if count % 5 == 0:
            # A comment marker is output every 5th entry to assist with debugging.
            enum_list.append("    /* {} */".format(count))

    define_list = []
    for name in defines:
        # Each gattdbgen symbolic name is then output as a pair of macros calling the
        # getGattAttributeValue() method, allowing this auto-generated header file
        # to easily substitute the existing gatt_handler_db.h file.
        define_list.append("#define HAVEDB_{}".format(name))
        define_list.append("#ifndef {}".format(name))
        define_list.append("#define {:<50} getGattAttributeValue({},0)".format(name,name.lower()))
        define_list.append("#define {:<50} getGattAttributeValue({},n)".format(name+"_N(n)",name.lower()))
        define_list.append("#endif")

    template = getHeaderTemplate()
    result = template.substitute({'module' : dbname,
                                  'header_guard' : dbname.upper(),
                                  'enum_list' : "\n".join(enum_list),
                                  'define_list' : "\n".join(define_list)})
    with open(outfile,"wb") as fh:
        print(result, file=fh)

# Output a source file, based on the extracted list of gattdbgen symbolic names.
def generateSource(defines,outfile):
    bname = os.path.basename(outfile)
    dbname= re.sub("_db_if\.[ch]$","",bname)

    # Build a list of used enumerations, and the return
    # values associated with it.
    seen_enums = {}
    for name in defines:
        ename = name.lower()
        m = re.search("\d+",ename)
        if m:
            # This is an N'th example, so extract N and test for it.
            ename = re.sub("\d+","",ename)
            if not ename in seen_enums:
                seen_enums[ename] = (name,[])
            seen_enums[ename][1].append("if ( n == {} ) return {};".format(m.group(0),name))
        else:
            if not ename in seen_enums:
                seen_enums[ename] = (name,[])
            seen_enums[ename][1].append("return {};".format(name))

    # Generate a switch/case for each seen enum constant,
    # and output all the lines associated with that case.
    lines = []
    for e in seen_enums:
        # Turn HAVEDB_HANDLE_BATTERY_LEVEL1 -> HAVEDB_HANDLE_BATTERY_LEVEL
        defname = re.sub(r'[0-9]+([^0-9]*)$',r'\1',seen_enums[e][0])
        lines.append("#ifdef HAVEDB_{}".format(defname))
        lines.append("{:<8}case {}:".format("",e))
        for v in seen_enums[e][1]:
            lines.append("{:<12}{}".format("",v))
        lines.append("#endif")
        # KCC reports Warning: break statement cannot be reached (BreakUnreached)
        # lines.append("{:<12}/* NOTREACHED */".format(""))
        # lines.append("{:<12}break;".format(""))

    lines.append("{:<8}default:".format(""))
    lines.append("{:<12}break;".format(""))

    template = getSourceTemplate()
    result = template.substitute({'module' : dbname,
                                  'switch_cases' : "\n".join(lines)})
    with open(outfile,"wb") as fh:
        print(result, file=fh)

# Parse the command line.
# The input file(s) are scanned for appropriate gattdbgen symbolic names,
# then either a header file or source file is output.
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate a GATT database interface')
    parser.add_argument('files', type=str, nargs='+', help='*.db files for --header, gatt_handler_db.h for --source')
    parser.add_argument('--header', type=str, nargs=1, help='The filename to write a .h file to')
    parser.add_argument('--source', type=str, nargs=1, help='The filename to write a .c file to')
    args = parser.parse_args()

    if 'header' in args and args.header:
        defines = []
        for file in args.files:
            getDefines(file,defines)
        generateHeader(defines,args.header[0])

    if 'source' in args and args.source:
        defines = []
        getDefines(args.files[0],defines)
        generateSource(defines,args.source[0])
