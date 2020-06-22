/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */
/*! \file
    Private header 
*/

#ifndef GATT_ROOT_KEY_CLIENT_PRIVATE_H_
#define GATT_ROOT_KEY_CLIENT_PRIVATE_H_

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include <gatt_manager.h>
#include "gatt_root_key_client.h"
#include "gatt_root_key_client_debug.h"


/* Macros for creating messages */
#define MAKE_ROOT_KEY_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))
#define MAKE_ROOT_KEY_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicNull(calloc(1,sizeof(TYPE##_T) + ((LEN) - 1) * sizeof(uint8)))


/*! Message sent internally when receiving an indication on the
    challenge control characteristic.
 */
typedef struct
{
    const GATT_ROOT_KEY_CLIENT *instance;
    uint8 opcode;
    GRKS_KEY_T value;
} ROOT_KEY_CLIENT_INTERNAL_INDICATION_T;



/* Enum for root key client library internal message. */
typedef enum
{
    ROOT_KEY_CLIENT_INTERNAL_INDICATION,
} root_key_internal_msg_t;


#endif /* GATT_ROOT_KEY_CLIENT_PRIVATE_H_ */
