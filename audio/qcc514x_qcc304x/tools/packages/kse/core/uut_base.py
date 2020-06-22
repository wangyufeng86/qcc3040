#
# Copyright (c) 2018 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
UUT Base class
'''

from abc import ABCMeta, abstractmethod, abstractproperty

from six import add_metaclass


@add_metaclass(ABCMeta)
class UutBase(object):
    '''
    Unit Under Test abstraction
    '''

    def __init__(self, *_, **__):
        pass

    @abstractproperty
    def platform(self):
        '''
        str: Platform name
        '''

    @abstractproperty
    def interface(self):
        '''
        str: Interface name
        '''

    @abstractmethod
    def timer_get_time(self):
        '''
        Get current time.

        Returns:
            float: Current simulation time in seconds
        '''

    @abstractmethod
    def timer_add_relative(self, period, callback=None):
        '''
        Add a relative timer.

        To check for timer expiration use timer_check_expired and/or provide a callback
        with the format.

        In the callback function there should be no blocking operations.
        The format of the callback is:

        .. code-block:: python

            def timer_fired(timer_id):
                pass

        Args:
            period (float): Period in seconds
            callback (func(timer_id)): Callback function to be called when timer expires

        Returns:
            int: Timer id
        '''

    @abstractmethod
    def timer_add_absolute(self, period, callback=None):
        '''
        Add an absolute timer

        To check for timer expiration use timer_check_expired and/or provide a callback
        with the format.

        In the callback function there should be no blocking operations.
        The format of the callback is:

        .. code-block:: python

            def timer_fired(timer_id):
                pass

        Args:
            period (float): Period in seconds
            callback (func(timer_id)): Callback function to be called when timer expires

        Returns:
            int: Timer id
        '''

    @abstractmethod
    def timer_cancel(self, timer_id):
        '''
        Cancel a timer

        Args:
            period (float): Period in seconds

        Returns:
            bool: True if there was a timer and it has been cancelled
        '''

    @abstractmethod
    def timer_check_expired(self, timer_id):
        '''
        Check if a timer has expired

        Args:
            period (float): Period in seconds

        Returns:
            bool: True if there is not a pending timer with the supplied id
        '''

    @abstractmethod
    def timer_wait_relative(self, period):
        '''
        Block until certain time elapses

        Args:
            period (float): Period in seconds
        '''

    @abstractmethod
    def message_register_handler(self, header, callback=None, flt=None):
        '''
        Register handler for send_message and send_recv_message operations.

        Any message received will be sent to a queue belonging to this handler, receive or
        send-receive operations will read from that queue.

        Additionally a callback function can be provided that will be called unconditionally.
        In the callback function there should be no blocking operations.
        The format of the callback is:

        .. code-block:: python

            def message_received(message):
                pass

        Args:
            header (kalcmd_msg_header): Message header
            callback (func(list[int])): Message reception callback
            flt (func): Filtering function receiving one parameter being the message received and
                returning True if the message is accepted in the filter, False otherwise

        Returns:
            dict: Handler
        '''

    @abstractmethod
    def message_unregister_handler(self, handler):
        '''
        Unregister handler for message_send, message_send_recv operations and callback on
        message received.

        Args:
            handler (dict): Message handler
        '''

    @abstractmethod
    def message_send(self, handler, msg):
        '''
        Send a message

        Args:
            handler (dict): Message handler registered with register_handler
            msg (list[int]): Message to send
        '''

    @abstractmethod
    def message_recv(self, handler, timeout=None, flt=None):
        '''
        Received a message

        Args:
            handler (dict): Message handler registered with register_handler
            timeout (float): Timeout for response in seconds, None for default
            flt (func): Filtering function receiving one parameter being the message received and
                returning True if the message is accepted in the filter, False otherwise

        Returns:
            list[int]: Reply
        '''

    @abstractmethod
    def message_send_recv(self, handler, msg, timeout=None, flt=None):
        '''
        Send a message and wait for response

        Example code

        .. code-block:: python

            def filter_by_sequence(seq, payload):
                return len(payload) > 3 and seq == payload[2]

            from functools import partial
            payload = uut.message_send_recv(handler, payload,
                                            flt=partial(filter_by_sequence, sequence_num))


        Args:
            handler (dict): Message handler registered with register_handler
            msg (list[int]): Message to send
            timeout (float): Timeout for response in seconds, None for default
            flt (func): Filtering function receiving one parameter being the message received and
                returning True if the message is accepted in the filter, False otherwise

        Returns:
            list[int]: Reply


        Raises:
            RuntimeError: If there is a response reception timeout
        '''

    @abstractmethod
    def mem_peek(self, memory_space, address, size):
        '''
        Read memory address

        Args:
            memory_space (int): Message type to access (PM, DM)
            address (int): Memory address
            size (int): Read size in bytes

        Returns:
            int: Memory data value
        '''

    @abstractmethod
    def mem_poke(self, memory_space, address, size, value):
        '''
        Write memory address

        Args:
            memory_space (int): Message type to access (PM, DM)
            address (int): Memory address
            size (int): Write size in bytes
            value (int): Memory data value
        '''

    @abstractmethod
    def mem_block_read(self, memory_space, address, num_dwords):
        '''
        Read block memory

        Args:
            memory_space (int): Message type to access (PM, DM)
            address (int): Memory address
            num_dwords (int): Number of 32 bits blocks

        Returns:
            list[int]: Memory data dword values
        '''

    @abstractmethod
    def mem_block_write(self, memory_space, address, dwords):
        '''
        Write block memory

        Args:
            memory_space (int): Message type to access (PM, DM)
            address (int): Memory address
            dwords (list[int]): Memory data dword values
        '''

    @abstractmethod
    def interrupt(self, interrupt):
        '''
        Assert an interrupt in kalimba

        Args:
            interrupt (int): Interrupt number to assert, starting with 0
        '''

    @abstractmethod
    def clock_get_frequency(self):
        '''
        Get current clock frequency

        Returns:
            (float): Clock frequency in hertzs
        '''
