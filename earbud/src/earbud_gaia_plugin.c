/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the gaia framework earbud plugin
*/

#include "earbud_gaia_plugin.h"

#include <logging.h>
#include <panic.h>

#include "gaia_framework_feature.h"


/*! \brief Function pointer definition for the command handler

    \param pdu_id      PDU specific ID for the message

    \param length      Length of the payload

    \param payload     Payload data
*/
static void earbudGaiaPlugin_MainHandler(uint8 pdu_id, uint8 payload_length, const uint8 *payload);

/*! \brief Function that sends all available notifications
*/
static void earbudGaiaPlugin_SendAllNotifications(void);


void EarbudGaiaPlugin_Init(void)
{
    DEBUG_LOG("EarbydGaiaPlugin_Init");

    GaiaFramework_RegisterFeature(GAIA_EARBUD_FEATURE_ID, EARBUD_GAIA_PLUGIN_VERSION, earbudGaiaPlugin_MainHandler, earbudGaiaPlugin_SendAllNotifications);
}

static void earbudGaiaPlugin_MainHandler(uint8 pdu_id, uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("earbudGaiaPlugin_MainHandler called for %d", pdu_id);

    UNUSED(payload_length);
    UNUSED(payload);

    switch (pdu_id)
    {
        default:
            DEBUG_LOG("earbudGaiaPlugin_MainHandler UNHANDLED call for %d", pdu_id);
            /*! @todo VMCSA-3608 - send error unknown PDU id */
            break;
    }
}


static void earbudGaiaPlugin_SendAllNotifications(void)
{
    DEBUG_LOG("earbudGaiaPlugin_SendAllNotifications");
}

void EarbudGaiaPlugin_PrimaryAboutToChange(earbud_plugin_handover_types_t handover_type, uint8 delay)
{
    DEBUG_LOG("EarbudGaiaPlugin_PrimaryHasChanged");

    uint8 payload[2] = {handover_type, delay};

    GaiaFramework_SendNotification(GAIA_EARBUD_FEATURE_ID, primary_earbud_about_to_change, 2, payload);
}
