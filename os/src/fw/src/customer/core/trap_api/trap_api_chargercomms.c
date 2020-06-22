/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 */

#include "trap_api/trap_api_private.h"


#if TRAPSET_CHARGERCOMMS

Task MessageChargerCommsTask(Task task)
{
    return trap_api_register_message_task(task, IPC_MSG_TYPE_CHARGER_COMMS);
}

#endif /* TRAPSET_CHARGERCOMMS */
