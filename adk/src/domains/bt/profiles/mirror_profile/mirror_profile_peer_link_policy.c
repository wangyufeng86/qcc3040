/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Peer earbud link policy control.
*/

#ifdef INCLUDE_MIRRORING

#include <app/bluestack/dm_prim.h>
#include <connection_manager.h>
#include <panic.h>
#include <message.h>

#include "mirror_profile_private.h"

/*! Common sniff params */
#define SNIFF_INVERVAL 50
#define SNIFF_ATTEMPT 2
#define SNIFF_TIMEOUT 2

/*! When idle (not A2DP or eSCO mirroring), subrating is used to reduce power
    consumption when no data is being transferred */
#define SNIFF_SUBRATE_IDLE 6
#define SNIFF_SUBRATE_TIMEOUT_IDLE 6

/*! Subrating is disabled completely when eSCO is active */
#define SNIFF_SUBRATE_ESCO_ACTIVE 1
#define SNIFF_SUBRATE_TIMEOUT_ESCO_ACTIVE 0

/*! Subrating is enabled during A2DP shadowing to minimise impact on shadowing
    whilst allowing reasonable data transfer rates (e.g. for audio sync messages). */
#define SNIFF_SUBRATE_A2DP_ACTIVE 2
#define SNIFF_SUBRATE_TIMEOUT_A2DP_ACTIVE 0

#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->common.op_code = TYPE; prim->common.length = sizeof(TYPE##_T);

#define SNIFF_MODE_REQ_COMMON { DM_HCI_SNIFF_MODE_REQ, sizeof(DM_HCI_SNIFF_MODE_REQ_T) }

#define SNIFF_SUB_RATE_REQ_COMMON { DM_HCI_SNIFF_SUB_RATE_REQ, sizeof(DM_HCI_SNIFF_SUB_RATE_REQ_T) }

/* The number of sniff instances to account for when computing expected peer link transmission time. */
#define NUM_SNIFF_INSTANCES 2

static void mirrorProfile_PeerEnterSniff(const DM_HCI_SNIFF_MODE_REQ_T *params)
{
    MESSAGE_MAKE(prim, DM_HCI_SNIFF_MODE_REQ_T);
    *prim = *params;
    BdaddrConvertVmToBluestack(&prim->bd_addr, &MirrorProfile_GetAudioSyncL2capState()->peer_addr);
    VmSendDmPrim(prim);
    MIRROR_LOG("mirrorProfile_PeerEnterSniff min:%d max:%d attempt:%d timeout:%d", params->min_interval,
                    params->min_interval, params->attempt, params->timeout);
}

static void mirrorProfile_PeerExitSniff(void)
{
    MAKE_PRIM_C(DM_HCI_EXIT_SNIFF_MODE_REQ);
    BdaddrConvertVmToBluestack(&prim->bd_addr, &MirrorProfile_GetAudioSyncL2capState()->peer_addr);
    VmSendDmPrim(prim);
    MIRROR_LOG("mirrorProfile_PeerExitSniff");
}

static void mirrorProfile_PeerSniffSubrate(const DM_HCI_SNIFF_SUB_RATE_REQ_T *params)
{
    MESSAGE_MAKE(prim, DM_HCI_SNIFF_SUB_RATE_REQ_T);
    *prim = *params;
    BdaddrConvertVmToBluestack(&prim->bd_addr, &MirrorProfile_GetAudioSyncL2capState()->peer_addr);
    VmSendDmPrim(prim);
    MIRROR_LOG("mirrorProfile_PeerSniffSubrate maxrl:%d minrt:%d minlt:%d",
                params->max_remote_latency, params->min_remote_timeout, params->min_local_timeout);
}

static const DM_HCI_SNIFF_SUB_RATE_REQ_T sniff_sub_rate_req_idle = {
    .common             = SNIFF_SUB_RATE_REQ_COMMON,
    .max_remote_latency = SNIFF_SUBRATE_IDLE * SNIFF_INVERVAL,
    .min_remote_timeout = SNIFF_SUBRATE_TIMEOUT_IDLE * SNIFF_INVERVAL,
    .min_local_timeout = SNIFF_SUBRATE_TIMEOUT_IDLE * SNIFF_INVERVAL,
};

static const DM_HCI_SNIFF_SUB_RATE_REQ_T sniff_sub_rate_req_a2dp_active = {
    .common             = SNIFF_SUB_RATE_REQ_COMMON,
    .max_remote_latency = SNIFF_SUBRATE_A2DP_ACTIVE * SNIFF_INVERVAL,
    .min_remote_timeout = SNIFF_SUBRATE_TIMEOUT_A2DP_ACTIVE * SNIFF_INVERVAL,
    .min_local_timeout = SNIFF_SUBRATE_TIMEOUT_A2DP_ACTIVE * SNIFF_INVERVAL,
};

static const DM_HCI_SNIFF_SUB_RATE_REQ_T sniff_sub_rate_req_esco_active = {
    .common             = SNIFF_SUB_RATE_REQ_COMMON,
    .max_remote_latency = SNIFF_SUBRATE_ESCO_ACTIVE * SNIFF_INVERVAL,
    .min_remote_timeout = SNIFF_SUBRATE_TIMEOUT_ESCO_ACTIVE * SNIFF_INVERVAL,
    .min_local_timeout = SNIFF_SUBRATE_TIMEOUT_ESCO_ACTIVE * SNIFF_INVERVAL,
};

static const DM_HCI_SNIFF_MODE_REQ_T sniff_mode_req = {
    .common       = SNIFF_MODE_REQ_COMMON,
    .max_interval = SNIFF_INVERVAL,
    .min_interval = SNIFF_INVERVAL,
    .attempt      = SNIFF_ATTEMPT,
    .timeout      = SNIFF_TIMEOUT,
};

static void mirrorProfile_SendLinkPolicyTimeout(void)
{
    MessageSendLater(MirrorProfile_GetTask(), MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT,
                     NULL, mirrorProfileConfig_LinkPolicyIdleTimeout());
}

void MirrorProfile_PeerLinkPolicyInit(void)
{
    MIRROR_LOG("MirrorProfile_PeerLinkPolicyInit");
    /* Start with most agressive subrating parameters */
    mirrorProfile_PeerSniffSubrate(&sniff_sub_rate_req_esco_active);
    MirrorProfile_Get()->peer_lp_mode = PEER_LP_ESCO;
    MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT);
    /* Start in active mode, then move to idle on timeout */
    mirrorProfile_SendLinkPolicyTimeout();
}

void MirrorProfile_PeerLinkPolicySetA2dpActive(void)
{
    MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT);

    if (MirrorProfile_IsAudioSyncL2capConnected() &&
        MirrorProfile_Get()->peer_lp_mode != PEER_LP_A2DP)
    {
        mirrorProfile_PeerSniffSubrate(&sniff_sub_rate_req_a2dp_active);
        MirrorProfile_Get()->peer_lp_mode = PEER_LP_A2DP;
    }
}

void MirrorProfile_PeerLinkPolicySetEscoActive(void)
{
    MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT);

    if (MirrorProfile_IsAudioSyncL2capConnected() &&
        MirrorProfile_Get()->peer_lp_mode != PEER_LP_ESCO)
    {
        mirrorProfile_PeerSniffSubrate(&sniff_sub_rate_req_esco_active);
        MirrorProfile_Get()->peer_lp_mode = PEER_LP_ESCO;
    }
}

void MirrorProfile_PeerLinkPolicySetIdle(void)
{
    MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT);

    if (MirrorProfile_IsAudioSyncL2capConnected() &&
        MirrorProfile_Get()->peer_lp_mode != PEER_LP_IDLE)
    {
        /* Move back to most agressive active parameters before timeout, then revert
           to idle parameters */
        MirrorProfile_PeerLinkPolicySetEscoActive();
        mirrorProfile_SendLinkPolicyTimeout();
    }
}

void MirrorProfile_PeerLinkPolicyHandleIdleTimeout(void)
{
    if (MirrorProfile_IsAudioSyncL2capConnected())
    {
        mirrorProfile_PeerSniffSubrate(&sniff_sub_rate_req_idle);
        MirrorProfile_Get()->peer_lp_mode = PEER_LP_IDLE;
    }
}

void MirrorProfile_UpdatePeerLinkPolicy(lp_power_mode mode)
{
    switch (mode)
    {
        case lp_sniff:
            mirrorProfile_PeerEnterSniff(&sniff_mode_req);
        break;
        case lp_active:
            mirrorProfile_PeerExitSniff();
        break;
        case lp_passive:
        break;
        default:
        break;
    }
    MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_IDLE_PEER_ENTER_SNIFF);
}

bool MirrorProfile_WaitForPeerLinkMode(lp_power_mode mode, uint32 timeout_ms)
{
    tp_bdaddr tpbdaddr;
    uint32 timeout;

    BdaddrTpFromBredrBdaddr(&tpbdaddr, &MirrorProfile_GetAudioSyncL2capState()->peer_addr);
    timeout = VmGetClock() + timeout_ms;
    do
    {
        lp_power_mode current_mode = lp_active;

        switch (VmGetAclMode(&tpbdaddr))
        {
            case HCI_BT_MODE_ACTIVE:
                current_mode = lp_active;
                break;

            case HCI_BT_MODE_SNIFF:
                current_mode = lp_sniff;
                break;

            case HCI_BT_MODE_MAX:
                /* Connection no longer exists */
                MIRROR_LOG("MirrorProfile_WaitForPeerLinkMode link lost");
                return FALSE;

            default:
                Panic();
                break;
        }

        if (current_mode == mode)
        {
            return TRUE;
        }

    } while(VmGetClock() < timeout);

    return FALSE;
}

bool MirrorProfile_UpdatePeerLinkPolicyBlocking(lp_power_mode mode, uint32 timeout_ms)
{
    MirrorProfile_UpdatePeerLinkPolicy(mode);
    return MirrorProfile_WaitForPeerLinkMode(mode, timeout_ms);
}

uint32 MirrorProfilePeerLinkPolicy_GetExpectedTransmissionTime(void)
{
    uint32 peer_relay_time_usecs = 0;
    switch (MirrorProfile_Get()->peer_lp_mode)
    {
    case PEER_LP_IDLE:
        peer_relay_time_usecs = SNIFF_INTERVAL_US(SNIFF_SUBRATE_IDLE * SNIFF_INVERVAL * NUM_SNIFF_INSTANCES);
        break;
    case PEER_LP_A2DP:
        peer_relay_time_usecs = SNIFF_INTERVAL_US(SNIFF_SUBRATE_A2DP_ACTIVE * SNIFF_INVERVAL * NUM_SNIFF_INSTANCES);
        break;
    case PEER_LP_ESCO:
        peer_relay_time_usecs = SNIFF_INTERVAL_US(SNIFF_SUBRATE_ESCO_ACTIVE * SNIFF_INVERVAL * NUM_SNIFF_INSTANCES);
        break;
    default:
        Panic();
        break;
    }
    return peer_relay_time_usecs;
}


#endif
