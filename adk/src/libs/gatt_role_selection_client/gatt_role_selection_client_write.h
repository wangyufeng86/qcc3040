/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_CLIENT_WRITE_H_
#define GATT_ROLE_SELECTION_CLIENT_WRITE_H_

#include "gatt_role_selection_client_private.h"

/*!
    Write the client configuration descriptor value for state, 
    enabling indications when the peers state changes

    \param  instance            The clients data for this instance
*/
void roleSelectionWriteStateClientConfigValue(GATT_ROLE_SELECTION_CLIENT *instance);


/*!
    Write the client configuration descriptor value for the figure of 
    merit, enabling indications when the peers figure of merit changes

    \param  instance            The clients data for this instance
*/
void roleSelectionWriteFigureOfMeritClientConfigValue(GATT_ROLE_SELECTION_CLIENT *instance);


/*!
    Handle response as a result of writing a characteristic value.
*/
void handleRoleSelectionWriteValueResp(GATT_ROLE_SELECTION_CLIENT *instance, 
                                       const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm);


/*! Internal function to write a command to the peer

    \note This function cannot directly fail (as the gatt functions used do not return
    failures)

    \param      instance    The clients data for this instance
    \param      command     Command opcode to include in the message
*/
void gattRoleSelectionClientWritePeerControl(GATT_ROLE_SELECTION_CLIENT *instance, 
                                             GattRoleSelectionServiceControlOpCode command);


/*! Internal function to send a response to the application

    \param      instance    The clients data for this instance
    \param      command     The status of the gatt write to include in the message
*/
void gattRoleSelectionSendClientCommandCfmMsg(GATT_ROLE_SELECTION_CLIENT *instance, 
                                              gatt_status_t status);

#endif /* GATT_ROLE_SELECTION_CLIENT_WRITE_H_ */
