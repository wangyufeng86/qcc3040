/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       qualcomm_connection_manager_private.h
\brief      Private header file for Qualcomm Connection Manager
*/

#ifndef __QCOM_CON_MANAGER_PRIVATE_H
#define __QCOM_CON_MANAGER_PRIVATE_H

#ifdef INCLUDE_QCOM_CON_MANAGER

#define QHS_ENABLE_BIT_MASKS_OCTET0  0xF0
#define QHS_ENABLE_BIT_MASK_OCTET1   0x01

/*! Qualcomm Connection Manager module task data. */
typedef struct
{
    /*! The task information for the qualcomm connection manager */
    TaskData task;

    /*! List of tasks registered for notifications from qcom connection manager module */
    task_list_t client_tasks;

} qcomConManagerTaskData;

/*!< Qualcomm connection manager task data */
qcomConManagerTaskData  qcom_con_manager;

#define QcomConManagerGet() (&qcom_con_manager)

#define QcomConManagerGetTask() (&qcom_con_manager.task)

/*! Construct a VSDM prim of the given type */
#define MAKE_VSDM_PRIM_T(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->type = TYPE;

#endif /* INCLUDE_QCOM_CON_MANAGER */

#endif /*__QCOM_CON_MANAGER_PRIVATE_H*/
