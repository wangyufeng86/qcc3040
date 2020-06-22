#
# Copyright (c) 2018 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Generic Codec endpoint class
'''

import logging

from kats.core.endpoint_base import EndpointBase
from kats.framework.library.docstring import inherit_docstring
from .mapping import map_endpoint


class EndpointCodecHydra(EndpointBase):
    '''
    Kymera Codec Endpoint

    - *endpoint_type* can be:

        - *source*, pushing data to the uut.
        - *sink*, extracting data from the uut.

    - *instance* is the codec instance
    - *channel* is the channel, 0 for left, 1 for right, 2 for both
    - *codec_input_rate* is the sample clock rate in hertzs (only for source endpoints)
    - *codec_output_rate* is the sample clock rate in hertzs (only for sink endpoints)
    - *codec_input_gain* is the input gain scale in 3 dB steps
    - *codec_output_gain* is the output gain scale in 3 dB steps
    - *raw_input_gain* is the finer control gain

        - bit 15 Enable fine gain
        - bit 15=1

            - bits 8:0 digital gain in -30 dB steps, 1 max attenuation, 511 max gain)

        - bit 15=0

            - bits 8:6 analog gain, 0 max attenuation, 5 unity, 7 max gain
            - bits 3:0 digital gain in legacy mode

    - *raw_output_gain* is the finer control gain

        - bit 15 Enable fine gain
        - bit 15=1

            - bits 8:0 digital gain in -30 dB steps, 1 max attenuation, 511 max gain)

        - bit 15=0

            - bits 8:6 analog gain, 0 max attenuation, 5 unity, 7 max gain
            - bits 3:0 digital gain in legacy mode

    - *codec_output_gain_boost_enable* 0 for disable, 1 for enable
    - *codec_sidetone_gain* Gain applied to all the sidetone channels.
      BlueCore5 and prior (values 0 to 7 valid), BlueCore6 (values 0 to 9 valid)-9,
      BlueCore7 (values 0 to 15 valid)

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

    - *codec_sidetone_enable* is the enable for sidetone on both channels, 0 for disable,
      1 for enable
    - *codec_mic_input_enable* 0 for disable, 1 for 9 dB extra gain
    - *codec_low_power_output_stage_enable* 0 for disable, 1 enable
    - *codec_quality_mode* 0 for telephony, 1 for normal, 2 for high,
      3 for bypass in Amp (input only)
    - *codec_output_interp_filter_mode*

        - 0 long FIR mode (not available at 96 KHz)
        - 1 short FIR mode (for reduced latency requirements, but less good band stoip rejection),
        - 2 narrow FIR mode to meet G.722 requirements

    - *codec_output_power_mode*

        - 0 16 ohm normal power
        - 1 32 ohm normal power
        - 2 32 ohm low power

    - *codec_sidetone_source*

        - 0 ADC instance 0 or Digital Mic instance 0. Channel A goes into DAC A and channel B goes
          into DAC B.
        - 1 ADC instance 1 or Digital mic instance 1. Channel C goes into DAC A and channel D goes
          into DAC B.
        - 2 Digital mic instance 2. DMIC channel E goes into DAC A and DMIC channel F goes into
          DAC B.
        - 3 Digital mic instance 3. DMIC channel G goes into DAC A and DMIC channel H goes into
          DAC B

    - *codec_sidetone_source_point* source point for sidetone data from ADC.

        - 0 ADC data is taken before digital gain.
        - 1 ADC data is taken after digital gain.

    - *codec_sidetone_injection_point* injection point for sidetone data to DAC.

        - 0 Sidetone data is inserted at interpolation stage in DAC (before digital gain)
        - 1 Sidetone data is inserted at gain stage in DAC

    - *codec_sidetone_source_mask* mask that selects at most 2 ADC/MIC sources whose sum will be
      used as sidetone source for a particular DAC channel.

        - 0x00 No sidetone source. As good as sidetone is not enabled, 0x01 Channel A(ADC A/ DMIC A)
          is the sidetone source
        - 0x02 Channel B (ADC B/ DMIC B) is the sidetone source
        - 0x03 (Channel A + Channel B) is the sidetone source
        - 0x81 (Channel A + Channel H) is the sidetone source gain of a particular sidetone channel.

    - *codec_individual_sidetone_gain* gain of a particular sidetone channel. In contrast, when
      codec_sidetone_gain is used, gain of all sidetone channels is changed simultaneously.
      Values are as for codec_sidetone_gain enable/disable sidetone signal for a particular DAC
    - *codec_individual_sidetone_enable* enable/disable sidetone signal for a particular DAC
      channel. In contrast, codec_sidetone_enable is used to enable/disable sidetone signal for all
      DAC channels. 0 disable, 1 enable
    - *codec_adc_data_source_point* adc data source selection

        - 0 ADC data is taken from output of IIR filter (as on CSR8670)
        - 1 ADC data is taken from output of digital gain filter.

    - *codec_adc_route* choose order of ADC chain. 0, Final IIR filter is before digital gain
      stage (as on CSR8670), 1  Final IIR filter is after digital gain stage.
    - *codec_sidetone_invert* 0 do not invert, 1 invert
    - *codec_g722_filter_enable* Enables optional G.722 filter that improves noise performance
    - *codec_g722_fir_enable* Enables optional FIR filter inside G.722 filter that droops the
      response slightly. 0 disable, 1 enable
    - *codec_input_termination*

        - 0 dual differential (LINEIN1_PN, LINEIN2_PN)
        - 1 dual single ended (LINEIN1_P, LINEIN2_P)
        - 2 single ended inverted (LINEIN1_N, LINEIN2_N)
        - 3 single differential (MICIN1_PN)

    Args:
        kymera (kats.kymera.kymera.kymera_base.KymeraBase): Instance of class Kymera
        endpoint_type (str): Type of endpoint source or sink
        instance (int): Codec instance
        channel (int): 0 for left, 1 for right, 2 for both
    '''

    platform = ['crescendo', 'stre', 'streplus', 'mora']
    interface = 'codec'

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

        super(EndpointCodecHydra, self).__init__(kymera, endpoint_type)

    def create(self, *_, **__):
        self._create('codec', [self._instance, self._channel])

    def config(self):

        for entry in self.__args:
            self.config_param(entry[0], entry[1])

        super(EndpointCodecHydra, self).config()
