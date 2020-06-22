/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Definition of interface for role change related callbacks.
*/

#ifndef _TWS_TOPOLOGY_ROLE_CHANGE_CLIENT_IF_H_
#define _TWS_TOPOLOGY_ROLE_CHANGE_CLIENT_IF_H_

#include "tws_topology.h"
#include "stdlib.h"

/*\{*/
/*! Definition of messages that TWS Topology Client Notifier can recieve from clients. */
typedef enum
{   /*! Confirmation from a Role Change client of Acceptance(or Non-Acceptance) to proposal*/
    TWS_ROLE_CHANGE_ACCEPTANCE_CFM= TWS_TOPOLOGY_CLIENT_NOTIFIER_MESSAGE_BASE,
    
    /*! Confirmation from a Role Change client of completing preparation for Role Change*/
    TWS_ROLE_CHANGE_PREPARATION_CFM

} tws_topology_client_notifier_message_t;

/*! Indication of a acceptance of role change proposal */
typedef struct
{
    /*! indication of whether role change was accepted by client */
    bool role_change_accepted;
} TWS_ROLE_CHANGE_ACCEPTANCE_CFM_T;

/*! Macro to create a role change acceptance message. */
#define MAKE_TWS_ROLE_CHANGE_ACCEPTANCE_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))

/*! \brief Data structure to specify the callback functions relating to role change control and indication(s)*/
typedef struct
{
    void (*Initialise)(Task server, int32_t reconnect_delay); /* initialisation settings for client*/
    void (*RoleChangeIndication)(tws_topology_role role); /* issued when Role state is changed */
    void (*ProposeRoleChange)(void); /* issued to client to ask for acceptance of role change proposal */
    void (*ForceRoleChange)(void);  /* informs client that non-negotiable role change is expected */
    void (*PrepareRoleChange)(void); /* informs client to continue preparation for role change 
                                        as all interested role change clients have accepted proprosal*/
    void (*CancelRoleChange)(void); /* issue to client inform that role change has been cancelled */
}role_change_client_callback_t;

/*! Helper macro to perform role change client registration at build/link time.
    Registrations created using this macro are placed in a const linker data section.
*/

#define TWS_ROLE_CHANGE_CLIENT_REGISTRATION_MAKE(CLIENT, INITIALISATION, ROLE_CHANGE_INDICATION, \
    PROPOSE_ROLE_CHANGE, FORCE_ROLE_CHANGE, PREPARE_ROLE_CHANGE, CANCEL_ROLE_CHANGE) \
_Pragma("datasection role_change_client_notifications") \
const role_change_client_callback_t role_change_client_registrations_##CLIENT = \
    {INITIALISATION, ROLE_CHANGE_INDICATION, PROPOSE_ROLE_CHANGE, FORCE_ROLE_CHANGE, \
    PREPARE_ROLE_CHANGE, CANCEL_ROLE_CHANGE }

/*! Linker defined consts referencing the location of the section containing
    the role change client registrations. */
extern const role_change_client_callback_t role_change_client_registrations_begin[];
extern const role_change_client_callback_t role_change_client_registrations_end[];


#endif /*_TWS_TOPOLOGY_ROLE_CHANGE_CLIENT_IF_H_*/
