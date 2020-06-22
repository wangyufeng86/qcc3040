'''
 Copyright (c) 2017-2019 Qualcomm Technologies International, Ltd.
'''

from __future__ import print_function
import sys
import os
import glob
import argparse

ME = os.path.abspath(__file__)
MEDIR = os.path.dirname(ME)
sys.path.insert(0, MEDIR)

# pylint: disable=wrong-import-position,relative-import
from chaingen_mod import process_file


def main():
    ''' Generate the chain '''
    parser = argparse.ArgumentParser(description="Generate chains from an XML description of the chain")
    parser.add_argument('filename',
                        type=str,
                        help='The name of the xml file containing the description of the chain. Can include wildcards.')

    parser.add_argument('--header',
                        action='store_true')

    parser.add_argument('--source',
                        action='store_true')

    parser.add_argument('--uml',
                        action='store_true')

    parser.add_argument('--file',
                        dest='write_to_file',
                        action='store_true',
                        help='Generate file. Default is print to std:out')

    parser.add_argument('--output_folder',
                        type=str,
                        default=None,
                        help='Folder to generate source. Use location of source xml file if not specified')

    args = parser.parse_args()

    files = glob.glob(args.filename)
    if not files:
        print('WARNING: Files argument does not match any files: {}'.format(args.filename), file=sys.stderr)

    for filename in files:
        process_file(args.header, args.source, args.uml, args.write_to_file, filename, args.output_folder)

    return True


if __name__ == "__main__":
    if main() is True:
        sys.exit(0)
    else:
        sys.exit(1)
