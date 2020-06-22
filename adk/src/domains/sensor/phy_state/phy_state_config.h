/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       phy_state_config.h
\brief      Configuration parameters for the Phy State component.
*/

/*! \brief Define a minimum time between generation of PHY_STATE_CHANGED_IND messages.
    
    \note If no PHY_STATE_CHANGED_IND has been sent for appPhyStateNotificationsLimitMs
          the event will always be sent.

    \note If a PHY_STATE_CHANGED_IND was sent less than appPhyStateNotificationsLimitMs
          ago, wait until that time has elapsed before sending the message.
          If after appPhyStateNotificationsLimitMs has elapsed the state is the same
          as last notified to clients, then DO NOT send the indication.

    \note Set to 0 to disable restrictions on PHY_STATE_CHANGED_IND messages and send
          PHY_STATE_CHANGED_IND message for event change.
 */ 
#define appPhyStateNotificationsLimitMs()   (500)
