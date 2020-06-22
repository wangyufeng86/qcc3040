/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Generic interface to TWS Topology procedure for enabling/disabling UI Audio Indications.
*/

#ifndef TWS_TOPOLOGY_PROCEDURE_ENABLE_AUDIO_INDICATIONS_H
#define TWS_TOPOLOGY_PROCEDURE_ENABLE_AUDIO_INDICATIONS_H

#include "tws_topology_procedures.h"
#include <message.h>

typedef struct
{
    bool enable;
} ENABLE_AUDIO_INDICATIONS_PARAMS_T;

extern const ENABLE_AUDIO_INDICATIONS_PARAMS_T proc_enable_audio_indications_enable;
#define PROC_ENABLE_AUDIO_INDICATIONS_ENABLE  ((Message)&proc_enable_audio_indications_enable)

extern const ENABLE_AUDIO_INDICATIONS_PARAMS_T proc_enable_audio_indications_disable;
#define PROC_ENABLE_AUDIO_INDICATIONS_DISABLE  ((Message)&proc_enable_audio_indications_disable)

extern const procedure_fns_t proc_enable_audio_indications_fns;

#endif /* TWS_TOPOLOGY_PROCEDURE_ENABLE_AUDIO_INDICATIONS_H */
