/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to HEADSET Topology utility functions for sending messages to clients.
*/

#ifndef HEADSET_TOPOLOGY_CLIENT_MSGS_H
#define HEADSET_TOPOLOGY_CLIENT_MSGS_H

#include "headset_topology.h"

/*! \brief Send indication to registered clients that handset disconnected goal has been reached.
*/
void HeadsetTopology_SendHandsetDisconnectedIndication(void);

#endif /* HEADSET_TOPOLOGY_CLIENT_MSGS_H */
