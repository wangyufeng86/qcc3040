/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file        fast_pair_advertising.h
\brief      Handles Fast Pair Advertising 
*/

#ifndef FAST_PAIR_ADVERTISING_H_
#define FAST_PAIR_ADVERTISING_H_

#include <connection.h>
#include <task_list.h>
#include <connection_manager.h>

#include "domain_message.h"
#include "le_advertising_manager.h"



/*! \brief Global data structure for fastpair adverts */
typedef struct
{
    le_adv_mgr_register_handle adv_register_handle;
    uint8   *account_key_filter_adv_data;
    bool    identifiable;
}fastpair_advert_data_t;


/*! Private API to initialise fastpair advertising module

    Called from Fast pair state manager to initialise the fastpair advertising module

 */
void fastPair_SetUpAdvertising(void);


/*! @brief Private API to handle change in BR/EDR Connectable state and notify the LE Advertising Manager

    Called from Fast pair state manager based on its registration as observer for BR/EDR connections on connection status changes

 */
bool fastPair_AdvNotifyChangeInConnectableState(uint16 ind);


/*! @brief Private API to provide BR/EDR discoverablity information to fastpair state manager

     Called from Fast pair state manager to decide on reading KBP public key based on this

*/
bool fastPair_AdvIsBrEdrDiscoverable(void);

/*! @brief Private API to set the identifiable parameter according to the data set returned by Adv Mgr

     Called from Fast Pair state manager on getting CL_SM_AUTHENTICATE_CFM to make handset unidentifiable

 */
void fastPair_SetIdentifiable(const le_adv_data_set_t data_set);


#endif /* FAST_PAIR_ADVERTISING_H_ */
