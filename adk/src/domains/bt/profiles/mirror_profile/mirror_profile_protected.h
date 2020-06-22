/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Protected interface to mirror profile. These functions should not
            be used by customer code.
*/

#ifndef MIRROR_PROFILE_PROTECTED_H_
#define MIRROR_PROFILE_PROTECTED_H_

#include "mirror_profile.h"
#include "handover_if.h"

/*!
    @brief Exposes mirror profile interface for handover.
*/
extern const handover_interface mirror_handover_if;

/*! \brief Set peer link policy to the requested mode.

    \param mode The new mode to set.
 */
void MirrorProfile_UpdatePeerLinkPolicy(lp_power_mode mode);

/*! \brief Set peer link policy to the requested mode and block waiting for
           the mode to change.

    \param mode The new mode to set.
    \param timeout_ms  The time to wait for the mode to change in milli-seconds.

    \return TRUE if the mode was changed before the timeout expired. FALSE otherwise.
 */
bool MirrorProfile_UpdatePeerLinkPolicyBlocking(lp_power_mode mode, uint32 timeout_ms);

/*! \brief Wait for the link to the peer to enter the defined mode.

    \param mode The mode.
    \param timeout_ms  The time to wait for the mode to change in milli-seconds.

    \return TRUE if the mode was changed before the timeout expired. FALSE otherwise.
*/
bool MirrorProfile_WaitForPeerLinkMode(lp_power_mode mode, uint32 timeout_ms);

/*! \brief Handle Veto check during handover

    \return TRUE If mirror profile is not in connected(ACL/ESCO/A2DP) then veto handover. FALSE otherwise.
*/
bool MirrorProfile_Veto(void);

/*! \brief Get the mirror profile state.

    \return The mirror profile state.
*/
uint16 MirrorProfile_GetMirrorState(void);

#endif
