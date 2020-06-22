/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       tws_topology_procedure_notify_role_change_clients.h
\brief      contains definitions for notify role change clients procedure messages 
*/

#ifndef TWS_TOPOLOGY_NOTIFY_ROLE_CHANGE_CLIENTS_T
#define TWS_TOPOLOGY_NOTIFY_ROLE_CHANGE_CLIENTS_T

#include "script_engine.h"

typedef enum
{
    tws_notify_role_change_nothing_to_notify,
    tws_notify_role_change_force_notification,
    tws_notify_role_change_proposal_notification,
    tws_notify_role_change_cancel_notification
} role_change_notification_t;

extern procedure_fns_t proc_notify_role_change_clients_fns;

typedef struct
{
    role_change_notification_t notification;
} NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T; 

/*! Parameter definition for force notification */
extern const NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T proc_notify_role_change_clients_force_params;
#define PROC_NOTIFY_ROLE_CHANGE_CLIENTS_FORCE_NOTIFICATION  ((Message)&proc_notify_role_change_clients_force_params)

/*! Parameter definition for proposal notification */
extern const NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T proc_notify_role_change_clients_propose_params;
#define PROC_NOTIFY_ROLE_CHANGE_CLIENTS_PROPOSE_NOTIFICATION  ((Message)&proc_notify_role_change_clients_propose_params)

/*! Parameter definition for cancel notification */
extern const NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T proc_notify_role_change_clients_cancel_params;
#define PROC_NOTIFY_ROLE_CHANGE_CLIENTS_CANCEL_NOTIFICATION  ((Message)&proc_notify_role_change_clients_cancel_params)


#endif /* TWS_TOPOLOGY_NOTIFY_ROLE_CHANGE_CLIENTS_T */
