/*******************************************************************************
Copyright (c) 2014 - 2019 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_server_ama.h

DESCRIPTION
    Routines to handle messages sent from the GATT AMA Server Task.
    
NOTES

*/
#ifndef _GATT_SERVER_AMA_H_
#define _GATT_SERVER_AMA_H_

#include "gatt_manager.h"

#include <csrtypes.h>
#include <message.h>

/*******************************************************************************
NAME
    GattServerAma_Init

DESCRIPTION
    Initialise the AMA server tasks.    

PARAMETERS
    init_task

RETURNS
    TRUE if the GATT AMA server was initialised correctly, FALSE otherwise.
*/

bool GattServerAma_Init(Task init_task);

#endif /* _GATT_SERVER_AMA_H_ */
