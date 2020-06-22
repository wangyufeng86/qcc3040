/*****************************************************************************

Copyright (c) 2019 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_protocol.h

DESCRIPTION
    API for AMA library

*********************************************************************************/

#ifndef _AMA_PROTOCOL_H_
#define _AMA_PROTOCOL_H_

#include <library.h>
#include <csrtypes.h>

/* Specific 'pmalloc' pool configurations. */
#define PMALLOC_AMA_EXTRAS { 512, 4 },

typedef enum {
    ama_transport_ble,
    ama_transport_rfcomm,
    ama_transport_iap,
    ama_transport_none /* Keep this as the last one */
}ama_transport_t;

typedef enum
{
    AMA_SPEECH_STATE_IND = AMA_MSG_BASE,
    AMA_SPEECH_STOP_IND,
    AMA_SPEECH_PROVIDE_IND,
    AMA_SYNCHRONIZE_SETTING_IND,
    AMA_UPGRADE_TRANSPORT_IND,
    AMA_SWITCH_TRANSPORT_IND,
    AMA_ENABLE_CLASSIC_PAIRING_IND,
    AMA_STOP_ADVERTISING_AMA_IND,
    AMA_START_ADVERTISING_AMA_IND,
    AMA_SEND_AT_COMMAND_IND,
    AMA_SEND_TRANSPORT_VERSION_ID,
    AMA_SEND_PKT_IND,
    AMA_MESSAGE_TOP
}ama_message_type_t;

typedef enum{
    ama_speech_state_idle,
    ama_speech_state_listening,
    ama_speech_state_processing,
    ama_speech_state_speaking,
    ama_speech_state_err = 0xff
}ama_speech_state_t;

typedef enum{
    ama_at_cmd_ata_ind,
    ama_at_cmd_at_plus_chup_ind,
    ama_at_cmd_at_plus_bldn_ind,
    ama_at_cmd_at_plus_chld_eq_0_ind,
    ama_at_cmd_at_plus_chld_eq_1_ind,
    ama_at_cmd_at_plus_chld_eq_2_ind,
    ama_at_cmd_at_plus_chld_eq_3_ind,
    ama_at_cmd_atd_ind
}ama_at_cmd_t;

typedef struct
{
    ama_at_cmd_t at_command;
}AMA_SEND_AT_COMMAND_IND_T;


typedef struct{
    ama_speech_state_t speech_state;
}AMA_SPEECH_STATE_IND_T;

typedef struct{
    uint32 dailog_id;
}AMA_SPEECH_PROVIDE_IND_T;

typedef struct{
    ama_transport_t transport;
}AMA_SWITCH_TRANSPORT_IND_T;

typedef struct
{
    uint16 pkt_size;
    uint8 packet[1];
}AMA_SEND_PKT_IND_T;

typedef AMA_SEND_PKT_IND_T AMA_SEND_TRANSPORT_VERSION_ID_T;

/*!
    @brief AMA Data base for storing the device information.

    During AMA intialising library stores the application's device information configuration.
*/
typedef struct __ama_device_config
{
    char    *serial_number;
    char    *name;
    char    *device_type;
    bdaddr local_addr;
}ama_device_config_t;

/*!
    @brief AMA configuration.

    During AMA intialising library gets the configuration
*/
typedef struct __ama_config
{
    ama_device_config_t device_config;
    uint8 num_transports_supported;
}ama_config_t;

/*!
    @brief Initialise the AMA library before use

    @param application_task Application task handler to receive messages from the library
    @param ama_config_t The AMA configurations
	
	@return bool Result of the initialise operation
*/
bool AmaProtocol_Init(Task application_task,
             const ama_config_t *ama_config);

/*!
    @brief Entry point for incoming AMA data.

    @param stream Pointer to incoming data

    @param size Length of incoming data
*/
bool AmaProtocol_ParseData(const uint8* stream, uint16 size);

/*!
    @brief Reset AMA data parser.

*/
void AmaProtocol_ResetParser(void);

/*!
@brief  Sends the Provide Speech response to phone

@param accept TRUE is accepted else FALSE
@param ind pointer to provide_speech command coming from AVS

*/
void AmaProtocol_ProvideSpeechRsp(bool accept, const AMA_SPEECH_PROVIDE_IND_T* ind);

/*!
@brief  AVS transport connect confirmation

@param transport indicates which transport got connected

*/
void AmaProtocol_TransportConnCfm(ama_transport_t transport);

/*!
@brief  Prepare the voice packet with AVS information to be sent

@return size of the prepared packet

*/
uint16 AmaProtocol_PrepareVoicePacket(uint8* packet, uint16 packet_size);

#endif /* _AMA_PROTOCOL_H_ */
