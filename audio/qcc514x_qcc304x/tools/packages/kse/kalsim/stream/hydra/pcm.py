#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Hydra pcm streams
'''

from .hw import StreamHw


class StreamPcm(StreamHw):
    '''
    Hydra pcm streams

    *stream_type* can be:

        - *source*, pushing data to the uut.
        - *sink*, extracting data from the uut.

   - ``backing`` defines the origin (for sources) or destination (for sinks) of data,
     could be file or data. In the case of data it cones or goes to an external software component,
     this allow to have loops where a sink stream loops back to a source stream.

    *backing=file*

    - *device* is the mapping between stream and endpoint, being the first pcm source stream
      mapped to the first pcm source endpoint and so on, sources and sinks mapping is independent
      (parameter currently unused)
    - *filename* is the file to back the stream (mandatory).

        - raw files only store audio data but no information about number of channels,
          sampling frequency or data format.
          This information (*channels*, *sample_rate*, *sample_width*) has to be supplied
        - wav files store number of channels, sampling frequency and sample data format.
          Note that if *sample_rate* is provided then information in the file is overriden
        - qwav files store number of channels, sampling frequency, sample data format, optional
          metadata and optional packet based information
    - *channels* is the number of channels/streams in the audio file,
      only for source streams, sink streams are always created with 1 channel (optional default=1).
    - *channel* is the channel index in the audio file,
      only for source streams, sink streams are always created with 1 channel (optional default=0).
    - *sample_rate* is the sampling frequency in hertzs,
      for raw source files (mandatory), wav source files (optional) and all sink files (mandatory).
    - *sample_width* is the number of bits per sample,
      for raw source files (mandatory) and all sink files (mandatory).
    - *frame_size* is the number of samples per transaction,
      valid for all file types and stream types (optional, default=1).
    - *delay* indicates the delay between the stream start command and the actual start in seconds,
      only for source streams (optional default=0.0)
    - *loop* indicates the number of times the source is played, when the source gets to end of file
      it is rewinded and replayed, only for source streams (optional default=1)

    *backing=data*

    - *device* is the mapping between stream and endpoint, being the first pcm source stream
      mapped to the first pcm source endpoint and so on, sources and sinks mapping is independent
      (parameter currently unused)
    - *sample_rate* is the sampling frequency in hertzs,
      all sink files (mandatory).
    - *sample_width* is the number of bits per sample,
      valid for all file types and stream types (mandatory).
    - *frame_size* is the number of samples per transaction,
      only used in sink streams (optional default=1).
    - *callback_consume*, function to be invoked when data is available,
      only used in sink streams (mandatory) but can be set in the config method.

    Args:
        stream_type (str): Type of stream source or sink
        device (int): Currently unused
        filename (str): Filename to back the stream
        channel_number (int): Number of channels in file
        channel (int): Channel in the file
        sample_rate (int): Sample rate
        sample_width (int): Number of bit per sample
        frame_size (int): Number of frames per transfer
        delay (float): Delay in seconds from start to real start
        loop (int): Number of loops
        callback_consume (function(int)): Callback function when data is received
    '''

    platform = ['crescendo', 'stre', 'streplus', 'mora']
    interface = 'pcm'
