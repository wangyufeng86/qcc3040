/*****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_parse.c

*/

#include <panic.h>
#include <stream.h>
#include <util.h>
#include <vm.h>
#include <transform.h>
#include <gatt_ama_server.h>
#include "ama_receive_command.h"
#include "logging.h"
#include "ama_private.h"
#include <stdlib.h>

typedef enum
{
    amaParseTransportStateIdle,
    amaParseTransportStateHeader,
    amaParseTransportStateLength,
    amaParseTransportStateBody,
    amaParseTransportLast
}amaParseTransportState_t;

#define PACKET_INVALID_LENGTH 0xFFFF
#define AMA_HEADER_VERSION_OFFSET 12
#define AMA_HEADER_VERSION_MASK 0xF000
#define AMA_HEADER_STREAM_ID_OFFSET 7
#define AMA_HEADER_STREAM_ID_MASK 0x0F80
#define AMA_HEADER_LENTGH_MASK 0x0001

#define AMA_TRANSPORT_STREAM_ID_CONTROL 0x0
#define AMA_TRANSPORT_STREAM_ID_VOICE 0x1

static uint16 header = 0;
static uint16 protoPktLength = PACKET_INVALID_LENGTH;
static uint8* protoBody = NULL;
static uint16 bytesCopied = 0;
static amaParseTransportState_t amaParseState = amaParseTransportStateIdle;
static uint16 amaVersion = 0;

void AmaParse_ResetState(void)
{
    header = 0;
    protoPktLength = PACKET_INVALID_LENGTH;

    free(protoBody);
    protoBody = NULL;

    bytesCopied = 0;
    amaParseState = amaParseTransportStateIdle;
}

bool AmaParse_ParseData(const uint8* stream, uint16 size)
{

    DEBUG_LOG("AMA TRANSPORT AmaParse_ParseData, size is %d", size );

    while(size)
    {
        switch(amaParseState)
        {
            case amaParseTransportStateIdle:
                if(size > 0)
                {
                    header = (uint16)stream[0]<<8;
                    amaParseState = amaParseTransportStateHeader;
                    stream++;
                    size--;
                }

                if(size > 0)
                {
                    header |= (uint16)stream[0];
                    amaParseState = amaParseTransportStateLength;
                    stream++;
                    size--;
                }
                break;

            case amaParseTransportStateHeader:
                if(size > 0)
                {
                    header |= (uint16)stream[0];
                    amaParseState = amaParseTransportStateLength;
                    stream++;
                    size--;
                }
                break;

            case amaParseTransportStateLength:
                if(size > 0)
                {
                    if(header & AMA_HEADER_LENTGH_MASK)
                    {
                        if(protoPktLength == PACKET_INVALID_LENGTH)
                        {
                            protoPktLength = (uint16)stream[0]<<8;
                        }
                        else
                        {
                            protoPktLength |= (uint16)stream[0];
                            amaParseState = amaParseTransportStateBody;
                        }
                        stream++;
                        size--;
                    }
                    else
                    {
                        protoPktLength = (uint16)stream[0];
                        amaParseState = amaParseTransportStateBody;
                        stream++;
                        size--;
                    }

                    if(amaParseState == amaParseTransportStateBody)
                    {
                        if(protoBody != NULL)
                            Panic();
                        protoBody = PanicUnlessMalloc(protoPktLength);
                    }
                }
                break;

            case amaParseTransportStateBody:
                {
                    uint16 remainToCopy = protoPktLength - bytesCopied;
                    uint16 bytesToCopy = MIN(size,remainToCopy);

                    DEBUG_LOG("AMA TRANSPORT amaParseTransportStateBody bytes to Copy is %d", bytesToCopy);

                    if(bytesToCopy)
                    {
                        memcpy(protoBody + bytesCopied, stream, bytesToCopy);
                        bytesCopied += bytesToCopy;
                        stream += bytesToCopy;
                        size -= bytesToCopy;
                    }

                    remainToCopy = protoPktLength - bytesCopied;

                    if(remainToCopy == 0)
                    {
                        /* received a complete protobuff packet */
                        amaVersion = (header & AMA_HEADER_VERSION_MASK)>>AMA_HEADER_VERSION_OFFSET;

                        AmaReceive_Command((char*)protoBody, protoPktLength);

                        AmaParse_ResetState();
                    }
                }
                break;

            default:
                break;

        }
    } /* end if while(size) */

    return (amaParseState == amaParseTransportStateIdle);
}



uint16 AmaParse_PrepareVersionPacket(uint8* packet, uint8 major, uint8 minor)
{
    packet[1] = 0x03;
    packet[0] = 0xFE;
    packet[2] = major;
    packet[3] = minor;

    /* Transport packet size */
    packet[4] = 0;
    packet[5] = 0;

    /* Maximum transactional data size */
    packet[6] = 0;
    packet[7] = 0;

    return sizeof(uint8) * AMA_VERSION_EXCHANGE_SIZE;
}



uint16 AmaParse_PrepareVoiceData(uint8 *packet, uint16 length)
{
    uint8 headerSize = 4;
    uint16 streamHeader = 0;

    streamHeader = (amaVersion<<AMA_HEADER_VERSION_OFFSET) & AMA_HEADER_VERSION_MASK;
    streamHeader |= (AMA_TRANSPORT_STREAM_ID_VOICE<<AMA_HEADER_STREAM_ID_OFFSET) & AMA_HEADER_STREAM_ID_MASK;

    if(length > 255)
    {
        streamHeader |= AMA_HEADER_LENTGH_MASK;

        packet[2] = (uint8) (length>>8);
        packet[3] = (uint8) (length & 0xFF);
    }
    else
    {
        packet[2] = length;

        headerSize = 3;
    }

    packet[0] = (uint8) (streamHeader>>8);
    packet[1] = (uint8) (streamHeader & 0xFF);

    length += headerSize;
    return length;
}

uint16 AmaParse_PrepareControlData(uint8* packet, uint16 length)
{
    uint16 streamHeader = 0;
    uint8 headerSize = 3;

    streamHeader = (amaVersion <<AMA_HEADER_VERSION_OFFSET) & AMA_HEADER_VERSION_MASK;
    streamHeader |= (AMA_TRANSPORT_STREAM_ID_CONTROL<<AMA_HEADER_STREAM_ID_OFFSET) & AMA_HEADER_STREAM_ID_MASK;

    if(length > 255)
    {
        streamHeader |= AMA_HEADER_LENTGH_MASK;

        packet[2] = (uint8) (length>>8);
        packet[3] = (uint8) (length & 0xFF);

        headerSize = 4;
    }
    else
    {
        packet[2] = length;
    }

    packet[0] = (uint8) (streamHeader>>8);
    packet[1] = (uint8) (streamHeader & 0xFF);

    length += headerSize;
    return length;
}

