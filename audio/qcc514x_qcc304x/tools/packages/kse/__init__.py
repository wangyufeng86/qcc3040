#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
KATS main package file
'''

import os

from ._version import __version__, version_info

bin_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), 'bin'))
try:
    dirs = os.listdir(bin_dir)
except Exception:  # pylint:disable=broad-except
    dirs = []

for directory in dirs:
    directory = os.path.join(bin_dir, directory)
    if os.path.isdir(directory):
        os.environ.setdefault('PATH', '')
        os.environ['PATH'] += os.pathsep + directory
