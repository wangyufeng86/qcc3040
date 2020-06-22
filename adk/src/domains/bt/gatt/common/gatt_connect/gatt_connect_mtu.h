/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for GATT connect MTU
*/

#ifndef GATT_CONNECT_MTU_H_
#define GATT_CONNECT_MTU_H_

#include <gatt_manager.h>

/*! @brief Handle remote initiated MTU exchange

    \param ind      The exchange indication received from gatt_manager
*/
void gattConnect_HandleExchangeMtuInd(GATT_EXCHANGE_MTU_IND_T* ind);

/*! @brief Initiate an MTU exchange

    \param task     The gatt_connect task
    \param cid      The GATT connection ID
*/
void GattConnect_SendExchangeMtuReq(Task task, unsigned cid);

/*! @brief Handle confirmation of locally initiated MTU exchange

    \param cfm      The exchange confirmation received from gatt_manager
*/
void gattConnect_HandleExchangeMtuCfm(GATT_EXCHANGE_MTU_CFM_T* cfm);

/*! @brief Set MTU for a connection

    \param connection   The exchange indication received from gatt_manager
    \param mtu          The MTU for the connection
*/
void GattConnect_SetMtu(gatt_connection_t* connection, unsigned mtu);

/*! @brief Initialise gatt_connect_mtu
*/
void GattConnect_MtuInit(void);

#endif 
