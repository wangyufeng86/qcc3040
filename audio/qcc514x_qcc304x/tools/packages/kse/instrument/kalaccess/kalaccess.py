#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Kalaccess instrument
'''

import logging
import os
import sys

from kats.framework.library.instrument import Instrument
from kats.framework.library.log import log_input

INSTRUMENT = 'kalaccess'
TRANSPORT = 'transport'
TRANSPORT_KALSIM = 'kalsim'
SUBSYSTEM = 'subsystem'
PORT = 'port'
PATH = 'path'


class Kalaccess(Instrument):
    '''
    kalimba python tools kalacess instrument.

    This instrument instantiates a kalaccess class instance from kalimba python tools.

    Args:
        transport (str): Transport to use
        subsystem (int): Subsystem to connect to
        port (int): Debugger port (only valid for kalsim transport)
        path (str): Path to kalimba python tools package directory, if not set then the module
            will be imported as kal_python_tools
    '''

    interface = 'kalaccess'
    schema = {
        'type': 'array',
        'minItems': 1,
        'uniqueItems': True,
        'items': {
            'type': 'object',
            # 'required': [],
            'properties': {
                TRANSPORT: {'type': 'string', 'default': TRANSPORT_KALSIM},
                SUBSYSTEM: {'type': 'integer', 'default': 3},
                PORT: {'type': 'integer', 'minimum': 0, 'maximum': 65535, 'default': 31400},
                PATH: {'type': 'string'},
            }
        }
    }

    def __init__(self, transport='kalsim', subsystem=3, port=31400, path=None):
        self._log = logging.getLogger(__name__) if not hasattr(self, '_log') else self._log
        self._log.info('init subsystem:%s port:%s', subsystem, port)

        # kalsim is special as it requires the port in an environment variable
        self._transport = 'kalsim SPIPORT=2' if transport == TRANSPORT_KALSIM else transport
        self._subsystem = subsystem
        self._port = port
        self._path = path

        # specify new kalsim binding
        os.environ["KALSIM_HOSTS"] = "localhost=" + str(self._port)

        if path:
            kal_path = os.path.abspath(path)
        else:
            # even if we have the package installed it is not a real package and does not work
            # well with python 3
            # we need to add its location to the path
            import kal_python_tools
            kal_path = os.path.abspath(os.path.dirname(kal_python_tools.__file__))

        if kal_path not in sys.path:
            sys.path.insert(1, kal_path)

        import imp
        mod = imp.load_source('_kalaccess_', os.path.join(kal_path, 'kalaccess.py'))
        self._kal = mod.Kalaccess()

    @log_input(logging.INFO)
    def connect(self):
        '''
        Connect kalaccess to kalsim

        Raises:
            RuntimeError: If the instrument is already started
        '''
        self._kal.connect(trans=self._transport, subsys=self._subsystem)
        self._kal.run()

    def check_connected(self):
        '''
        Check if instrument is connected to kalsim and ready for operation

        Returns:
            bool: kalaccess connected
        '''
        return self._kal.is_connected()

    @log_input(logging.INFO)
    def disconnect(self):
        '''
        Disconnect instrument.
        '''
        self._kal.disconnect()

    def get_kal_access(self):
        '''
        Get Kalaccess instance

        Returns
            kalaccess.Kalaccess: kalaccess instance
        '''
        return self._kal
