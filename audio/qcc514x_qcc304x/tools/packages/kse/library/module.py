#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Module support library
'''

import os


def get_parent_module(module_name, module_filename):
    '''
    Get the parent module path of a given module path and file

    The module_filename is needed to compute the parent module
    file file1.file2.py inside the module kats would be module kats.file1.file2

    .. code-block:: python

        __name__
        'kats.kymera.uut.uut'
        __file__
        'C:/Users/martina/project/kats4/kats/kats/kalimba/uut/uut.pyc'
        get_parent_module(__name__, __file__)
        kats.kymera.uut

    Args:
        param (str): Module name
        module_filename (str): Filename for that module

    Returns:
        str: Parent module
    '''

    module_path = os.path.splitext(os.path.basename(module_filename))[0]
    return module_name.rstrip(module_path).rstrip('.')
