#
# Copyright (c) 2019 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Generic DIGMIC endpoint class
'''

import logging

from kats.core.endpoint_base import EndpointBase
from kats.framework.library.docstring import inherit_docstring
from .mapping import map_endpoint


class EndpointDigitalMicHydra(EndpointBase):
    '''
    Kymera DIGMIC Endpoint

    - *endpoint_type* can be:

        - *source*, pushing data to the uut.
        - *sink*, extracting data from the uut.

    - *instance* is the digmic instance
    - *channel* is the channel, 0 for left, 1 for right, 2 for both
    - *digital_mic_input_rate* is the sample rate in Hertzs
    - *digital_mic_input_gain* is the input gain

        - bit 15 Enable fine gain
        - bit 15=1

            - bits 8:0 digital gain in -30 dB steps, 1 max attenuation, 511 max gain)

        - bit 15=0

            - bits 3:0 digital gain in legacy mode

    - *digital_mic_sidetone_gain* Gain applied to all the sidetone channels.

        - 0  -32.6 dB
        - 1  -30.1 dB
        - 2  -26.6 dB
        - 3  -24.1 dB
        - 4  -20.6 dB
        - 5  -18.1 dB
        - 6  -14.5 dB
        - 7  -12   dB
        - 8   -8.5 dB
        - 9   -6.0 dB
        - 10  -2.5 dB
        - 11   0   dB
        - 12  +3.5 dB
        - 13  +6.0 dB
        - 14  +9.5 dB
        - 15 +12.0 dB

    - *digital_mic_sidetone_enable* is the enable for sidetone on both channels, 0 for disable,
    - *digital_mic_clock_rate* is the digital interface clock rate in KHz

        - 500 for 500 KHz
        - 1000 for 1 MHz
        - 2000 for 2 MHz
        - 4000 for 4 MHz

    - *digital_mic_sidetone_source_point* source point for sidetone data from ADC.

        - 0 ADC data is taken before digital gain.
        - 1 ADC data is taken after digital gain.

    - *digital_mic_individual_sidetone_gain* gain of a particular sidetone channel. In contrast,
      when digital_mic_sidetone_gain is used, gain of all sidetone channels is changed
      simultaneously.
      Values are as for digital_mic_sidetone_gain enable/disable sidetone signal for a
      particular DAC

    - *digital_mic_data_source_point* Digital mic data source selection

        - 0 Digital mic data is taken from output of IIR filter
        - 1 Digital mic data is taken from output of digital gain filter.

    - *digital_mic_route* choose order of ADC chain. 0, Final IIR filter is before digital gain
      stage, 1  Final IIR filter is after digital gain stage.
    - *digital_mic_g722_filter_enable* Enables optional G.722 filter that improves noise performance
    - *digital_mic_g722_fir_enable* Enables optional FIR filter inside G.722 filter that droops the
      response slightly. 0 disable, 1 enable
    - *digital_mic_amp_sel* Configure LO_AMP_SEL and HI_AMP_SEL values of DMIC instance.
        External digital mic devices usually have a sigma-delta modulator providing one bit per
        sampling interval. This key controls how these are converted to the 3-bit value used
        internally. Usually logic 1 is translated to 7 and logic 0 is translated to 0.
        The lower 16 bits sets the LO_AMP_SEL and the higher 16 bits select the HI_AMP_SEL.
        The two values with a maximum of 7 and minimum of 0, should be set to be symmetrical around
        the value of 3.5 (such as 0,7 or 1,6 or 2,5 or 3,4 or 4,3 or 5,2 or 6,1 or 7,0).
        If the two values do not add to 7, a DC bias appears in the signal

    Args:
        kymera (kats.kymera.kymera.kymera_base.KymeraBase): Instance of class Kymera
        endpoint_type (str): Type of endpoint source or sink
        instance (int): Digital Mic instance
        channel (int): Channel 0 to 1
    '''

    platform = ['crescendo', 'stre', 'streplus', 'mora']
    interface = 'digital_mic'

    def __init__(self, kymera, endpoint_type, *args, **kwargs):
        self._log = logging.getLogger(__name__) if not hasattr(self, '_log') else self._log
        inherit_docstring(self)

        kwargs = map_endpoint(self.interface, endpoint_type, kwargs)
        self._instance = kwargs.pop('instance', 0)
        self._channel = kwargs.pop('channel', 0)

        # initialise values
        self.__args = []
        for entry in args:
            if not isinstance(entry, list):
                raise RuntimeError('arg %s invalid should be a list' % (entry))
            elif len(entry) != 2:
                raise RuntimeError('arg %s invalid should be list of 2 elements' % (entry))
            self.__args.append(entry)

        self.__args += list(kwargs.items())

        super(EndpointDigitalMicHydra, self).__init__(kymera, endpoint_type)

    def create(self, *_, **__):
        self._create('digital_mic', [self._instance, self._channel])

    def config(self):

        for entry in self.__args:
            self.config_param(entry[0], entry[1])

        super(EndpointDigitalMicHydra, self).config()
