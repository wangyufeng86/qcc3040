#
# Copyright (c) 2018 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Hydra sco error insertion library
'''

import copy
import logging
import random


class ScoError(object):
    '''
    Utility class to generate sco errors

    Args:
        rnd (random.Random): Random module instance
        packet_number (int): Number of packets (transmissions) in each packet
        per (float): Packet error rate, in percentage
        ber (float): Bit error rate, in percentage
    '''

    def __init__(self, rnd=None, packet_number=1, per=0, ber=0):
        self._log = logging.getLogger(__name__) if not hasattr(self, '_log') else self._log
        self._random = rnd if rnd else random.Random()
        self._packet_number = packet_number
        self._per = per
        self._ber = ber

    def _get_data_from_packets(self, packets):
        '''
        This will compute single output data from multiple copies of data from multiple packets

        Args:
            packets (list[list[int]]): Packets with bit data

        Returns:
            list[int]): Output bit data
        '''
        if len(packets) == 1:
            data = packets[0]
        else:
            reorder = zip(*packets)
            data = []
            for entry in reorder:
                if entry.count(0) > entry.count(1):
                    val = 0
                elif entry.count(0) < entry.count(1):
                    val = 1
                else:
                    val = self._random.choice([0, 1])
                data.append(val)
        return data

    def byte_to_bit(self, data):
        '''
        Convert a list of unsigned bytes into a list of bits

        THis will obviously return a list 8 times bigger the size of the input

        Args:
            data (list[int]): Input unsigned byte data

        Returns:
            list[int]: Bit data
        '''
        return [(data_byte >> ind) & 1 for data_byte in data for ind in range(8)]

    def bit_to_byte(self, data):
        '''
        Convert a list of bits into a list of unsigned bytes

        THis will obviously return a list 8 times smaller the size of the input

        Args:
            data (list[int]): Input bit data

        Returns:
            list[int]: Unsigned byte data
        '''
        data_byte = []
        for ind, bit in enumerate(data):
            if (ind % 8) == 0:
                byte = 0
            byte |= bit << (ind % 8)
            if (ind % 8) == 7:
                data_byte.append(byte)
        return data_byte

    def insert_error(self, data):
        '''
        Insert error data

        This method expects and returns data in byte (unsigned mode).
        It is up to the caller to convert before and after call to any other format

        Args:
            data_bit (list[int]): Input unsigned byte data

        Returns:
            tuple:
                data (list[int]): Output unsigned data
                packets (list[list[int]]): Packet unsigned byte data
                bit_error (list[int]): Bit error data
        '''
        data_bit = self.byte_to_bit(data)

        # apply per
        packets = []
        for ind in range(self._packet_number):
            per_val = 100 * self._random.random()
            if per_val < self._per:
                self._log.info('sco data inserting packet error at packet:%s', ind)
            else:
                packets.append(copy.deepcopy(data_bit))

        # apply ber
        bit_error = [0] * len(data_bit)
        for ind, packet in enumerate(packets):
            for ind2, data_bit in enumerate(packet):
                ber_val = 100 * self._random.random()
                if ber_val < self._ber:
                    data_bit = 0 if data_bit else 1
                    packets[ind][ind2] = data_bit
                    bit_error[ind2] = 1
                    self._log.info('sco data inserting bit error at packet:%s bit:%s', ind, ind2)

        # prepare modified data
        if not len(packets):
            data_bit_out = [0] * len(data_bit)
        else:
            data_bit_out = self._get_data_from_packets(packets)

        packets = [self.bit_to_byte(entry) for entry in packets]
        data_out = self.bit_to_byte(data_bit_out)
        return data_out, packets, bit_error
