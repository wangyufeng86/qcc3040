/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Earbud Prompts UI Indicator configuration table
*/
#include "earbud_prompts_config_table.h"

#include <domain_message.h>
#include <ui_prompts.h>

#include <av.h>
#include <pairing.h>
#include <telephony_messages.h>
#include <power_manager.h>
#include <voice_ui.h>

#if INCLUDE_PROMPTS
const ui_event_indicator_table_t earbud_ui_prompts_table[] =
{
    {.sys_event=POWER_ON,                {.prompt.filename = "power_on.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE },
                                          .await_indication_completion = TRUE },
    {.sys_event=POWER_OFF,              { .prompt.filename = "power_off.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE },
                                          .await_indication_completion = TRUE },
    {.sys_event=PAIRING_ACTIVE,         { .prompt.filename = "pairing.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=PAIRING_COMPLETE,       { .prompt.filename = "pairing_successful.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=PAIRING_FAILED,         { .prompt.filename = "pairing_failed.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=TELEPHONY_CONNECTED,    { .prompt.filename = "connected.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=TELEPHONY_DISCONNECTED, { .prompt.filename = "disconnected.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=VOICE_UI_MIC_OPEN,      { .prompt.filename = "mic_open.sbc",
                                          .prompt.rate = 16000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = FALSE }},
    {.sys_event=VOICE_UI_MIC_CLOSE,     { .prompt.filename = "mic_close.sbc",
                                          .prompt.rate = 16000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = TRUE,
                                          .prompt.queueable = FALSE }},
    {.sys_event=VOICE_UI_DISCONNECTED,  { .prompt.filename = "bt_va_not_connected.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }}
};
#endif

uint8 EarbudPromptsConfigTable_GetSize(void)
{
#if INCLUDE_PROMPTS
    return ARRAY_DIM(earbud_ui_prompts_table);
#else
    return 0;
#endif
}

