/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       connection_manager_params.h
\brief      Header file for Connection Manager Parameters
*/

#ifndef __CON_MANAGER_PARAMS_H
#define __CON_MANAGER_PARAMS_H

#include <connection_manager.h>

#define LE_CON_EVENT_LENGTH_MIN     0
#define LE_CON_EVENT_LENGTH_MAX     160

extern const ble_connection_params* const cm_qos_params[cm_qos_max];

#endif
