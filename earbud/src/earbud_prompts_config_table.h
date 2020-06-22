/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Header for Earbud Prompts UI Indicator configuration
*/
#ifndef EARBUD_PROMPTS_CONFIG_TABLE_H
#define EARBUD_PROMPTS_CONFIG_TABLE_H

#include <csrtypes.h>
#include <ui_prompts.h>

extern const ui_event_indicator_table_t earbud_ui_prompts_table[];

uint8 EarbudPromptsConfigTable_GetSize(void);

#endif // EARBUD_PROMPTS_CONFIG_TABLE_H
