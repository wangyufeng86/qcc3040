/*******************************************************************************
Copyright (c) 2019  Qualcomm Technologies International, Ltd.


FILE NAME
    anc_config_write.c

DESCRIPTION
    Wirte the gain parameter to Audio PS key
*/

#include <stdlib.h>
#include <string.h>
#include "anc.h"
#include "anc_debug.h"
#include "anc_config_data.h"
#include "anc_data.h"
#include "anc_config_read.h"
#include "anc_tuning_data.h"
#include "anc_config_write.h"
#include <ps.h>


/*! Set the ANC Tuning Item
*/
static void appAncSetTuningItem16Bit(uint16 *data, uint16 gain)
{
    data[1] = gain;
}


/*! Write fine gain to the Audio PS key for the current mode and gain path specified 
*/
bool ancWriteFineGain(anc_mode_t mode, audio_anc_path_id gain_path, uint16 gain)
{
    uint16 anc_audio_ps[ANC_TUNING_CONFIG_TOTAL_SIZE];
    bool status = FALSE;
    uint16 total_key_length = 0;

    memset(anc_audio_ps, 0, sizeof(anc_audio_ps));

    /*Since the audio keys can't be partially updated, the entire value of the key must be read and written.*/
    if  ((PsReadAudioKey(AUDIO_PS_ANC_TUNING(mode), anc_audio_ps, ANC_TUNING_CONFIG_TOTAL_SIZE,
                0, &total_key_length) == ANC_TUNING_CONFIG_TOTAL_SIZE)
            && (total_key_length == ANC_TUNING_CONFIG_TOTAL_SIZE))
    {

        switch(gain_path)
        {
            case AUDIO_ANC_PATH_ID_FFA:
                appAncSetTuningItem16Bit(&anc_audio_ps[FFA_GAIN_OFFSET+ANC_TUNING_CONFIG_HEADER_SIZE], gain);
                status=TRUE;
                break;
            case AUDIO_ANC_PATH_ID_FFB:
                appAncSetTuningItem16Bit(&anc_audio_ps[FFB_GAIN_OFFSET+ANC_TUNING_CONFIG_HEADER_SIZE], gain);
                status=TRUE;
                break;
            case AUDIO_ANC_PATH_ID_FB:
                appAncSetTuningItem16Bit(&anc_audio_ps[FB_GAIN_OFFSET+ANC_TUNING_CONFIG_HEADER_SIZE], gain);
                status=TRUE;
                break;
            default:
                break;
        }        
    }

    if (status)
    {
        if (PsUpdateAudioKey(AUDIO_PS_ANC_TUNING(mode), anc_audio_ps, ANC_TUNING_CONFIG_TOTAL_SIZE, 0, ANC_TUNING_CONFIG_TOTAL_SIZE))
        {
            status = TRUE;
        }
        else
        {
            status = FALSE;
        }
    }

    return status;
}



