/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */
/*!
\file
    Functions that handle the security aspects of the challenge exchange.

    The production and handling of messages are not implemented in this 
    module. 
*/

#ifndef GATT_ROOT_KEY_CLIENT_CHALLENGE_H_
#define GATT_ROOT_KEY_CLIENT_CHALLENGE_H_


#include "gatt_root_key_client_private.h"


/*! Generate the hash for use in the challenge

    \param  instance    Data for this instance of the client
*/
void gattRootKeyGenerateHashB(GATT_ROOT_KEY_CLIENT *instance);


/*! Check that the hashes in the challenge match

    \param  instance    Data for this instance of the client
*/
bool gattRootKeyCheckHash(GATT_ROOT_KEY_CLIENT *instance);



#endif /* GATT_ROOT_KEY_CLIENT_CHALLENGE_H_ */
