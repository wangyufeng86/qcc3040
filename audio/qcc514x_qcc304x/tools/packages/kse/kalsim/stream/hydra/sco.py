#
# Copyright (c) 2018 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Hydra sco streams
'''

import argparse
import copy
import logging
import random
import struct
from functools import partial

from kats.core.stream_base import STREAM_TYPE_SOURCE, STREAM_TYPE_SINK, \
    CALLBACK_EOF, STREAM_NAME, STREAM_RATE, STREAM_DATA_WIDTH, STATE_STARTED
from kats.framework.library.docstring import inherit_docstring
from kats.framework.library.log import log_input
from kats.framework.library.schema import DefaultValidatingDraft4Validator
from kats.kalsim.hydra_service.sco_processing_service import HydraScoProcessingService, \
    TIMESLOT_DURATION
from kats.kalsim.hydra_service.util import get_buffer_stats
from kats.kalsim.stream.kalsim_helper import get_user_stream_config
from kats.kalsim.stream.kalsim_stream import KalsimStream
from kats.kalsim.stream.packet.packetiser import Packetiser
from kats.library.audio_file.audio import audio_get_instance
from kats.library.registry import register_instance, get_instance, get_instance_num
from .hydra import HYDRA_TYPE, HYDRA_TYPE_SUBSYSTEM, HYDRA_BAC_HANDLE
from .sco_error import ScoError

BACKING = 'backing'
BACKING_FILE = 'file'
BACKING_DATA = 'data'
FILENAME = 'filename'
CHANNELS = 'channels'
CHANNELS_DEFAULT = 1
CHANNEL = 'channel'
CHANNEL_DEFAULT = 0
SAMPLE_RATE = 'sample_rate'
SAMPLE_RATE_DEFAULT = 8000
SAMPLE_WIDTH = 'sample_width'
SAMPLE_WIDTH_DEFAULT = 16
FRAME_SIZE = 'frame_size'
FRAME_SIZE_DEFAULT = 1
DELAY = 'delay'
DELAY_DEFAULT = 0
LOOP = 'loop'
LOOP_DEFAULT = 1
CALLBACK_CONSUME = 'callback_consume'
SEED = 'seed'
SEED_DEFAULT = None
PACKET_NUMBER = 'packet_number'
PACKET_NUMBER_DEFAULT = 1
PER = 'per'
PER_DEFAULT = 0
BER = 'ber'
BER_DEFAULT = 0
CRC_ERROR = 'crc_error'
CRC_ERROR_DEFAULT = 0.0
NOTHING_RECEIVED_ERROR = 'nothing_received_error'
NOTHING_RECEIVED_ERROR_DEFAULT = 0.0
NEVER_SCHEDULED_ERROR = 'never_scheduled_error'
NEVER_SCHEDULED_ERROR_DEFAULT = 0.0
NEVER_SCHEDULED_ERROR_PACKETS = 'never_scheduled_error_packets'
NEVER_SCHEDULED_ERROR_PACKETS_DEFAULT = 2
DATA_ERROR = 'data_error'
METADATA_ENABLE = 'metadata_enable'
METADATA_ENABLE_DEFAULT = False
METADATA_FORMAT = 'metadata_format'
METADATA_FORMAT_STANDARD = 'standard'
METADATA_FORMAT_ZEAGLE = 'zeagle'
METADATA_FORMAT_DEFAULT = METADATA_FORMAT_STANDARD
STREAM = 'stream'
STREAM_DEFAULT = None

SERVICE_TAG = 'service_tag'
SERVICE_TAG_DEFAULT = None

PARAM_SCHEMA = {
    'oneOf': [
        {
            'type': 'object',
            'required': [BACKING, FILENAME],
            'properties': {
                BACKING: {'type': 'string', 'enum': [BACKING_FILE]},
                FILENAME: {'type': 'string'},
                CHANNELS: {'type': 'integer', 'minimum': 1},
                CHANNEL: {'type': 'integer', 'minimum': 0},
                SAMPLE_RATE: {'type': 'number', 'minimum': 0},
                SAMPLE_WIDTH: {'type': 'integer', 'enum': [8, 16]},
                FRAME_SIZE: {'type': 'integer', 'minimum': 1},
                DELAY: {'type': 'number', 'minimum': 0},
                LOOP: {'type': 'integer', 'minimum': 1},

                SEED: {'type': ['integer', 'null'], 'default': SEED_DEFAULT},
                PACKET_NUMBER: {'type': 'integer', 'minimum': 1, 'default': PACKET_NUMBER_DEFAULT},
                PER: {'type': 'number', 'minimum': 0, 'maximum': 100, 'default': PER_DEFAULT},
                BER: {'type': 'number', 'minimum': 0, 'maximum': 100, 'default': BER_DEFAULT},
                CRC_ERROR: {'type': 'number', 'minimum': 0, 'maximum': 100,
                            'default': CRC_ERROR_DEFAULT},
                NOTHING_RECEIVED_ERROR: {'type': 'number', 'minimum': 0, 'maximum': 100,
                                         'default': NOTHING_RECEIVED_ERROR_DEFAULT},
                NEVER_SCHEDULED_ERROR: {'type': 'number', 'minimum': 0, 'maximum': 100,
                                        'default': NEVER_SCHEDULED_ERROR_DEFAULT},
                NEVER_SCHEDULED_ERROR_PACKETS: {'type': 'integer', 'minimum': 2,
                                                'default': NEVER_SCHEDULED_ERROR_PACKETS_DEFAULT},
                DATA_ERROR: {'type': 'boolean', 'default': True},
                METADATA_ENABLE: {'type': 'boolean', 'default': METADATA_ENABLE_DEFAULT},
                METADATA_FORMAT: {'type': 'string', 'default': METADATA_FORMAT_DEFAULT,
                                  'enum': [METADATA_FORMAT_STANDARD, METADATA_FORMAT_ZEAGLE]},
                STREAM: {'default': STREAM_DEFAULT},

                SERVICE_TAG: {'default': SERVICE_TAG_DEFAULT},
            }
        },
        {
            'type': 'object',
            'required': [BACKING],
            'properties': {
                BACKING: {'type': 'string', 'enum': [BACKING_DATA]},
                SAMPLE_RATE: {'type': 'number', 'minimum': 0},  # only for sink
                SAMPLE_WIDTH: {'type': 'integer', 'enum': [8, 16]},
                FRAME_SIZE: {'type': 'integer', 'minimum': 1},

                SEED: {'type': ['integer', 'null'], 'default': SEED_DEFAULT},
                PACKET_NUMBER: {'type': 'integer', 'minimum': 1, 'default': PACKET_NUMBER_DEFAULT},
                PER: {'type': 'number', 'minimum': 0, 'maximum': 100, 'default': PER_DEFAULT},
                BER: {'type': 'number', 'minimum': 0, 'maximum': 100, 'default': BER_DEFAULT},
                CRC_ERROR: {'type': 'number', 'minimum': 0, 'maximum': 100,
                            'default': CRC_ERROR_DEFAULT},
                NOTHING_RECEIVED_ERROR: {'type': 'number', 'minimum': 0, 'maximum': 100,
                                         'default': NOTHING_RECEIVED_ERROR_DEFAULT},
                NEVER_SCHEDULED_ERROR: {'type': 'number', 'minimum': 0, 'maximum': 100,
                                        'default': NEVER_SCHEDULED_ERROR_DEFAULT},
                NEVER_SCHEDULED_ERROR_PACKETS: {'type': 'integer', 'minimum': 2,
                                                'default': NEVER_SCHEDULED_ERROR_PACKETS_DEFAULT},
                DATA_ERROR: {'type': 'boolean', 'default': True},
                METADATA_ENABLE: {'type': 'boolean', 'default': METADATA_ENABLE_DEFAULT},
                METADATA_FORMAT: {'type': 'string', 'default': METADATA_FORMAT_DEFAULT,
                                  'enum': [METADATA_FORMAT_STANDARD, METADATA_FORMAT_ZEAGLE]},
                STREAM: {'default': STREAM_DEFAULT},

                SERVICE_TAG: {'default': SERVICE_TAG_DEFAULT},
            }
        }
    ]
}

METADATA_SYNC_WORD = 0x5c5c
METADATA_STATUS_OK = 0
METADATA_STATUS_CRC_ERROR = 1
METADATA_STATUS_NOTHING_RECEIVED = 2
METADATA_STATUS_NEVER_SCHEDULED = 3
METADATA_STATUS_ZEAGLE = 4

METADATA_STATUS_ZEAGLE_OK = 0
METADATA_STATUS_ZEAGLE_ONE_PACKET_WITH_BAD_CRC = 1
METADATA_STATUS_ZEAGLE_MULTIPLE_PACKETS_WITH_BAD_CRC = 2
METADATA_STATUS_ZEAGLE_PACKET_LOST = 3


class StreamHydraSco(KalsimStream):
    '''
    Hydra sco streams

    Error insertion process

    Every time a packet has to be inserted in a sco source stream, its data is replicated in as
    many packets as defined (packet_number).
    Of those packets per is applied to each packet, if per is unsuccessful the packet is dropped
    Of those not dropped packets, ber is applied to every single bit in every packet,
    if ber is unsuccessful then that bit in that packet is changed

    **Standard format**

    +--------------------------------+---------------------------+---------------------------------+
    | Packet                         | Status                    | Data                            |
    +================================+===========================+=================================+
    | All packets have been received | STATUS_OK *               | Original data                   |
    | and have not bit errors        |                           |                                 |
    +--------------------------------+---------------------------+---------------------------------+
    | No packets actually received   | STATUS_OK                 | Original data                   |
    | have bit errors                |                           |                                 |
    +--------------------------------+---------------------------+---------------------------------+
    | All packets are lost           | STATUS_NOTHING_RECEIVED * | All bits to 0                   |
    +--------------------------------+---------------------------+---------------------------------+
    | Only one packet received       | STATUS_CRC_ERROR *        | Data after applied bit errors   |
    | and it has bit errors          |                           |                                 |
    +--------------------------------+---------------------------+---------------------------------+
    | Multiple packets received and  | STATUS_CRC_ERROR          | Majority voting of rx packets   |
    | at least one has bit errors    |                           |                                 |
    +--------------------------------+---------------------------+---------------------------------+
    It should be used only with packet_number set to 1, so combinations with * are the only ones
    that apply.

    **Zeagle format**

    +--------------------------------+-------------------------------+--------------------------------+--------------------------------+
    | Packet                         | Status                        | Data                           | Bit error pattern              |
    +================================+===============================+================================+================================+
    | All packets have been received | GOOD_CRC                      | Original data                  | All bits 0                     |
    | and have not bit errors        |                               |                                |                                |
    +--------------------------------+-------------------------------+--------------------------------+--------------------------------+
    | No packets actually received   | GOOD_CRC                      | Original data                  | All bits 0                     |
    | have bit errors                |                               |                                |                                |
    +--------------------------------+-------------------------------+--------------------------------+--------------------------------+
    | All packets are lost           | PACKET_LOST                   | All bits to 0                  | All bits 1                     |
    +--------------------------------+-------------------------------+--------------------------------+--------------------------------+
    | Only one packet received       | ONE_PACKET_CRC_ERROR          | Data after applied bit errors  | All bits 1                     |
    | and it has bit errors          |                               |                                |                                |
    +--------------------------------+-------------------------------+--------------------------------+--------------------------------+
    | Multiple packets received and  | MULTIPLE_PACKETS_WITH_BAD_CRC | Majority voting of rx packets  | All bits that have discrepancy |
    | at least one has bit errors    |                               |                                | across data packets set to 1   |
    +--------------------------------+-------------------------------+--------------------------------+--------------------------------+
    Bit error count is the number of 1s in bit error pattern always

    Majority voting

    Is simply to determine each bit depending of all the corresponding bits of all available
    (not dropped) packets. If there are more 1s than 0s, more 0s than 1s or equal,
    the output will be 1, 0 or 50% 1 or 0.

    *stream_type* can be:

        - *source*, pushing data to the uut.
        - *sink*, extracting data from the uut.

    - ``backing`` defines the origin (for sources) or destination (for sinks) of data,
      could be file or data. In the case of data it cones or goes to an external software component,
      this allow to have loops where a sink stream loops back to a source stream.

    *backing=file*

    - *filename* is the file to back the stream (mandatory).

        - raw files only store audio data but no information about number of channels,
          sampling frequency or data format.
          This information (*channels*, *sample_rate*, *sample_width*) has to be supplied
        - wav files store number of channels, sampling frequency and sample data format,
          Note that if *sample_rate* is provided then information in the file is overriden
        - qwav files store number of channels, sampling frequency, sample data format, optional
          metadata and optional packet based information
    - *channels* is the number of channels/streams in the audio file,
      only for source streams, sink streams are always created with 1 channel (optional default=1).
    - *channel* is the channel index in the audio file,
      only for source streams, sink streams are always created with 1 channel (optional default=0).
    - *sample_rate* is the sampling frequency in hertzs,
      for raw source files (mandatory), wav source files (optional) and all sink files (mandatory).
      If it is 0 it means as fast as possible.
    - *sample_width* is the number of bits per sample,
      for raw source files (mandatory) and all sink files (mandatory).
    - *frame_size* is the number of samples per transaction,
      valid for all file types and stream types (optional, default=1). Currently unused
    - *delay* indicates the delay between the stream start command and the actual start in seconds,
      only for source streams (optional default=0.0)
    - *loop* indicates the number of times the source is played, when the source gets to end of file
      it is rewinded and replayed, only for source streams (optional default=1)
    - *seed* seed to compute error occurrence, integer value or None for create automatically
      only for source streams (optional default=None)
    - *packet_number* number of packets (with retransmission) available,
      only for source streams (optional default=1)
    - *per* packet error rate, in percentage
      only for source streams (optional default=0.0)
    - *ber* bit error rate, in percentage
      only for source streams (optional default=0.0)
    - *never_scheduled_error* percentage of never scheduled packets error to insert, in percentage
      only for source streams (optional default=0.0)
    - *never_scheduled_error_packets* number of slots that make the never scheduled error
      only for source streams (optional minimum=2, default=2)
    - *data_error* if set send garbage data with crc, nothing received and never scheduled errors
      else do not send any data
    - *metadata_enable* indicates if metadata should be sent alongside the audio data, this metadata
      will be auto-generated if the format is not qwav or extracted from the qwav file,
      for source streams (optional default=False)
    - *metadata_format* indicates if metadata format to be used, this only applies is metadata is
      enabled, valid values are 'standard' and 'zeagle'
      for source streams (optional default='standard)
    - *stream* indicates the parent stream, the sco services supports one tx and one rx channel,
      for the second stream in the service this provides the parent sco stream index
    - wallclock_accuracy (float): Wallclock simulation accuracy in part per million. This allows
      the wallclock simulation to not run at the same rate as TIMER_TIME kymera clock,
      sco insertion/extraction is synchronised with wallclock accuracy
    - role (str): Bluetooth link role, master or slave
    - air_compression_factor (int): SPS to/from-air compression factor,
      this parameter is passed to the DUT and not used by KATS
      (mandatory, 1 or 2)
    - air_buffer_size (int): SPS to/from-air buffer size in octets,
      this parameter is passed to the DUT and not used by KATS
      (optional)
    - air_packet_length (int): SPS to/from-air packet length in octets
      this parameter is passed to the DUT and not used by KATS
      (mandatory)
    - tesco (int): SPS BT link TeSCO (interval between transmissions)
      this parameter is used by KATS to compute insertion time and passed to the DUT
      (mandatory, 2, 4, 6, 8, 10, 12, 14, 16, 18)
    - wesco (int): SPS BT link WeSCO (retransmission window), this is only used to compute to air
      and from air latency parameters, those parameters are directly passed to the DUT and not used
      in any other case
      (mandatory)
    - slot_occupancy (int): Number of slots on the BT physical channel taken by one packet,
      this parameters is used to compute the exact slot to insert or extract data
      (optional, default=1, 1 or 3)

    *backing=data*

    - *sample_width* is the number of bits per sample,
      valid for all file types and stream types (mandatory).
    - *frame_size* is the number of samples per transaction,
      only used in sink streams (optional default=1).
    - *callback_data_received*, function to be invoked when data is available,
      only used in sink streams (mandatory).
    - *seed* seed to compute error occurrence, integer value or None for create automatically
      only for source streams (optional default=None)
    - *packet_number* number of packets (with retransmission) available,
      only for source streams (optional default=1)
    - *per* packet error rate, in percentage
      only for source streams (optional default=0.0)
    - *ber* bit error rate, in percentage
      only for source streams (optional default=0.0)
    - *never_scheduled_error* percentage of never scheduled packets error to insert, in percentage
      only for source streams (optional default=0.0)
    - *never_scheduled_error_packets* number of slots that make the never scheduled error
      only for source streams (optional minimum=2, default=2)
    - *data_error* if set send garbage data with crc, nothing received and never scheduled errors
      else do not send any data
    - *metadata_enable* indicates if metadata should be sent alongside the audio data, this metadata
      will be auto-generated if the format is not qwav or extracted from the qwav file,
      for source streams (optional default=False)
    - *metadata_format* indicates if metadata format to be used, this only applies is metadata is
      enabled, valid values are 'standard' and 'zeagle'
      for source streams (optional default='standard)
    - *stream* indicates the parent stream, the sco services supports one tx and one rx channel,
      for the second stream in the service this provides the parent sco stream index
    - wallclock_accuracy (float): Wallclock simulation accuracy in part per million. This allows
      the wallclock simulation to not run at the same rate as TIMER_TIME kymera clock,
      sco insertion/extraction is synchronised with wallclock accuracy
    - role (str): Bluetooth link role, master or slave
    - air_compression_factor (int): SPS to/from-air compression factor,
      this parameter is passed to the DUT and not used by KATS
      (mandatory, 1 or 2)
    - air_buffer_size (int): SPS to/from-air buffer size in octets,
      this parameter is passed to the DUT and not used by KATS
      (optional)
    - air_packet_length (int): SPS to/from-air packet length in octets
      this parameter is passed to the DUT and not used by KATS
      (mandatory)
    - tesco (int): SPS BT link TeSCO (interval between transmissions)
      this parameter is used by KATS to compute insertion time and passed to the DUT
      (mandatory, 2, 4, 6, 8, 10, 12, 14, 16, 18)
    - wesco (int): SPS BT link WeSCO (retransmission window), this is only used to compute to air
      and from air latency parameters, those parameters are directly passed to the DUT and not used
      in any other case
      (mandatory)
    - slot_occupancy (int): Number of slots on the BT physical channel taken by one packet,
      this parameters is used to compute the exact slot to insert or extract data
      (optional, default=1, 1 or 3)
    - *callback_consume*, function to be invoked when data is available,
      only used in sink streams (mandatory) but can be set in the config method.

    Args:
        stream_type (str): Type of stream source or sink
        filename (str): Filename to back the stream
        channels (int): Number of channels in file
        channel (int): Channel in the file
        sample_rate (int): Sample rate
        sample_width (int): Number of bit per sample
        frame_size (int): Number of frames per transfer
        delay (float): Delay in seconds from start to real start
        loop (int): Number of loops
        callback_data_received (function(int)): Callback function when data is received
    '''

    platform = ['crescendo', 'stre', 'streplus', 'mora']
    interface = 'sco'

    def __init__(self, stream_type, **kwargs):
        self._log = logging.getLogger(__name__) if not hasattr(self, '_log') else self._log
        inherit_docstring(self)

        DefaultValidatingDraft4Validator(PARAM_SCHEMA).validate(kwargs)

        self.__helper = argparse.Namespace()  # helper modules
        self.__helper.uut = get_instance('uut')
        self.__helper.kalcmd = get_instance('kalcmd')
        self.__helper.hydra_prot = get_instance('hydra_protocol')

        self.__config = argparse.Namespace()  # configuration values
        self.__config.backing = kwargs.pop(BACKING)
        self.__config.callback_data_received = None  # backing=data sink stream callback
        self.__config.delay = None
        self.__config.seed = kwargs.pop(SEED)
        self.__config.packet_number = kwargs.pop(PACKET_NUMBER)
        self.__config.per = kwargs.pop(PER)
        self.__config.ber = kwargs.pop(BER)
        self.__config.never_scheduled_error = kwargs.pop(NEVER_SCHEDULED_ERROR)
        self.__config.never_scheduled_error_packets = kwargs.pop(NEVER_SCHEDULED_ERROR_PACKETS)
        self.__config.data_error = kwargs.pop(DATA_ERROR)
        self.__config.metadata_enable = kwargs.pop(METADATA_ENABLE)
        self.__config.metadata_format = kwargs.pop(METADATA_FORMAT)
        self.__config.stream = kwargs.pop(STREAM)
        self.__config.wallclock_accuracy = kwargs.get('wallclock_accuracy', 0.0)

        self.__config.service_tag = kwargs.pop(SERVICE_TAG)

        self.__data = argparse.Namespace()
        self.__data.loop_timer_id = None  # timer when looping
        self.__data.loop = 1
        self.__data.source_packetiser = None  # source with packet based streaming
        self.__data.source_metadata = None
        self.__data.source_timer_id = None
        self.__data.source_timer_remain = 0.0  # source manual timer remainder
        self.__data.sink_timer_id = None  # sink manual streaming timer id
        self.__data.sink_timer_remain = 0.0  # sink manual timer remainder
        self.__data.sink_audio_buffer = None  # sink manual streaming audio data buffer
        self.__data.stream2 = None
        self.__data.bt_clock = 0
        self.__data.audio_data = None
        self.__data.source_packetiser = None  # source with packet based streaming
        self.__data.total_samples = 0
        self.__data.sent_samples = 0
        self.__data.buffer_size = None
        self.__data.buffer_width = None
        self.__data.random = None
        self.__data.sent_packets = 0
        self.__data.sent_per = 0
        self.__data.sent_ber = 0
        self.__data.sent_crc_error = 0
        self.__data.sent_nothing_received_error = 0
        self.__data.sent_never_scheduled_error = 0
        self.__data.never_scheduled_cnt = 0
        self.__data.buffer = []

        self.__data.sco_service = None

        self._init_seed()

        if self.__config.backing == BACKING_FILE:
            self.__config.filename = kwargs.pop(FILENAME)
            if stream_type == STREAM_TYPE_SOURCE:
                self.__config.delay = kwargs.pop(DELAY, DELAY_DEFAULT)
            else:
                self.__config.delay = None
            self.__config.loop = kwargs.pop(LOOP, LOOP_DEFAULT)
            self.__config.user_callback_eof = kwargs.get(CALLBACK_EOF, lambda *args, **kwargs: None)

            params = getattr(self, '_init_%s_file' % (stream_type))(kwargs)
            params[CALLBACK_EOF] = self.__eof  # required for loop
        else:  # BACKING_DATA
            if stream_type == STREAM_TYPE_SINK:
                self.__config.sample_rate = kwargs.pop(SAMPLE_RATE)  # needed in sco
            self.__config.sample_width = kwargs.pop(SAMPLE_WIDTH)
            if stream_type == STREAM_TYPE_SINK:
                self.__config.frame_size = kwargs.pop(FRAME_SIZE, FRAME_SIZE_DEFAULT)
                self.__config.callback_consume = kwargs.pop(CALLBACK_CONSUME, None)
            else:
                self.__config.frame_size = None
            self.__config.loop = 1

            params = get_user_stream_config(self.__config.sample_width)

        self.__parameters = {}
        self.__parameters[HYDRA_TYPE] = HYDRA_TYPE_SUBSYSTEM
        self.__parameters[HYDRA_BAC_HANDLE] = 0  # we will modify this when we know the handle

        self._sco_kwargs = copy.deepcopy(kwargs)

        super(StreamHydraSco, self).__init__(stream_type, **params)

    def _compute_period(self, period, remainder, resolution=1e-6):
        '''
        Helper function to compute next timer period based of the nominal period, a remainder
        value and the resolution of the timer

        Example:
            Overflow period example::

                period = 0.00000166667
                new_period, remain = self._compute_period(period, 0)
                print(new_period, remain)
                new_period, remain = self._compute_period(period, remain)
                print(new_period, remain)

        Args:
            period (float): Timer nominal period
            remainder (float): Carried remainder from previous timer
            resolution (float): Timer resolution f.i. 0.001 for msecs, 0.000001 for usecs

        Returns:
            tuple:
                float: Timer period
                float: Carried remainder

        '''
        inv_res = 1.0 / resolution
        period_new = int((period + remainder) * inv_res) / inv_res
        remainder_new = (int(int((period + remainder) * 1e9) % int(1e9 / inv_res))) / 1e9
        return period_new, remainder_new

    def _init_source_file(self, kwargs):
        channels = kwargs.pop(CHANNELS, CHANNELS_DEFAULT)
        channel = kwargs.pop(CHANNEL, CHANNEL_DEFAULT)
        self.__config.sample_rate = kwargs.pop(SAMPLE_RATE, None)
        self.__config.sample_width = kwargs.pop(SAMPLE_WIDTH, None)
        self.__config.frame_size = kwargs.pop(FRAME_SIZE, FRAME_SIZE_DEFAULT)

        audio_kwargs = {
            'channels': channels,  # some formats do not require
            'sample_rate': self.__config.sample_rate,  # some formats do not require
            'sample_width': self.__config.sample_width,  # some formats do not require
            'allow_encoded': False,
        }
        audio_instance = audio_get_instance(self.__config.filename, **audio_kwargs)
        channels = audio_instance.get_audio_stream_num()
        # we allow overriding sample_rate from what is in the file
        self.__config.sample_width = audio_instance.get_audio_stream_sample_width(channel)

        if channel >= channels:
            raise RuntimeError('channels:%s channel:%s inconsistency' % (channels, channel))
        if audio_instance.get_audio_stream_sample_rate(channel) is None:
            raise RuntimeError('stream filename:%s sample_rate not set' % (self.__config.filename))
        if self.__config.sample_width is None:
            raise RuntimeError('stream filename:%s sample_width not set' % (self.__config.filename))

        # FIXME here we are only supporting streaming manually with kats
        # might be interesting to add kalsim support in the future whenever is possible
        # check if the format supports packet_info and that packet_info actually is there
        audio_data = audio_instance.get_audio_stream_data(channel)
        if (hasattr(audio_instance, 'get_packet_data_size') and
                audio_instance.get_packet_data_size('audio', channel)):
            packet_info = audio_instance.get_packet_data('audio', channel)
            self.__data.source_packetiser = Packetiser(self, audio_data, packet_info)
        else:
            self.__data.audio_data = audio_data
            self.__data.total_samples = len(audio_instance.get_audio_stream_data(channel))
        params = get_user_stream_config(self.__config.sample_width)
        params[STREAM_NAME] = self.__config.filename
        params[STREAM_RATE] = self.__config.sample_rate  # pointless in qwav with packet_info
        params[STREAM_DATA_WIDTH] = self.__config.sample_width

        del audio_instance
        return params

    def _init_sink_file(self, kwargs):
        self.__config.channels = 1
        self.__config.channel = 0
        self.__config.sample_rate = kwargs.pop(SAMPLE_RATE)
        self.__config.sample_width = kwargs.pop(SAMPLE_WIDTH)
        self.__config.frame_size = kwargs.pop(FRAME_SIZE, FRAME_SIZE_DEFAULT)

        # As we have to check received packet timing is correct and discard or insert packets
        # we need to be able to extract data at all times so user streams are required
        self.__data.sink_audio_buffer = []
        params = get_user_stream_config(self.__config.sample_width)
        params[STREAM_NAME] = self.__config.filename
        params[STREAM_RATE] = self.__config.sample_rate
        params[STREAM_DATA_WIDTH] = self.__config.sample_width

        return params

    def _start(self, timer_id):
        _ = timer_id
        self.__data.loop_timer_id = None
        super(StreamHydraSco, self).start()

        if self.get_type() == STREAM_TYPE_SOURCE:
            self.__data.sent_packets = 0
            self.__data.sent_per = 0
            self.__data.sent_ber = 0
            self.__data.sent_crc_error = 0
            self.__data.sent_nothing_received_error = 0
            self.__data.sent_never_scheduled_error = 0
            self.__data.never_scheduled_cnt = 0
            self.__data.buffer = []

            if self.__config.backing == BACKING_FILE:
                self.__data.sent_samples = 0
                self.__data.sco_service.start_channel(1, partial(self._data_transmit, 0))
            else:
                # we receive data with calls to consume
                pass
        else:
            aux_handle = self.__data.sco_service.get_to_air_handle()
            self.__data.buffer_size = self.__helper.kalcmd.get_buffer_size(aux_handle)
            self.__data.buffer_width = self.__helper.kalcmd.get_handle_sample_size(aux_handle)
            if self.__config.backing == BACKING_FILE:
                self.__data.sco_service.start_channel(0, partial(self._data_received, 0))
            else:  # backing=data
                # we receive data from sco endpoint, but we need to poll it
                self.__data.sco_service.start_channel(0, partial(self._data_received, 0))

    def _init_seed(self):
        if self.__config.seed is None:
            self.__config.seed = random.randrange(2 ** 32 - 1)
        self._log.info('random with seed:%s', self.__config.seed)
        self.__data.random = random.Random(self.__config.seed)

    def _compute_byte_metadata(self, data_length, status=METADATA_STATUS_OK):
        # compute metadata in 8 bit format
        metadata = [METADATA_SYNC_WORD, 5, data_length, status, self.__data.bt_clock & 0xFFFF]
        return list(bytearray(struct.pack('<%sH' % len(metadata), *metadata)))

    def _generate_packet(self, data):
        '''
        Build the actual packet to be sent

        Args:
            data (list[int]): Packet data (no metadata)

        Returns:
            list[int]: Packet data (with optional metadata including error insertion)
        '''

        # ensure data is a bytearray
        if self.__config.sample_width == 16:
            data = list(bytearray(struct.pack('<%sh' % (len(data)), *data)))

        data_out = []
        never_val = 100 * self.__data.random.random()

        # check if we are already in never scheduled mode
        if self.__data.never_scheduled_cnt:
            self.__data.never_scheduled_cnt += 1
            if self.__data.never_scheduled_cnt >= self.__config.never_scheduled_error_packets:
                self.__data.buffer += data
                if self.__config.metadata_enable:
                    data_out = self._compute_byte_metadata(len(self.__data.buffer),
                                                           METADATA_STATUS_NEVER_SCHEDULED)
                data_out += copy.deepcopy(self.__data.buffer)
                self.__data.buffer = []
                self.__data.never_scheduled_cnt = 0
            else:
                self._log.warning('sco data skipping packet as we are in never scheduled mode')
                self.__data.buffer += [0] * len(data) if self.__config.data_error else []

        # check if we have to get into never scheduled mode
        elif never_val < self.__config.never_scheduled_error:  # compute never scheduled
            self._log.warning('sco data inserting never scheduled error')
            self.__data.never_scheduled_cnt = 1
            self.__data.sent_never_scheduled_error += 1
            self.__data.buffer = [0] * len(data) if self.__config.data_error else []

        # insert errors
        else:
            sco_error = ScoError(
                rnd=self.__data.random,
                packet_number=self.__config.packet_number,
                per=self.__config.per,
                ber=self.__config.ber)
            data_out, packets, bit_error = sco_error.insert_error(data)
            self.__data.sent_per += self.__config.packet_number - len(packets)
            self.__data.sent_ber += bit_error.count(1)

            # generate metadata
            metadata = []
            pos_data = []
            if self.__config.metadata_enable:
                if self.__config.metadata_format == METADATA_FORMAT_STANDARD:
                    status = METADATA_STATUS_OK
                    if not len(packets):
                        self.__data.sent_nothing_received_error += 1
                        status = METADATA_STATUS_NOTHING_RECEIVED
                    elif any(bit_error):
                        self.__data.sent_crc_error += 1
                        status = METADATA_STATUS_CRC_ERROR

                    if status != METADATA_STATUS_OK:
                        data_out = data_out if self.__config.data_error else []
                    metadata = self._compute_byte_metadata(len(data_out), status=status)
                else:
                    metadata = self._compute_byte_metadata(2 * len(data_out) + 2,
                                                           status=METADATA_STATUS_ZEAGLE)
                    status = METADATA_STATUS_ZEAGLE_OK
                    if not len(packets):
                        self.__data.sent_nothing_received_error += 1
                        status = METADATA_STATUS_ZEAGLE_PACKET_LOST
                    elif len(packets) == 1 and any(bit_error):
                        self.__data.sent_crc_error += 1
                        status = METADATA_STATUS_ZEAGLE_ONE_PACKET_WITH_BAD_CRC
                    elif len(packets) > 2 and any(bit_error):
                        self.__data.sent_crc_error += 1
                        status = METADATA_STATUS_ZEAGLE_MULTIPLE_PACKETS_WITH_BAD_CRC

                    weak_header = sum(bit_error) | (status << 14)
                    pos_data = [weak_header & 0xFF, (weak_header >> 8) & 0xFF] + \
                               sco_error.bit_to_byte(bit_error)

            data_out = metadata + data_out + pos_data

        # convert data back to original format (if needed)
        if self.__config.sample_width == 16:
            data_out = list(struct.unpack('<%sh' % int(len(data_out) / 2), bytearray(data_out)))

        # FIXME this is enforcing that if set up as data backed there are consume calls
        # keeping the right sample_rate/sample_width/tesco parameters
        self.__data.bt_clock += self.__data.sco_service.get_tesco() * 2

        return data_out

    def _data_transmit(self, timer_id):
        _ = timer_id
        self.__data.source_timer_id = None

        if self.get_state() == STATE_STARTED:
            if self.__data.source_packetiser:  # with packetiser support
                self.__data.source_packetiser.start()
            else:
                data_length = int(
                    self.__config.sample_rate * self.__data.sco_service.get_tesco() * TIMESLOT_DURATION)

                if self.__data.sent_samples < self.__data.total_samples:
                    if (self.__data.total_samples - self.__data.sent_samples) < data_length:
                        data_length = self.__data.total_samples - self.__data.sent_samples

                    data = list(self.__data.audio_data[
                                self.__data.sent_samples:self.__data.sent_samples + data_length])
                    data_to_send = self._generate_packet(data)
                    if data_to_send:
                        self.insert(data_to_send)
                        self.__data.sent_packets += 1

                    self.__data.sent_samples += data_length

                    period = ((1e6 + self.__config.wallclock_accuracy) / 1e6 *
                              TIMESLOT_DURATION * self.__data.sco_service.get_tesco())
                    period, self.__data.source_timer_remain = self._compute_period(
                        period, self.__data.source_timer_remain)
                    self.__data.source_timer_id = self.__helper.uut.timer_add_relative(
                        period, self._data_transmit)
                else:
                    self.eof()

    def _get_stats(self):
        wr_handle = self.__data.sco_service.get_to_air_write_handle()
        aux_handle = self.__data.sco_service.get_to_air_handle()
        used, free = get_buffer_stats(aux_handle, wr_handle,
                                      buffer_size=self.__data.buffer_size,
                                      buffer_width=self.__data.buffer_width)
        used = int((used * 8) / self.get_data_width())
        free = int((free * 8) / self.get_data_width())
        return used, free

    def _check_packet(self, data, expected_data_length):
        '''
        Check if a received packet (with optional metadata as configured in the stream) is valid
        Only metadata in standard format is supported in kymera

        Args:
            data (list[int]): Data received
            expected_data_length (int): Expected data length in samples

        Returns:
            list[int]: User data in packet, not including metadata

        '''
        if self.__config.metadata_enable:
            if self.__config.sample_width == 8:
                if len(data) < 10:
                    self._log.warning('received packet length:%s less than metadata packet size',
                                      len(data))
                    return []
                metadata = list(struct.unpack('<5H', bytearray(data[:10])))
                data = data[10:]
            elif self.__config.sample_width == 16:
                if len(data) < 10:
                    self._log.warning('received packet length:%s less than metadata packet size',
                                      len(data))
                    return []
                metadata = list(data[:5])
                data = data[5:]

            if metadata[0] != METADATA_SYNC_WORD:
                self._log.warning('received invalid metadata sync word:0x%04x', metadata[0])
                return []

            expected_bytes = int(expected_data_length * (self.get_data_width() / 8))
            if metadata[2] != expected_bytes:
                self._log.warning('received invalid metadata packet_length:%s expected:%s',
                                  metadata[2], expected_bytes)
                return []

        if len(data) != expected_data_length:
            self._log.warning('received invalid data_length:%s expected:%s',
                              len(data), expected_data_length)
            return []

        return data

    def _data_received(self, timer_id):
        _ = timer_id
        self.__data.sink_timer_id = None

        data_length = int(self.__data.sco_service.get_frame_length(0) / (self.get_data_width() / 8))
        metadata_length = 0
        if self.__config.metadata_enable:
            metadata_length += int(10 / (self.get_data_width() / 8))
        total_length = metadata_length + data_length

        if self.get_state() == STATE_STARTED:
            # read the buffer statistics to see how many samples are available
            # if there is the right amount of audio then read it.
            # if there is not enough audio and streaming has not started then ignore
            # if there is not enough audio and streaming has started then
            #   assume an all silence packet and log it
            #   (test checks with pesq should be able to catch it).
            # if there are multiple packets then discard all but the last and log
            # if the number of available bytes does not match a multiple of packet size then log it.
            data = []
            used, free = self._get_stats()
            self._log.info('sco data expected:%s available:%s space:%s', total_length, used, free)
            if used < total_length:
                self._log.warning('sco data not enough available avail:%s expected:%s',
                                  used, total_length)
            else:
                if used > total_length:
                    self._log.warning('sco data more than enough available avail:%s expected:%s',
                                      used, total_length)
                    if used % total_length != 0:
                        self._log.warning('sco data avail:%s is not a multiple of expected:%s',
                                          used, total_length)
                    while used >= 2 * total_length:
                        _ = self.extract(total_length)
                        used -= total_length

                # FIXME not using frame_size
                tmp = self.extract(total_length)
                data = self._check_packet(tmp, data_length)

            if self.__config.backing == BACKING_FILE:
                if data:
                    self.__data.sink_audio_buffer += data
            else:
                if data and self.__config.callback_consume:
                    self.__config.callback_consume[0](data=data)

            period = ((1e6 + self.__config.wallclock_accuracy) / 1e6 *
                      data_length / self.__config.sample_rate)
            period, self.__data.sink_timer_remain = self._compute_period(
                period, self.__data.sink_timer_remain)
            self.__data.sink_timer_id = self.__helper.uut.timer_add_relative(
                period, self._data_received)

    def __eof(self):
        '''
        This is our own callback for an End of File.

        In the case of source file backed streams we install this callback handler when there is a
        stream eof. This will cause to check if there are any additional loops to be done and
        in case there are rewind the stream and replay
        '''
        self.__data.loop = 0 if self.__data.loop <= 1 else self.__data.loop - 1
        if self.__data.loop > 0:
            self.stop()
            self._start(0)
        else:
            self.__config.user_callback_eof()

    @log_input(logging.INFO)
    def create(self):
        '''
        Start service and create stream

        TODO: If stream_type of a SCO Processing Service instance is easily available,
        raise a RuntimeError if we are trying to start two instances with the same
        stream_type and hci_handle.
        '''
        if self.__config.stream is not None:  # FIXME and we are not the first stream (murphy compat)
            stream = get_instance('stream_sco')
            if stream.get_type() == self.get_type():
                raise RuntimeError('trying to start two sco streams of same type')
            self.__data.sco_service = stream.get_sco_service()
        else:
            if self.__config.service_tag is not None:
                service_tag = self.__config.service_tag
            else:
                service_tag = get_instance_num('sco_processing_service') + 100

            self.__data.sco_service = HydraScoProcessingService(service_tag=service_tag,
                                                                **self._sco_kwargs)
            register_instance('sco_processing_service', self.__data.sco_service)
            self.__data.sco_service.start()
            self.__data.sco_service.config()

        return super(StreamHydraSco, self).create()

    @log_input(logging.INFO)
    def config(self, **kwargs):
        if CALLBACK_CONSUME in kwargs:
            self.__config.callback_consume = kwargs.pop(CALLBACK_CONSUME)
            if not isinstance(self.__config.callback_consume, list):
                raise RuntimeError('callback_consume:%s invalid' % (self.__config.callback_consume))
            if len(self.__config.callback_consume) != 1:
                raise RuntimeError('callback_consume:%s invalid' % (self.__config.callback_consume))

        if self.get_type() == STREAM_TYPE_SOURCE:
            bac_handle = self.__data.sco_service.get_from_air_handle()
        else:
            bac_handle = self.__data.sco_service.get_to_air_handle()

        self.__parameters[HYDRA_BAC_HANDLE] = bac_handle

        for key in self.__parameters:
            self._config_param(key, self.__parameters[key])

        super(StreamHydraSco, self).config(**kwargs)

        for key in self.__parameters:
            _ = self.query(key)

    @log_input(logging.INFO)
    def start(self):
        '''
        Start streaming

        Notes:
        Before we start streaming:

        - we check if Audio FW is ready to supply or consume data - check_for_channels_ready()
        - start BT clock if Audio ready
        - Audio FW ready is indicated by a 'run state' message which should have
          already been handled by SCO Processing Service

        Raises:
            RuntimeError: - If Audio FW not ready to process data
        '''

        self.__data.loop = self.__config.loop
        if self.__config.delay:
            self._log.info('delaying start for %s seconds', self.__config.delay)
            self.__data.loop_timer_id = self.__helper.uut.timer_add_relative(self.__config.delay,
                                                                             callback=self._start)
        else:
            self._start(0)

    @log_input(logging.INFO)
    def stop(self):
        '''
        Stop stream
        '''

        if self.get_type() == STREAM_TYPE_SOURCE:
            self._log.info(
                'packets sent:%s per:%s ber:%s crc errors:%s nothing received errors:%s never scheduled errors:%s seed:%s',
                self.__data.sent_packets,
                self.__data.sent_per,
                self.__data.sent_ber,
                self.__data.sent_crc_error,
                self.__data.sent_nothing_received_error,
                self.__data.sent_never_scheduled_error,
                self.__config.seed)

        if self.__data.loop_timer_id is not None:
            self.__helper.uut.timer_cancel(self.__data.loop_timer_id)
            self.__data.loop_timer_id = None
        if self.__data.source_packetiser:
            self.__data.source_packetiser.stop()
        if self.__data.source_timer_id:
            self.__helper.uut.timer_cancel(self.__data.source_timer_id)
            self.__data.source_timer_id = None
        if self.__data.sink_timer_id:
            self.__helper.uut.timer_cancel(self.__data.sink_timer_id)
            self.__data.sink_timer_id = None

        super(StreamHydraSco, self).stop()

    @log_input(logging.INFO)
    def destroy(self):

        if self.__config.backing == BACKING_FILE and self.__data.sink_audio_buffer is not None:
            self._log.info('creating file %s', self.__config.filename)
            audio_instance = audio_get_instance(self.__config.filename, 'w', allow_encoded=False)
            audio_instance.add_audio_stream(self.__config.sample_rate,
                                            self.__config.sample_width,
                                            self.__data.sink_audio_buffer)
            audio_instance.write()
            del audio_instance
            self.__data.sink_audio_buffer = []

        # Note that stopping the service will destroy the endpoint associated with it
        # hence we delay stopping the service until we are destroying the stream
        if self.__data.sco_service.check_started():
            self.__data.sco_service.stop()
        super(StreamHydraSco, self).destroy()

        # FIXME unregister instance, be careful with multiple channels

    def check_active(self):
        # FIXME this code assumes that we are connected after a kymera graph
        # if we are not it should be removed
        # a similar thing happens with a2dp and pcm streams
        if self.get_type() == STREAM_TYPE_SOURCE and self.__config.backing == BACKING_DATA:
            return False
        return super(StreamHydraSco, self).check_active()

    def consume(self, input_num, data):
        if (input_num == 0 and
                self.get_type() == STREAM_TYPE_SOURCE and
                self.__config.backing == BACKING_DATA):
            data_to_send = self._generate_packet(data)
            if data_to_send:
                self.insert(data_to_send)

    def eof_detected(self, input_num):
        if (input_num == 0 and
                self.get_type() == STREAM_TYPE_SOURCE and
                self.__config.backing == BACKING_DATA):
            self.eof()

    def get_endpoint_id(self):
        '''
        Get endpoint id

        Returns:
            int: Endpoint id
        '''
        return self.__helper.audio_hydra.get_endpoint_id()

    def get_sco_service(self):
        '''
        Return sco service instance

        Returns:
            HydraScoProcessingService: sco service
        '''
        return self.__data.sco_service

    def get_hci_handle(self):
        '''
        Get hci handle

        Returns:
            int: hci handle
        '''
        return self.__data.sco_service.get_hci_handle()
