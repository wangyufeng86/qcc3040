/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Protected interface to module managing le scanning.
*/
#ifndef LE_SCAN_MANAGER_PROTECTED_H_
#define LE_SCAN_MANAGER_PROTECTED_H_

#include <task_list.h>
#include "le_scan_manager.h"

/*! States used internally by the LE Scan Manager Module */
typedef enum le_scan_manger_states
{
    LE_SCAN_MANAGER_STATE_UNINITIALIZED,
    /*! Scan Manager module has not yet been initialised */
    LE_SCAN_MANAGER_STATE_INITIALIZED,
    /*! Scan Manager module has been disabled */
    LE_SCAN_MANAGER_STATE_DISABLED,
    /*! Scan Manager module has been enabled */
    LE_SCAN_MANAGER_STATE_ENABLED,
    /*! Scan Manager module has is scanning */
    LE_SCAN_MANAGER_STATE_SCANNING,
     /*! Scan Manager module has is paused */
    LE_SCAN_MANAGER_STATE_PAUSED,
} scanState;

/*! Current Command as received by LE Scan Manager Module from external Commands */
typedef enum le_scan_manager_commands
{
    /*! Scan Manager module Enable Command */
    LE_SCAN_MANAGER_CMD_ENABLE,
    /*! Scan Manager module Disable Command */
    LE_SCAN_MANAGER_CMD_DISABLE,
    /*! Scan Manager module Start Command */
    LE_SCAN_MANAGER_CMD_START,
    /*! Scan Manager module Stop Command */
    LE_SCAN_MANAGER_CMD_STOP,
    /*! Scan Manager module Pause Command */
    LE_SCAN_MANAGER_CMD_PAUSE,
    /*! Scan Manager module Resume Command */
    LE_SCAN_MANAGER_CMD_RESUME,
    /*! Scan Manager module Default Command */
    LE_SCAN_MANAGER_CMD_DEFAULT,

} scanCommand;

/*! \brief LE scan parameters. */
typedef struct
{
    uint16  scan_interval;
    uint16  scan_window;
} le_scan_parameters_t;

/* \brief LE scan settings. */
typedef struct
{
    le_scan_interval_t scan_interval;
    le_advertising_report_filter_t  filter;
    Task scan_task;
} le_scan_settings_t;

/*! \brief LE scan manager task and state machine Strcuture. */
typedef struct
{
   /*! Task for Scan Manager*/
   TaskData  task;
   /*! State for Scan Manager Module */
   scanState state;
   /*! Current scan settings */
   le_scan_settings_t *confirmation_settings;
   /*! All scan settings */
   le_scan_settings_t *active_settings[MAX_ACTIVE_SCANS];
   /*! Scan parameters */
   le_scan_parameters_t scan_parameters;
   /*! Pause State */
   bool is_paused;
   /*! Scan Command*/
   scanCommand command;
   /*! Current Task Requester for Scan Manager*/
   Task  requester;
   /*! Current Task Requester for Scan Manager*/
   bool is_busy;
   /*! List Of tasks which to get response of Adverts*/
   task_list_t    *client_list;  
}le_scan_manager_data_t;   

void leScanManager_Handler(Task task, MessageId id, Message message);

#endif /* LE_SCAN_MANAGER_PROTECTED_H_ */
