/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Header file for the GAP Server module.

Component that deals with initialising the gatt_gap_server library, 
and deals with messages sent from this library.
Also responsible for LE advertising of the local name.

*/

#ifndef GATT_SERVER_GAP_H_
#define GATT_SERVER_GAP_H_


/*! \brief Initialise the GAP Server.

    \param init_task    Task to send init completion message to

    \returns TRUE
*/
bool GattServerGap_Init(Task init_task);


/*! \brief Tells the GAP server to use the complete local name, or if it can be shortened.

    The GAP server handles the LE advertising of local name which can be shortened depending on the
    space in the advertising data.
    This function can ensure the complete local name is always supplied in the advertising data.
    
    \param complete    Set to TRUE so the complete local name is always used. Otherwise can be shortened.
*/
void GattServerGap_UseCompleteLocalName(bool complete);


#endif /* GATT_SERVER_GAP_H_ */
