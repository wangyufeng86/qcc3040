/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_role_selection_server_notify.h"
#include "gatt_role_selection_server_debug.h"
#include "gatt_role_selection_server_private.h"

#include "gatt_role_selection_server_db.h"


/****************************************************************************/
void gattRoleSelectionServerSendStateNotification(GATT_ROLE_SELECTION_SERVER *instance,
                                                  uint16 cid,
                                                  GattRoleSelectionServiceMirroringState state)
{
    uint8 notification[GRSS_SIZE_MIRROR_STATE_PDU_OCTETS] = {state};

    GattManagerRemoteClientNotify(&instance->lib_task,
                                  cid,
                                  HANDLE_ROLE_SELECTION_MIRRORING_STATE,
                                  GRSS_SIZE_MIRROR_STATE_PDU_OCTETS,
                                  notification);
}


void gattRoleSelectionServerSendFigureNotification(GATT_ROLE_SELECTION_SERVER *instance,
                                                   uint16 cid,
                                                   grss_figure_of_merit_t figure_of_merit)
{
    uint8 notification[GRSS_SIZE_FIGURE_OF_MERIT_PDU_OCTETS] = {figure_of_merit&0xFF, (figure_of_merit>>8)&0xFF};

    GattManagerRemoteClientNotify(&instance->lib_task,
                                  cid,
                                  HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT,
                                  GRSS_SIZE_FIGURE_OF_MERIT_PDU_OCTETS,
                                  notification);
}


void handleRoleSelectionServerStateChanged(GATT_ROLE_SELECTION_SERVER *instance,
                                           const ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED_T *update)
{
    if (instance->mirror_client_config
        && !instance->mirror_state_notified)
    {
        gattRoleSelectionServerSendStateNotification(instance, update->cid, instance->mirror_state);
        instance->mirror_state_notified = TRUE;
    }
}


void handleRoleSelectionServerFigureChanged(GATT_ROLE_SELECTION_SERVER *instance,
                                           const ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED_T *update)
{
    if (   instance->merit_client_config
        && !instance->figure_of_merit_notified)
    {
        gattRoleSelectionServerSendFigureNotification(instance, update->cid, instance->figure_of_merit);
        instance->figure_of_merit_notified = TRUE;
    }
}


/*! Send an internal message for the mirror state having changed */
void sendInternalMirrorStateChanged(GATT_ROLE_SELECTION_SERVER *instance, uint16 cid)
{
    MAKE_ROLE_SELECTION_MESSAGE(ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED);

    message->cid = cid;

    MessageSend(&instance->lib_task, ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED, message);
}


/*! Send an internal message for the mirror state having changed */
void sendInternalFigureOfMeritChanged(GATT_ROLE_SELECTION_SERVER *instance, uint16 cid)
{
    MAKE_ROLE_SELECTION_MESSAGE(ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED);

    message->cid = cid;

    MessageSend(&instance->lib_task, ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED, message);
}


static void handleRoleSelectionNotificationDelivered(GATT_ROLE_SELECTION_SERVER *instance,
                                                     bool delivered)
{
    if (!delivered)
    {
        /*! \todo Is there any point to this. We don't want to retry 
            immediately, especially for this service, and the flag 
            makes no immediate sense. Perhaps introducing an 'error'
            state would help. */
        instance->mirror_state_notified = FALSE;
    }
}


void handleRoleSelectionNotificationCfm(GATT_ROLE_SELECTION_SERVER *instance,
                                        const GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM_T *payload)
{
    switch (payload->status)
    {
        case gatt_status_success:
            handleRoleSelectionNotificationDelivered(instance, TRUE);
            break;

        default:
            handleRoleSelectionNotificationDelivered(instance, FALSE);
            /*! \todo Temporary panic in development. Need to check expected error codes */
            GATT_ROLE_SELECTION_SERVER_DEBUG_PANIC();
            break;
    }
}

