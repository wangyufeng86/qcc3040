/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Link policy manager legacy (non-mirroring products)
*/

#include "adk_log.h"

#include <connection_manager.h>
#include <hfp_profile.h>
#include <scofwd_profile.h>
#include "scofwd_profile_config.h"

#include <panic.h>
#include <connection.h>
#include <sink.h>

#include "link_policy_config.h"
#include "av.h"

#include <app/bluestack/dm_prim.h>

#ifndef INCLUDE_MIRRORING

/*! Make and populate a bluestack DM primitive based on the type.

    \note that this is a multiline macro so should not be used after a
    control statement (if, while) without the use of braces
 */
#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->common.op_code = TYPE; prim->common.length = sizeof(TYPE##_T);

Task test_task = NULL;

/*! \brief Allow role switching

    Update the policy for the connection (if any) to the specified
    Bluetooth address, so as to allow future role switching.

    \param  bd_addr The Bluetooth address of the device
*/
void appLinkPolicyAllowRoleSwitch(const bdaddr *bd_addr)
{
    MAKE_PRIM_C(DM_HCI_WRITE_LINK_POLICY_SETTINGS_REQ);
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->link_policy_settings = ENABLE_MS_SWITCH | ENABLE_SNIFF;
    VmSendDmPrim(prim);
    DEBUG_LOG("appLinkPolicyAllowRoleSwitch");
}

/*! \brief Allow role switching
    \param sink The sink for which to allow role switching
*/
void appLinkPolicyAllowRoleSwitchForSink(Sink sink)
{
    tp_bdaddr bd_addr;
    if (SinkGetBdAddr(sink, &bd_addr))
    {
        appLinkPolicyAllowRoleSwitch(&bd_addr.taddr.addr);
    }
}

/*! \brief Prevent role switching

    Update the policy for the connection (if any) to the specified
    Bluetooth address, so as to prevent any future role switching.

    \param  bd_addr The Bluetooth address of the device
*/
void appLinkPolicyPreventRoleSwitch(const bdaddr *bd_addr)
{
    MAKE_PRIM_C(DM_HCI_WRITE_LINK_POLICY_SETTINGS_REQ);
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->link_policy_settings = ENABLE_SNIFF;
    VmSendDmPrim(prim);
    DEBUG_LOG("appLinkPolicyPreventRoleSwitch");
}

/*! \brief Prevent role switching
    \param sink The sink for which to prevent role switching
*/
void appLinkPolicyPreventRoleSwitchForSink(Sink sink)
{
    tp_bdaddr bd_addr;
    if (SinkGetBdAddr(sink, &bd_addr))
    {
        appLinkPolicyPreventRoleSwitch(&bd_addr.taddr.addr);
    }
}

void appLinkPolicyBredrSecureConnectionHostSupportOverrideEnable(const bdaddr *bd_addr)
{
    UNUSED(bd_addr);
}

void appLinkPolicyBredrSecureConnectionHostSupportOverrideRemove(const bdaddr *bd_addr)
{
    UNUSED(bd_addr);
}

/*! \brief Check and update links

    This function checks the role of the individual links and attempts
    role switches when required.

    Role switches are only attempts to avoid scatternets.

    We don't try to role switch when streaming from the phone
    We don't try to role switch when forwarding to the peer


    TWS Standard Setup - Need to setup before starting streaming or SCO

                        Peer Disc.  Peer Conn.
        Handset Disc.      N/A         P:D
        Handset AV         H:M       H:M  P:M
        Handset HFP        H:M       H:M  P:M


    TWS+ Setup - Need to setup before starting streaming with TWS+ codec or SCO

                        Peer Disc.  Peer Conn.
        Handset Disc.      N/A       H:X  P:D
        Handset AV         H:S       H:S  P:M
        Handset HFP        H:S       H:S  P:M
*/
static void appLinkPolicyCheckRole(void)
{
    avInstanceTaskData *av_inst_source, *av_inst_sink, *tws_inst_sink;
    lpTaskData *theLp = LinkPolicyGetTaskData();
    bdaddr handset_bd_addr;

    DEBUG_LOG("appLinkPolicyCheckRole");

    /* We don't need to role switch if don't have a handset connected */
    if (!appDeviceIsHandsetAnyProfileConnected())
    {
        DEBUG_LOG("appLinkPolicyCheckRole, no handset connected");
        return;
    }

    /* We have a handset connected */

    /* Check the links if connected to a TWS Standard device */
    if (appDeviceGetHandsetBdAddr(&handset_bd_addr) && !appDeviceIsTwsPlusHandset(&handset_bd_addr))
    {
        /* Get AV source instance (NULL if not connected) */
        av_inst_source = appAvGetA2dpSource();
        av_inst_sink = appAvGetA2dpSink(AV_CODEC_NON_TWS);
        bool src_streaming = (av_inst_source != NULL) && appA2dpIsStreaming(av_inst_source);
        bool sink_streaming = (av_inst_sink != NULL) && appA2dpIsStreaming(av_inst_sink);

        /* Check the current role of the peer link
         * Only allowed to role switch if we are not streaming or SCO forwarding to the peer */

        DEBUG_LOG("src_streaming %u, appScoFwdIsSending %u, appScoFwdIsConnected %u",
                  src_streaming, ScoFwdIsSending(), ScoFwdIsConnected());

        if (!src_streaming && !ScoFwdIsSending())
        {
            /* Check if we need to role swap sco forwarding link */
            if (ScoFwdIsConnected() && theLp->scofwd_role != hci_role_master)
            {
                DEBUG_LOG("appLinkPolicyCheckRole, ScoFwd link is slave, attemping role switch");
                ConnectionSetRole(&theLp->task, ScoFwdGetSink(), hci_role_master);
                theLp->scofwd_role = hci_role_master;
            }

            /* Try and be master of the source link to the peer */
            if (av_inst_source && (theLp->av_source_role != hci_role_master))
            {
                DEBUG_LOG("appLinkPolicyCheckRole, av_source link is slave, attempting role switch");
                ConnectionSetRole(&theLp->task, appAvGetSink(av_inst_source), hci_role_master);
                theLp->av_source_role = hci_role_master;
            }
        }
        else
        {
            DEBUG_LOG("appLinkPolicyCheckRole, can't switch peer as streaming or sco forwarding");
        }

        /* Check current role of the handset link
         * Only allowed to role switch if we are not streaming and don't have an active SCO */
        if (!sink_streaming && !appHfpIsScoActive())
        {
            /* Try and be the master of the HFP link */
            if (appHfpIsConnected() && (theLp->hfp_role != hci_role_master))
            {
                DEBUG_LOG("appLinkPolicyCheckRole, hfp link is slave, attempting role switch");
                ConnectionSetRole(&theLp->task, appHfpGetSink(), hci_role_master);
                theLp->hfp_role = hci_role_master;
            }

            /* Try and be master of the sink link to the handset */
            if (av_inst_sink && (theLp->av_sink_role != hci_role_master))
            {
                DEBUG_LOG("appLinkPolicyCheckRole, av_sink link is slave, attempting role switch");
                ConnectionSetRole(&theLp->task, appAvGetSink(av_inst_sink), hci_role_master);
                theLp->av_sink_role = hci_role_master;
            }
        }
    }
    else /* TWS+ Handset */
    {
        tws_inst_sink = appAvGetA2dpSink(AV_CODEC_TWS);
        bool tws_sink_streaming = (tws_inst_sink != NULL) && appA2dpIsStreaming(tws_inst_sink);

        /* Check current role of the handset link
         * Only allowed to role switch if we are not streaming and don't have an active SCO */
        if (!tws_sink_streaming && !appHfpIsScoActive())
        {
            /* Try and be the slave of the HFP link */
            if (appHfpIsConnected() && (theLp->hfp_role != hci_role_slave))
            {
                DEBUG_LOG("appLinkPolicyCheckRole, hfp link is master to TWS+ phone, attempting role switch");
                ConnectionSetRole(&theLp->task, appHfpGetSink(), hci_role_slave);
                theLp->hfp_role = hci_role_slave;
            }

            if (tws_inst_sink && (theLp->av_sink_role != hci_role_slave))
            {
                DEBUG_LOG("appLinkPolicyCheckRole, tws_sink link is master to TWS+ phone, attempting role switch");
                ConnectionSetRole(&theLp->task, appAvGetSink(tws_inst_sink), hci_role_slave);
                theLp->av_sink_role = hci_role_slave;
            }
        }
    }
}

/*! \brief Update role of link

    This function is called whenver the role of a link has changed or been
    confirmed, it checks the Bluetooth Address of the updated link against
    the address of the HFP and A2DP links and updates the role of the matching
    link.
*/
static void appLinkPolicyUpdateRole(const bdaddr *bd_addr, hci_role role)
{
#ifdef INCLUDE_AV
    lpTaskData *theLp = LinkPolicyGetTaskData();
    avInstanceTaskData *av_source_inst, *av_sink_inst;

    if (role == hci_role_master)
        appLinkPolicyUpdateLinkSupervisionTimeout(bd_addr);

    /* Check if role confirmation for AV sink link */
    av_sink_inst = appAvGetA2dpSink(AV_CODEC_ANY);
    if (av_sink_inst != NULL)
    {
        tp_bdaddr av_bd_addr;
        if (SinkGetBdAddr(appAvGetSink(av_sink_inst), &av_bd_addr) &&
            BdaddrIsSame(bd_addr, &av_bd_addr.taddr.addr))
        {
            if (role == hci_role_master)
                DEBUG_LOG("appLinkPolicyUpdateRole, av sink, role=master");
            else
                DEBUG_LOG("appLinkPolicyUpdateRole, av sink, role=slave");

            theLp->av_sink_role = role;
        }
    }
    else
    {
        /* We don't have a link so reset the role*/
        theLp->av_sink_role = hci_role_dont_care;
    }

    /* Check if role confirmation for AV source link  */
    av_source_inst = appAvGetA2dpSource();
    if (av_source_inst != NULL)
    {
        tp_bdaddr av_bd_addr;
        if (SinkGetBdAddr(appAvGetSink(av_source_inst), &av_bd_addr) &&
            BdaddrIsSame(bd_addr, &av_bd_addr.taddr.addr))
        {
            if (role == hci_role_master)
                DEBUG_LOG("appLinkPolicyUpdateRole, av relay, role=master");
            else
                DEBUG_LOG("appLinkPolicyUpdateRole, av relay, role=slave");

            theLp->av_source_role = role;
        }
    }
    else
    {
        /* We don't have a link so reset the role*/
        theLp->av_source_role = hci_role_dont_care;
    }

    if (appConfigScoForwardingEnabled() && ScoFwdIsConnected())
    {
        tp_bdaddr scofwd_bdaddr;
        if (   SinkGetBdAddr(ScoFwdGetSink(), &scofwd_bdaddr)
            && BdaddrIsSame(bd_addr,&scofwd_bdaddr.taddr.addr))
        {
            if (role == hci_role_master)
            {
                DEBUG_LOG("appLinkPolicyUpdateRole, scofwd, role=master");
            }
            else
            {
                DEBUG_LOG("appLinkPolicyUpdateRole, scofwd, role=slave");
            }

            theLp->scofwd_role = role;

            /* Inform SCO forwarding of current role */
            ScoFwdNotifyRole(role);
        }
    }
    else
    {
        /* We don't have a link so reset the role*/
        theLp->scofwd_role = hci_role_dont_care;
    }

#ifdef INCLUDE_HFP
    /* Check if role confirmation for HFP link */
    if (appHfpIsConnected())
    {
        tp_bdaddr hfp_bd_addr;
        if (SinkGetBdAddr(appHfpGetSink(), &hfp_bd_addr) &&
            BdaddrIsSame(bd_addr, &hfp_bd_addr.taddr.addr))
        {
            if (role == hci_role_master)
                DEBUG_LOG("appLinkPolicyUpdateRole, hfp, role=master");
            else
                DEBUG_LOG("appLinkPolicyUpdateRole, hfp, role=slave");

            theLp->hfp_role = role;
        }
    }
    else
    {
        /* We don't have a link so reset the role*/
        theLp->hfp_role = hci_role_dont_care;
    }
#endif

#endif
}

/*! \brief Confirmation of link role

    This function is called to handle a CL_DM_ROLE_CFM message, this message is sent from the
    connection library in response to a call to ConnectionGetRole().

    Extract the Bluetooth address of the link and call appLinkPolicyUpdateRole to update the
    role of the appropriate link.

    \param  cfm The received confirmation
*/
static void appLinkPolicyHandleClDmRoleConfirm(CL_DM_ROLE_CFM_T *cfm)
{
    tp_bdaddr bd_addr;

    if (appConfigScoForwardingEnabled())
    {
        CL_DM_ROLE_CFM_T *fwd = PanicUnlessNew(CL_DM_ROLE_CFM_T);
        *fwd = *cfm;

        if(test_task != NULL)
        {
            MessageSend(test_task,CL_DM_ROLE_CFM,fwd);
        }
    }

    if (cfm->role == hci_role_master)
        DEBUG_LOG("appLinkPolicyHandleClDmRoleConfirm, status=%d, role=master", cfm->status);
    else
        DEBUG_LOG("appLinkPolicyHandleClDmRoleConfirm, status=%d, role=slave", cfm->status);

    if (SinkGetBdAddr(cfm->sink, &bd_addr))
    {
        appLinkPolicyUpdateRole(&bd_addr.taddr.addr, cfm->role);
        if (cfm->status == hci_success)
            appLinkPolicyCheckRole();
    }
}

/*! \brief Indication of link role

    This function is called to handle a CL_DM_ROLE_IND message, this message is sent from the
    connection library whenever the role of a link changes.

    Extract the Bluetooth address of the link and call appLinkPolicyUpdateRole to update the
    role of the appropriate link.  Call appLinkPolicyCheckRole to check if we need to perform
    a role switch.

    \param  ind The received indication
*/
static void appLinkPolicyHandleClDmRoleIndication(CL_DM_ROLE_IND_T *ind)
{
    if (ind->role == hci_role_master)
        DEBUG_LOG("appLinkPolicyHandleClDmRoleIndication, master");
    else
        DEBUG_LOG("appLinkPolicyHandleClDmRoleIndication, slave");

    appLinkPolicyUpdateRole(&ind->bd_addr, ind->role);
    appLinkPolicyCheckRole();
}


bool appLinkPolicyHandleConnectionLibraryMessages(MessageId id,Message message, bool already_handled)
{
    switch (id)
    {
        case CL_DM_ROLE_CFM:
            appLinkPolicyHandleClDmRoleConfirm((CL_DM_ROLE_CFM_T *)message);
            return TRUE;

        case CL_DM_ROLE_IND:
            appLinkPolicyHandleClDmRoleIndication((CL_DM_ROLE_IND_T *)message);
            return TRUE;
    }
    return already_handled;
}


/*! \brief Get updated link role

    Request from application to check and update the link role for the
    specified sink.

    \param  sink    The Sink to check and update
*/
void appLinkPolicyUpdateRoleFromSink(Sink sink)
{
    lpTaskData *theLp = LinkPolicyGetTaskData();

    /* Get current role for this link */
    ConnectionGetRole(&theLp->task, sink);
}

void appLinkPolicyHandleClDmAclOpendedIndication(const CL_DM_ACL_OPENED_IND_T *ind)
{
    const bool is_local = !!(~ind->flags & DM_ACL_FLAG_INCOMING);

    /* Set default link supervision timeout if locally inititated (i.e. we're master) */
    if (is_local)
    {
        appLinkPolicyUpdateLinkSupervisionTimeout(&ind->bd_addr.addr);
    }
}

void appLinkPolicyRegisterTestTask(Task task)
{
    test_task = task;
}

#endif
