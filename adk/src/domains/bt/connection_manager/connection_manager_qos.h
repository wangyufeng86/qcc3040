/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       connection_manager_qos.h
\brief      Header file for Connection Manager QoS
*/

#ifndef __CON_MANAGER_QOS_H
#define __CON_MANAGER_QOS_H

#include <connection_manager.h>

/*! \brief Apply parameters on connection */
void ConManagerApplyQosOnConnect(const tp_bdaddr *tpaddr);

/*! \brief Apply parameters before connection */
void ConManagerApplyQosPreConnect(const tp_bdaddr *tpaddr);

/*! \brief Initialise connection parameters */
void ConnectionManagerQosInit(void);

#endif
