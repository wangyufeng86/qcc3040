/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the gaia framework API
*/

#include "gaia_framework_command.h"

#include <gaia_features.h>
#include <logging.h>
#include <panic.h>


#include "gaia_framework_feature.h"


/*! \brief Builds and Sends a Gaia V2 acknowledgement packet

    \param vendor_id        Vendor ID

    \param command_id       Command ID

    \param status           Status code

    \param payload_length   Payload length

    \param payload          Payload data
*/
static void gaiaFrameworkCommand_SendV2Response(uint16 vendor_id, uint16 command_id, uint16 status,
                          uint16 payload_length, uint8 *payload);

/*! \brief Builds and Sends a Gaia V2 protocol packet

    \param vendor_id        Vendor ID

    \param command_id       Command ID

    \param status           Status code

    \param payload_length   Payload length

    \param payload          Payload data
*/
static void gaiaFrameworkCommand_SendV2Packet(uint16 vendor_id, uint16 command_id, uint16 status,
                          uint16 payload_length, uint8 *payload);

/*! \brief Builds and Sends a Gaia V2 protocol reply with the V3 API version

    \param command  The unhandled command in order to get information required out of it
*/
static void gaiaFrameworkCommand_ReplyV2GetApiVersion(const GAIA_UNHANDLED_COMMAND_IND_T *command);


static gaia_framework_vendor_specific_handler_fn_t vendor_specific_handler;


void GaiaFrameworkCommand_CommandHandler(Task task, const GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint8 feature_id = gaiaFrameworkCommand_GetFeatureID(command->command_id);
    uint8 pdu_type = gaiaFrameworkCommand_GetPduType(command->command_id);
    uint8 pdu_specific_id = gaiaFrameworkCommand_GetPduSpecificId(command->command_id);

    UNUSED(task);

    DEBUG_LOG("gaiaFramework_CommandHandler GAIA Vendor ID %d , Command Id:0x%04x Len:%d",
                command->vendor_id, command->command_id, command->size_payload);

    if (command->vendor_id == GAIA_V3_VENDOR_ID)
    {

        DEBUG_LOG("gaiaFramework_CommandHandler GAIA Feature ID %d , PDU Type %d , PDU Specifi Id %d",
                feature_id, pdu_type, pdu_specific_id);

        if (!GaiaFrameworkFeature_SendToFeature(feature_id, pdu_type, pdu_specific_id, command->size_payload, command->payload))
        {
            DEBUG_LOG("gaiaFramework_CommandHandler Unexpected GAIA command Feature ID &d", feature_id);
            GaiaFramework_SendError(feature_id, pdu_specific_id, feature_not_supported);
        }
    }
    else if (command->vendor_id == GAIA_VENDOR_QTIL)
    {
        if ((command->command_id & GAIA_COMMAND_TYPE_MASK) == GAIA_COMMAND_GET_API_VERSION)
        {
            DEBUG_LOG("gaiaFramework_CommandHandler send Get API V2 reply with V3 version");
            gaiaFrameworkCommand_ReplyV2GetApiVersion(command);
        }
        else
        {
            DEBUG_LOG("gaiaFramework_CommandHandler send V2 status not supported");
            gaiaFrameworkCommand_SendV2Response(command->vendor_id, command->command_id, GAIA_STATUS_NOT_SUPPORTED, 0, NULL);
        }
    }
    else
    {
        /* Vendpor Specific command handler */
    }
}

void GaiaFrameworkCommand_ResetVendorSpecificHandler(void)
{
    DEBUG_LOG("GaiaFrameworkCommand_ResetVendorSpecificHandler");

    vendor_specific_handler = NULL;
}

bool GaiaFrameworkCommand_RegisterVendorSpecificHandler(gaia_framework_vendor_specific_handler_fn_t command_handler)
{
    bool registered = FALSE;

    DEBUG_LOG("GaiaFrameworkCommand_RegisterVendorSpecificHandler");

    if (vendor_specific_handler == NULL)
    {
        vendor_specific_handler = command_handler;
        registered = TRUE;
    }

    return registered;
}

static void gaiaFrameworkCommand_SendV2Response(uint16 vendor_id, uint16 command_id, uint16 status,
                          uint16 payload_length, uint8 *payload)
{
    gaiaFrameworkCommand_SendV2Packet(vendor_id, command_id | GAIA_ACK_MASK, status,
                     payload_length, payload);
}

static void gaiaFrameworkCommand_SendV2Packet(uint16 vendor_id, uint16 command_id, uint16 status,
                          uint16 payload_length, uint8 *payload)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();

    if(transport) /* Only attempt to send when transport up */
    {
        uint16 packet_length;
        uint8 *packet;
        uint8 flags = GaiaTransportGetFlags(transport);

        DEBUG_LOG("gaiaFramework_SendV2Packet cmd:%d sts:%d len:%d [flags x%x]",command_id,status,payload_length,flags);

        packet_length = GAIA_HEADER_SIZE + payload_length + 2;
        packet = PanicNull(malloc(packet_length));

        if (packet)
        {
            packet_length = GaiaBuildResponse(packet, flags,
                                              vendor_id, command_id,
                                              status, payload_length, payload);

            GaiaSendPacket(transport, packet_length, packet);
        }
    }
}

static void gaiaFrameworkCommand_ReplyV2GetApiVersion(const GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint8 payload[3] = { command->protocol_version, GAIA_V3_VERSION_MAJOR, GAIA_V3_VERSION_MINOR };

    gaiaFrameworkCommand_SendV2Response(command->vendor_id, command->command_id, GAIA_STATUS_SUCCESS, 3, payload);
}