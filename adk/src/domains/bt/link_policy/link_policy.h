/*!
\copyright  Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       link_policy.h
\brief	    Header file for the Link policy manager
*/

#ifndef LINK_POLICY_H_
#define LINK_POLICY_H_

#include <connection.h>

/*! Link policy task structure */
typedef struct
{
    TaskData task;              /*!< Link policy manager task */
    /*! \todo How do we end up with 3 (now 4) link states when
        we just have AG & peer */
#ifdef INCLUDE_HFP
    hci_role hfp_role:2;        /*!< Current role of HFP link */
#endif
#ifdef INCLUDE_AV
    hci_role av_sink_role:2;    /*!< Current role of AV link as A2DP Sink */
    hci_role av_source_role:2;  /*!< Current role of AV link as A2DP Source */
#endif
    hci_role scofwd_role:2;     /*!< Current role of peer link */
} lpTaskData;

#define appLinkPolicyGetCurrentScoFwdRole() \
    appGetLp()->scofwd_role

/*! Power table indexes */
typedef enum lp_power_table_index
{
    POWERTABLE_A2DP,
    POWERTABLE_A2DP_STREAMING_SOURCE,
    POWERTABLE_A2DP_STREAMING_TWS_SINK,
    POWERTABLE_A2DP_STREAMING_SINK,
    POWERTABLE_HFP,
    POWERTABLE_HFP_SCO,
    POWERTABLE_AVRCP,     /* Used for both Handset and Peer links */
    POWERTABLE_PEER_MODE, /* Used Peer links when SCO forwarding or TWS+ with A2DP Streaming */
    POWERTABLE_PEER_MIRRORING,
    /*! Must be the final value */
    POWERTABLE_UNASSIGNED,
} lpPowerTableIndex;

/*! Link policy state per ACL connection, stored for us by the connection manager. */
typedef struct
{
    lpPowerTableIndex pt_index;     /*!< Current powertable in use */
} lpPerConnectionState;

/*!< Link Policy Manager data structure */
extern lpTaskData  app_lp;

/*! Get pointer to Link Policy Manager data structure */
#define LinkPolicyGetTaskData()  (&app_lp)

bool appLinkPolicyInit(Task init_task);

/*! @brief Update the link supervision timeout.
    @param bd_addr The Bluetooth address of the remote device.
*/
void appLinkPolicyUpdateLinkSupervisionTimeout(const bdaddr *bd_addr);

/*! @brief Update the link power table based on the system state.
    @param bd_addr The Bluetooth address of the remote device.
*/
void appLinkPolicyUpdatePowerTable(const bdaddr *bd_addr);

/*! @brief Force an update the link power table based on the system state.
    @param bd_addr The Bluetooth address of the remote device.
*/
void appLinkPolicyForceUpdatePowerTable(const bdaddr *bd_addr);

void appLinkPolicyAllowRoleSwitch(const bdaddr *bd_addr);
void appLinkPolicyAllowRoleSwitchForSink(Sink sink);
void appLinkPolicyPreventRoleSwitch(const bdaddr *bd_addr);
void appLinkPolicyPreventRoleSwitchForSink(Sink sink);
void appLinkPolicyUpdateRoleFromSink(Sink sink);
void appLinkPolicyUpdateRoleFromBdAddr(const bdaddr *bd_addr);

/*! Handler for connection library messages not sent directly

    This function is called to handle any connection library messages sent to
    the application that the link policy module is interested in. If a message
    is processed then the function returns TRUE.

    \note Some connection library messages can be sent directly as the
        request is able to specify a destination for the response.

    \param  id              Identifier of the connection library message
    \param  message         The message content (if any)
    \param  already_handled Indication whether this message has been processed by
                            another module. The handler may choose to ignore certain
                            messages if they have already been handled.

    \returns TRUE if the message has been processed, otherwise returns the
        value in already_handled
 */
extern bool appLinkPolicyHandleConnectionLibraryMessages(MessageId id,Message message, bool already_handled);

void appLinkPolicyAllowSniffMode(const bdaddr *bd_addr);
void appLinkPolicyPreventSniffMode(const bdaddr *bd_addr);

/*! \brief Request this device is always ACL master of links to the defined bd_addr.
           The host will accept incoming ACL connections requesting role switch.
           The host will initiate ACL connection prohibiting role switch.
    \param bd_addr The address of the remote device (which the caller would
    like to always be slave).
 */
void appLinkPolicyAlwaysMaster(const bdaddr *bd_addr);

/*! \brief Enable BR/EDR secure connections host support override for a device.
    \param bd_addr The address of the remote device to enable BR/EDR SC host support override.
 */
void appLinkPolicyBredrSecureConnectionHostSupportOverrideEnable(const bdaddr *bd_addr);

/*! \brief Remove BR/EDR secure connections host support override for a device.
    \param bd_addr The address of the remote device.
 */
void appLinkPolicyBredrSecureConnectionHostSupportOverrideRemove(const bdaddr *bd_addr);

/*! \brief Link policy performs actions early on acl opened.
    \param ind The indication.
*/
void appLinkPolicyHandleClDmAclOpendedIndication(const CL_DM_ACL_OPENED_IND_T *ind);

/*! \brief Make changes to link policy following an address swap.
*/
void appLinkPolicyHandleAddressSwap(void);

/*! \brief Register used to testing/debugging.

    \param test_task Task to be registered.
*/
void appLinkPolicyRegisterTestTask(Task test_task);

#endif /* LINK_POLICY_H_ */
