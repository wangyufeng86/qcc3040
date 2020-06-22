/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt.h>

#include "gatt_role_selection_client_private.h"

#include "gatt_role_selection_client_msg_handler.h"
#include "gatt_role_selection_client_read.h"
#include "gatt_role_selection_client_write.h"
#include "gatt_role_selection_client_discover.h"
#include "gatt_role_selection_client_notification.h"
#include "gatt_role_selection_client_debug.h"

/****************************************************************************/
static void handleGattManagerMsg(Task task, MessageId id, Message payload)
{
    GATT_ROLE_SELECTION_CLIENT *instance = (GATT_ROLE_SELECTION_CLIENT *)task;
    
    switch (id)
    {
        case GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND:
            handleRoleSelectionNotification(instance, 
                                            (const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *)payload);
            break;

        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM:
            roleSelectionHandleDiscoverAllCharacteristicsResp(instance, (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)payload);
            break;

        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM:
            roleSelectionHandleDiscoverAllCharacteristicDescriptorsResp(instance, (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)payload);
            break;

        case GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM:
            handleRoleSelectionReadValueResp(instance, 
                                             (const GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *)payload);
            break;

        case GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM:
            handleRoleSelectionWriteValueResp(instance, 
                                              (const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *)payload);
            break;

        default:
            GATT_ROLE_SELECTION_CLIENT_DEBUG("handleGattManagerMsg: Unhandled message [0x%x]\n", id);
            break;
    }
}

/****************************************************************************/
static void handleGattMsg(MessageId id)
{
    UNUSED(id);

    GATT_ROLE_SELECTION_CLIENT_DEBUG("handleGattMsg: Unhandled [0x%x]\n", id);
    /*! \todo Not sure what, if any we get, or if we should care */
    GATT_ROLE_SELECTION_CLIENT_DEBUG_PANIC();
}


static void handleRoleSelectionClientInternalChangeRole(Task task,
                                    const ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE_T *role)
{
    GATT_ROLE_SELECTION_CLIENT *instance = (GATT_ROLE_SELECTION_CLIENT *)task;

    GATT_ROLE_SELECTION_CLIENT_DEBUG("handleRoleSelectionClientInternalChangeRole");

    GattRoleSelectionClientChangePeerRoleImpl(instance, role->role, TRUE);
}


/****************************************************************************/
static void handleInternalRoleSelectionMsg(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE:
            handleRoleSelectionClientInternalChangeRole(task, 
                                    (const ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE_T*)message);
            break;

        default:
            GATT_ROLE_SELECTION_CLIENT_DEBUG("handleInternalRoleSelectionMsg: Unhandled [0x%x]\n", id);
            break;
    }
}

/****************************************************************************/
void roleSelectionClientMsgHandler(Task task, MessageId id, Message payload)
{
    if ((id >= GATT_MANAGER_MESSAGE_BASE) && (id < GATT_MANAGER_MESSAGE_TOP))
    {
        handleGattManagerMsg(task, id, payload);
    }
    else if ((id >= GATT_MESSAGE_BASE) && (id < GATT_MESSAGE_TOP))
    {
        handleGattMsg(id);
    }
    else
    {
        handleInternalRoleSelectionMsg(task, id, payload);
    }
}

