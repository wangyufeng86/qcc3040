/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   tws TWS Topology
\ingroup    topologies       
\brief      TWS topology public interface.
*/

#ifndef TWS_TOPOLOGY_H_
#define TWS_TOPOLOGY_H_

#include <message.h>
#include <bdaddr.h>

#include <peer_find_role.h>
#include <power_manager.h>

#include "domain_message.h"

/*\{*/
/*! Definition of messages that TWS Topology can send to clients. */
typedef enum
{
    /*! Confirmation that the TWS Topology module has initialised, sent 
        once #TwsTopology_Init() has completed */
    TWS_TOPOLOGY_INIT_CFM =  TWS_TOPOLOGY_MESSAGE_BASE,

    /*! Confirmation that TWS Topology has started, sent in response
        to #TwsTopology_Start() */
    TWS_TOPOLOGY_START_CFM,

    /*! Indication to clients that the Earbud role has changed. */
    TWS_TOPOLOGY_ROLE_CHANGED_IND,

    /*! Indication to clients that handset have been disconnected. */
    TWS_TOPOLOGY_HANDSET_DISCONNECTED_IND,
} tws_topology_message_t;

/*! Definition of status code returned by TWS Topology. */
typedef enum
{
    /*! The operation has been successful */
    tws_topology_status_success,

    /*! The requested operation has failed. */
    tws_topology_status_fail,
} tws_topology_status_t;

/*! Definition of the Earbud roles in a TWS Topology. */
typedef enum
{
    /*! Role is not yet known. */
    tws_topology_role_none,

    /*! Earbud has the Primary role. */
    tws_topology_role_primary,

    /*! Earbud has the Secondary role. */
    tws_topology_role_secondary,

    /*! Earbud has the DFU role. This is a special mode intended only for DFU usage. */
    tws_topology_role_dfu,

    /*! Earbud has the shutdown role. todo THIS IS TRANSITIONAL AND TEMPORARY -
     * Topology is taking steps to handover Primary role - if required - and shutdown. */
    tws_topology_role_shutdown

} tws_topology_role;

/*! Definition of the #TWS_TOPOLOGY_START_CFM message. */
typedef struct 
{
    /*! Result of the #TwsTopology_Start() operation. */
    tws_topology_status_t       status;

    /*! Current role of the Earbud. */
    tws_topology_role           role;
} TWS_TOPOLOGY_START_CFM_T;

/*! Indication of a change in the Earbud role. */
typedef struct
{
    /*! New Earbud role. */
    tws_topology_role           role;
} TWS_TOPOLOGY_ROLE_CHANGED_IND_T;

/*! \brief Initialise the TWS topology component

    \param init_task    Task to send init completion message (if any) to

    \returns TRUE
*/
bool TwsTopology_Init(Task init_task);


/*! \brief Start the TWS topology

    The topology will run semi-autonomously from this point.

    \todo To allow for the application behaviour to be adapted, error
    conditions are reported to the application. This avoids continual
    retries and may allow applications to try different behaviour.
    
    \param requesting_task Task to send messages to
*/
void TwsTopology_Start(Task requesting_task);

/*! \brief Register client task to receive TWS topology messages.
 
    \param[in] client_task Task to receive messages.
*/
void TwsTopology_RegisterMessageClient(Task client_task);

/*! \brief Unregister client task to stop receiving TWS topology messages.
 
    \param[in] client_task Task to unregister.
*/
void TwsTopology_UnRegisterMessageClient(Task client_task);

/*! \brief Find the current role of the Earbud.
    \return tws_topology_role Role of the Earbud.
*/
tws_topology_role TwsTopology_GetRole(void);

/*! \brief Utility function to easily determine Primary role.
    \return bool TRUE if Earbud is the Primary (including acting), 
        otherwise FALSE.
*/
bool TwsTopology_IsPrimary(void);

/*! \brief Utility function to easily determine Primary role.

    \return bool TRUE if Earbud is the Primary (excluding acting), 
        otherwise FALSE.
*/
bool TwsTopology_IsFullPrimary(void);

/*! \brief Utility function to easily determine Primary role.
    \return bool TRUE if Earbud is the Primary, otherwise FALSE.
*/
bool TwsTopology_IsActingPrimary(void);

/*! \brief Utility function to easily determine Primary role.
    \return bool TRUE if Earbud is the Primary, otherwise FALSE.
*/

bool TwsTopology_IsSecondary(void);

/*! \brief Switch the topology to the DFU role

    Tell the topology to use the DFU ruleset from now on.

    There is no confirmation. If the request changes the current rule set
    then this will be notified in a #TWS_TOPOLOGY_ROLE_CHANGED_IND message.
 */
void TwsTopology_SwitchToDfuRole(void);

/*! \brief End DFU role

    Tell the topology that DFU has completed, regardless of whether it
    was successful.

    There is no confirmation. If the request changes the current rule set
    then this will be notified in a #TWS_TOPOLOGY_ROLE_CHANGED_IND message.

    \todo May need to hint whether primary, secondary or find role
 */
void TwsTopology_EndDfuRole(void);


void TwsTopology_SelectPrimaryAddress(bool primary);

/*! \brief Set Peer Profiles on Peer Connect during BDFU
 */
void TwsTopology_SetProfilesOnPeerConnect(void);

/*\}*/

/*! \brief function for Application to prohibit or allow handover.
   
   If app sets prohibit to TRUE, handover will not occur. 
   If app sets prohibit to FALSE, handover may occur when system
   determines handover should be performed.

   Note: By default handover is allowed. App may prohibit handover by calling this 
   function with TRUE parameter. 

    \param prohibit To prohibit or allow handover.
*/
void TwsTopology_ProhibitHandover(bool prohibit);

/*! \brief function to prohibit or allow connection to handset in TWS topology.
   
    Prohibits or allows topology to connect handset. When prohibited any connection attempt in progress will
    be cancelled and any connected handset will be disconnected.

    Note: By default handset connection is allowed.

    \param prohibit TRUE to prohibit handset connection, FALSE to allow.
*/
void TwsTopology_ProhibitHandsetConnection(bool prohibit);

#endif /* TWS_TOPOLOGY_H_ */
