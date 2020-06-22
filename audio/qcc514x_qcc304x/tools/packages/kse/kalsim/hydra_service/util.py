#
# Copyright (c) 2018 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Hydra utility library
'''

from kats.library.registry import get_instance


def get_buffer_stats(read_handle, write_handle, buffer_size=None, buffer_width=None):
    '''
    Get kalcmd buffer statistics

    The values returned by Kalsim for handle offsets and buffer sizes are in
    octets and include all the octets in the buffer, useful or wasted. If the BAC sample
    size is configured to 8|16|24_BIT_UNPACKED a part of the buffer is wasted
    (e.g. 16_BIT_UNPACKED, only half of the buffer contains useful data - the lower
    16 bits of each 32-bit word).

    Args:
        read_handle (int): Read BAC handle
        write_handle (int): Write BAC handle
        buffer_size (int): Buffer size or None to autodetect
        buffer_width (int): Buffer width or None to autodetect

    Returns:
        tuple:
            int: Used bytes
            int: Free bytes
    '''

    kalcmd = get_instance('kalcmd')
    if buffer_size is None:
        buffer_size = kalcmd.get_buffer_size(read_handle)

    if buffer_width is None:
        buffer_width = kalcmd.get_handle_sample_size(read_handle)

    rd_offset = kalcmd.get_handle_offset(read_handle)
    wr_offset = kalcmd.get_handle_offset(write_handle)
    if wr_offset >= rd_offset:
        used = wr_offset - rd_offset
    else:
        used = buffer_size - (rd_offset - wr_offset)
    free = buffer_size - used - 1

    # FIXME this might only work for unpacked data
    used = int((used * buffer_width) / 32)
    free = int((free * buffer_width) / 32)
    return used, free
