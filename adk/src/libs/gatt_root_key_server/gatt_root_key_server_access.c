/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_server_private.h"

#include "gatt_root_key_server_access.h"
#include "gatt_root_key_server_db.h"

#include <string.h>

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_CLIENT_CONFIG_OCTET_SIZE sizeof(uint8) * 2


/***************************************************************************
NAME
    sendRootKeyAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void sendRootKeyAccessRsp(Task task,
                          uint16 cid,
                          uint16 handle,
                          uint16 result,
                          uint16 size_value,
                          const uint8 *value)
 {
    if (!GattManagerServerAccessResponse(task, cid, handle, result, size_value, value))
    {
        /* The GATT Manager should always know how to send this response */
        GATT_ROOT_KEY_SERVER_DEBUG("sendRootKeyAccessRsp: Couldn't send GATT access response");
        Panic();
    }
}

/***************************************************************************
NAME
    sendRootKeyAccessErrorRsp

DESCRIPTION
    Send an error access response to the GATT Manager library.
*/
static void sendRootKeyAccessErrorRsp(const GATT_ROOT_KEY_SERVER *instance, 
                                      const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind, 
                                      uint16 error)
{
    sendRootKeyAccessRsp((Task)&instance->lib_task, 
                         access_ind->cid, access_ind->handle, error, 
                         0, NULL);
}


/*! Helper function to check if an allowed read has been requested/

    If any other request has been made an error response is generated.

    \return TRUE if a read was requested, FALSE otherwise.
*/
static bool rootKeyServicePermittedReadRequested(GATT_ROOT_KEY_SERVER *instance,
                                                 const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        return TRUE;
    }

    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        sendRootKeyAccessErrorRsp(instance, access_ind, gatt_status_write_not_permitted);
    }
    else
    {
        /* Only read/write should ever be allowed. */
        sendRootKeyAccessErrorRsp(instance, access_ind, gatt_status_request_not_supported);
    }

    return FALSE;
}


/*! Helper function to check if an allowed write has been requested/

    If any other request has been made an error response is generated.

    \return TRUE if a write was requested, FALSE otherwise.
*/
static bool rootKeyServicePermittedWriteRequested(GATT_ROOT_KEY_SERVER *instance,
                                                  const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        return TRUE;
    }

    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendRootKeyAccessErrorRsp(instance, access_ind, gatt_status_read_not_permitted);
    }
    else
    {
        /* Only read/write should ever be allowed. */
        sendRootKeyAccessErrorRsp(instance, access_ind, gatt_status_request_not_supported);
    }

    return FALSE;
}


/*!
    Handle accesses to the service handle itself.

    The service handle is only ever readable so process reads and ignore 
    anything else.
*/
static void rootKeyServiceAccess(GATT_ROOT_KEY_SERVER *instance,
                                 const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (rootKeyServicePermittedReadRequested(instance, access_ind))
    {
        sendRootKeyAccessRsp(&instance->lib_task, access_ind->cid,
                             HANDLE_ROOT_TRANSFER_SERVICE, gatt_status_success, 0, NULL);
    }
}

/*!
    Processes access of the RTS Features characteristic.
*/
static void rootKeyFeaturesAccess(GATT_ROOT_KEY_SERVER *instance, 
                                  const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (rootKeyServicePermittedReadRequested(instance, access_ind))
    {
        uint8 features[GRKS_SIZE_FEATURES_OCTETS];
        features[0] = instance->features;
        features[1] = (instance->features >> 8);

        sendRootKeyAccessRsp(&instance->lib_task, access_ind->cid,
                             HANDLE_ROOT_TRANSFER_SERVICE_FEATURES, gatt_status_success, 
                             GRKS_SIZE_FEATURES_OCTETS, features);
    }
}


/*!
    Processes access of the RTS Status characteristic.
*/
static void rootKeyStatusAccess(GATT_ROOT_KEY_SERVER *instance, 
                                const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (rootKeyServicePermittedReadRequested(instance, access_ind))
    {
        sendRootKeyAccessRsp(&instance->lib_task, access_ind->cid,
                             HANDLE_ROOT_TRANSFER_SERVICE_STATUS, gatt_status_success, 
                             GRKS_SIZE_STATUS_OCTETS, (uint8*)&instance->status);
    }
}


static void sendInternalChallengeWriteMessage(GATT_ROOT_KEY_SERVER *instance, 
                                     uint16 cid,
                                     GattRootKeyServiceMirrorChallengeControlOpCode opcode,
                                     const uint8 *key)
{
    MAKE_ROOT_KEY_MESSAGE(ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE);

    message->instance = instance;
    message->cid = cid;
    message->opcode = opcode;
    memcpy(message->value.key, key, sizeof(message->value.key));

    MessageSend(&instance->lib_task, ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE, message);
}


static void sendInternalKeysWriteMessage(GATT_ROOT_KEY_SERVER *instance, 
                                         uint16 cid,
                                         GattRootKeyServiceKeysControlOpCode opcode,
                                         const uint8 *key)
{
    MAKE_ROOT_KEY_MESSAGE(ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE);

    message->instance = instance;
    message->cid = cid;
    message->opcode = opcode;
    memcpy(message->value.key, key, sizeof(message->value.key));

    MessageSend(&instance->lib_task, ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE, message);
}


static void sendInternalKeysCommitMessage(GATT_ROOT_KEY_SERVER *instance, uint16 cid)
{
    MAKE_ROOT_KEY_MESSAGE(ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT);

    message->instance = instance;
    message->cid = cid;

    MessageSend(&instance->lib_task, ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT, message);
}


static void rootKeyMirrorControlPointAccess(GATT_ROOT_KEY_SERVER *instance, 
                                            const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (rootKeyServicePermittedWriteRequested(instance, access_ind))
    {
        uint16 response = gatt_status_invalid_length;

        if (access_ind->size_value == GRKS_SIZE_CONTROL_WITH_KEY_OCTETS)
        {
            uint8 opcode = access_ind->value[GRKS_OFFSET_OPCODE];
            response = gatt_status_invalid_pdu;

            switch (opcode)
            {
                case GattRootKeyScOpcodeIncomingRequest:
                case GattRootKeyScOpcodeIncomingResponse:
                    sendInternalChallengeWriteMessage(instance, access_ind->cid,
                                                      opcode,
                                                      &access_ind->value[1]);
                    return;

                case GattRootKeyScOpcodeOutgoingRequest:
                case GattRootKeyScOpcodeOutgoingResponse:
                    response = gatt_status_invalid_pdu;
                    break;

                default:
                    response = gatt_status_invalid_pdu;
                    break;
            }
        }

        /*! \todo Probably need to only send this here for some limited error cases
            and have the app/profile send the remainder */
        sendRootKeyAccessRsp(&instance->lib_task, access_ind->cid,
                             HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT, response, 
                             0, NULL);
    }
}

static void rootKeyKeysControlPointAccess(GATT_ROOT_KEY_SERVER *instance, 
                                          const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (rootKeyServicePermittedWriteRequested(instance, access_ind))
    {
        uint16 response = gatt_status_invalid_length;

        if (access_ind->size_value >= GRKS_SIZE_JUST_OPCODE_OCTETS)
        {
            uint8 opcode = access_ind->value[GRKS_OFFSET_OPCODE];
            response = gatt_status_invalid_pdu;

            switch (opcode)
            {
                case GattRootKeyKeysOpcodePrepareIr:
                case GattRootKeyKeysOpcodePrepareEr:
                    if (access_ind->size_value == GRKS_SIZE_CONTROL_WITH_KEY_OCTETS)
                    {
                        sendInternalKeysWriteMessage(instance, access_ind->cid,
                                                     opcode,
                                                     &access_ind->value[1]);
                        return;
                    }
                    response = gatt_status_key_missing;
                    break;

                case GattRootKeyKeysOpcodeExecuteRootUpdate:
                    if (access_ind->size_value == GRKS_SIZE_JUST_OPCODE_OCTETS)
                    {
                        sendInternalKeysCommitMessage(instance, access_ind->cid);
                        return;
                    }
                    response = gatt_status_invalid_pdu;
                    break;

                default:
                    response = gatt_status_invalid_pdu;
                    break;
            }
        }

        /*! \todo Probably need to only send this here for some limited error cases
            and have the app/profile send the remainder */
        sendRootKeyAccessErrorRsp(instance, access_ind, response);
    }
}


/*!
    Handle access to the client config option

    Read and write of the values are handled by the server.

    \todo MAY need to add notification to the application to allow
    the setting to be persisted.
*/
static void rootKeyClientConfigAccess(GATT_ROOT_KEY_SERVER *instance,
                                      const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendRootKeyConfigAccessRsp(instance, access_ind->cid, instance->mirror_client_config);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        if (access_ind->size_value == GATT_CLIENT_CONFIG_OCTET_SIZE)
        {
            uint16 config_value =    (access_ind->value[0] & 0xFF) 
                                  + ((access_ind->value[1] << 8) & 0xFF00);

            instance->mirror_client_config = config_value;

            sendRootKeyAccessRsp(&instance->lib_task, 
                                 access_ind->cid, HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT_CLIENT_CONFIG, 
                                 gatt_status_success, 0, NULL);
        }
        else
        {
            sendRootKeyAccessErrorRsp(instance, access_ind, gatt_status_invalid_length);
        }
    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        sendRootKeyAccessErrorRsp(instance , access_ind, gatt_status_request_not_supported);
    }
}

/***************************************************************************/
void handleRootKeyAccess(GATT_ROOT_KEY_SERVER *instance,
                         const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    GATT_ROOT_KEY_SERVER_DEBUG("handleRootKeyAccess: handle %d",access_ind->handle);

    switch (access_ind->handle)
    {
        case HANDLE_ROOT_TRANSFER_SERVICE:
            rootKeyServiceAccess(instance, access_ind);
            break;

        case HANDLE_ROOT_TRANSFER_SERVICE_FEATURES:
            rootKeyFeaturesAccess(instance, access_ind);
            break;

        case HANDLE_ROOT_TRANSFER_SERVICE_STATUS:
            rootKeyStatusAccess(instance, access_ind);
            break;

        case HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT:
            rootKeyMirrorControlPointAccess(instance, access_ind);
            break;

        case HANDLE_ROOT_TRANSFER_SERVICE_KEYS_CONTROL:
            rootKeyKeysControlPointAccess(instance, access_ind);
            break;

        case HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT_CLIENT_CONFIG:
            rootKeyClientConfigAccess(instance, access_ind);
            break;
        
        default:
            sendRootKeyAccessErrorRsp(instance, access_ind, gatt_status_invalid_handle);
        break;
    }
}


/***************************************************************************/
void sendRootKeyConfigAccessRsp(const GATT_ROOT_KEY_SERVER *instance, 
                                uint16 cid, uint16 client_config)
{
    uint8 config_resp[GATT_CLIENT_CONFIG_OCTET_SIZE];

    config_resp[0] = client_config & 0xFF;
    config_resp[1] = (client_config >> 8) & 0xFF;

    sendRootKeyAccessRsp((Task)&instance->lib_task, cid, 
                         HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT_CLIENT_CONFIG, 
                         gatt_status_success, 
                         GATT_CLIENT_CONFIG_OCTET_SIZE, config_resp);
}


