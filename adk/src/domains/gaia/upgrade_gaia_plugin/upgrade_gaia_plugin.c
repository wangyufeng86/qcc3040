/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the  upgrade gaia framework plugin
*/

#include "upgrade_gaia_plugin.h"

#include <gaia.h>
#include <logging.h>
#include <panic.h>


/*! \brief Function pointer definition for the command handler

    \param pdu_id      PDU specific ID for the message

    \param length      Length of the payload

    \param payload     Payload data
*/
static void upgradeGaiaPlugin_MainHandler(uint8 pdu_id, uint8 payload_length, const uint8 *payload);

/*! \brief Function that sends all available notifications
*/
static void upgradeGaiaPlugin_SendAllNotifications(void);

static void upgradeGaiaPlugin_UpgradeConnect(void);
static void upgradeGaiaPlugin_UpgradeDisonnect(void);
static void upgradeGaiaPlugin_UpgradeControl(uint8 payload_length, const uint8 *payload);
static void upgradeGaiaPlugin_GetDataEndpoint(void);
static void upgradeGaiaPlugin_SetDataEndpoint(const uint8 *payload);

static void upgradeGaiaPlugin_SendDataIndicationNotification(uint8 payload_length, const uint8 *payload);
static void upgradeGaiaPlugin_SendDataConfirmResponse(const uint8 *payload);


void UpgradeGaiaPlugin_Init(void)
{
    DEBUG_LOG("UpgradeGaiaPlugin_Init");

    GaiaFramework_RegisterFeature(GAIA_DFU_FEATURE_ID, UPGRADE_GAIA_PLUGIN_VERSION, upgradeGaiaPlugin_MainHandler, upgradeGaiaPlugin_SendAllNotifications);
}

static void upgradeGaiaPlugin_MainHandler(uint8 pdu_id, uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("upgradeGaiaPlugin_MainHandler called for %d", pdu_id);

    switch (pdu_id)
    {
        case upgrade_connect:
            upgradeGaiaPlugin_UpgradeConnect();
            break;

        case upgrade_disconnect:
            upgradeGaiaPlugin_UpgradeDisonnect();
            break;

        case upgrade_control:
            upgradeGaiaPlugin_UpgradeControl(payload_length, payload);
            break;

        case get_data_endpoint:
            upgradeGaiaPlugin_GetDataEndpoint();
            break;

        case set_data_endpoint:
            upgradeGaiaPlugin_SetDataEndpoint(payload);
            break;

        case send_upgrade_data_indication_notification:
            upgradeGaiaPlugin_SendDataIndicationNotification(payload_length, payload);
            break;

        case send_upgrade_data_cfm_response:
            upgradeGaiaPlugin_SendDataConfirmResponse(payload);
            break;

        default:
            DEBUG_LOG("upgradeGaiaPlugin_MainHandler UNHANDLED call for %d", pdu_id);
            break;
    }
}


static void upgradeGaiaPlugin_UpgradeConnect(void)
{
    DEBUG_LOG("upgradeGaiaPlugin_UpgradeConnect");

    if (GaiaUpgradeConnect())
    {
        GaiaFramework_SendResponse(GAIA_DFU_FEATURE_ID, upgrade_connect, 0, NULL);
    }
    else
    {
        DEBUG_LOG("upgradeGaiaPlugin_UpgradeConnect, INCORRECT STATE");
        GaiaFramework_SendError(GAIA_DFU_FEATURE_ID, upgrade_connect, GAIA_STATUS_INCORRECT_STATE);
    }
}

static void upgradeGaiaPlugin_UpgradeDisonnect(void)
{
    DEBUG_LOG("upgradeGaiaPlugin_UpgradeDisonnect");

    if (GaiaUpgradeDisconnect())
    {
        GaiaFramework_SendResponse(GAIA_DFU_FEATURE_ID, upgrade_disconnect, 0, NULL);
    }
    else
    {
        DEBUG_LOG("upgradeGaiaPlugin_UpgradeConnect, INCORRECT STATE");
        GaiaFramework_SendError(GAIA_DFU_FEATURE_ID, upgrade_disconnect, GAIA_STATUS_INCORRECT_STATE);
    }
}

static void upgradeGaiaPlugin_UpgradeControl(uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("upgradeGaiaPlugin_UpgradeControl");

    if (!GaiaUpgradeControl((uint8 *)payload, payload_length))
    {
        DEBUG_LOG("upgradeGaiaPlugin_UpgradeConnect, INCORRECT STATE");
        GaiaFramework_SendError(GAIA_DFU_FEATURE_ID, upgrade_control, GAIA_STATUS_INCORRECT_STATE);
    }
}

static void upgradeGaiaPlugin_GetDataEndpoint(void)
{
    uint8 data_endpoint_mode = GaiaGetDataEndpointMode();

    DEBUG_LOG("upgradeGaiaPlugin_GetDataEndpoint");

    GaiaFramework_SendResponse(GAIA_DFU_FEATURE_ID, get_data_endpoint, 1, &data_endpoint_mode);
}

static void upgradeGaiaPlugin_SetDataEndpoint(const uint8 *payload)
{
    DEBUG_LOG("upgradeGaiaPlugin_SetDataEndpoint");

    if (GaiaSetDataEndpointMode(payload[0]))
    {
        GaiaFramework_SendResponse(GAIA_DFU_FEATURE_ID, set_data_endpoint, 0, NULL);
    }
    else
    {
        DEBUG_LOG("upgradeGaiaPlugin_UpgradeConnect, INCORRECT STATE");
        GaiaFramework_SendError(GAIA_DFU_FEATURE_ID, set_data_endpoint, GAIA_STATUS_INVALID_PARAMETER);
    }
}

static void upgradeGaiaPlugin_SendDataConfirmResponse(const uint8 *payload)
{
    uint8 status_code = payload[0];

    DEBUG_LOG("upgradeGaiaPlugin_SendDataConfirmResponse");

    GaiaFramework_SendResponse(GAIA_DFU_FEATURE_ID, upgrade_control, sizeof(status_code), &status_code);
}

static void upgradeGaiaPlugin_SendAllNotifications(void)
{
    DEBUG_LOG("upgradeGaiaPlugin_SendAllNotifications");
}

static void upgradeGaiaPlugin_SendDataIndicationNotification(uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("upgradeGaiaPlugin_SendAllNotifications");

    GaiaFramework_SendNotification(GAIA_DFU_FEATURE_ID, upgrade_data_indication, payload_length, (uint8 *)payload);
}
