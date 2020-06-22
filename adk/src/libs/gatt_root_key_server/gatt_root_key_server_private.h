/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_SERVER_PRIVATE_H_
#define GATT_ROOT_KEY_SERVER_PRIVATE_H_

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include <gatt_manager.h>

#include "gatt_root_key_server.h"
#include "gatt_root_key_server_debug.h"

/* Macros for creating messages */
#define MAKE_ROOT_KEY_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))


#define GRKS_SIZE_FEATURES_OCTETS           (2)
#define GRKS_SIZE_STATUS_OCTETS             (1)

#define GRKS_SIZE_JUST_OPCODE_OCTETS        (1)
#define GRKS_OFFSET_OPCODE                  (0)
#define GRKS_OFFSET_KEY_IN_CONTROL_OCTETS   (1)

#define GRKS_SIZE_CONTROL_WITH_KEY_OCTETS   (GRKS_SIZE_JUST_OPCODE_OCTETS + GRKS_KEY_SIZE_128BIT_OCTETS)


/*! Message sent internally when receiving a valid write to the
    challenge control characteristic.
 */
typedef struct
{
    GATT_ROOT_KEY_SERVER *instance;
    uint16 cid;
    GattRootKeyServiceMirrorChallengeControlOpCode opcode;
    GRKS_KEY_T value;
} ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE_T;


/*! Message sent internally when receiving a valid write to the
    challenge control characteristic.
 */
typedef struct
{
    GATT_ROOT_KEY_SERVER *instance;
    uint16 cid;
    GattRootKeyServiceKeysControlOpCode opcode;
    GRKS_KEY_T value;
} ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE_T;


/*! Message sent internally when asked to commit keys
 */
typedef struct
{
    GATT_ROOT_KEY_SERVER *instance;
    uint16 cid;
} ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT_T;


/* Enum for root key server library internal message. */
typedef enum
{
    ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE,
    ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE,
    ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT,
} root_key_internal_msg_t;

#endif /* GATT_ROOT_KEY_SERVER_PRIVATE_H_ */
