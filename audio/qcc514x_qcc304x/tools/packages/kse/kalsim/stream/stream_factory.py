#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Stream factory class
'''

import logging

from kats.core.stream_base import StreamBase
from kats.framework.library.factory import find_subclass
from kats.framework.library.log import log_input, log_output, log_exception
from kats.library.module import get_parent_module
from kats.library.registry import register_instance, unregister_instance

PLUGIN_PLATFORM = 'platform'
PLUGIN_CLASS = 'class'


class StreamFactory(object):
    '''
    Stream factory

    This class handles streams.
    It supports a factory interface, where stream are classified in platforms
    and there could be multiple streams for each platform and one stream could support multiple
    platforms.
    When this class starts it autodiscovers all the stream types available for
    a set of platforms.
    Discovered streams should subclass StreamBase in a subpackage.

    .. code-block:: python

        kwargs = {
            "backing": "file",
            "filename": "resource/Female_1c_8000_16b.raw",
            "sample_rate": 8000,
            "sample_width": 16
        }

        st = stream.get_instance('pcm', 'source', **kwargs)
        st.create() # create stream
        st.config() # send all configuration messages
        st.start()
        ...
        st.stop()
        st.destroy()
        stream.put_instance(st)

    Args:
        platform (list[str]): Platform
        kalcmd (kats.instrument.kalcmd.kalcmd.Kalcmd): Instance of class Kalcmd
    '''

    def __init__(self, platform, kalcmd):
        self._log = logging.getLogger(__name__) if not hasattr(self, '_log') else self._log
        self._platform = platform
        self._kalcmd = kalcmd
        self._plugins = {}

        mod_name = get_parent_module(__name__, __file__)
        class_list = find_subclass(mod_name, StreamBase)
        for entry in class_list:
            try:
                if list(set(entry.platform).intersection(platform)):
                    self._register(entry.platform, entry.interface, entry)
            except Exception:  # pylint: disable=broad-except
                pass

    def _register(self, platform, interface, plugin):
        self._log.info('registering stream interface:%s platform:%s class:%s',
                       plugin.interface, platform, plugin.__name__)
        self._plugins[interface] = {
            PLUGIN_PLATFORM: platform,
            PLUGIN_CLASS: plugin
        }

    @log_input(logging.INFO)
    @log_exception
    def get_class(self, interface):
        '''
        Search in the discovered plugins for a stream interface and return its class

        Args:
            interface (str): Stream interface

        Returns:
            any: Stream class

        Raises:
            ValueError: If unable to find stream interface
        '''
        if interface in self._plugins:  # search by name (interface)
            operator = self._plugins[interface][PLUGIN_CLASS]
            return operator
        raise ValueError('unable to find stream interface:%s' % (interface))

    @log_input(logging.INFO)
    def get_instance(self, interface, stream_type, *args, **kwargs):
        '''
        Search in the discovered plugins for an stream interface and return an instance of it

        Args:
            interface (str): Stream interface
            stream_type (str): Stream type name

        Returns:
            any: Stream instance

        Raises:
            ValueError: If unable to find stream interface
        '''
        if interface in self._plugins:
            stream = self._plugins[interface][PLUGIN_CLASS](stream_type,
                                                            *args,
                                                            **kwargs)
            register_instance('stream_' + interface, stream)
            return stream
        raise ValueError('unable to find stream interface:%s' % (interface))

    @log_input(logging.INFO)
    def put_instance(self, stream):
        '''
        Destroy a stream instance

        Args:
            stream (any): Stream instance
        '''
        unregister_instance('stream_' + stream.interface, stream)
        del stream

    @log_output(logging.INFO)
    @log_exception
    def enum_interface(self):
        '''
        Get a list of registered streams

        Returns:
            list[str]: Stream id and names
        '''
        return self._plugins
