#
# Copyright (c) 2018 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Hydra file streams
'''

from kats.kalsim.hydra_service.constants import DEVICE_TYPE_FILE
from .audio_data import StreamHydraAudioData, DEVICE_TYPE


class StreamHydraFile(StreamHydraAudioData):
    '''
    Hydra file streams

    Args:
        stream_type (str): Type of stream source or sink
    '''

    platform = ['crescendo', 'stre', 'streplus', 'mora']
    interface = 'file'

    def __init__(self, stream_type, **kwargs):
        kwargs.setdefault(DEVICE_TYPE, DEVICE_TYPE_FILE)
        super(StreamHydraFile, self).__init__(stream_type, **kwargs)
