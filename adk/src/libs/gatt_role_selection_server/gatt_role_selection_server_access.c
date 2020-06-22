/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_role_selection_server_access.h"
#include "gatt_role_selection_server_notify.h"
#include "gatt_role_selection_server_debug.h"
#include "gatt_role_selection_server_db.h"
#include "gatt_role_selection_server_private.h"

#include <string.h>


/*! Send an access response to the GATT Manager library.
*/
void sendRoleSelectionAccessRsp(Task task,
                                uint16 cid,
                                uint16 handle,
                                uint16 result,
                                uint16 size_value,
                                const uint8 *value)
 {
    if (!GattManagerServerAccessResponse(task, cid, handle, result, size_value, value))
    {
        /* The GATT Manager should always know how to send this response */
        GATT_ROLE_SELECTION_SERVER_DEBUG("sendRoleSelectionAccessRsp: Couldn't send GATT access response");
        Panic();
    }
}

/*! Send an error access response to the GATT Manager library.
*/
static void sendRoleSelectionAccessErrorRsp(const GATT_ROLE_SELECTION_SERVER *instance, 
                                            const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind, 
                                            uint16 error)
{
    sendRoleSelectionAccessRsp((Task)&instance->lib_task, 
                               access_ind->cid, access_ind->handle, error, 
                               0, NULL);
}


/*! Helper function to check if an allowed read has been requested/

    If any other request has been made an error response is generated.

    \return TRUE if a read was requested, FALSE otherwise.
*/
static bool roleSelectionPermittedReadRequested(GATT_ROLE_SELECTION_SERVER *instance,
                                                 const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        return TRUE;
    }

    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        sendRoleSelectionAccessErrorRsp(instance, access_ind, gatt_status_write_not_permitted);
    }
    else
    {
        /* Only read/write should ever be allowed. */
        sendRoleSelectionAccessErrorRsp(instance, access_ind, gatt_status_request_not_supported);
    }

    return FALSE;
}


/*! Helper function to check if an allowed write has been requested/

    If any other request has been made an error response is generated.

    \return TRUE if a write was requested, FALSE otherwise.
*/
static bool roleSelectionServicePermittedWriteRequested(GATT_ROLE_SELECTION_SERVER *instance,
                                                        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        return TRUE;
    }

    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendRoleSelectionAccessErrorRsp(instance, access_ind, gatt_status_read_not_permitted);
    }
    else
    {
        /* Only read/write should ever be allowed. */
        sendRoleSelectionAccessErrorRsp(instance, access_ind, gatt_status_request_not_supported);
    }

    return FALSE;
}


/*!
    Handle accesses to the service handle itself.

    The service handle is only ever readable so process reads and ignore 
    anything else.
*/
static void roleSelectionServiceAccess(GATT_ROLE_SELECTION_SERVER *instance,
                                       const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (roleSelectionPermittedReadRequested(instance, access_ind))
    {
        sendRoleSelectionAccessRsp(&instance->lib_task, access_ind->cid,
                                   HANDLE_ROLE_SELECTION_SERVICE, gatt_status_success, 0, NULL);
    }
}

/*!
    Processes access of the RTS Features characteristic.
*/
static void roleSelectionMirroringStateAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                              const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (roleSelectionPermittedReadRequested(instance, access_ind))
    {
        uint8 state = (uint8)instance->mirror_state;
        instance->mirror_state_notified = TRUE;
        sendRoleSelectionAccessRsp(&instance->lib_task, access_ind->cid,
                                   HANDLE_ROLE_SELECTION_MIRRORING_STATE, gatt_status_success, 
                                   GRSS_SIZE_MIRROR_STATE_PDU_OCTETS, &state);
    }
}


static void roleSelectionFigureOfMeritAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                             const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (roleSelectionPermittedReadRequested(instance, access_ind))
    {
        uint8 figure_of_merit[GRSS_SIZE_FIGURE_OF_MERIT_PDU_OCTETS] = {instance->figure_of_merit&0xff,
                                                                       (instance->figure_of_merit>>8)&0xff};

        instance->figure_of_merit_notified = TRUE;
        sendRoleSelectionAccessRsp(&instance->lib_task, access_ind->cid,
                                   HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT, gatt_status_success, 
                                   GRSS_SIZE_FIGURE_OF_MERIT_PDU_OCTETS, figure_of_merit);
    }
}

static void roleSelectionSendChangeRoleInd(GATT_ROLE_SELECTION_SERVER *instance, 
                               GattRoleSelectionServiceControlOpCode opcode)
{
    MAKE_ROLE_SELECTION_MESSAGE(GATT_ROLE_SELECTION_SERVER_CHANGE_ROLE_IND);

    message->command = opcode;
    MessageSend(instance->app_task, GATT_ROLE_SELECTION_SERVER_CHANGE_ROLE_IND, message);
}

static void roleSelectionControlAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                       const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (roleSelectionServicePermittedWriteRequested(instance, access_ind))
    {
        uint16 response = gatt_status_invalid_length;

        if (access_ind->size_value >= GRSS_SIZE_CONTROL_PRI_SEC_PDU_OCTETS)
        {
            uint8 opcode = access_ind->value[GRSS_CONTROL_PRI_SEC_OFFSET_OPCODE];
            response = gatt_status_invalid_pdu;

            switch (opcode)
            {
                case GrssOpcodeBecomePrimary:
                case GrssOpcodeBecomeSecondary:
                    if (GRSS_SIZE_CONTROL_PRI_SEC_PDU_OCTETS == access_ind->size_value)
                    {
                        roleSelectionSendChangeRoleInd(instance, opcode);
                        response = gatt_status_success;
                    }
                    else
                    {
                        response = gatt_status_invalid_length;
                    }
                    break;

                default:
                    break;
            }
        }

        /*! \todo Probably need to only send this here for some limited error cases
            and have the app/profile send the remainder */
        sendRoleSelectionAccessRsp(&instance->lib_task, access_ind->cid,
                             HANDLE_ROLE_SELECTION_CONTROL, response, 
                             0, NULL);
    }
}


/*!
    Handle access to the client config option for the mirroring state

    Read and write of the values are handled by the server.

    \todo MAY need to add notification to the application to allow
    the setting to be persisted.
*/
static void roleSelectionStateClientConfigAccess(GATT_ROLE_SELECTION_SERVER *instance,
                                                 const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendRoleSelectionConfigAccessRsp(instance, access_ind->cid, 
                                         instance->mirror_client_config);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        if (access_ind->size_value == GRSS_CLIENT_CONFIG_OCTET_SIZE)
        {
            uint16 config_value =    (access_ind->value[0] & 0xFF) 
                                  + ((access_ind->value[1] << 8) & 0xFF00);
            uint16 old_config = instance->mirror_client_config;

            instance->mirror_client_config = config_value;

            sendRoleSelectionAccessRsp(&instance->lib_task, 
                                 access_ind->cid, HANDLE_ROLE_SELECTION_MIRRORING_STATE_CLIENT_CONFIG,
                                 gatt_status_success, 0, NULL);

            if (config_value && !old_config
                && !instance->mirror_state_notified)
            {
                /* Now enabled and latest state has not been read/notified */
                sendInternalMirrorStateChanged(instance, access_ind->cid);
            }
        }
        else
        {
            sendRoleSelectionAccessErrorRsp(instance, access_ind, gatt_status_invalid_length);
        }
    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        sendRoleSelectionAccessErrorRsp(instance , access_ind, gatt_status_request_not_supported);
    }
}


/*!
    Handle access to the client config option for the figure of merit

    Read and write of the values are handled by the server.

    \todo MAY need to add notification to the application to allow
    the setting to be persisted.
*/
static void roleSelectionFigureClientConfigAccess(GATT_ROLE_SELECTION_SERVER *instance,
                                                  const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendRoleSelectionConfigAccessRsp(instance, access_ind->cid, 
                                         instance->merit_client_config);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        if (access_ind->size_value == GRSS_CLIENT_CONFIG_OCTET_SIZE)
        {
            uint16 config_value =    (access_ind->value[0] & 0xFF) 
                                  + ((access_ind->value[1] << 8) & 0xFF00);

            instance->merit_client_config = config_value;

            sendRoleSelectionAccessRsp(&instance->lib_task, 
                                 access_ind->cid, HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT_CLIENT_CONFIG,
                                 gatt_status_success, 0, NULL);

            /* If notifications are enabled, always send the current value. */
            if (config_value)
            {
                /* Reset the notified flag because the client has explicitly
                   asked for notifications and may not have received previous ones. */
                instance->figure_of_merit_notified = FALSE;

                sendInternalFigureOfMeritChanged(instance, access_ind->cid);
            }
        }
        else
        {
            sendRoleSelectionAccessErrorRsp(instance, access_ind, gatt_status_invalid_length);
        }
    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        sendRoleSelectionAccessErrorRsp(instance , access_ind, gatt_status_request_not_supported);
    }
}



/***************************************************************************/
void handleRoleSelectionServiceAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                      const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    switch (access_ind->handle)
    {
        case HANDLE_ROLE_SELECTION_SERVICE:
            roleSelectionServiceAccess(instance, access_ind);
            break;

        case HANDLE_ROLE_SELECTION_MIRRORING_STATE:
            roleSelectionMirroringStateAccess(instance, access_ind);
            break;

        case HANDLE_ROLE_SELECTION_CONTROL:
            roleSelectionControlAccess(instance, access_ind);
            break;

        case HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT:
            roleSelectionFigureOfMeritAccess(instance, access_ind);
            break;

        case HANDLE_ROLE_SELECTION_MIRRORING_STATE_CLIENT_CONFIG:
            roleSelectionStateClientConfigAccess(instance, access_ind);
            break;

        case HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT_CLIENT_CONFIG:
            roleSelectionFigureClientConfigAccess(instance, access_ind);
            break;

        default:
            sendRoleSelectionAccessErrorRsp(instance, access_ind, gatt_status_invalid_handle);
        break;
    }
}


/***************************************************************************/
void sendRoleSelectionConfigAccessRsp(const GATT_ROLE_SELECTION_SERVER *instance, 
                                      uint16 cid, uint16 client_config)
{
    uint8 config_resp[GRSS_CLIENT_CONFIG_OCTET_SIZE];

    config_resp[0] = client_config & 0xFF;
    config_resp[1] = (client_config >> 8) & 0xFF;

    sendRoleSelectionAccessRsp((Task)&instance->lib_task, cid,
                                HANDLE_ROLE_SELECTION_MIRRORING_STATE_CLIENT_CONFIG,
                                gatt_status_success,
                                GRSS_CLIENT_CONFIG_OCTET_SIZE, config_resp);
}


void GattRoleSelectionServerSetMirrorState(GATT_ROLE_SELECTION_SERVER *instance,
                                           uint16 cid,
                                           GattRoleSelectionServiceMirroringState mirror_state)
{
    GattRoleSelectionServiceMirroringState old_state = instance->mirror_state;
    bool old_notified = instance->mirror_state_notified;

    if (mirror_state != old_state)
    {
        instance->mirror_state = mirror_state;
        instance->mirror_state_notified = FALSE;
        sendInternalMirrorStateChanged(instance, cid);
    }
    else if (!old_notified)
    {
        sendInternalMirrorStateChanged(instance, cid);
    }
}


bool GattRoleSelectionServerSetFigureOfMerit(GATT_ROLE_SELECTION_SERVER *instance,
                                             uint16 cid,
                                             grss_figure_of_merit_t figure_of_merit,
                                             bool force_notify)
{

    if (!instance->initialised)
    {
        return FALSE;
    }

    grss_figure_of_merit_t old_fom = instance->figure_of_merit;
    bool old_notified = instance->figure_of_merit_notified;

    GATT_ROLE_SELECTION_SERVER_DEBUG("GattRoleSelectionServerSetFigureOfMerit old_fom 0x%x old_notified %d", old_fom, old_notified);

    if ((figure_of_merit != old_fom) || force_notify)
    {
        instance->figure_of_merit = figure_of_merit;
        instance->figure_of_merit_notified = FALSE;
        sendInternalFigureOfMeritChanged(instance, cid);
    }
    else if (!old_notified)
    {
        sendInternalFigureOfMeritChanged(instance, cid);
    }

    return TRUE;
}

