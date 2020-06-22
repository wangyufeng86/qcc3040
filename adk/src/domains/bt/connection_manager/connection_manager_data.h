/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       connection_manager_data.h
\brief      Header file for Connection Manager internal data types
*/

#ifndef __CON_MANAGER_DATA_H
#define __CON_MANAGER_DATA_H

#include <connection_manager.h>
#include <le_scan_manager.h>

/*! Connection Manager module task data. */
typedef struct
{
    /*! The task (message) information for the connection manager module */
    TaskData         task;

    /*! Flag indicating if incoming handset connections are allowed */
    bool handset_connect_allowed:1;

    cm_transport_t connectable_transports;
    
    bool  is_le_scan_paused;

    /*! BT address of handset requested opening ACL first, 0 otherwise */
    bdaddr   handset_to_pair_with_bdaddr;

    /*! Ensure only First ACL open requested handset can be authorised*/
    uint16   handset_authorise_lock;

    /*! Task that has requested a forced disconnect */
    Task forced_disconnect_task;

    /*! Task that has requested all LE connections are disconnected */
    Task all_le_disconnect_requester;
} conManagerTaskData;


/*! Return the task for connection manager */
Task ConManagerGetConManagerTask(void);

#endif
