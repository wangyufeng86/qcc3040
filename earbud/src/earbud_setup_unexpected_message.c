/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Unexpected message handlers.

*/

#include "logging.h"

#include "earbud_sm.h"
#include "av.h"
#include "earbud_setup_unexpected_message.h"

#include "unexpected_message.h"

static void earbud_HandleUnexpectedMessage(MessageId id)
{
#if defined(INCLUDE_AV) && (INCLUDE_HFP)
    DEBUG_LOG_VERBOSE("earbud_HandleUnexpectedMessage, id = x%x, sm = %d, av = %d", (id), SmGetTaskData()->state, AvGetTaskData()->bitfields.state);
#elif defined(INCLUDE_AV)
    DEBUG_LOG_VERBOSE("earbud_HandleUnexpectedMessage, ID = x%x, sm = %d, av = %d", (id),  SmGetTaskData()->state, AvGetTaskData()->bitfields.state);
#elif defined(INCLUDE_HFP)
    DEBUG_LOG_VERBOSE("earbud_HandleUnexpectedMessage, id = x%x, sm = %d", (id),  SmGetTaskData()->sm.state);
#else
    UNUSED(id);
#endif
}

static void earbud_HandleUnexpectedSysMessage(MessageId id)
{
    DEBUG_LOG_VERBOSE("earbud_HandleUnexpectedSysMessage, id = 0x%x (%d)", (id), (id));
}

void Earbud_SetupUnexpectedMessage(void)
{
    UnexpectedMessage_RegisterHandler(earbud_HandleUnexpectedMessage);
    UnexpectedMessage_RegisterSysHandler(earbud_HandleUnexpectedSysMessage);
}
