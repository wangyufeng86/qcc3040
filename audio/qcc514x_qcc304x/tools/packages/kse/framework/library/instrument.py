#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
KATS framework Test Base Instrument class
'''

from abc import ABCMeta, abstractproperty

from six import add_metaclass


@add_metaclass(ABCMeta)  # pylint: disable=too-few-public-methods
class Instrument(object):
    '''
    Base Class that every instrument must subclass
    '''

    @abstractproperty
    def interface(self):
        '''
        str: Instrument name/interface
        '''

    @abstractproperty
    def schema(self):
        '''
        dict: Instrument schema
        '''
