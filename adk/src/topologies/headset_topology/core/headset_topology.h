/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   headset HS Topology
\ingroup    topologies       
\brief      Headset topology public interface.
*/

#ifndef HEADSET_TOPOLOGY_H_
#define HEADSET_TOPOLOGY_H_

#include "domain_message.h"


/*! Definition of messages that Headset Topology can send to clients. */
typedef enum
{
    /*! Indication to clients that handset have been disconnected. */
    HEADSET_TOPOLOGY_HANDSET_DISCONNECTED_IND = HEADSET_TOPOLOGY_MESSAGE_BASE,
} headset_topology_message_t;


/*! \brief Initialise the  Headset topology component

    \param init_task    Task to send init completion message (if any) to

    \returns TRUE
*/
bool HeadsetTopology_Init(Task init_task);


/*! \brief Start the  Headset topology

    The topology will run semi-autonomously from this point.

    \returns TRUE
*/
bool HeadsetTopology_Start(void);


/*! \brief Register client task to receive  Headset topology messages.
 
    \param[in] client_task Task to receive messages.
*/
void HeadsetTopology_RegisterMessageClient(Task client_task);


/*! \brief Unregister client task from Headset topology.
 
    \param[in] client_task Task to unregister.
*/
void HeadsetTopology_UnRegisterMessageClient(Task client_task);


/*! \brief function to prohibit or allow connection to handset in  Headset topology.

    Prohibits or allows topology to connect handset. When prohibited any connection attempt in progress will
    be cancelled and any connected handset will be disconnected.

    Note: By default handset connection is allowed.

    \param prohibit TRUE to prohibit handset connection, FALSE to allow.
*/
void HeadsetTopology_ProhibitHandsetConnection(bool prohibit);

#endif /* HEADSET_TOPOLOGY_H_ */
