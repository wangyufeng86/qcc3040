/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt.h>
#include <gatt_manager.h>

#include "gatt_role_selection_server.h"
#include "gatt_role_selection_server_msg_handler.h"
#include "gatt_role_selection_server_debug.h"
#include "gatt_role_selection_server_access.h"
#include "gatt_role_selection_server_notify.h"

#include <panic.h>

/****************************************************************************/
void roleSelectionServerMsgHandler(Task task, MessageId id, Message payload)
{
    GATT_ROLE_SELECTION_SERVER *instance= (GATT_ROLE_SELECTION_SERVER*)task;

    switch (id)
    {
            /* GATT MANAGER messages */
        case GATT_MANAGER_SERVER_ACCESS_IND:
            handleRoleSelectionServiceAccess(instance, 
                                             (const GATT_MANAGER_SERVER_ACCESS_IND_T *)payload);
            break;

        case GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM:
            handleRoleSelectionNotificationCfm(instance, 
                                              (const GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM_T *)payload);
            break;

            /* INTERNAL messages */
        case ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED:
            handleRoleSelectionServerStateChanged(instance,
                                                  (const ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED_T *)payload);
            break;

        case ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED:
            handleRoleSelectionServerFigureChanged(instance,
                                                  (const ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED_T *)payload);
            break;

            /* UNKNOWN messages */
        default:
            GATT_ROLE_SELECTION_SERVER_DEBUG("roleSelectionServerMsgHandler: Msg 0x%x not handled", id);
            /*! \todo This panic may be a step too far */
            GATT_ROLE_SELECTION_SERVER_DEBUG_PANIC();
            break;
    }
}

