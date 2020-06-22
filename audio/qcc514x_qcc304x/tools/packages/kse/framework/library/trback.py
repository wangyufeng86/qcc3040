#
# Copyright (c) 2018 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Traceback inter thread notification library
'''

_TRACEBACK = {'msg': ''}


def get_traceback():
    '''
    Gets last thread traceback set

    Returns:
        str: Traceback
    '''
    return _TRACEBACK['msg']


def set_traceback(traceback):
    '''
    Sets last thread traceback

    Args:
        traceback (str): Traceback
    '''
    _TRACEBACK['msg'] = traceback
