/*******************************************************************************
Copyright (c) 2017-2020 Qualcomm Technologies International, Ltd.


FILE NAME
    anc_configure_coefficients.h

DESCRIPTION

*/

#ifndef ANC_CONFIGURE_COEFFICIENTS_H_
#define ANC_CONFIGURE_COEFFICIENTS_H_

#include <app/audio/audio_if.h>
#include "anc_config_data.h"

anc_instance_config_t * getInstanceConfig(audio_anc_instance instance);
void ancConfigureMutePathGains(void);
void ancConfigureCoefficientsAndGains(void);
bool ancConfigureGainForFFApath(audio_anc_instance instance, uint8 gain);
bool ancConfigureGainForFFBpath(audio_anc_instance instance, uint8 gain);
bool ancConfigureGainForFBpath(audio_anc_instance instance, uint8 gain);
#endif /* ANC_CONFIGURE_COEFFICIENTS_H_ */
