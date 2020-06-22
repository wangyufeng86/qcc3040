/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/****************************************************************************
FILE
    charger_comms_if.h

CONTAINS
    Definitions for the charger communication subsystem.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

#ifndef __APP_CHARGER_COMMS_IF_H__
#define __APP_CHARGER_COMMS_IF_H__

/*! @brief Status of the previous charger comms message sent by the ChargerCommsTransmit() trap.*/
typedef enum
{
    /*! The message was sent to the charger successfully.*/
    CHARGER_COMMS_MSG_SUCCESS,

    /*! Charger was removed during message transmission. */
    CHARGER_COMMS_MSG_INTERRUPTED,

    /*! An unexpected error occured. */
    CHARGER_COMMS_MSG_UNKNOWN_ERROR
} charger_comms_msg_status;

#endif /* __APP_CHARGER_COMMS_IF_H__  */
