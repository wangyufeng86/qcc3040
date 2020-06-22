#
# Copyright (c) 2018 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Mapping support for endpoints
'''

import copy
import logging

from kats.library.registry import get_instance_num, get_instance


def map_endpoint(interface, endpoint_type, kwargs):
    '''
    Remap kymera endpoint

    Args:
        interface (str): Interface type
        endpoint_type (str):  Type of endpoint (source or sink)
        kwargs (dict): Endpoint parameters

    Returns:
        dict: Modified (remapped) endpoint parameters
    '''
    outp = kwargs
    if get_instance_num('uut_mapping'):
        mapping = get_instance('uut_mapping')

        entry = copy.deepcopy(kwargs)
        inp = {
            'interface': interface,
            'type': endpoint_type
        }
        inp.update(entry)

        instance0 = inp.get('instance', None)
        channel0 = inp.get('channel', None)
        outp = mapping.map(inp)

        if outp:
            instance1 = outp.get('instance', None)
            channel1 = outp.get('channel', None)
            logging.getLogger(__name__).info(
                'interface:%s endpoint_type:%s instance:%s channel:%s remapped to '
                'instance:%s channel:%s',
                interface, endpoint_type, instance0, channel0, instance1, channel1)
        else:
            outp = {}

        entry.update(outp)
        outp = entry
        outp.pop('interface', None)
        outp.pop('type', None)

    return outp
