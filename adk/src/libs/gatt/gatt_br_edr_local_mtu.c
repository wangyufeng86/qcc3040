/****************************************************************************
Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.


FILE NAME
    gatt_br_edr_local_mtu.c

DESCRIPTION
    This file contains the management entity responsible for arbitrating
    access to functionality in BlueStack that registers the local MTU for all
    future BR/EDR ATT connections

NOTES

*/

/***************************************************************************
    Header Files
*/

#include "gatt.h"
#include "gatt_private.h"

#include <stdlib.h>
#include <vm.h>

void GattSetBrEdrLocalMtuRequest(Task theAppTask, uint16 mtu)
{
    /* All requests are sent through the internal state handler */
    MAKE_GATT_MESSAGE(GATT_INTERNAL_SET_BREDR_LOCAL_MTU_REQ);
    message->theAppTask = theAppTask;
    message->mtu = mtu;

    MessageSend(gattGetTask(), GATT_INTERNAL_SET_BREDR_LOCAL_MTU_REQ, message);
}

/****************************************************************************
NAME
    gattHandleInternalSetBrEdrLocalMtuReq

DESCRIPTION
    Handle setting the local MTU for all future BR/EDR ATT connections

RETURN

*/
void gattHandleInternalSetBrEdrLocalMtuReq(const GATT_INTERNAL_SET_BREDR_LOCAL_MTU_REQ_T *m)
{
    MAKE_ATT_PRIM(ATT_SET_BREDR_LOCAL_MTU_REQ);
    prim->context = (context_t)(m->theAppTask);
    prim->mtu = m->mtu;
    VmSendAttPrim(prim);
}

/****************************************************************************
NAME
    gattHandleAttSetBrEdrLocalMtuCfm

DESCRIPTION
    Confirm containing the request status and the MTU received in the request

RETURN

*/
void gattHandleAttSetBrEdrLocalMtuCfm(const ATT_SET_BREDR_LOCAL_MTU_CFM_T *m)
{
    /* send confirmation to the application */
    MAKE_GATT_MESSAGE(GATT_SET_BREDR_LOCAL_MTU_CFM);

    message->status = gatt_message_status(m->result);
    message->mtu    = m->mtu;

    MessageSend((Task) m->context, GATT_SET_BREDR_LOCAL_MTU_CFM, message);
}
