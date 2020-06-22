/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.

FILE NAME
    gatt_proxy_stream.c

DESCRIPTION
    Proxy data from client to server using streams
*/

#include "gatt_proxy_stream.h"

#include <byte_utils.h>
#include <message.h>
#include <print.h>
#include <vmtypes.h>
#include <string.h>
#include <stream.h>
#include <source.h>
#include <sink.h>
#include <panic.h>



#define SIZE_HANDLE 2

/* We currently only need to proxy up to 2 AGs with 2 GATT services with 2 notify characteristics */
#define MAX_PROXIES 8
 
struct gatt_proxy_stream_tag
{
    uint16 cid;
    uint16 client_handle;
    uint16 server_handle;
    Source source;
};

typedef enum {
    write_failed_no_space_available,
    write_failed_sink_invalid,
    write_success
} write_status_enum_t;

static struct gatt_proxy_stream_tag gatt_proxies[MAX_PROXIES];

static void gattProxyStreamHandler(Task task, MessageId id, Message message);
static const TaskData proxy_task = {gattProxyStreamHandler};

/******************************************************************************/
static uint8* writeHandle(uint8* send, uint16 send_handle)
{
    send[0] = LOBYTE(send_handle);
    send[1] = HIBYTE(send_handle);
    return send + SIZE_HANDLE;
}

/******************************************************************************/
static uint16 getAttHandle(const uint8* stream_data, uint16 stream_data_size)
{
    if ((stream_data == NULL) || (stream_data_size < SIZE_HANDLE))
        Panic();

    return MAKEWORD(stream_data[0], stream_data[1]);
}

/******************************************************************************/
static bool isSourceUsed(Source source)
{
    unsigned i;
    for(i = 0; i < MAX_PROXIES; i++)
    {
        if (gatt_proxies[i].source == source)
            return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
static gatt_proxy_stream_t findProxy(Source source, uint16 client_handle)
{
    unsigned i;
    for(i = 0; i < MAX_PROXIES; i++)
    {
        if ((gatt_proxies[i].source == source) && (gatt_proxies[i].client_handle == client_handle))
            return &gatt_proxies[i];
    }
    return NULL;
}

/******************************************************************************/
static void panicIfSinkValid(Sink sink)
{
    if(SinkIsValid(sink))
        Panic();
}

/******************************************************************************/
#ifdef DEBUG_PRINT_ENABLED
static void debugData(const uint8* source_data, uint16 source_size)
{
    uint16 i;
    PRINT(("Proxy: ["));
    for(i = 0; i < source_size; i++)
        PRINT(("%02x ", source_data[i]));
    PRINT(("]\n"));
}
#else
#define debugData(source_data, source_size) ((void)0)
#endif

/******************************************************************************/
static write_status_enum_t writeDataToProxy(const uint8* source_data, uint16 source_size, gatt_proxy_stream_t proxy)
{
    Sink sink;
    uint8* sink_data;
    
    sink = StreamAttServerSink(proxy->cid);
    PRINT(("Proxy: Forward on %d 0x%04x -> 0x%04x\n", proxy->cid, proxy->client_handle, proxy->server_handle));
    debugData(source_data, source_size);
    if(SinkSlack(sink) >= source_size)
    {
        uint16 offset = SinkClaim(sink, source_size);

        if (offset == 0xFFFF)
        {
            panicIfSinkValid(sink);
            return write_failed_sink_invalid;
        }

        sink_data = SinkMap(sink);

        if (sink_data == NULL)
        {
            panicIfSinkValid(sink);
            return write_failed_sink_invalid;
        }

        sink_data = writeHandle(sink_data, proxy->server_handle);
        memmove(sink_data, source_data + SIZE_HANDLE, (size_t)(source_size - SIZE_HANDLE));
        
        if (SinkFlush(sink, source_size))
            return write_success;
        else
        {
            panicIfSinkValid(sink);
            return write_failed_sink_invalid;
        }
    }
    else
    {
        if (!SinkIsValid(sink))
            return write_failed_sink_invalid;
        return write_failed_no_space_available;
    }
}

/******************************************************************************/
static void gattProxyStreamHandler(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case MESSAGE_MORE_DATA:
        {
            const MessageMoreData* msg = (const MessageMoreData*)message;
            uint16 source_size = SourceBoundary(msg->source);
            
            while(source_size)
            {
                const uint8* source_data = SourceMap(msg->source);
                gatt_proxy_stream_t proxy = findProxy(msg->source, getAttHandle(source_data, source_size));
                
                if (proxy)
                {
                    if (writeDataToProxy(source_data, source_size, proxy) == write_failed_no_space_available)
                    {
                        PRINT(("Proxy: Sink Full\n"));
                        MESSAGE_MAKE(m, MessageMoreData);
                        m->source = msg->source;
                        MessageSend(task, id, m);
                        break;
                    }
                }
                else
                    PRINT(("Proxy: No proxy found, drop source data\n"));

                SourceDrop(msg->source, source_size);
                source_size = SourceBoundary(msg->source);
            }
        }
        break;
        
        default:
        break;
    }
}

/******************************************************************************/
void GattProxyStreamReset(void)
{
    unsigned i;
    for(i = 0; i < MAX_PROXIES; i++)
        GattProxyStreamDestroy(&gatt_proxies[i]);
}

/******************************************************************************/
gatt_proxy_stream_t GattProxyStreamNew(uint16 cid, uint16 client_handle, uint16 server_handle)
{
    gatt_proxy_stream_t proxy = NULL;
    
    if(cid && client_handle && server_handle)
    {
        proxy = findProxy(NULL, 0);
        
        if(proxy)
        {
            Source source = StreamAttClientSource(cid);

            /* Ensure client and server sinks are initialised */
            (void)StreamAttClientSink(cid);
            (void)StreamAttServerSink(cid);
            SourceConfigure(source, VM_SOURCE_MESSAGES, VM_MESSAGES_SOME);
            MessageStreamTaskFromSource(source, (Task)&proxy_task);

            if(StreamAttAddHandle(source, client_handle))
            {
                PRINT(("Proxy: New on %d 0x%04x -> 0x%04x\n", cid, client_handle, server_handle));

                proxy->cid = cid;
                proxy->server_handle = server_handle;
                proxy->client_handle = client_handle;
                proxy->source = source;
            }
            else
            {
                PRINT(("Proxy: Failed to create proxy, add handle failed\n"));
                proxy = NULL;
            }
        }
    }
    else
    {
        PRINT(("Proxy: Failed to create proxy, list is full\n"));
    }

    return proxy;
}

/******************************************************************************/
void GattProxyStreamDestroy(gatt_proxy_stream_t proxy)
{
    Source source;

    PanicNull(proxy);
    PRINT(("Proxy: Destroy %d 0x%04x -> 0x%04x\n", proxy->cid, proxy->client_handle, proxy->server_handle));

    source = proxy->source;
    memset(proxy, 0, sizeof(struct gatt_proxy_stream_tag));

    if (isSourceUsed(source) == FALSE)
        MessageStreamTaskFromSource(source, NULL);
}
