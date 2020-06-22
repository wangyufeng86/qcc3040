/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_SERVER_PRIVATE_H_
#define GATT_ROLE_SELECTION_SERVER_PRIVATE_H_

#include <panic.h>
#include <stdlib.h>

/* Macros for creating messages */
#define MAKE_ROLE_SELECTION_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))


/*! Internal message sent when the state has been updated and should
    possibly be notified to the client. */
typedef struct
{
    uint16 cid;
} ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED_T;


/*! Internal message sent when the figure of merit has been updated 
    and should possibly be notified to the client. */
typedef struct
{
    uint16 cid;
} ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED_T;


/* Enum for root key server library internal message. */
typedef enum
{
    ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED,
    ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED,
} role_selection_server_internal_msg_t;

#endif /* GATT_ROLE_SELECTION_SERVER_PRIVATE_H_ */
