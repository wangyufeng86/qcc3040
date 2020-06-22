/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Peer find role handover interfaces

*/
#ifdef INCLUDE_MIRRORING
#include "app_handover_if.h"
#include "peer_find_role_private.h"
#include "av.h"
#include "mirror_profile.h"

#include <panic.h>
#include <logging.h>

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool peerFindRole_Veto(void);

static void peerFindRole_Commit(bool is_primary);

/******************************************************************************
 * Global Declarations
 ******************************************************************************/
REGISTER_HANDOVER_INTERFACE_NO_MARSHALLING(PEER_FIND_ROLE, peerFindRole_Veto, peerFindRole_Commit);

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*!
    \brief Handle Veto check during handover
    \return Never veto handover.
*/
static bool peerFindRole_Veto(void)
{
    return FALSE;
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void peerFindRole_Commit(bool is_primary)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    pfr->scoring_info.handset_connected = is_primary;

    DEBUG_LOG("peerFindRole_Commit. media_busy %d",pfr->media_busy);

    /*Check if media is active and update the media_busy flag */
    if(is_primary)
    {
        /* Check if SCO is active */
        if(MirrorProfile_IsEscoActive())
        {
            pfr->media_busy |= PEER_FIND_ROLE_CALL_ACTIVE;
        }

        /* Check if streaming is active */
        if(appAvIsStreaming())
        {
            pfr->media_busy |= PEER_FIND_ROLE_AUDIO_STREAMING;
        }
    }
    else
    {
        /* Clear the media_busy flag for media active state in secondary to initiate scan for peer find role */
        uint16 media_disable_mask = PEER_FIND_ROLE_CALL_ACTIVE | PEER_FIND_ROLE_AUDIO_STREAMING;
        pfr->media_busy &= ~media_disable_mask;
    }
    DEBUG_LOG("peerFindRole_Commit. post update media_busy %d",pfr->media_busy);
}

#endif /* INCLUDE_MIRRORING */
