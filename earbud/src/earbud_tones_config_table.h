/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Header for Earbud Tones UI Indicator configuration
*/
#ifndef EARBUD_TONES_CONFIG_TABLE_H
#define EARBUD_TONES_CONFIG_TABLE_H

#include <csrtypes.h>
#include <ui_tones.h>

extern const ui_event_indicator_table_t earbud_ui_tones_table[];
extern const ui_repeating_indication_table_t earbud_ui_repeating_tones_table[];

uint8 EarbudTonesConfigTable_SingleGetSize(void);
uint8 EarbudTonesConfigTable_RepeatingGetSize(void);

#endif // EARBUD_TONES_CONFIG_TABLE_H
