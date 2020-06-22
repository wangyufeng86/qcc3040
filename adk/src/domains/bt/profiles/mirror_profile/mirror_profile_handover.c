/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Mirror profile handover interfaces

*/
#ifdef INCLUDE_MIRRORING
#include "handover_if.h"
#include "mirror_profile_private.h"
#include "av.h"
#include "a2dp_profile_sync.h"

#include <panic.h>
#include <logging.h>

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*!
    \brief Handle Veto check during handover
    \return If mirror profile is not in connected(ACL/ESCO/A2DP) then veto handover.
*/
bool MirrorProfile_Veto(void)
{
    int32 due;
    /* Count the number of message pending that do not cause veto */
    uint32 no_veto_msg_count = 0;

    if(!MirrorProfile_IsSteadyState(MirrorProfile_GetState()) ||
       (MirrorProfile_GetState() == MIRROR_PROFILE_SUB_STATE_DISCONNECTED) ||
       (MirrorProfilePeerMode_GetState() != MIRROR_PROFILE_PEER_MODE_STATE_SNIFF))
    {
        DEBUG_LOG("MirrorProfile_Veto, vetoing the handover state 0x%x", MirrorProfile_GetState());
        return TRUE;
    }

    /* This is a local message */
    if (MessagePendingFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT, NULL))
    {
        no_veto_msg_count++;
    }

    if (MessagePendingFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_DELAYED_KICK, NULL))
    {
        no_veto_msg_count++;
    }

    if (no_veto_msg_count != MessagesPendingForTask(MirrorProfile_GetTask(), &due))
    {
        DEBUG_LOG("MirrorProfile_Veto, vetoing the handover, message due in %dms", due);
        return TRUE;
    }

    /* Veto any handover if the hfp voice source is routed but it cannot be
       mirrored. If it cannot be mirrored then the mirror eSCO will not be
       connected */
    if ((VoiceSources_GetRoutedSource() == voice_source_hfp_1)
        && !MirrorProfile_IsEscoConnected())
    {
        DEBUG_LOG("MirrorProfile_Veto, vetoing the handover, voice source active but not mirrored");
        return TRUE;
    }

    return FALSE;
}

static bool MirrorProfile_Marshal(const tp_bdaddr *tp_bd_addr,
                                  uint8 *buf,
                                  uint16 length,
                                  uint16 *written)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(written);
    /* nothing to be done */
    return TRUE;
}

static bool MirrorProfile_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                    const uint8 *buf,
                                    uint16 length,
                                    uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(consumed);
    /* nothing to be done */
    return TRUE;
}

static void MirrorProfile_HandoverCommit(const tp_bdaddr *tp_bd_addr, bool is_primary)
{
    UNUSED(tp_bd_addr);

    if (is_primary)
    {
        appDeviceGetSecondaryBdAddr(&MirrorProfile_GetAudioSyncL2capState()->peer_addr);
    }
    else
    {
        appDeviceGetPrimaryBdAddr(&MirrorProfile_GetAudioSyncL2capState()->peer_addr);
    }
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void MirrorProfile_HandoverComplete(bool is_primary)
{
    MirrorProfile_SetRole(is_primary);
    mirror_profile_a2dp_t *a2dp = MirrorProfile_GetA2dpState();

    if (is_primary)
    {
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&(MirrorProfile_GetAclState()->bd_addr));
        if (theInst)
        {
            appA2dpSyncRegister(theInst, MirrorProfile_GetSyncIf());
        }

        /* The new primary kicks the state machine, in case a pending SM kick
           was cancelled on the old primary */
        MirrorProfile_SetDelayKick();
        MirrorProfile_SmKick();
    }
    else
    {
        /* The new secondary ignores any pending SM kicks, it is the new primaries
        responsibility to kick the SM.  */
        MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_DELAYED_KICK);
    }

    if (a2dp->cid != L2CA_CID_INVALID)
    {
        /* Refresh the handover policy on the new stream post-handover */
        Source media_source = StreamL2capSource(a2dp->cid);
        if (media_source)
        {
            SourceConfigure(media_source, STREAM_SOURCE_HANDOVER_POLICY, 0x1);
        }
    }
}

static void MirrorProfile_HandoverAbort(void)
{
    return;
}

extern const handover_interface mirror_handover_if =  {
    &MirrorProfile_Veto,
    &MirrorProfile_Marshal,
    &MirrorProfile_Unmarshal,
    &MirrorProfile_HandoverCommit,
    &MirrorProfile_HandoverComplete,
    &MirrorProfile_HandoverAbort
};

#endif /* INCLUDE_MIRRORING */
