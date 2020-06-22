/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for GATT connect list
*/

#ifndef GATT_CONNECT_LIST_H_
#define GATT_CONNECT_LIST_H_


typedef struct
{
    unsigned cid;
    unsigned mtu;
    unsigned pending_disconnects;
} gatt_connection_t;

/*! @brief Find a GATT connection using GATT connection ID

    \param cid          The GATT connection ID
*/
gatt_connection_t* GattConnect_FindConnectionFromCid(unsigned cid);

/*! @brief Create a new GATT connection with GATT connection ID

    \param cid          The GATT connection ID
*/
gatt_connection_t* GattConnect_CreateConnection(unsigned cid);

/*! @brief Destroy a GATT connection using GATT connection ID

    \param cid          The GATT connection ID
*/
void GattConnect_DestroyConnection(unsigned cid);

/*! @brief Initialise gatt_connect_list
*/
void GattConnect_ListInit(void);

/*! @brief Find a GATT connection using tp_bdaddr

    \param tpaddr_in     The tp_bdaddr address to find 
*/
gatt_connection_t* GattConnect_FindConnectionFromTpaddr(const tp_bdaddr *tpaddr_in);

#endif 
