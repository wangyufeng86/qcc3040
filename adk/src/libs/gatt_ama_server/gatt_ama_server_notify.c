/* Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd. */

#include "gatt_ama_server_private.h"
#include "gatt_ama_server_db.h"
#include "gatt_ama_server_access.h"
#include <gatt_ama_server.h>
#include <sink.h>
#include <stream.h>
#include <stdio.h>
#include <string.h>

extern GAMASS *AmaServer;

#define INVALID_SINK        (0xFFFF)
#define HANDLE_OFFSET       (2)

static gatt_ama_server_status_t sendAttStreamData(uint16 cid, uint16 handle, const uint8_t *data, uint32_t length)
{
    Sink sink = StreamAttServerSink(cid);
    uint16 sink_size = length + HANDLE_OFFSET;

    if (sink == NULL)
        return gatt_ama_server_status_invalid_sink;

    if (SinkSlack(sink) >= sink_size)
    {
        uint16 offset;
        uint8 *sink_data = NULL;

        offset = SinkClaim(sink, sink_size);

        if (offset == INVALID_SINK)
            return gatt_ama_server_status_invalid_sink;

        sink_data = SinkMap(sink) + offset;
        sink_data[0] = handle & 0xFF;
        sink_data[1] = handle >> 8;
        memmove(&sink_data[HANDLE_OFFSET], data, length);

        if (SinkFlush(sink, sink_size))
            return gatt_ama_server_status_success;
        else
            return gatt_ama_server_status_invalid_sink;
    }
    else
    {
        if (!SinkIsValid(sink))
            return gatt_ama_server_status_invalid_sink;
        return gatt_ama_server_status_no_space_available;
    }
}

bool GattAmaServerSendNotification(uint8 *data, uint16 length)
{
    if (sendAttStreamData(AmaServer->cid, AmaServer->start_handle + HANDLE_AMA_ALEXA_RX_CHAR - 1, data, length) != gatt_ama_server_status_success)
    {
        /* Send notification to GATT Manager */
        GattManagerRemoteClientNotify((Task)&AmaServer->lib_task, AmaServer->cid, HANDLE_AMA_ALEXA_RX_CHAR, length, data);
    }
    return TRUE;
}

