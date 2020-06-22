/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Implementation of scoring for role selection.
            Most scoring inputs are collected automatically and stored in the task 
            data
*/

#include <logging.h>
#include <panic.h>

#include <phy_state.h>
#include <charger_monitor.h>
#include <acceleration.h>
#include <battery_monitor.h>
#include <bt_device.h>
#include <gatt_handler.h>

#include "peer_find_role_scoring.h"
#include "peer_find_role_private.h"

void PeerFindRole_OverrideScore(grss_figure_of_merit_t score)
{
    peer_find_role_scoring_t *scoring = PeerFindRoleGetScoring();

    DEBUG_LOG("PeerFindRole_OverrideScore override 0x%x", score); 

    /* set the override and call recalc, which will update the service. */
    scoring->score_override = score;
    peer_find_role_update_server_score();
}

void peer_find_role_scoring_setup(void)
{
    batteryRegistrationForm battery_reading = {PeerFindRoleGetTask(), battery_level_repres_percent, 3};

    (void)appPhyStateRegisterClient(PeerFindRoleGetTask());
    (void)appChargerClientRegister(PeerFindRoleGetTask());
    (void)appAccelerometerClientRegister(PeerFindRoleGetTask());
    PanicFalse(appBatteryRegister(&battery_reading));
}


/*! Internal function to re-request setting of score

    Send ourselves a message to try updating the score after a short delay.
*/
static void peer_find_role_request_score_update_later(void)
{
    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_UPDATE_SCORE);
    MessageSendLater(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_UPDATE_SCORE, NULL, 10);
}

void peer_find_role_calculate_score(void)
{
    peer_find_role_scoring_t *scoring = PeerFindRoleGetScoring();
    grss_figure_of_merit_t score = PEER_FIND_ROLE_SCORE_LOWEST;

    score += PEER_FIND_ROLE_SCORE(PHY_STATE, (uint16)scoring->phy_state);
    score += PEER_FIND_ROLE_SCORE(POWERED, scoring->charger_present);
    score += PEER_FIND_ROLE_SCORE(BATTERY, scoring->battery_level_percent);
    score += PEER_FIND_ROLE_SCORE(ROLE_CP, peer_find_role_is_central());
    score += PEER_FIND_ROLE_SCORE(HANDSET_CONNECTED, scoring->handset_connected);

    scoring->last_local_score = score;

    DEBUG_LOG("peer_find_role_calculate_score. Score 0x%x (%d)", score, score);
}

grss_figure_of_merit_t peer_find_role_score(void)
{
    peer_find_role_scoring_t *scoring = PeerFindRoleGetScoring();

    if (scoring->score_override)
    {
        DEBUG_LOG("peer_find_role_score. Overriden:x%x (%d)",
                    scoring->score_override, scoring->score_override);
        return scoring->score_override;
    }

    return scoring->last_local_score;
}

static void peer_find_role_set_server_score(grss_figure_of_merit_t score, bool force_notify)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_set_server_score cid 0x%x score 0x%x", pfr->gatt_cid, score);

    if (INVALID_CID != pfr->gatt_cid)
    {
        bool server_score_set;

        server_score_set = GattRoleSelectionServerSetFigureOfMerit(&pfr->role_selection_server,
                                                                   pfr->gatt_cid,
                                                                   score,
                                                                   force_notify);

        if (!server_score_set)
        {
            DEBUG_LOG("peer_find_role_set_server_score. Unable to set. Try again");
            peer_find_role_request_score_update_later();
        }
    }
}

void peer_find_role_update_server_score(void)
{
    grss_figure_of_merit_t score;

    peer_find_role_calculate_score();
    score = peer_find_role_score();

    DEBUG_LOG("peer_find_role_update_server_score score 0x%x", score);

    peer_find_role_set_server_score(score, TRUE);
}

/*  Reset the figure of merit in the gatt server by setting it to
    GRSS_FIGURE_OF_MERIT_INVALID. */
void peer_find_role_reset_server_score(void)
{
    peer_find_role_scoring_t *scoring = PeerFindRoleGetScoring();
    DEBUG_LOG("peer_find_role_reset_server_score");

    scoring->last_local_score = GRSS_FIGURE_OF_MERIT_INVALID;
    peer_find_role_set_server_score(scoring->last_local_score, FALSE);
}

void PeerFindRole_SetPeerScore(grss_figure_of_merit_t score)
{
    peer_find_role_scoring_t *scoring = PeerFindRoleGetScoring();
    scoring->peer_score = score;
}

grss_figure_of_merit_t PeerFindRole_GetPeerScore(void)
{
    peer_find_role_scoring_t *scoring = PeerFindRoleGetScoring();
    return scoring->peer_score;
}
