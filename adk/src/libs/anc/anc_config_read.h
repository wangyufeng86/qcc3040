/*******************************************************************************
Copyright (c) 2017 Qualcomm Technologies International, Ltd.


FILE NAME
    anc_config_read.h

DESCRIPTION

*/

#ifndef ANC_CONFIG_READ_H_
#define ANC_CONFIG_READ_H_

#include "anc.h"

bool ancConfigReadPopulateAncData(anc_config_t * config_data, anc_mode_t set_mode);

/*!
    @brief Read fine gain from PS store for the gain path and mode specified

    @param mode            ANC mode in use
    @param gain_path       ANC gain path
    @param gain            ANC gain value

    @return TRUE indicating PS update was successful else FALSE
*/
bool ancReadFineGain(anc_mode_t mode, audio_anc_path_id gain_path, uint8 *gain);

/*!
    @brief Read coarse gain from specific ANC HW instance

    @param instance      ANC HW Instance
    @param gain_path       ANC gain path
    @param is_fine_gain If set then read the fine gain else coarse gain
    @param gain            ANC gain value
*/
void ancReadGainFromInst(audio_anc_instance inst, audio_anc_path_id gain_path, bool is_fine_gain, uint8 *gain);

#endif
