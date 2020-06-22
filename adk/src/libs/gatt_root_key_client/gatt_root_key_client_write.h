/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_CLIENT_WRITE_H_
#define GATT_ROOT_KEY_CLIENT_WRITE_H_


#include "gatt_root_key_client_private.h"


/*!
    Write the client configuration descriptor value, enabling indications

    \param  instance            The clients data for this instance
*/
void rootKeyWriteClientConfigValue(GATT_ROOT_KEY_CLIENT *instance);


/*!
    Handle response as a result of writing a characteristic value.
*/
void handleRootKeyWriteValueResp(GATT_ROOT_KEY_CLIENT *instance, 
                                 const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm);


/*! Internal function to write a value to the remote challenge control point 

    \note This function cannot directly fail (as the gatt functions used do not return
    failures)

    \param      instance    The clients data for this instance
    \param      opcode      Opcode to include in the message
    \param[in]  content     Content to include in the message
*/
void gattRootKeyClientWriteChallenge(GATT_ROOT_KEY_CLIENT *instance, 
                                     GattRootKeyServiceMirrorChallengeControlOpCode opcode,
                                     const GRKS_KEY_T *content);


/*! Send an indication including the completion status of a peer challenge

    \param  instance    The clients data for this instance
    \param  status_code The status to include in the indication
*/
void gattRootKeySendChallengePeerInd(GATT_ROOT_KEY_CLIENT *instance, 
                                     gatt_root_key_challenge_status_t status_code);



/*! Internal function to write a value to the remote key control point 

    \note This function cannot directly fail (as the gatt functions used do not return
    failures)

    \param      instance    The clients data for this instance
    \param      opcode      Opcode to include in the message
    \param[in]  content     Content to include in the message. This is either
                            a key value or NULL. If NULL is passed then only
                            an opcode is included in the message.
*/
void gattRootKeyClientWriteKeyControlPoint(GATT_ROOT_KEY_CLIENT *instance, 
                                           GattRootKeyServiceKeysControlOpCode opcode,
                                           const GRKS_KEY_T *content);


/*! Send an indication including the completion status of key transfer

    \param  instance    The clients data for this instance
    \param  status_code The status to include in the indication
*/
void gattRootKeySendWriteKeyInd(GATT_ROOT_KEY_CLIENT *instance, 
                                gatt_root_key_client_write_key_status_t status_code);

#endif /* GATT_ROOT_KEY_CLIENT_WRITE_H_ */
