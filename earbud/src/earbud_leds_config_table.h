/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Header for Earbud LED UI Indicator configuration
*/
#ifndef EARBUD_LEDS_CONFIG_TABLE_H
#define EARBUD_LEDS_CONFIG_TABLE_H

#include <csrtypes.h>
#include <ui_leds.h>

extern const ui_event_indicator_table_t earbud_ui_leds_table[];

extern const ui_provider_context_consumer_indicator_table_t earbud_ui_leds_context_indications_table[];

uint8 EarbudLedsConfigTable_EventsTableGetSize(void);

uint8 EarbudLedsConfigTable_ContextsTableGetSize(void);

#endif // EARBUD_LEDS_CONFIG_TABLE_H
