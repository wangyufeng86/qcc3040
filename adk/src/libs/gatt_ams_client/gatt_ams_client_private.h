/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_CLIENT_PRIVATE_H_
#define GATT_AMS_CLIENT_PRIVATE_H_

#include "gatt_ams_client.h"
#include <print.h>
#include <panic.h>
#include <message.h>

#ifdef GATT_AMS_CLIENT_DEBUG_LIB
#define DEBUG_PANIC(x) {PRINT(x); Panic();}
#else
#define DEBUG_PANIC(x) {PRINT(x);}
#endif

#define PANIC(x) {PRINT(x); Panic();}

/* Macros for creating messages */
#define MAKE_AMS_CLIENT_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T)
#define MAKE_AMS_CLIENT_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicUnlessMalloc(sizeof(TYPE##_T) + ((LEN) - 1) * sizeof(uint8))

/* GATT AMS Client Characteristic Identification */
#define GATT_AMS_CLIENT_REMOTE_COMMAND    (0x01)
#define GATT_AMS_CLIENT_ENTITY_UPDATE     (0x02)
#define GATT_AMS_CLIENT_ENTITY_ATTRIBUTE  (0x03)
#define GATT_AMS_CLIENT_MASK              (0x03)
#define GATT_AMS_CLIENT_WIDTH             (2)

#define GATT_AMS_CLIENT_FIELD_START(x)    ((x) * GATT_AMS_CLIENT_WIDTH)
#define GATT_AMS_CLIENT_FIELD_MASK(x)     (uint8)(GATT_AMS_CLIENT_MASK << GATT_AMS_CLIENT_FIELD_START(x))

#define GATT_AMS_CLIENT_MAX_CHAR          (0x03)

/*!
* AMS UUIDs
*/
#define AMS_CLIENT_REMOTE_COMMAND_UUID   (0x9B3C81D857B14A8AB8DF0E56F7CA51C2)
#define AMS_CLIENT_ENTITY_UPDATE_UUID    (0x2F7CABCE808D411F9A0CBB92BA96C102)
#define AMS_CLIENT_ENTITY_ATTRIBUTE_UUID (0xC6B2F38C23AB46D8A6ABA3A870BBD5D7)

/*!
 * Macro to check valid handle
 */
#define CHECK_VALID_HANDLE(handle) (handle != GATT_AMS_INVALID_HANDLE)

/*!
 * Macro to check the UUID of AMS 
 */
#define CHECK_AMS_REMOTE_COMMAND_UUID(char_cfm) (char_cfm->uuid[3] == 0xF7CA51C2u) && \
                    (char_cfm->uuid[2] == 0xB8DF0E56u) && \
                    (char_cfm->uuid[1] == 0x57B14A8Au) && \
                    (char_cfm->uuid[0] == 0x9B3C81D8u)

#define CHECK_AMS_ENTITY_UPDATE_UUID(char_cfm) (char_cfm->uuid[3] == 0xBA96C102u) && \
                    (char_cfm->uuid[2] == 0x9A0CBB92u) && \
                    (char_cfm->uuid[1] == 0x808D411Fu) && \
                    (char_cfm->uuid[0] == 0x2F7CABCEu)

#define CHECK_ENTITY_ATTRIBUTE_UUID(char_cfm) (char_cfm->uuid[3] == 0x70BBD5D7u) && \
                    (char_cfm->uuid[2] == 0xA6ABA3A8u) && \
                    (char_cfm->uuid[1] == 0x23AB46D8u) && \
                    (char_cfm->uuid[0] == 0xC6B2F38Cu)

typedef enum
{
    ams_pending_none = 0,

    /* Pending commands directly corresponding to external API calls */
    ams_pending_init,
    ams_pending_set_remote_command_notification,
    ams_pending_set_entity_update_notification,
    ams_pending_write_remote_command,
    ams_pending_write_entity_update,
    ams_pending_write_entity_attribute,
    ams_pending_read_entity_attribute,

    /* Pending commands corresponding to internal API calls */
    ams_pending_write_remote_command_cconfig,
    ams_pending_write_entity_update_cconfig,
    ams_pending_remote_command_notify_enable,
    ams_pending_remote_command_notify_disable,
    ams_pending_entity_update_notify_enable,
    ams_pending_entity_update_notify_disable
} pending_cmd_ids_t;

typedef enum
{
    MSG_SET_CHARACTERISTIC_NOTIFICATION,
    MSG_WRITE_CHARACTERISTIC,
    MSG_READ_CHARACTERISTIC
} internal_msg_ids_t;

typedef struct
{
    Task   task_pending_cfm;
    uint16 cmd_to_set_as_pending;
    bool   notifications_enable;
} MSG_SET_CHARACTERISTIC_NOTIFICATION_T;

typedef struct
{
    Task    task_pending_cfm;
    uint16  cmd_to_set_as_pending;
    uint16  size_command_data;
    uint8   command_data[1];
} MSG_WRITE_CHARACTERISTIC_T;

typedef struct
{
    Task   task_pending_cfm;
    uint16 cmd_to_set_as_pending;
} MSG_READ_CHARACTERISTIC_T;

uint16 gattAmsGetCharacteristicHandle(GAMS *ams, uint8 characteristic);

#endif /* GATT_AMS_CLIENT_PRIVATE_H_ */
