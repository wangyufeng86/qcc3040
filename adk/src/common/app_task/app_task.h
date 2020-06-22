/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Set of macros to get access to an application state.

*/

#ifndef APP_TASK_H_
#define APP_TASK_H_

#include <message.h>

/*! Application task data */
typedef struct appTaskData
{
    TaskData            task;                   /*!< Application task */
    TaskData            systask;                /*!< Handler for system messages */
} appTaskData;

/*! The global application data structure.

\note Do not access directly */
extern appTaskData globalApp;

/*! Get pointer to application data structure */
#define appGetApp()         (&globalApp)

/*! Get pointer to application task */
#define appGetAppTask()     (&globalApp.task)

/*! Get pointer to the system message task */
#define appGetSysTask()     (&globalApp.systask)

#endif /* APP_TASK_H_ */
