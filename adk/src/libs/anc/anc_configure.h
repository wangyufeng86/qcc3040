/*******************************************************************************
Copyright (c) 2015-2020 Qualcomm Technologies International, Ltd.


FILE NAME
    anc_configure.h

DESCRIPTION
    Functions required to configure the ANC Sinks/Sources.
*/

#ifndef ANC_CONFIGURE_H_
#define ANC_CONFIGURE_H_

#include "anc.h"

#include <csrtypes.h>

/******************************************************************************
NAME
    ancConfigure

DESCRIPTION
    Configure the ANC hardware
*/
bool ancConfigure(bool enable);

/******************************************************************************
NAME
    ancConfigureAfterModeChange

DESCRIPTION
    (Re)Configure following an ANC mode change
*/
bool ancConfigureAfterModeChange(void);

/******************************************************************************
NAME
    ancConfigureFilterPathGain

DESCRIPTION
    (Re)Configure filter path (FFA or FFB or FB) gain for a given ANC hardware instance

    @param instance The audio ANC hardware instance number.
    @param path The ANC filter path (valid range: FFA or FFB or FB) to set
    @param gain The ANC filter path FFA/FFb/FB gain to set.

    @return TRUE indicating the ANC filter path gain was successfully changed
            otherwise FALSE.
*/
bool ancConfigureFilterPathGain(audio_anc_instance instance, audio_anc_path_id path, uint8 gain);

#endif
