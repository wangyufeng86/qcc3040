/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_CLIENT_READ_H_
#define GATT_ROLE_SELECTION_CLIENT_READ_H_

#include "gatt_role_selection_client_private.h"


/*!
    Handle response as a result of writing a characteristic value.
*/
void handleRoleSelectionReadValueResp(GATT_ROLE_SELECTION_CLIENT *instance, 
                                      const GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *write_cfm);


/*! Internal function to write a command to the peer

    \note This function cannot directly fail (as the gatt functions used do not return
    failures)

    \param      instance    The clients data for this instance
    \param      command     Command opcode to include in the message
*/
void gattRoleSelectionClientWritePeerControl(GATT_ROLE_SELECTION_CLIENT *instance, 
                                             GattRoleSelectionServiceControlOpCode command);


/*!
    Sends a GATT_ROLE_SELECTION_CLIENT_STATE_IND message to the application task.
*/
void makeRoleSelectionClientStateIndMsg(GATT_ROLE_SELECTION_CLIENT *instance, 
                                        GattRoleSelectionServiceMirroringState state);


/*!
    Sends a GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND message to 
    the application task.
*/
void makeRoleSelectionClientFigureOfMeritIndMsg(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                grss_figure_of_merit_t figure_of_merit);


#endif /* GATT_ROLE_SELECTION_CLIENT_READ_H_ */
