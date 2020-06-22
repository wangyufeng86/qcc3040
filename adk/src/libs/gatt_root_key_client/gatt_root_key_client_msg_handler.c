/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt.h>

#include "gatt_root_key_client_private.h"

#include "gatt_root_key_client_msg_handler.h"

#include "gatt_root_key_client_state.h"
#include "gatt_root_key_client_challenge.h"
#include "gatt_root_key_client_discover.h"
#include "gatt_root_key_client_indication.h"
#include "gatt_root_key_client_write.h"


/****************************************************************************/
static void handleGattManagerMsg(Task task, MessageId id, Message payload)
{
    GATT_ROOT_KEY_CLIENT *instance = (GATT_ROOT_KEY_CLIENT *)task;

    GATT_ROOT_KEY_CLIENT_DEBUG("GRKC handleGattManagerMsg: %d [0x%x]\n", id, id);

    switch (id)
    {
        case GATT_MANAGER_REMOTE_SERVER_INDICATION_IND:
            handleRootKeyIndication(instance, (const GATT_MANAGER_REMOTE_SERVER_INDICATION_IND_T *)payload);
            break;

        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM:
            rootKeyHandleDiscoverAllCharacteristicsResp(instance, (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)payload);
            break;

        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM:
            rootKeyHandleDiscoverAllCharacteristicDescriptorsResp(instance, (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)payload);
            break;

        case GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM:
            handleRootKeyWriteValueResp(instance, (const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *)payload);
            break;

        default:
            GATT_ROOT_KEY_CLIENT_DEBUG("handleGattManagerMsg: Unhandled message [0x%x]\n", id);
            break;
    }
}

/****************************************************************************/
static void handleGattMsg(MessageId id)
{
    UNUSED(id);

    GATT_ROOT_KEY_CLIENT_DEBUG("GRKC handleGattMsg: Unhandled [0x%x]\n", id);
}


static void rootKeyHandleIndication(GATT_ROOT_KEY_CLIENT *instance, const ROOT_KEY_CLIENT_INTERNAL_INDICATION_T *ind)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (   root_key_client_starting_challenge == state
        && GattRootKeyScOpcodeOutgoingRequest == ind->opcode)
    {
        instance->remote_random = ind->value;

        gattRootKeyGenerateHashB(instance);

        gattRootKeyClientWriteChallenge(instance, GattRootKeyScOpcodeIncomingResponse,
                                        &instance->hashB_out);
        gattRootKeyClientSetState(instance, root_key_client_finishing_challenge);
        return;
    }
    else if (   root_key_client_finishing_challenge == state
             && GattRootKeyScOpcodeOutgoingResponse == ind->opcode)
    {
        instance->hashA_in = ind->value;

        if (!gattRootKeyCheckHash(instance))
        {
            GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleIndication HASH match failure");
        }
        else
        {
            gattRootKeySendChallengePeerInd(instance, gatt_root_key_challenge_status_success);
            gattRootKeyClientSetState(instance, root_key_client_authenticated);
            return;
        }
        
    }
    else
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleIndication Bad opcode/state. Op:%d Pending:%d",
                                ind->opcode, state);
    }
    gattRootKeyClientSetState(instance, root_key_client_error);
}


/****************************************************************************/
static void handleInternalRootKeyMsg(Task task, MessageId id, Message payload)
{
    GATT_ROOT_KEY_CLIENT *instance = (GATT_ROOT_KEY_CLIENT *)task;

    switch (id)
    {
        case ROOT_KEY_CLIENT_INTERNAL_INDICATION:
            rootKeyHandleIndication(instance, (const ROOT_KEY_CLIENT_INTERNAL_INDICATION_T *)payload);
            break;

        default:
            GATT_ROOT_KEY_CLIENT_DEBUG("handleInternalRootKeyMsg: Unhandled [0x%x]\n", id);
            break;
    }
}

/****************************************************************************/
void rootKeyClientMsgHandler(Task task, MessageId id, Message payload)
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
        handleInternalRootKeyMsg(task, id, payload);
    }
}

