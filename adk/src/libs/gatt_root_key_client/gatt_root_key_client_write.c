/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <gatt.h>

#include "gatt_root_key_client_write.h"
#include "gatt_root_key_client_indication.h"
#include "gatt_root_key_client_init.h"
#include "gatt_root_key_client_state.h"
#include "gatt_root_key_client_challenge.h"


/****************************************************************************/
void rootKeyWriteClientConfigValue(GATT_ROOT_KEY_CLIENT *instance)
{
    uint8 value[2] = {ROOT_KEY_SERVICE_NOTIFICATION_VALUE, 0};

    GattManagerWriteCharacteristicValue((Task)&instance->lib_task, 
                                        instance->handle_challenge_control_config, 
                                        sizeof(value)/sizeof(uint8), value);
}


/****************************************************************************/
void handleRootKeyWriteValueResp(GATT_ROOT_KEY_CLIENT *instance, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (gatt_status_success == write_cfm->status)
    {
        switch(state)
        {
            case root_key_client_enabling_indications:
                gattRootKeyClientSetState(instance, root_key_client_initialised);
                break;

            case root_key_client_starting_challenge:
            case root_key_client_finishing_challenge:
                /* We don't care about the write, we want the indication */
                GATT_ROOT_KEY_CLIENT_DEBUG("handleRootKeyWriteValueResp: written challenge... awaiting indication");
                break;

            case root_key_client_writing_ir:
                gattRootKeyClientWriteKeyControlPoint(instance, GattRootKeyKeysOpcodePrepareEr, &instance->er);
                gattRootKeyClientSetState(instance, root_key_client_writing_er);
                break;

            case root_key_client_writing_er:
                gattRootKeyClientWriteKeyControlPoint(instance, GattRootKeyKeysOpcodeExecuteRootUpdate, NULL);
                gattRootKeyClientSetState(instance, root_key_client_committing);
                break;

            case root_key_client_committing:
                gattRootKeyClientSetState(instance, root_key_client_exchanged);
                gattRootKeySendWriteKeyInd(instance, gatt_root_key_client_write_keys_success);
                break;

            default:
                GATT_ROOT_KEY_CLIENT_DEBUG("handleRootKeyWriteValueResp: Unexpected response in state %d",
                                            state);
                gattRootKeyClientSetState(instance, root_key_client_error);
                break;
        }
    }
    else
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("handleRootKeyWriteValueResp: response:%d not expected in state %d",
                                   write_cfm->status, state);
        gattRootKeyClientSetState(instance, root_key_client_error);
    }
}


bool GattRootKeyClientChallengePeer(GATT_ROOT_KEY_CLIENT *instance,
                                    const GRKS_KEY_T *secret,
                                    const bdaddr   *local_address,
                                    const bdaddr   *remote_address)
{
    if (!instance || !secret || !local_address || !remote_address)
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientChallengePeer Null Pointer passed");
        return FALSE;
    }

    if (BdaddrIsZero(local_address) || BdaddrIsZero(remote_address))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientChallengePeer Null bdaddr passed");
        return FALSE;
    }

    if (root_key_client_disabled == gattRootKeyClientGetState(instance))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientChallengePeer Client is disabled.");
        return FALSE;
    }

    if (root_key_client_initialised != gattRootKeyClientGetState(instance))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientChallengePeer Not in correct state. State:%x",
                    gattRootKeyClientGetState(instance));
        return FALSE;
    }

    gattRootKeyServiceGenerateRandom(&instance->local_random);

    instance->secret = *secret;
    instance->local_address = *local_address;
    instance->remote_address = *remote_address;
    instance->challenge_response_needed = TRUE;

    gattRootKeyClientWriteChallenge(instance, GattRootKeyScOpcodeIncomingRequest,
                                    &instance->local_random);
    gattRootKeyClientSetState(instance, root_key_client_starting_challenge);

    return TRUE;
}


void gattRootKeyClientWriteChallenge(GATT_ROOT_KEY_CLIENT *instance, 
                                     GattRootKeyServiceMirrorChallengeControlOpCode opcode,
                                     const GRKS_KEY_T *content)
{
    uint8 value[GRKS_KEY_SIZE_128BIT_OCTETS + 1];

    value[0] = (uint8)opcode;
    memcpy(&value[1], content, GRKS_KEY_SIZE_128BIT_OCTETS);

    GATT_ROOT_KEY_CLIENT_DEBUG("gattRootKeyClientWriteChallenge. To handle %d",
                               instance->handle_challenge_control);

    GattManagerWriteCharacteristicValue(&instance->lib_task, instance->handle_challenge_control,
                                        GRKS_KEY_SIZE_128BIT_OCTETS + 1, value);
}


void gattRootKeySendChallengePeerInd(GATT_ROOT_KEY_CLIENT *instance, 
                                     gatt_root_key_challenge_status_t status_code)
{
    MAKE_ROOT_KEY_MESSAGE(GATT_ROOT_KEY_CLIENT_CHALLENGE_PEER_IND);

    message->instance = instance;
    message->status = status_code;

    MessageSend(instance->app_task, GATT_ROOT_KEY_CLIENT_CHALLENGE_PEER_IND, message);

    instance->challenge_response_needed = FALSE;
}


bool GattRootKeyClientWriteKeyPeer(GATT_ROOT_KEY_CLIENT *instance,
                                   const GRKS_IR_KEY_T *ir_key,
                                   const GRKS_ER_KEY_T *er_key)
{
    if (!instance || !ir_key || !er_key)
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientWriteKey Null Pointer passed");
        return FALSE;
    }

    if (root_key_client_disabled == gattRootKeyClientGetState(instance))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientWriteKey Client is disabled.");
        return FALSE;
    }

    if (root_key_client_authenticated != gattRootKeyClientGetState(instance))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientWriteKey Not in correct state. State %d",
                                    gattRootKeyClientGetState(instance));
        return FALSE;
    }

    instance->ir = *ir_key;
    instance->er = *er_key;

    instance->writekeys_response_needed = TRUE;

    gattRootKeyClientWriteKeyControlPoint(instance, GattRootKeyKeysOpcodePrepareIr,
                                          &instance->ir);
    gattRootKeyClientSetState(instance, root_key_client_writing_ir);

    return TRUE;
}


void gattRootKeyClientWriteKeyControlPoint(GATT_ROOT_KEY_CLIENT *instance, 
                                           GattRootKeyServiceKeysControlOpCode opcode,
                                           const GRKS_KEY_T *content)
{
    uint8 value[GRKS_KEY_SIZE_128BIT_OCTETS + 1];
    uint16 length = 1;

    value[0] = (uint8)opcode;
    if (content)
    {
        memcpy(&value[length], content, GRKS_KEY_SIZE_128BIT_OCTETS);
        length += GRKS_KEY_SIZE_128BIT_OCTETS;
    }

    GattManagerWriteCharacteristicValue(&instance->lib_task, instance->handle_keys_control,
                                        length, value);
}


void gattRootKeySendWriteKeyInd(GATT_ROOT_KEY_CLIENT *instance, 
                                gatt_root_key_client_write_key_status_t status_code)
{
    MAKE_ROOT_KEY_MESSAGE(GATT_ROOT_KEY_CLIENT_WRITE_KEY_IND);

    message->instance = instance;
    message->status = status_code;

    MessageSend(instance->app_task, GATT_ROOT_KEY_CLIENT_WRITE_KEY_IND, message);

    instance->writekeys_response_needed = FALSE;
}


