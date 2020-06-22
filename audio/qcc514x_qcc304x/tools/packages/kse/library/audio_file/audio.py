#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Audio file helper
'''

import logging
import os

from kats.framework.library.factory import find_subclass
from kats.library.module import get_parent_module
from .audio_base import AudioBase, MODE_DEFAULT

DEFAULT_INTERFACE = '_default_interface_'
REGISTERED_AUDIO_FILE = {DEFAULT_INTERFACE: None}

MOD_NAME = get_parent_module(__name__, __file__)  # get parent module
try:
    CLASS_LIST = find_subclass(MOD_NAME, AudioBase)  # get all available interfaces
    for entry in CLASS_LIST:
        log = logging.getLogger(__name__)

        if not entry.interface in REGISTERED_AUDIO_FILE:
            log.info('registering audio file interface:%s class:%s', entry.interface,
                     entry.__name__)
            REGISTERED_AUDIO_FILE[entry.interface] = entry

            if entry.default_interface:
                if not REGISTERED_AUDIO_FILE[DEFAULT_INTERFACE] and not entry.encoded:
                    REGISTERED_AUDIO_FILE[DEFAULT_INTERFACE] = entry
                else:
                    log.warning('audio file interface:%s class:%s cannot be default',
                                entry.interface, entry.__name__)

        else:
            log.warning(
                'not registering audio file interface:%s class:%s as interface already registered',
                entry.interface, entry.__name__)

except ImportError:
    logging.getLogger(__name__).info('no audio file interfaces found')


def audio_get_instance(filename, mode=MODE_DEFAULT, allow_encoded=True, *args, **kwargs):
    '''
    Audio file class loader

    This function loads the relevant audio file class.
    It supports a factory interface, defined by the filename extension, where it will try to
    autodetect a class subclassing AudioBase abstract class

    Args:
        filename (str): Filename
        mode (str): File open mode 'r' for read, 'w' for write
        allow_encoded (bool): Allow encoded data file handlers
    '''

    ext = os.path.splitext(filename)[1][1:]

    # If this extension is registered and complies with encoded then return it
    if ext in REGISTERED_AUDIO_FILE and (
            not REGISTERED_AUDIO_FILE[ext].encoded or allow_encoded):
        return REGISTERED_AUDIO_FILE[ext](filename, mode, *args, **kwargs)

    # if there is default extension then return it
    if REGISTERED_AUDIO_FILE[DEFAULT_INTERFACE]:
        # pylint: disable=not-callable
        return REGISTERED_AUDIO_FILE[DEFAULT_INTERFACE](filename, mode, *args, **kwargs)

    raise RuntimeError('filename %s unable to find registered handler' % (filename))


def audio_get_class(ext, allow_encoded=True):
    '''
    Get audio class an extension has registered to

    Args:
        ext (str): Filename extension
        allow_encoded (bool): Allow encoded data file handlers

    Returns:
        AudioBase: Audio class
    '''

    # If this extension is registered and complies with encoded then return it
    if ext in REGISTERED_AUDIO_FILE and (
            not REGISTERED_AUDIO_FILE[ext].encoded or allow_encoded):
        return REGISTERED_AUDIO_FILE[ext]

    # if there is default extension then return it
    if REGISTERED_AUDIO_FILE[DEFAULT_INTERFACE]:
        return REGISTERED_AUDIO_FILE[DEFAULT_INTERFACE]

    raise RuntimeError('extension %s unable to find registered handler' % (ext))
