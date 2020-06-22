/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       handover_profile_private.h
\defgroup   handover_profile Handover Profile
\ingroup    profiles
\brief      Handover Profile private declarations
*/

#ifndef HANDOVER_PROFILE_PRIVATE_H_
#define HANDOVER_PROFILE_PRIVATE_H_

#ifdef INCLUDE_MIRRORING

#include <task_list.h>
#include <sink.h>
#include <source.h>
#include <connection_manager.h>

#include "handover_profile.h"
#include "power_manager.h"
#include "link_policy_config.h"

/*! Enable dumping of marshal data */
#undef HANDOVER_PROFILE_DUMP_MARSHAL_DATA

/*! Handover header offsets and lengths in the data stream */
#define HANDOVER_DATA_HEADER_OFFSET                                         (0)
#define HANDOVER_DATA_HEADER_LEN                                            (1)
#define HANDOVER_DATA_OPCODE_OFFSET                                         (1)
#define HANDOVER_PROFILE_START_REQ_NUM_OF_HANDSETS_FIELD_OFFSET             (0)
#define HANDOVER_PROFILE_START_REQ_NUM_OF_HANDSETS_FIELD_LEN                (1)
#define HANDOVER_PROFILE_START_REQ_HANDSETS_ADDR_FIELD_OFFSET               (1)
#define HANDOVER_PROFILE_START_REQ_HANDSETS_ADDR_FIELD_LEN                  (2)
#define HANDOVER_PROFILE_MARSHAL_HEADER_OFFSET                              (0)
#define HANDOVER_PROFILE_P0_P1_TAG_OFFSET                                   (0)
#define HANDOVER_PROFILE_P0_P1_TAG_LEN                                      (1)
#define HANDOVER_PROFILE_P0_DATA_LEN_OFFSET                                 (1)
#define HANDOVER_PROFILE_P0_DATA_LEN                                        (2)
#define HANDOVER_PROFILE_P0_HEADER_LEN                    (HANDOVER_PROFILE_P0_P1_TAG_LEN + \
                                                          HANDOVER_PROFILE_P0_DATA_LEN)
/*! Should be minimum of P1 client's data type so that some valid data is sent in the marshal packet */
#define HANDOVER_PROFILE_STREAM_MARSHAL_MIN_LEN                             (1)

#define HANDOVER_PROFILE_STREAM_MARSHAL_MIN_P1_PKT_LEN    (HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN + \
                                                           HANDOVER_PROFILE_STREAM_MARSHAL_MIN_LEN)

/*! Minimum P0/BTSS marshal data length */
#define HANDOVER_PROFILE_STREAM_P0_MARSHAL_MIN_LEN                             (50)

#define HANDOVER_PROFILE_STREAM_MARSHAL_MIN_P0_PKT_LEN    (HANDOVER_PROFILE_P0_HEADER_LEN + \
                                                           HANDOVER_PROFILE_STREAM_P0_MARSHAL_MIN_LEN)

/*! Handover tags in the data stream */
#define HANDOVER_MARSHAL_P0_TAG                                             (0xEF)
#define HANDOVER_MARSHAL_P1_TAG                                             (0x82)
#define HANDOVER_MARSHAL_END_TAG                                            (0xFF)
#define HANDOVER_NUMBER_OF_HANDSETS                                         (0x01)
#define HANDOVER_ROLE_SWITCH_RETRY_COUNT                                    (10)

/*! Marshal(0x80) + P0/P1 Tag (0x81/0x82) + len(2bytes) */
#define HANDOVER_PROFILE_MARSHAL_HEADER_LEN                                 (4)
/* Marshal(0x80) */
#define HANDOVER_PROFILE_MARSHAL_P1_HEADER_LEN                              (2)
/*! Client_ID(1byte) + Data Len (2bytes) */
#define HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN                               (3)
#define HANDOVER_PROFILE_P1_CLIENT_ID_OFFSET                                (0)
#define HANDOVER_PROFILE_P1_CLIENT_DATA_LEN_OFFSET                          (1)
#define HANDOVER_PROFILE_OPCODE_FIELD_LEN                                   (1)
#define HANDOVER_PROFILE_L2CAP_MTU_SIZE                                     (0x037F)

/*! Timeout values for various APIs and protocol messages */
#define HANDOVER_PROFILE_ACL_RECEIVE_ENABLE_TIMEOUT_USEC                    (750000)
#define HANDOVER_PROFILE_ACL_RECEIVED_DATA_PROCESSED_TIMEOUT_USEC           (500000)
/* Allowing enough time to send the P0 data is critical, we must be sure that
   either the data has been sent, or the link has been lost. Therefore setting
   the timeout to the link supervision timeout */
#define HANDOVER_PROFILE_P0_TRANSMIT_DATA_PENDING_TIMEOUT_MSEC              appConfigEarbudLinkSupervisionTimeout()
#define HANDOVER_PROFILE_ACL_TRANSMIT_DATA_PENDING_TIMEOUT_MSEC             (500)
#define HANDOVER_PROFILE_P0_MARSHAL_TIMEOUT_MSEC                            (100)
#define HANDOVER_PROFILE_P1_MARSHAL_TIMEOUT_MSEC                            (100)
#define HANDOVER_PROFILE_PROTOCOL_MSG_TIMEOUT_MSEC                          (2100)
#define HANDOVER_PROFILE_P0_UNMARSHAL_TIMEOUT_MSEC                          (1000)
#define HANDOVER_PROFILE_P0_UNMARSHAL_SINK_CLAIM_TIMEOUT_MSEC               (100)
#define HANDOVER_PROFILE_P1_UNMARSHAL_TIMEOUT_MSEC                          (100)
#define HANDOVER_PROFILE_NUM_OF_CLIENT                                      (4)
#define HANDOVER_PROFILE_INVALID_SINK_CLAIM_SIZE                            (0xFFFF)
#define HANDOVER_PROFILE_UNMARSHAL_TIMEOUT_MSEC                             (8000)
#define HANDOVER_PROFILE_EXIT_SNIFF_TIMEOUT_MSEC                            (500)
#define HANDOVER_PROFILE_ROLE_SWITCH_TIMEOUT_MSEC                           (500)
#define HANDOVER_PROFILE_ACL_HANDOVER_PREPARE_TIMEOUT_MSEC                  (20)
#define HANDOVER_PROFILE_NO_OF_TIMES_SNIFF_INTERVAL                         (2)
#define HANDOVER_PROFILE_REENTER_SNIFF_TIMEOUT_MSEC                         (100)

/*! Maximum number of times to try the SDP search for the peer_signalling attributes.
    After this many attempts the connection request will be failed. */
#define HandoverProfile_GetSdpSearchTryLimit()                              (3)

/*! Maximum number of times to try the ACL handover prepare and ACL handover prepared.
    After this many attempts we send handover vetoed error */
#define HandoverProfile_GetAclHandoverPrepareRetryLimit()                   (3)


/* Returns the handover profile state */
#define HandoverProfile_GetState(ho_inst) (ho_inst->state)

/* Macro to make prim */
#define MAKE_PRIM(TYPE) \
    TYPE##_T *prim = zpnew(TYPE##_T); prim->type = TYPE

/*! Enable toggling on PIO18 and PIO19 during handover procedure. 
    This is useful for determining the time taken in the different
    parts of the handover procedure.

    The PIOs need to be setup in pydbg as outputs controlled by P1:
    mask = (1<<18 | 1<<19)
    apps1.fw.call.PioSetMapPins32Bank(0, mask, mask)
    apps1.fw.call.PioSetDir32Bank(0, mask, mask)
*/
#ifdef HANDOVER_PIO_TOGGLE
#include "pio.h"
#define HANDOVER_PIO_MASK ((1<<18) | (1<<19))
#define HandoverPioSet() PioSet32Bank(0, HANDOVER_PIO_MASK, HANDOVER_PIO_MASK)
#define HandoverPioClr() PioSet32Bank(0, HANDOVER_PIO_MASK, 0)
#else
#define HandoverPioSet()
#define HandoverPioClr()
#endif

/*!

@startuml
[*] -d-> INITIALISING : Module init
    INITIALISING : Register SDP record for L2CAP
    INITIALISING -d-> DISCONNECTED : CL_SDP_REGISTER_CFM

    DISCONNECTED : No peer connection
    DISCONNECTED --> CONNECTING_SDP_SEARCH : Startup request (ACL connected)
    DISCONNECTED --> DISCONNECTED : BD-Addr Not valid or ACL not connected
    DISCONNECTED --> CONNECTING_REMOTE : Remote L2CAP connect indication

    CONNECTING_SDP_SEARCH : Performing SDP search for Handover profile service
    CONNECTING_SDP_SEARCH --> CONNECTING_LOCAL : SDP success
    CONNECTING_SDP_SEARCH --> CONNECTING_SDP_SEARCH : SDP retry
    CONNECTING_SDP_SEARCH --> DISCONNECTED : Shutdown request(Cancel SDP)
    CONNECTING_SDP_SEARCH --> DISCONNECTED : SDP error
    
    CONNECTING_LOCAL : Local initiated connection
    CONNECTING_LOCAL --> CONNECTED : L2CAP connect cfm (success)
    CONNECTING_LOCAL --> DISCONNECTED : L2CAP connect cfm (fail)
    CONNECTING_LOCAL --> DISCONNECTED : Remote L2CAP disconnect ind
    CONNECTING_LOCAL --> DISCONNECTING : Shutdown request

    CONNECTING_REMOTE : Remote initiated connection
    CONNECTING_REMOTE --> CONNECTED : L2CAP connect (success)
    CONNECTING_REMOTE --> DISCONNECTING : Shutdown request
    CONNECTING_REMOTE --> DISCONNECTED : L2CAP connect (fail)
    CONNECTING_REMOTE --> DISCONNECTED : Remote L2CAP disconnect ind

    CONNECTED : Handover profile active
    CONNECTED --> DISCONNECTING : Shutdown request
    CONNECTED --> DISCONNECTED : Remote L2CAP disconnect ind

    DISCONNECTING : Waiting for disconnect result
    DISCONNECTING --> DISCONNECTED : L2CAP disconnect cfm
@enduml
*/

/*! Returns true if data points to End of marshal */
#define handoverProfile_IsMarshalEnd(data) \
    ((data)[HANDOVER_PROFILE_P0_P1_TAG_OFFSET] == HANDOVER_MARSHAL_END_TAG)

/*! Returns true if data points to P0 tag */
#define handoverProfile_IsMarshalTagP0(data) \
    ((data)[HANDOVER_PROFILE_P0_P1_TAG_OFFSET] == HANDOVER_MARSHAL_P0_TAG)

/*! Returns true if data points to Handover Start Cfm opcode */
#define handoverProfile_IsHandoverStartCfm(data) \
    ((data)[0] == HANDOVER_PROTOCOL_START_CFM)

/*! Returns true if data points to Handover Unmarshal P1 Cfm opcode */
#define handoverProfile_IsHandoverUnmarshalP1Cfm(data) \
    ((data)[0] == HANDOVER_PROTOCOL_UNMARSHAL_P1_CFM)

/*! Returns true if Handover Start Cfm session Id matches 'id' */
#define handoverProfile_IsHandoverStartCfmSessionId(data, id) \
    ((data)[1] == id)

/*! Returns true if data points to Handover Start Cfm status */
#define handoverProfile_IsHandoverStartCfmStatusSuccess(data) \
    ((data)[2] == HANDOVER_PROTOCOL_STATUS_SUCCESS)

/*! Handover Profile state machine states */
typedef enum
{
    /*! Handover Profile not initialised */
    HANDOVER_PROFILE_STATE_NONE = 0,

    /*!< Handover Profile is initialised */
    HANDOVER_PROFILE_STATE_INITIALISING,

    /*! No connection */
    HANDOVER_PROFILE_STATE_DISCONNECTED,

    /*! Searching for Peer Signalling service */
    HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH,

    /*! Locally initiated connection in progress */
    HANDOVER_PROFILE_STATE_CONNECTING_LOCAL,

    /*! Remotely initiated connection is progress */
    HANDOVER_PROFILE_STATE_CONNECTING_REMOTE,

    /*! Connnected */
    HANDOVER_PROFILE_STATE_CONNECTED,

    /*! Disconnection in progress */
    HANDOVER_PROFILE_STATE_DISCONNECTING
}handover_profile_state_t;

/*! Handover Profile handover data marshal states */
typedef enum
{
    /*! Handover Profile not recieved any marshal data */
    HANDOVER_PROFILE_MARSHAL_STATE_IDLE = 0,

    /*!< Handover Profile marshalling P1 data */
    HANDOVER_PROFILE_MARSHAL_STATE_P1_MARSHALLING,

    /*!< Handover Profile marshalling P0 data */
    HANDOVER_PROFILE_MARSHAL_STATE_P0_MARSHALLING,
}handover_profile_marshal_state_t;

/*! Handover Profile module state. */
typedef struct
{
    /*!< Handover Profile task */
    TaskData task;
    /*!< TRUE if role is Primary */
    bool is_primary;
    /*!< Marshalling state */
    handover_profile_marshal_state_t marshal_state;
    /*!< Bluetooth address of the handset */
    tp_bdaddr tp_handset_addr;
    /*!< ACL handle */
    uint16 acl_handle;
    /*!< L2CAP PSM registered */
    uint16 local_psm;
    /*!< L2CAP PSM registered by peer device */
    uint16 remote_psm;
    /*!< The sink of the L2CAP link */
    Sink link_sink;
    /*!< The source of the L2CAP link */
    Source link_source;
    /*!< The sink for unmarshalling P0 data */
    Sink marshal_sink;
    /*!< Amount of data written into the sink stream until sink flushed */
    uint16  sink_written;
    /*!< TRUE if link-loss has occurred */
    bool link_loss_occured;
    /*!< Bluetooth address of the peer we are signalling */
    bdaddr peer_addr;
    /*!< Current state of the handover profile */
    handover_profile_state_t state;
    /*!< Store the Task which requested a connect. */
    Task connect_task;
    /*!< Store the Task which requested a disconnect. */
    Task disconnect_task;
    /*!< List of tasks registered for notifications from handover profile */
    task_list_t handover_client_tasks;
    /* Handover client list */
    const handover_interface **ho_clients;
    /* Number of clients in ho_clients */
    uint8   num_clients;
    /*!< Count of failed SDP searches */
    uint16 sdp_search_attempts;
    /*!< Handover protocol session identifier */
    uint8 session_id;
}handover_profile_task_data_t;

extern handover_profile_task_data_t ho_profile;

/*! Get pointer to the handover profile task structure */
#define Handover_GetTaskData() (&ho_profile)

/*! \brief Internal messages used by handover profile. */
typedef enum
{
    /*! Message to bring up link to peer */
    HANDOVER_PROFILE_INTERNAL_STARTUP_REQ,

    /*! Message to shut down link to peer */
    HANDOVER_PROFILE_INTERNAL_SHUTDOWN_REQ
}handover_profile_internal_msgs_t;

/*! \brief Handover Protocol Opcodes. */
typedef enum
{
    /*! Handover Protocol Start request */
    HANDOVER_PROTOCOL_START_REQ,
    /*! Handover Protocol Start confirmation */
    HANDOVER_PROTOCOL_START_CFM,
    /*! Handover Protocol Cancel indication */
    HANDOVER_PROTOCOL_CANCEL_IND,
    /*! Handover Protocol Unmarshal P1 confirmation */
    HANDOVER_PROTOCOL_UNMARSHAL_P1_CFM,
    /*! Handover Protocol Unmarshal complete indication */
    HANDOVER_PROTOCOL_UNMARSHAL_COMPLETE_IND,
    /*! Handover Protocol Marshal data */
    HANDOVER_MARSHAL_DATA=0x80
} handover_protocol_msg_t;

/*! \brief Handover Protocol Status. */
typedef enum
{
    /*! Handover Protocol Status Success */
    HANDOVER_PROTOCOL_STATUS_SUCCESS,
    /*! Handover Protocol Status Failure */
    HANDOVER_PROTOCOL_STATUS_VETOED
} handover_protocol_status_t;

/*! Internal message sent to start initiate handover profile connection 
    to a peer */
typedef struct
{
    bdaddr peer_addr;           /*!< Address of peer */
} HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T;

/*! Handover Start Request message sent to peer device to start the handover procedure */
typedef struct
{
    handover_protocol_msg_t   opcode;
    uint8   session_id;
    uint8   last_tx_seq;
    uint8   last_rx_seq;
    uint16  mirror_state;
    uint8   num_of_devices;
    bdaddr  handset_addr;
} HANDOVER_PROFILE_HANDOVER_START_REQ_T;

/*! Handover Start confirmation message sent to primary device by peer device with the 
    status to proceed for handover */
typedef struct
{
    handover_protocol_msg_t   opcode;
    uint8   session_id;
    handover_protocol_status_t   status;
} HANDOVER_PROFILE_HANDOVER_START_CFM_T;

/*! Handover Cancel Indication message sent to secondary device to abort handover */
typedef struct
{
    handover_protocol_msg_t   opcode;
} HANDOVER_PROFILE_HANDOVER_CANCEL_IND_T;

/*! Handover unmarshal complete message sent to primary post unmarshalling is complete at secondary */
typedef HANDOVER_PROFILE_HANDOVER_CANCEL_IND_T HANDOVER_PROFILE_HANDOVER_UNMARSHAL_COMPLETE_IND_T;

/*! Handover unmarshal P1 complete confirmation message sent to primary post unmarshalling of P1 data at secondary */
typedef HANDOVER_PROFILE_HANDOVER_CANCEL_IND_T HANDOVER_PROFILE_HANDOVER_UNMARSHAL_P1_CFM_T;

/*! \brief Set handover profile to new state.

    \param[in] state      Refer \ref handover_profile_state_t, new state.

*/
void HandoverProfile_SetState(handover_profile_state_t state);

/*! \brief Handle result of L2CAP PSM registration request.

    Handles registration of handover-profile with the L2CAP and register the handover 
    profile with the SDP.
    
    \param[in] cfm      Refer \ref CL_L2CAP_REGISTER_CFM_T, pointer to L2CAP register 
                        confirmation message.

*/
void HandoverProfile_HandleClL2capRegisterCfm(const CL_L2CAP_REGISTER_CFM_T *cfm);

/*! \brief Handle result of the SDP service record registration request.

    Handles confirmation received for registration of handover-profile service record with SDP 
    and move to HANDOVER_PROFILE_STATE_DISCONNECTED state.
    
    \param[in] cfm      Refer \ref CL_SDP_REGISTER_CFM_T, pointer to SDP register 
                        confirmation message.

*/
void HandoverProfile_HandleClSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *cfm);


/*! \brief Handle the result of a SDP service attribute search.

    The returned attributes are checked to make sure they match the expected format of a 
    handover profile service record.
    
    \param[in] cfm      Refer \ref CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T, pointer to SDP searched 
                        attribute results.

*/
void HandoverProfile_HandleClSdpServiceSearchAttributeCfm(const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm);

/*! \brief Handle a L2CAP connection request that was initiated by the remote peer device.

    \param[in] ind      Refer \ref CL_L2CAP_CONNECT_IND_T, pointer to L2CAP connection 
                        indication message.

*/
void HandoverProfile_HandleL2capConnectInd(const CL_L2CAP_CONNECT_IND_T *ind);

/*! \brief Handle the result of a L2CAP connection request.

    This is called for both local and remote initiated L2CAP requests.
    
    \param[in] cfm      Refer \ref CL_L2CAP_CONNECT_CFM_T, pointer to L2CAP connect 
                        confirmation.

*/
void HandoverProfile_HandleL2capConnectCfm(const CL_L2CAP_CONNECT_CFM_T *cfm);

/*! \brief Handle a L2CAP disconnect initiated by the remote peer.

    \param[in] ind      Refer \ref CL_L2CAP_DISCONNECT_IND_T, pointer to L2CAP disconnect 
                        indication.

*/
void HandoverProfile_HandleL2capDisconnectInd(const CL_L2CAP_DISCONNECT_IND_T *ind);

/*! \brief Handle a L2CAP disconnect confirmation.

    This is called for both local and remote initiated disconnects.
    
    \param[in] cfm      Refer \ref CL_L2CAP_DISCONNECT_CFM_T, pointer to L2CAP disconnect 
                        confirmation.

*/
void HandoverProfile_HandleL2capDisconnectCfm(const CL_L2CAP_DISCONNECT_CFM_T *cfm);

/*! \brief Handles internal startup request of handover profile.

    Handles internal startup request by intiating connection to peer device based on 
    the current state machine.
    
    \param[in] req      Refer \ref HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T, pointer to 
                        internal startup request message.

*/
void HandoverProfile_HandleInternalStartupRequest(HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T *req);

/*! \brief Handles internal shut-down request of handover-profile.

    Handles internal shutdown request by intiating disconnection to peer device based on 
    the current state machine state.
*/
void HandoverProfile_HandleInternalShutdownReq(void);

/*! \brief Shutdown (or disconnect) the Handover profile connection.

    \param[in] task   Client task
*/
void HandoverProfile_Shutdown(Task task);

/*! \brief Start Handover Signalling channel

    Start handover profile signalling channel by establishing the L2CAP connection.

    \param[in] task         Client task requesting the Handover profile connection.
    \param[in] peer_addr    Address of the peer device
*/
void HandoverProfile_Startup(Task task, const bdaddr *peer_addr);

#ifdef HANDOVER_PROFILE_DUMP_MARSHAL_DATA
/*! 
    \brief Dumps the data onto the debug terminal.

    \param[in] buf     Buffer to dump.
    \param[in] size    Amount of data in bytes to dump.

*/
void HandoverProfile_DumpData(const uint8 *buf, size_t size);
#else
#define HandoverProfile_DumpData(buf, size) /* Nothing to do */
#endif

/*! 
    \brief Marshal all P0 client data and send to peer.

    If HANDOVER_PROFILE_L2CAP_MTU_SIZE bytes are written to the sink then send
    the packet to peer and block for HANDOVER_PROFILE_L2CAP_MTU_SIZE bytes space
    in the L2CAP sink for a maximum duration of 
    HANDOVER_PROFILE_P0_MARSHAL_TIMEOUT_MSEC. If the required space is claimed,
    then continue marshaling the P0 client data else return false.

    \param[in] bd_addr         Bluetooth address of the link to be marshalled.
    \param[out] stream_offset  Points to the sink buffer to continue to write
                               marshal data.

    \return \ref handover_profile_status_t. Returns,
            1. HANDOVER_PROFILE_STATUS_SUCCESS if marshal of P0 client is successful.
            2. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if handover terminated due to SinkFlush() failure.
            3. HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT if any other failure occured.

*/
handover_profile_status_t HandoverProfile_MarshalP0Clients(const tp_bdaddr *bd_addr);

/*! 
    \brief Marshal all P1 clients data and send to to peer.

    Marshal the data in two scenarios,
    1. Append P1 client data in the same packet in which P0 marshal data is complete.
    2. Start P1 client data in a new packet.

    If HANDOVER_PROFILE_L2CAP_MTU_SIZE is reached send the packet to peer and block for
    space in the l2cap sink for a maxium duration of HANDOVER_PROFILE_P1_MARSHAL_TIMEOUT_MSEC.
    If the required space is claimed, then continue marshaling the P1 client data else return false.

    \param[in] bd_addr        Bluetooth address of the link to be marshalled.
    \param[in] stream_offset  Points to the sink buffer to write marshal data.

    \return \ref handover_profile_status_t. Returns,
            1. HANDOVER_PROFILE_STATUS_SUCCESS if marshal of P1 client is successful.
            2. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if handover terminated due to SinkFlush() failure.
            3. HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT if any other failure occured.

*/
handover_profile_status_t HandoverProfile_MarshalP1Clients(const tp_bdaddr *bd_addr);

/*! 
    \brief Calls commit function of all the P1 clients registered with the
           Handover Profile.
           This function can be called in Primary or Secondary role.

    \param[in] is_primary    TRUE if committing to new primary.
                             FALSE if committing to new secondary
 */
void HandoverProfile_CommitP1Clients(bool is_primary);

/*! 
    \brief Calls Complete function of all the P1 clients registered with the
           Handover Profile.
           This function can be called in Primary or Secondary role.

     \param[in] is_primary    TRUE if committing to new primary.
                              FALSE if committing to new secondary

 */
void HandoverProfile_CompleteP1Clients(bool is_primary);

/*! 
    \brief Calls complete function of all the P1 clients registered with the
           Handover Profile.
           This function can be called in Primary or Secondary role.

    \param[in] is_primary    TRUE if Completing to new primary.
                             FALSE if Completing to new secondary
 */
void HandoverProfile_CompleteP1Clients(bool is_primary);

/*! 
    \brief Handles the failure of handover procedure at various stages before
           marshaling of data starts at the Primary by,
           1. Sending HANDOVER_PROTOCOL_CANCEL_IND message to secondary.
           2. Resumes the inbound data flow for the ACL associated with 
              the addr.

    \param[in] addr    Bluetooth address of the handset.

*/
void HandoverProfile_HandlePrepareForMarshalFailure(const tp_bdaddr *addr);

/*! 
    \brief Handles the failure of handover procedure at various stages by taking
           the following actions during marshaling data at Primary by,
           1. Send HANDOVER_PROTOCOL_CANCEL_IND message to secondary.
           2. Re-enable BaseBand flow on the Primary.
           3. Resumes the inbound data flow for the ACL associated with 
              the addr.
           Any failure in the above steps results in Panic.

    \param[in] addr    Bluetooth address of the handset.

*/
void HandoverProfile_HandleMarshalFailure(const tp_bdaddr *addr);

/*! 
    \brief  Performs below,
            1. Disables the inbound data flow for the ACL associated with the
               addr at a L2CAP packet boundary.
            2. P0 mirror manager to process Inbound data.
            3. Transmit any outbound data.
            4. Check if any of P1 client Vetos
            5. Check if any of P0 client Vetos

    \param[in] addr      Bluetooth address of the handset.
    \return Refer \ref handover_profile_status_t.
*/
handover_profile_status_t HandoverProfile_PrepareForMarshal(const tp_bdaddr *addr);

/*!
    \brief Send handover protocol start request message to peer.

    \param[in] handset_addr Address of the handset.
    \param[in] session_id   Handover session identifier.

    \return HANDOVER_PROFILE_STATUS_SUCCESS: If successfully sent the message to peer.
            HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT: Timed out waiting for space in the sink.

*/
handover_profile_status_t HandoverProfile_SendHandoverStartReq(const bdaddr *handset_addr, uint8 session_id);

/*!
    \brief Send handover protocol cancel indication message to peer.

    \return HANDOVER_PROFILE_STATUS_SUCCESS: If successfully sent the message to peer.
            HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT: Timed out waiting for space in the sink.

*/
handover_profile_status_t HandoverProfile_SendHandoverCancelInd(void);

/*! 
    \brief Wait for handover start confirm message from peer.

    \return handover_profile_status_t Returns,
            1. HANDOVER_PROFILE_STATUS_SUCCESS if handover start confirm message received.
            2. HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT if timeout waiting for the message.
            3. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if the cofirmation status is not success
            4. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if any other failure occured.

*/
handover_profile_status_t HandoverProfile_ProcessProtocolStartCfm(void);

/*! 
    \brief Blocks on stream source until data is received for a duration of time
           mentioned in parameter 'timeout'.

    \param[in] src      Stream source from which data is expected.
    \param[out] size    Amount of data in bytes recieved in l2cap_src.
    \param[in] timeout  Duration in milliseconds to block on l2cap_src for data.

    \return Pointer to the data received in l2cap_src if successful.
            NULL: Otherwise.

*/
const uint8* HandoverProfile_BlockReadSource(Source src,
    uint16 *size,
    uint32 timeout);

/*! 
    \brief Process the message received from the Primary/Secondary handover
           l2cap connection.

    \param[in] mmd    Message type received when more data has arrived at the
                      Source.

*/
void HandoverProfile_ProcessHandoverMessage(const MessageMoreData *mmd);

/*! 
    \brief Check if the role for peer ACL link is changed to the paramter 
    'role'.

    \param[in] role         ACL role to check.
    \param[in] timeout_ms   Time in milliseconds to enqire.

    \return TRUE If the role for the peer Acl link is same as parameter 'role'
            FALSE otherwise.

*/
bool HandoverProfile_GetAclRoleBlocking(const tp_bdaddr * acl_address, hci_role_t role, uint32 timeout_ms);

/*! 
    \brief Wait for unmarshal P1 confirm message from peer.

    \return handover_profile_status_t Returns,
            1. HANDOVER_PROFILE_STATUS_SUCCESS if handover start confirm message received.
            2. HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT if timeout waiting for the message.
            3. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if the cofirmation status is not success
            4. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if any other failure occured.

*/
handover_profile_status_t HandoverProfile_ProcessProtocolUnmarshalP1Cfm(void);

/*! 
    \brief Calls abort function of all the P1 clients registered with the
           Handover Profile.

*/
void HandoverProfile_AbortP1Clients(void);

/*! 
    \brief Checks if any of the P1 clients vetos.

    \return TRUE: If any of the P1 client veto'ed.
            FALSE: Otherwise.

*/
bool HandoverProfile_VetoP1Clients(void);

/*! 
    \brief Checks if all the data sent over ACL has been transmitted out.
    \params[in] addr     Peer device address.
    \params[in] timeout  Polling timeout (millesecond).
    \return TRUE:  Data has not been transmitted.
            FALSE: Data has been transmitted.
*/
bool HandoverProfile_IsAclTransmitPending(const tp_bdaddr *addr, uint32 timeout);
#endif /* INCLUDE_MIRRORING */
#endif /*HANDOVER_PROFILE_PRIVATE_H_*/
