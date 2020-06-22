/*!
\copyright  Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Link policy manager common code for mirroring and non-mirroring products.
*/

#include "adk_log.h"

#include <connection_manager.h>
#include <hfp_profile.h>
#include <scofwd_profile.h>
#include <peer_signalling.h>
#include "scofwd_profile_config.h"

#include <panic.h>
#include <connection.h>
#include <sink.h>

#include "link_policy_config.h"
#include "av_typedef.h"
#include "av.h"

#include <app/bluestack/dm_prim.h>

/*! Make and populate a bluestack DM primitive based on the type.

    \note that this is a multiline macro so should not be used after a
    control statement (if, while) without the use of braces
 */
#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->common.op_code = TYPE; prim->common.length = sizeof(TYPE##_T);

/*! Lower power table for A2DP */
static const lp_power_table powertable_a2dp[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_active,     0,            0,            0,       0,       5},  /* Active mode for 5 sec */
    {lp_passive,    0,            0,            0,       0,       1},  /* Passive mode for 1 sec */
    {lp_sniff,      48,           400,          2,       4,       0}   /* Enter sniff mode*/
};

/*! Lower power table for the A2DP with media streaming as source */
static const lp_power_table powertable_a2dp_streaming_source[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_passive,    0,            0,            0,       0,       0},  /* Passive mode */
};

/*! Lower power table for the A2DP with media streaming as TWS sink */
static const lp_power_table powertable_a2dp_streaming_tws_sink[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_active,     0,            0,            0,       0,       5},  /* Active mode for 5 sec */
    {lp_passive,    0,            0,            0,       0,       1},  /* Passive mode for 1 sec */
    {lp_sniff,      48,           48,           2,       4,       0}   /* Enter sniff mode*/
};

/*! Lower power table for the A2DP with media streaming as sink */
static const lp_power_table powertable_a2dp_streaming_sink[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_active,     0,            0,            0,       0,       5},  /* Active mode for 5 sec */
    {lp_passive,    0,            0,            0,       0,       1},  /* Passive mode for 1 sec */
    {lp_sniff,      48,           48,           2,       4,       0}   /* Enter sniff mode*/
};

/*! Lower power table for the HFP. */
static const lp_power_table powertable_hfp[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_active,     0,            0,            0,       0,       1},  /* Active mode for 1 second */
    {lp_sniff,      48,           800,          2,       1,       0}   /* Enter sniff mode (30-500ms)*/
};

/*! Lower power table for the HFP when an audio connection is open
*/
static const lp_power_table powertable_hfp_sco[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_passive,    0,            0,            0,       0,       1},  /* Passive mode */
    {lp_sniff,      48,           144,          2,       8,       0}   /* Enter sniff mode (30-90ms)*/
};

/*! Power table for the peer link when SCO forwarding active
*/
static const lp_power_table powertable_peer_SCO_fwd[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_active,     0,            0,            0,       0,       5},  /* Active mode for 5 seconds */
    {lp_passive,    0,            0,            0,       0,       10}, /* Passive mode. 10 seconds... redial ? */
    {lp_sniff,      48,           144,          2,       8,       0}   /* Enter sniff mode (30-90ms)*/
};

/*! Lower power table for TWS+ HFP when an audio connection is open */
static const lp_power_table powertable_twsplus_hfp_sco[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_sniff,      48,           144,          2,       8,       0}   /* Stay in sniff mode */
};

/*! Lower power table for AVRCP */
static const lp_power_table powertable_avrcp[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_active,     0,            0,            0,       0,       1},  /* Active mode for 1 second */
    {lp_sniff,      48,           800,          1,       0,       0}   /* Enter sniff mode*/
};

/*! Lower power table for PEER Link wiht TWS+ and streaming */
static const lp_power_table powertable_twsplus_peer_streaming[]=
{
    /* mode,        min_interval, max_interval, attempt, timeout, duration */
    {lp_sniff,      80,          100,             2,       4,      0}   /* Stay in sniff mode */
};

/*! \cond helper */
#define ARRAY_AND_DIM(ARRAY) (ARRAY), ARRAY_DIM(ARRAY)
/*! \endcond helper */

/*!< Link Policy Manager data structure */
lpTaskData  app_lp;

/*! Structure for storing power tables */
struct powertable_data
{
    const lp_power_table *table;
    uint16 rows;
};

/*! Array of structs used to store the power tables for standard phones */
static const struct powertable_data powertables_standard[] = {
    [POWERTABLE_A2DP] =                    {ARRAY_AND_DIM(powertable_a2dp)},
    [POWERTABLE_A2DP_STREAMING_SOURCE] =   {ARRAY_AND_DIM(powertable_a2dp_streaming_source)},
    [POWERTABLE_A2DP_STREAMING_TWS_SINK] = {ARRAY_AND_DIM(powertable_a2dp_streaming_tws_sink)},
    [POWERTABLE_A2DP_STREAMING_SINK] =     {ARRAY_AND_DIM(powertable_a2dp_streaming_sink)},
    [POWERTABLE_HFP] =                     {ARRAY_AND_DIM(powertable_hfp)},
    [POWERTABLE_HFP_SCO] =                 {ARRAY_AND_DIM(powertable_hfp_sco)},
    [POWERTABLE_AVRCP] =                   {ARRAY_AND_DIM(powertable_avrcp)},
    [POWERTABLE_PEER_MODE] =               {ARRAY_AND_DIM(powertable_peer_SCO_fwd)},
};

/*! Array of structs used to store the power tables for TWS+ phones */
static const struct powertable_data powertables_twsplus[] = {
    [POWERTABLE_A2DP] =                    {ARRAY_AND_DIM(powertable_a2dp)},
    [POWERTABLE_A2DP_STREAMING_SOURCE] =   {ARRAY_AND_DIM(powertable_a2dp_streaming_source)},
    [POWERTABLE_A2DP_STREAMING_TWS_SINK] = {ARRAY_AND_DIM(powertable_a2dp_streaming_tws_sink)},
    [POWERTABLE_A2DP_STREAMING_SINK] =     {ARRAY_AND_DIM(powertable_a2dp_streaming_sink)},
    [POWERTABLE_HFP] =                     {ARRAY_AND_DIM(powertable_hfp)},
    [POWERTABLE_HFP_SCO] =                 {ARRAY_AND_DIM(powertable_twsplus_hfp_sco)},
    [POWERTABLE_AVRCP] =                   {ARRAY_AND_DIM(powertable_avrcp)},
    [POWERTABLE_PEER_MODE] =               {ARRAY_AND_DIM(powertable_twsplus_peer_streaming)},
};

void appLinkPolicyUpdateLinkSupervisionTimeout(const bdaddr *bd_addr)
{
    uint16 timeout;

    if (appDeviceIsPeer(bd_addr))
        timeout = appConfigEarbudLinkSupervisionTimeout() * 1000UL / 625;
    else
        timeout = appConfigDefaultLinkSupervisionTimeout() * 1000UL / 625;

    MAKE_PRIM_C(DM_HCI_WRITE_LINK_SUPERV_TIMEOUT_REQ);
    prim->handle = 0;
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->timeout = timeout;
    VmSendDmPrim(prim);
}

/* \brief Re-check and select link settings to reduce power consumption
        where possible

    This function checks what activity the application currently has,
    and decides what the best link settings are for the connection
    to the specified device. This may include full power (#lp_active),
    sniff (#lp_sniff), or passive(#lp_passive) where full power is
    no longer required but the application would prefer not to enter
    sniff mode yet.

    \param bd_addr  Bluetooth address of the device to update link settings
    \param force The link policy will be updated, even if no change in link
    is required.
*/
static void appLinkPolicyUpdatePowerTableImpl(const bdaddr *bd_addr, bool force)
{
    lpPowerTableIndex pt_index = POWERTABLE_UNASSIGNED;
    avInstanceTaskData *av_inst = NULL;
    Sink sink = 0;
    lpPerConnectionState lp_state = {0};
    bdaddr handset_bd_addr;
    bool isTwsPlusHandset;
    bool is_peer = appDeviceIsPeer(bd_addr);
	
    /* Determine if we are in TWS+ mode as we have different settings */
    isTwsPlusHandset = appDeviceGetHandsetBdAddr(&handset_bd_addr) &&
                       appDeviceIsTwsPlusHandset(&handset_bd_addr);

    /* Update peer link power table if we are SCO forwarding */
    if (is_peer && appConfigScoForwardingEnabled() && ScoFwdIsStreaming())
    {
        pt_index = POWERTABLE_PEER_MODE;
        sink = ScoFwdGetSink();
    }
    /* Update peer power table if we are streamng in TWS+ mode */
    else if (is_peer &&
             appDeviceIsHandsetA2dpStreaming() &&
             isTwsPlusHandset)
    {
        av_inst = appAvInstanceFindFromBdAddr(bd_addr);
        if (av_inst)
        {
            DEBUG_LOG("appLinkPolicyUpdatePowerTable Updating peer lp mode due to TWS+ streaming");
            pt_index = POWERTABLE_PEER_MODE;
            sink = appAvGetSink(av_inst);
        }
    }
#ifdef INCLUDE_HFP
    else if (appHfpIsScoActive())
    {
        pt_index = POWERTABLE_HFP_SCO;
        sink = appHfpGetSink();
    }
#endif
#ifdef INCLUDE_MIRRORING
    if (is_peer)
    {
        return;
    }
#endif
#ifdef INCLUDE_AV
    if (pt_index == POWERTABLE_UNASSIGNED)
    {
        av_inst = appAvInstanceFindFromBdAddr(bd_addr);
        if (av_inst)
        {
            if (appA2dpIsStreaming(av_inst))
            {
                if (appA2dpIsSinkNonTwsCodec(av_inst))
                {
                    pt_index = POWERTABLE_A2DP_STREAMING_SINK;
                }
                else if (appA2dpIsSinkTwsCodec(av_inst))
                {
                    pt_index = POWERTABLE_A2DP_STREAMING_TWS_SINK;
                }
                else if (appA2dpIsSourceCodec(av_inst))
                {
                    pt_index = POWERTABLE_A2DP_STREAMING_SOURCE;
                }
            }
            else if (!appA2dpIsDisconnected(av_inst))
            {
                pt_index = POWERTABLE_A2DP;
            }
            else if (appAvrcpIsConnected(av_inst))
            {
                pt_index = POWERTABLE_AVRCP;
            }
            sink = appAvGetSink(av_inst);
        }
    }
#endif
#ifdef INCLUDE_HFP
    if (pt_index == POWERTABLE_UNASSIGNED)
    {
        if (appHfpIsConnected())
        {
            pt_index = POWERTABLE_HFP;
            sink = appHfpGetSink();
        }
    }
#endif

    ConManagerGetLpState(bd_addr, &lp_state);
    if (((pt_index != lp_state.pt_index) || force) &&
        sink && 
        (pt_index < POWERTABLE_UNASSIGNED))
    {
        const struct powertable_data *selected = isTwsPlusHandset ?
                                                    &powertables_twsplus[pt_index] :
                                                    &powertables_standard[pt_index];
        ConnectionSetLinkPolicy(sink, selected->rows, selected->table);
        if(is_peer)
        {
            DEBUG_LOG("appLinkPolicyUpdatePowerTable for peer, index=%d, prev=%d", pt_index, lp_state.pt_index);
        }
        else
        {
            DEBUG_LOG("appLinkPolicyUpdatePowerTable, index=%d, prev=%d", pt_index, lp_state.pt_index);
        }

        lp_state.pt_index = pt_index;
        ConManagerSetLpState(bd_addr, &lp_state);
    }
}

void appLinkPolicyUpdatePowerTable(const bdaddr *bd_addr)
{
    appLinkPolicyUpdatePowerTableImpl(bd_addr, FALSE);
}

void appLinkPolicyForceUpdatePowerTable(const bdaddr *bd_addr)
{
    appLinkPolicyUpdatePowerTableImpl(bd_addr, TRUE);
}

/*! \brief Allow sniff mode
*/
void appLinkPolicyAllowSniffMode(const bdaddr *bd_addr)
{
    MAKE_PRIM_C(DM_HCI_WRITE_LINK_POLICY_SETTINGS_REQ);
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->link_policy_settings = ENABLE_MS_SWITCH | ENABLE_SNIFF;
    VmSendDmPrim(prim);
    DEBUG_LOG("appLinkPolicyAllowSniffMode");
}

/*! \brief Prevent sniff mode
*/
void appLinkPolicyPreventSniffMode(const bdaddr *bd_addr)
{
    MAKE_PRIM_C(DM_HCI_WRITE_LINK_POLICY_SETTINGS_REQ);
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->link_policy_settings = ENABLE_MS_SWITCH;
    VmSendDmPrim(prim);
    DEBUG_LOG("appLinkPolicyPreventSniffMode");
}

/*! \brief Request this device is always ACL master of the link to the defined bd_addr.
           The host will accept incoming ACL connections requesting role switch.
           The host will initiate ACL connection prohibiting role switch.
 */
void appLinkPolicyAlwaysMaster(const bdaddr *bd_addr)
{
    MESSAGE_MAKE(prim, DM_LP_WRITE_ALWAYS_MASTER_DEVICES_REQ_T);
    prim->type = DM_LP_WRITE_ALWAYS_MASTER_DEVICES_REQ;
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->operation = DM_LP_WRITE_ALWAYS_MASTER_DEVICES_ADD;
    VmSendDmPrim(prim);
    DEBUG_LOG("appLinkPolicyAlwaysMaster");
}

void appLinkPolicyHandleAddressSwap(void)
{
    typed_bdaddr bd_addr_primary = {.type = TYPED_BDADDR_PUBLIC, .addr = {0}};
    typed_bdaddr bd_addr_secondary = {.type = TYPED_BDADDR_PUBLIC, .addr= {0}};

    PanicFalse(appDeviceGetPrimaryBdAddr(&bd_addr_primary.addr));
    PanicFalse(appDeviceGetSecondaryBdAddr(&bd_addr_secondary.addr));
    PanicFalse(!BdaddrIsSame(&bd_addr_primary.addr, &bd_addr_secondary.addr));

#ifdef INCLUDE_SM_PRIVACY_1P2
    ConnectionDmUlpSetPrivacyModeReq(&bd_addr_primary, privacy_mode_device);
    ConnectionDmUlpSetPrivacyModeReq(&bd_addr_secondary, privacy_mode_device);
#endif

#ifndef INCLUDE_MIRRORING
    /* Enabling this policy can cause an audio glitch when primary/secondary
    connect when A2DP is active. The glitch is only observed on devices that
    support mirroring. */
    appLinkPolicyAlwaysMaster(&bd_addr_secondary.addr);
#endif

    /* By default, BR/EDR secure connections is disabled.
    TWM requires the link between the two earbuds to have BR/EDR secure connections
    enabled, so selectively enable SC for connections to the other earbud.
    The addresses of both earbuds need to be overridden, as the addresses of the
    two devices swap during handover. Handover will fail if both addresses
    are not overridden. */
    appLinkPolicyBredrSecureConnectionHostSupportOverrideEnable(&bd_addr_primary.addr);
    appLinkPolicyBredrSecureConnectionHostSupportOverrideEnable(&bd_addr_secondary.addr);
}

/*! \brief Initialise link policy manager

    Call as startyp to initialise the link policy manager, set all
    the store rols to 'don't care'.
*/
bool appLinkPolicyInit(Task init_task)
{
    lpTaskData *theLp = LinkPolicyGetTaskData();

#ifdef INCLUDE_AV
    theLp->av_sink_role = hci_role_dont_care;
    theLp->av_source_role = hci_role_dont_care;
#endif
#ifdef INCLUDE_HFP
    theLp->hfp_role = hci_role_dont_care;
#endif
    theLp->scofwd_role = hci_role_dont_care;

    UNUSED(init_task);
    return TRUE;
}
