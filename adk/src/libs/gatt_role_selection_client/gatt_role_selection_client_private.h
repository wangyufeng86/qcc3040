/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */
/*! \file
    Private header 
*/

#ifndef GATT_ROLE_SELECTION_CLIENT_PRIVATE_H_
#define GATT_ROLE_SELECTION_CLIENT_PRIVATE_H_

#include <csrtypes.h>
#include <panic.h>
#include <stdlib.h>

#include <gatt_role_selection_service.h>
#include "gatt_role_selection_client.h"


/* Macros for creating messages */
#define MAKE_ROLE_SELECTION_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))

typedef struct
{
    GattRoleSelectionServiceControlOpCode   role;
} ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE_T;


/* Enum for role selection client library internal message. */
typedef enum
{
    ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE,
} role_selection_internal_msg_t;


/*! Command the peer to change their state to that requested

    Internal function implementing GattRoleSelectionClientChangePeerRole.
    Sends a message commanding the peer to change to the requested
    state. The application will be sent a GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM_T
    once the command is complete.

    \param[in]  instance    The gatt client instance memory
    \param      state       The role to change the peer to
    \param      queued      If this is from a queued message

*/
void GattRoleSelectionClientChangePeerRoleImpl(GATT_ROLE_SELECTION_CLIENT *instance,
                                               GattRoleSelectionServiceControlOpCode state,
                                               bool queued);


#endif /* GATT_ROLE_SELECTION_CLIENT_PRIVATE_H_ */
