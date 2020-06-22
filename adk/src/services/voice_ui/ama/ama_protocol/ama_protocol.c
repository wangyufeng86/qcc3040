/*****************************************************************************

Copyright (c) 2019 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_protocol.c

DESCRIPTION
    Implementation for the common services internal to all AMA library modules

*********************************************************************************/
#include <stdlib.h>
#include "ama_protocol.h"
#include <message.h>
#include <vm.h>
#include <vmal.h>
#include <audio.h>
#include <bt_device.h>
#include <md5.h>
#include <source.h>
#include "ama_rfcomm.h"
#include "ama_speech.h"
#include "ama_state.h"
#include "ama_send_command.h"
#include "ama_private.h"

#define AMA_VERSION_MAJOR  1
#define AMA_VERSION_MINOR  0

#define AMA_SURROGATE_SN_SIZE (2 * MD5_DIGEST_SIZE + 1)

static Task app_task = NULL;

 /* Return the task associated with Voice Assistant */
#define amaProtocol_GetTask() ((Task)&ama_task_data)

/* Store the Application's device information configurations. */
ama_config_t* ama_config = NULL;

/***************************************************************************/
static void amaProtocol_SetConfig(const ama_config_t* conf)
{
    /* just single instance */
    if(!ama_config)
        ama_config = PanicNull(calloc(1, sizeof (ama_config_t)));

    /* Store the AMA device information configurations. */
    ama_config->device_config.name = conf->device_config.name;
    ama_config->device_config.device_type  = conf->device_config.device_type;
    ama_config->device_config.serial_number = conf->device_config.serial_number;
    ama_config->device_config.local_addr = conf->device_config.local_addr;

    ama_config->num_transports_supported = conf->num_transports_supported;
}

/* Generate surrogate serial number by hashing right-hand device address */
static char *amaProtocol_GetSurrogateSerialNumber(void)
{
    static char serial[AMA_SURROGATE_SN_SIZE];

    if (serial[0] == '\0')
    {
        bdaddr rh_addr = {0};
        MD5_CTX md5_context;
        uint8 digest[MD5_DIGEST_SIZE];
        uint16 length;
        uint16 i;
        char *ptr;

        appDeviceGetMyBdAddr(&rh_addr);

        if (rh_addr.lap & 1)
        {
        /* If the device is part of a left-right pair then this has returned the address of the
         * left-hand unit.  Attempt to get the peer address (which will fail if the device is not
         * part of a pair).
         */
            appDeviceGetPeerBdAddr(&rh_addr);
        }

        /* Convert to string for portability */
        length = sprintf(serial, "%04x %02x %06x", rh_addr.nap, rh_addr.uap, rh_addr.lap);
        DEBUG_LOG("AMA rh_addr %s", serial);

        /* Hash for privacy */
        MD5Init(&md5_context);
        MD5Update(&md5_context, (uint8 *) serial, length);
        MD5Final(digest, &md5_context);

        /* Convert digest to hex string */
        ptr = serial;
        for (i = 0; i < MD5_DIGEST_SIZE; ++i)
        {
            ptr += sprintf(ptr, "%02X", digest[i]);
        }

        DEBUG_LOG("AMA serial %s", serial);
    }
    
    return serial;
}

ama_device_config_t * AmaProtocol_GetDeviceConfiguration(void)
{
    if (ama_config->device_config.serial_number == NULL)
    {
    /*  AMA device serial number not avaliable; use surrogate  */
        ama_config->device_config.serial_number = amaProtocol_GetSurrogateSerialNumber();
    }
    
    return &ama_config->device_config;
}

uint8 AmaProtocol_GetNumTransportSupported(void)
{
    return ama_config->num_transports_supported;
}

bdaddr* AmaProtocol_GetLocalAddress(void)
{
    return &ama_config->device_config.local_addr;
}

bool AmaProtocol_Init(Task application_task,
                 const ama_config_t *config)
{
    DEBUG_LOG("AMA AmaProtocol_Init");

    if( (NULL == application_task) || (NULL == config) )
        return FALSE;

    app_task = application_task;
    amaProtocol_SetConfig(config);
    AmaNotifyAppMsg_TransportSwitch(ama_transport_ble);
    AmaState_Init();

    return TRUE;
}

void AmaProtocol_MediaControl( AMA_MEDIA_CONTROL  control)
{
    UNUSED(control);
}


bool AmaProtocol_ParseData(const uint8* stream, uint16 size)
{
    return AmaParse_ParseData(stream, size);
}

void AmaProtocol_ResetParser(void)
{
    AmaParse_ResetState();
}

void AmaProtocol_ProvideSpeechRsp(bool accept, const AMA_SPEECH_PROVIDE_IND_T* ind)
{
    uint32 resp_id = ind->dailog_id;
    if(accept)
    {
        AmaSpeech_UpdateProvidedDialogId(resp_id);
        resp_id = AmaSpeech_GetCurrentDialogId();
    }
    AmaSendCommand_ProvideSpeechRsp(accept, resp_id);
}

void AmaProtocol_TransportConnCfm(ama_transport_t transport)
{
    if(Ama_GetActiveTransport() == ama_transport_ble)
    {
        DEBUG_LOG("AMA Implicit upgrade to %d", transport);
        Ama_TransportSwitched(transport);
    }

    MAKE_AMA_MESSAGE_WITH_LEN(AMA_SEND_TRANSPORT_VERSION_ID, AMA_VERSION_EXCHANGE_SIZE);
    message->pkt_size = AmaParse_PrepareVersionPacket(message->packet, AMA_VERSION_MAJOR, AMA_VERSION_MINOR);
    AmaProtocol_SendAppMsg(AMA_SEND_TRANSPORT_VERSION_ID, message);
}

void AmaProtocol_SendAppMsg(ama_message_type_t id, void* data)
{
    DEBUG_LOG("AMA Send Sink msg %d", id);
    if(app_task)
    {
        MessageSend(app_task, id, data);
    }
}

uint16 AmaProtocol_PrepareVoicePacket(uint8* packet, uint16 packet_size)
{
    return AmaParse_PrepareVoiceData(packet, packet_size);
}

