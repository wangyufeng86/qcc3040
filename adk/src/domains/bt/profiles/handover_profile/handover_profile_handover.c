/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       handover_profile_handover.c
\brief      Implementation of the Handover.
*/
#ifdef INCLUDE_MIRRORING

#include "handover_profile.h"
#include "handover_profile_private.h"
#include "peer_signalling.h"
#include "mirror_profile_protected.h"
#include "kymera.h"
#include "timestamp_event.h"

#include <app/bluestack/mdm_prim.h>
#include <acl.h>
#include <stream.h>
#include <panic.h>
#include <logging.h>

/* Various Marshal Data Packet formats possible are shown below

Data field values
MARSHAL_DATA    = 0x80
P0_TAG          = 0xEF
END_OF_MARSHAL  = 0xFF
P0_LEN          = <2 bytes>
CLIENT_ID       = <1 byte>
DATA_LEN        = <2 bytes>
*/

/*           Marshal data packet with complete P0 and P1 data including terminator tag(END_OF_MARSHAL)
Name        : |MARSHAL_DATA|P0_TAG |P0_LEN|P0_DATA||CLIENT_ID|DATA_LEN|DATA||CLIENT_ID...||END_OF_MARSHAL|
Bytes(Value): |1(0x80)     |1(0xEF)|2     |N      ||1        |2       |N   ||1           ||1(0xFF)       |
*/

/*           Marshal data packet with only P0 data. Note: Terminator tag(END_OF_MARSHAL) is for P1 data only
Name        : |MARSHAL_DATA|P0_TAG |P0_LEN|P0_DATA|
Bytes(Value): |1(0x80)     |1(0xEF)|2     |N      |
*/

/*           Marshal data packet with only P1 data including terminator tag(END_OF_MARSHAL)
Name        : |MARSHAL_DATA|CLIENT_ID|DATA_LEN|DATA||CLIENT_ID...||END_OF_MARSHAL|
Bytes(Value): |1(0x80)     |1        |2       |N   ||1           ||1(0xFF)       |
*/

/*           Marshal data packet with only the terminator tag(END_OF_MARSHAL)
Name        : |MARSHAL_DATA|END_OF_MARSHAL|
Bytes(Value): |1(0x80)     |1(0xFF)       |
*/

#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->common.op_code = TYPE; prim->common.length = sizeof(TYPE##_T);

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool handoverProfile_UnmarshalP1Clients(const uint8 *src_addr, uint16 src_len, uint16 *src_consumed);
static handover_profile_status_t handoverProfile_SendHandoverStartCfm(handover_protocol_status_t status, uint8 session_id);
static bool handoverProfile_ProcessProtocolStartReq(const uint8 *data, uint16 size, uint16 *consumed);
static void handoverProfile_ProcessProtocolCancelInd(void);
static handover_profile_status_t handoverProfile_SendHandoverMsg(void *src_addr, uint16 size, uint32 timeout);
static uint8* handoverProfile_ClaimSink(Sink sink, uint16 size, uint32 timeout);
static bool handoverProfile_ProcessMarshalData(const uint8 *src_addr, uint16 src_size, uint16 *consumed);

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*!
    \brief Send handover protocol start confirmation message to peer.

    \param[in] handset_addr Address of the handset.

    \return HANDOVER_PROFILE_STATUS_SUCCESS: If successfully sent the message to peer.
            HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT: Timed out waiting for space in the sink.

*/
static handover_profile_status_t handoverProfile_SendHandoverStartCfm(handover_protocol_status_t status, uint8 session_id)
{
    HANDOVER_PROFILE_HANDOVER_START_CFM_T   start_cfm;

    memset(&start_cfm, 0, sizeof(HANDOVER_PROFILE_HANDOVER_START_CFM_T));
    start_cfm.opcode=HANDOVER_PROTOCOL_START_CFM;
    start_cfm.session_id=session_id;
    start_cfm.status=status;

    return (handoverProfile_SendHandoverMsg(&start_cfm, 
                sizeof(HANDOVER_PROFILE_HANDOVER_START_CFM_T), 
                HANDOVER_PROFILE_PROTOCOL_MSG_TIMEOUT_MSEC));
}

/*!
    \brief Send handover protocol unmarshal P1 confirm message to peer.

    \return HANDOVER_PROFILE_STATUS_SUCCESS: If successfully sent the message to peer.
            HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT: Timed out waiting for space in the sink.

*/
static handover_profile_status_t handoverProfile_SendHandoverUnmarshalP1Cfm(void)
{
    HANDOVER_PROFILE_HANDOVER_UNMARSHAL_P1_CFM_T   start_req;

    memset(&start_req, 0, sizeof(HANDOVER_PROFILE_HANDOVER_UNMARSHAL_P1_CFM_T));
    start_req.opcode=HANDOVER_PROTOCOL_UNMARSHAL_P1_CFM;

    return (handoverProfile_SendHandoverMsg(&start_req, 
                sizeof(HANDOVER_PROFILE_HANDOVER_UNMARSHAL_P1_CFM_T), 
                HANDOVER_PROFILE_PROTOCOL_MSG_TIMEOUT_MSEC));
}

/*!
    \brief Send handover protocol message to peer.
    Claims size number of bytes from the l2cap stream for time mentioned in timeout parameter 
    and sends handover protocol message pointed to by src_addr to peer.

    \param[in] src_addr  Handover protocol message.
    \param[in] size      Size in bytes of the protocol message.
    \param[in] timeout   Timeout in milliseconds to wait for the sink space.

    \return HANDOVER_PROFILE_STATUS_SUCCESS: If successfully sent the message to peer.
            HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT: Timed out waiting for space in the sink.

*/
static handover_profile_status_t handoverProfile_SendHandoverMsg(void *src_addr, uint16 size, uint32 timeout)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    uint8 *dst_addr=NULL;

    dst_addr = handoverProfile_ClaimSink(ho_inst->link_sink, size, timeout);

    if(dst_addr)
    {
        memmove(dst_addr, src_addr, size);
        if(SinkFlush(ho_inst->link_sink, size) == 0)
        {
            DEBUG_LOG("handoverProfile_SendHandoverMsg: SinkFlush() failed");
            return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
        }

        return HANDOVER_PROFILE_STATUS_SUCCESS;
    }
    else
    {
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }
}

/*! \brief Determine if the secondary should veto handover */
static bool handoverProfile_SecondaryVeto(const bdaddr *addr, uint8 pri_tx_seq, uint8 pri_rx_seq, uint16 mirror_state)
{
    uint8 sec_tx_seq = appPeerSigGetLastTxMsgSequenceNumber();
    uint8 sec_rx_seq = appPeerSigGetLastRxMsgSequenceNumber();

    /* Validate if the received and the transmitted peer signalling messages are
    same on both primary and secondary earbuds. If the same, this means there
    are no in-flight peer signalling message. If not, veto to allow time for the
    messages to be cleared. */
    if(sec_rx_seq != pri_tx_seq || pri_rx_seq != sec_tx_seq)
    {
        DEBUG_LOG("handoverProfile_SecondaryVeto: PriTx:%x PriRx:%x SecTx:%x SecRx:%x",
                    pri_tx_seq, pri_rx_seq, sec_tx_seq, sec_rx_seq);
        return TRUE;
    }
    else if (mirror_state != MirrorProfile_GetMirrorState())
    {
        DEBUG_LOG("handoverProfile_SecondaryVeto: Mirror state mismatch: 0x%x 0x$x",
                        mirror_state, MirrorProfile_GetMirrorState());
        return TRUE;
    }
    else if (MirrorProfile_Veto() || kymera_a2dp_mirror_handover_if.pFnVeto())
    {
        return TRUE;
    }
    else if(appAvInstanceFindFromBdAddr(addr))
    {
        /* AV instance is present on Secondary. This is only possible if disconnection caused by
           previous handover is not complete yet. */
        DEBUG_LOG("handoverProfile_SecondaryVeto: AV instance exist!");
        return TRUE;
    }

    return FALSE;
}

/*! 
    \brief Process HANDOVER_START_REQ message sent from Primary.
           1. Send HANDOVER_START_CFM message to Primary.
           2. Block on l2cap stream for marshal data or handover cancel indication
              or timeout.

    \param[in] data       Handover start request payload from the Primary.
    \param[in] size       Length of the data.
    \param[out] consumed  Number of bytes processed in data.

    \return TRUE:  If HANDOVER_PROTOCOL_START_CFM message sent to peer.
            FALSE: Otherwise.

*/
static bool handoverProfile_ProcessProtocolStartReq(const uint8 *data,
    uint16 size, uint16 *consumed)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    bdaddr handset_addr;
    HANDOVER_PROFILE_HANDOVER_START_REQ_T req;

    if(ho_inst->is_primary ||
       ho_inst->state != HANDOVER_PROFILE_STATE_CONNECTED ||
       size != sizeof(HANDOVER_PROFILE_HANDOVER_START_REQ_T))
    {
        DEBUG_LOG("handoverProfile_ProcessProtocolStartReq failed, ho_inst->is_primary0x%x, ho_inst->state=0x%x, msg size=%d",
                  ho_inst->is_primary, ho_inst->state, size);
        return FALSE;
    }

    memmove(&req, data, size);
    *consumed = size;

    if (handoverProfile_SecondaryVeto(&req.handset_addr, req.last_tx_seq, req.last_rx_seq, req.mirror_state))
    {
        if(handoverProfile_SendHandoverStartCfm(HANDOVER_PROTOCOL_STATUS_VETOED, req.session_id) != HANDOVER_PROFILE_STATUS_SUCCESS)
        {
            DEBUG_LOG("handoverProfile_ProcessProtocolStartReq failed to send handover start cfm");
        }
        return FALSE;
    }

    /* Note: Only one handset address is sent in the handover start request */
    handset_addr.lap = req.handset_addr.lap;
    handset_addr.uap = req.handset_addr.uap;
    handset_addr.nap = req.handset_addr.nap;

    BdaddrTpFromBredrBdaddr(&ho_inst->tp_handset_addr, &handset_addr);

    DEBUG_LOG("handoverProfile_ProcessProtocolStartReq session id=%u, num of devices=%d, handset_addr %04x:%02x:%06x",
        req.session_id, req.num_of_devices, handset_addr.nap, handset_addr.uap, handset_addr.lap);

    /* If all the data not consumed then packet probably incorrectly formed */
    if(*consumed != size)
    {
        DEBUG_LOG("handoverProfile_ProcessProtocolStartReq size=%d, consumed=%d. Panic", size, *consumed);
        HandoverProfile_DumpData(data, size);
        Panic();
    }

    /* Send HANDOVER_START_CFM to Primary and wait on L2cap stream for marshal data */
    if(handoverProfile_SendHandoverStartCfm(HANDOVER_PROTOCOL_STATUS_SUCCESS, req.session_id) != HANDOVER_PROFILE_STATUS_SUCCESS)
    {
        DEBUG_LOG("handoverProfile_ProcessProtocolStartReq Failed to send Handover Start Cfm");
        /* Panic not required as Primary too would timeout and recover */
        return FALSE;
    }
    return TRUE;
}

/*! 
    \brief Process HANDOVER_PROTOCOL_CANCEL_IND message sent from Primary.

*/
static void handoverProfile_ProcessProtocolCancelInd(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    if(ho_inst->is_primary)
    {
        return;
    }

    DEBUG_LOG("handoverProfile_ProcessProtocolCancelInd marshal_state=0x%x", ho_inst->marshal_state);

    switch (ho_inst->marshal_state)
    {
        case HANDOVER_PROFILE_MARSHAL_STATE_IDLE:
            /* Ignore, already idle, this typically means there was a timeout
               waiting for marshal data, and secondary returned to idle state
               and already aborted P1 clients */
        break;

        case HANDOVER_PROFILE_MARSHAL_STATE_P1_MARSHALLING:
            /* Call abort to free up any P1 unmarshalled data */
            HandoverProfile_AbortP1Clients();
        break;

        case HANDOVER_PROFILE_MARSHAL_STATE_P0_MARSHALLING:
        default:
            /* Cannot handle cancelling after unmarshalling P0 */
            Panic();
        break;
    }
}

/*! 
    \brief Checks if any of the P1 clients vetos.

    \return TRUE: If any of the P1 client veto'ed.
            FALSE: Otherwise.

*/
bool HandoverProfile_VetoP1Clients(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    for(uint8 client_index=0; client_index < ho_inst->num_clients; client_index++)
    {
        if(ho_inst->ho_clients[client_index]->pFnVeto != NULL && ho_inst->ho_clients[client_index]->pFnVeto())
        {
            DEBUG_LOG("HandoverProfile_VetoP1Clients. handover Client (%d) Veto'ed", client_index);
            return TRUE;
        }
    }
    return FALSE;
}

/*! 
    \brief Calls abort function of all the P1 clients registered with the
           Handover Profile.

*/
void HandoverProfile_AbortP1Clients(void)
{
    uint8 client_index=0;
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    for(client_index = 0; client_index < ho_inst->num_clients; client_index++)
    {
        if(ho_inst->ho_clients[client_index]->pFnAbort != NULL)
        {
            ho_inst->ho_clients[client_index]->pFnAbort();
        }
    }
}

/*! 
    \brief Blocks on stream sink until number of bytes mentioned in the parameter
           'size' is claimed for a duration of time mentioned in the parameter
           'timeout'.

    \param[in] sink     Stream sink from which to claim space.
    \param[in] size     Amount of data in bytes to claim in sink.
    \param[in] timeout  Duration in milliseconds to block for space.

    \return Pointer to sink buffer claimed if successful.
            NULL: Otherwise.

*/
static uint8* handoverProfile_ClaimSink(Sink sink,
    uint16 size,
    uint32 timeout)
{
    uint16 offset = HANDOVER_PROFILE_INVALID_SINK_CLAIM_SIZE;
    bool timedout = FALSE;
    uint8 *dest = SinkMap(sink);
    uint16 already_claimed = SinkClaim(sink, 0);

    if(already_claimed == HANDOVER_PROFILE_INVALID_SINK_CLAIM_SIZE)
    {
        already_claimed = 0;
    }

    if(dest == NULL)
    {
        DEBUG_LOG("handoverProfile_ClaimSink SinkMap returned NULL");
        return NULL;
    }

    /* If required space is more than available then wait for more space. */
    if (size > already_claimed)
    {
        DEBUG_LOG("handoverProfile_ClaimSink attempt to claim %u", size-already_claimed);
        timedout = TRUE;
        timeout = VmGetClock() + timeout;
        do
        {
            offset = SinkClaim(sink, size - already_claimed);
            if(offset != HANDOVER_PROFILE_INVALID_SINK_CLAIM_SIZE)
            {
                /* Got enough space to write marshal data to the stream. */
                timedout = FALSE;
                break;
            }
        } while(VmGetClock() < timeout);
    }

    /* If timed out return. */
    if(timedout)
    {
        DEBUG_LOG("handoverProfile_ClaimSink timedout during attempt to claim %u bytes", size-already_claimed);
        return NULL;
    }

    /* No additional claim made */
    if (already_claimed >= size)
    {
        offset = already_claimed;
    }

    if(offset == HANDOVER_PROFILE_INVALID_SINK_CLAIM_SIZE)
    {
        DEBUG_LOG("handoverProfile_ClaimSink SinkClaim returned Invalid Offset");
        return NULL;
    }

    /* Return pointer to which data can be written */
    return (dest + offset - already_claimed);
}

/*! 
    \brief Unmarshal the P1 data sent from the Primary device.
           The function returns if,
           1. Unmarshal the P1 data for a valid CLIENT_ID. That is, until end of marshaling data 
              is received (HANDOVER_MARSHAL_END_TAG).
           2. If all the data is processed from the l2cap source.

           Else, panics if unknown/unexpected data is read from the marshal source.

    \param[in] src_addr     Pointer to the stream buffer holding l2cap data.
    \param[in] size         Size of the buffer src_addr pointing to in bytes.
    \param[out] consumed    Number of bytes processed in src_addr.

    \return TRUE:  HANDOVER_MARSHAL_END_TAG is received in src_addr.
            FALSE: Otherwise.

*/
static bool handoverProfile_UnmarshalP1Clients(const uint8 *src_addr,
    uint16 src_len,
    uint16 *consumed)
{
    uint8 client_id;
    uint16 client_datalen, client_consumed=0, p1_consumed_len=0;
    bool unmarshal_complete;
    uint32 timeout;
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    const uint8 *read_ptr = src_addr;

    timeout = VmGetClock() + HANDOVER_PROFILE_P1_UNMARSHAL_TIMEOUT_MSEC;
    DEBUG_LOG("handoverProfile_UnmarshalP1Clients src_len=%d, consumed=%d", src_len, *consumed);

    ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_P1_MARSHALLING;

    while(p1_consumed_len != src_len &&
          VmGetClock() < timeout)
    {
        client_datalen = 0;
        /* Retrieve the CLIENT_ID */
        client_id = read_ptr[HANDOVER_PROFILE_P1_CLIENT_ID_OFFSET];

        if(client_id != HANDOVER_MARSHAL_END_TAG)
        {
            /* Retrieve client DATA_LEN */
            client_datalen = CONVERT_TO_UINT16((&read_ptr[HANDOVER_PROFILE_P1_CLIENT_DATA_LEN_OFFSET]));
            DEBUG_LOG("handoverProfile_UnmarshalP1Clients Client ID=%d, Data Len=%d", client_id, client_datalen);
        }

        /* Unmarshal P1 client data */
        if( (client_id < ho_inst->num_clients) && 
            (ho_inst->ho_clients[client_id]->pFnUnmarshal != NULL))
        {
            /* Offset to payload (CLIENT_ID + DATA_LEN) */
            read_ptr += HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN;
            p1_consumed_len += HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN;

            unmarshal_complete = ho_inst->ho_clients[client_id]->pFnUnmarshal(&ho_inst ->tp_handset_addr,
                                   read_ptr,
                                   client_datalen, 
                                   &client_consumed);

            DEBUG_LOG("handoverProfile_UnmarshalP1Clients Client=%d, unmarshal_complete=%d, client_consumed=%d",
                      client_id, unmarshal_complete, client_consumed);

            p1_consumed_len += client_consumed;
            read_ptr += client_consumed;

            /* 1. Check if the client did not consume the complete data meant for the same.
               2. Check if the client data size is more than the available stream source 
                  mapped - Should never happen!!. Problem in marshaling.
            */
            if((client_datalen != client_consumed) ||
               (p1_consumed_len > src_len))
            {
                /* timedout */
                DEBUG_LOG("handoverProfile_UnmarshalP1Clients Client ID=%d failed to Unmarshal, src_len=%d, p1_consumed_len=%d, client_datalen=%d, client_consumed=%d. Panic!!", 
                          client_id, src_len, p1_consumed_len, client_datalen, client_consumed);
                HandoverProfile_DumpData(src_addr, src_len);
                Panic();
            }
            client_consumed = 0;
        }
        else
        {
            /* Check if end of marshal reached */
            if(handoverProfile_IsMarshalEnd(read_ptr))
            {
                p1_consumed_len += HANDOVER_PROFILE_P0_P1_TAG_LEN;
                /* If all the data not consumed then packet probably incorrectly formed */
                if(p1_consumed_len != src_len)
                {
                    DEBUG_LOG("handoverProfile_UnmarshalP1Clients, size=%d, p1_consumed_len=%d. Panic", src_len, p1_consumed_len);
                    HandoverProfile_DumpData(src_addr, src_size);
                    Panic();
                }
                *consumed = *consumed + p1_consumed_len;
                return TRUE;
            }
            else
            {
                DEBUG_LOG("handoverProfile_UnmarshalP1Clients Invalid data or Unmarshaler for Client ID=%d does not exist at Secondary. Panic!!", client_id);
                /* Dump the marshal data */
                HandoverProfile_DumpData(src_addr, src_len);
                Panic();
            }
        }
    }

    /* Check if timedout */
    if(p1_consumed_len != src_len)
    {
        DEBUG_LOG("handoverProfile_UnmarshalP1Clients timedout/clients did not consume all data while unmarshaling, src_len=%d, p1_consumed_len=%d. Panic!!", src_len, p1_consumed_len);
        HandoverProfile_DumpData(src_addr, src_len);
        Panic();
    }

    /* End of marshal data not reached. Update the data length consumed */
    *consumed = *consumed + p1_consumed_len;
    return FALSE;
}

/*! 
    \brief Unmarshal P0 or P1 data received from the Primary.

    \param[in] src_addr      Marshaled data from the Primary.
    \param[in] src_size      Marshaled data length.
    \param[out] consumed  Number of bytes processed in data.

    \return TRUE:  If unmarshaling is complete
                   (i.e., HANDOVER_MARSHAL_END_TAG received.in marshal data).
            FALSE: Otherwise.

*/
static bool handoverProfile_ProcessMarshalData(const uint8 *src_addr,
    uint16 src_size, uint16 *consumed)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    uint16 p0_data_size = 0, cur_len;
    const uint8* read_ptr=NULL;
    bool ret=FALSE;

    /* Check if we are in the unmarshal state */
    PanicFalse(!ho_inst->is_primary);

    /* Unmarshal data */
    DEBUG_LOG("handoverProfile_ProcessMarshalData Secondary received %d bytes, consumed=%d", src_size, *consumed);

    /* Check if end of marshal reached */
    if(handoverProfile_IsMarshalEnd(src_addr))
    {
        DEBUG_LOG("handoverProfile_ProcessMarshalData End of marshal data at the start of the packet");
        *consumed = *consumed + HANDOVER_PROFILE_P0_P1_TAG_LEN;
        /* If all the data not consumed then packet probably incorrectly formed */
        if(*consumed != (src_size + HANDOVER_DATA_HEADER_LEN))
        {
            DEBUG_LOG("handoverProfile_ProcessMarshalData, size=%d, consumed=%d. Panic", src_size, consumed);
            HandoverProfile_DumpData(src_addr, src_size);
            Panic();
        }
        /* Check if end-of-marshal tag received after P0 data, if so marshalling is complete */
        if(ho_inst->marshal_state == HANDOVER_PROFILE_MARSHAL_STATE_P0_MARSHALLING)
        {
            return TRUE;
        }
    }

    read_ptr = src_addr;
    cur_len = src_size;

    /* Check for P0 Marshal Tag */
    if(handoverProfile_IsMarshalTagP0(src_addr)  &&
       ((p0_data_size = CONVERT_TO_UINT16(&src_addr[HANDOVER_PROFILE_P0_DATA_LEN_OFFSET])) != 0))
    {
        HandoverPioSet();

        DEBUG_LOG("handoverProfile_ProcessMarshalData Received P0 data size=%d, total received size=%d", p0_data_size, src_size);
        /* Claim space from the marshal sink */
        uint8 *dst_addr;

        /* If P0 data received before P1 then just ignore */
        if(ho_inst->marshal_state == HANDOVER_PROFILE_MARSHAL_STATE_IDLE)
        {
            DEBUG_LOG("handoverProfile_ProcessMarshalData Received P0 data before P1 data. Ignore the data");
            return FALSE;
        }

        if(!ho_inst->marshal_sink)
        {
            ho_inst->marshal_sink = StreamAclMarshalSink(&ho_inst->tp_handset_addr);
        }

        PanicFalse(SinkIsValid(ho_inst->marshal_sink));

        dst_addr = handoverProfile_ClaimSink(ho_inst->marshal_sink, p0_data_size, HANDOVER_PROFILE_P0_UNMARSHAL_TIMEOUT_MSEC);

        if(!dst_addr)
        {
            DEBUG_LOG("handoverProfile_ProcessMarshalData Failed/Timedout to claim marshal sink. Panic");
            Panic();
        }

        /* Increment by P0 tag + P0 len */
        *consumed = *consumed + HANDOVER_PROFILE_P0_HEADER_LEN;

        /* Copy P0 data to the marshal sink */
        memmove(dst_addr, src_addr+HANDOVER_PROFILE_P0_HEADER_LEN, p0_data_size);
        *consumed = *consumed + p0_data_size;

        /* Send the P0 data to the marshal sink */
        PanicZero(SinkFlush(ho_inst->marshal_sink, p0_data_size));

        /* At secondary set the flag after unmarshalling P0 data */
        ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_P0_MARSHALLING;

        DEBUG_LOG("handoverProfile_ProcessMarshalData consumed=%d bytes of P0 data", *consumed);

        /* If more data is available then P0 is complete. Unmarshal End-of-packet */
        if(src_size > (p0_data_size + HANDOVER_PROFILE_P0_HEADER_LEN))
        {
            read_ptr = src_addr + p0_data_size + HANDOVER_PROFILE_P0_HEADER_LEN;
            cur_len = src_size - p0_data_size - HANDOVER_PROFILE_P0_HEADER_LEN;
            DEBUG_LOG("handoverProfile_ProcessMarshalData after P0 unmarshaling data of len=%d pending",
                src_size-(p0_data_size + HANDOVER_PROFILE_P0_HEADER_LEN));

            /* Check if end of marshal reached */
            if(handoverProfile_IsMarshalEnd(read_ptr))
            {
                DEBUG_LOG("handoverProfile_ProcessMarshalData End of marshal data");
                *consumed = *consumed + HANDOVER_PROFILE_P0_P1_TAG_LEN;
                /* If all the data not consumed then packet probably incorrectly formed */
                if(*consumed != (src_size + HANDOVER_DATA_HEADER_LEN))
                {
                    DEBUG_LOG("handoverProfile_ProcessMarshalData, size=%d, consumed=%d. Panic", src_size, consumed);
                    HandoverProfile_DumpData(src_addr, src_size);
                    Panic();
                }
                HandoverPioClr();
                return TRUE;
            }
            else
            {
                DEBUG_LOG("handoverProfile_ProcessMarshalData, Unknown data received");
            }
        }
        HandoverPioClr();
    }
    else
    {
        /* Unmarshal P1 data */
        ret = handoverProfile_UnmarshalP1Clients(read_ptr,
                                                  cur_len,
                                                  consumed);
        /* Check if P1 Unmarshal is complete, if so send HANDOVER_PROTOCOL_UNMARSHAL_P1_CFM  */
        if(ret)
        {
            if(handoverProfile_SendHandoverUnmarshalP1Cfm() != HANDOVER_PROFILE_STATUS_SUCCESS)
            {
                DEBUG_LOG("handoverProfile_ProcessMarshalData, Failed to send HANDOVER_PROTOCOL_UNMARSHAL_P1_CFM");
                HandoverProfile_AbortP1Clients();
            }
            /* Returning TRUE would exit secondary from unmarshal loop which should happen after P0 unmarshalling */
            ret = FALSE;
        }
    }

    /* If all the data not consumed then packet probably incorrectly formed */
    if(*consumed != (src_size + HANDOVER_DATA_HEADER_LEN))
    {
        DEBUG_LOG("handoverProfile_ProcessMarshalData, Post P1 Unmarhal size=%d, consumed=%d. Panic", src_size, *consumed);
        HandoverProfile_DumpData(src_addr, src_size);
        Panic();
    }

    /* Wait for more marshal data or other handover packets else timeout  */
    return ret;
}

/******************************************************************************
 * Global Function Definitions
 ******************************************************************************/
#ifdef HANDOVER_PROFILE_DUMP_MARSHAL_DATA
/*! 
    \brief Dumps the data onto the debug terminal.

    \param[in] buf     Buffer to dump.
    \param[in] size    Amount of data in bytes to dump.

*/
void HandoverProfile_DumpData(const uint8 *buf, size_t size)
{
    for (size_t i=0; i<size; i++)
        DEBUG_LOG("%x", buf[i]);
}
#endif

/*!
    \brief Send handover protocol start request message to peer.

    \param[in] handset_addr Address of the handset.

    \return HANDOVER_PROFILE_STATUS_SUCCESS: If successfully sent the message to peer.
            HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT: Timed out waiting for space in the sink.

*/
handover_profile_status_t HandoverProfile_SendHandoverStartReq(const bdaddr *handset_addr, uint8 session_id)
{
    HANDOVER_PROFILE_HANDOVER_START_REQ_T   start_req;

    memset(&start_req, 0, sizeof(HANDOVER_PROFILE_HANDOVER_START_REQ_T));
    start_req.opcode=HANDOVER_PROTOCOL_START_REQ;
    start_req.session_id = session_id;
    start_req.last_tx_seq = appPeerSigGetLastTxMsgSequenceNumber();
    start_req.last_rx_seq = appPeerSigGetLastRxMsgSequenceNumber();
    start_req.mirror_state = MirrorProfile_GetMirrorState();
    start_req.num_of_devices=HANDOVER_NUMBER_OF_HANDSETS;
    memcpy(&start_req.handset_addr, handset_addr, sizeof(bdaddr));

    return (handoverProfile_SendHandoverMsg(&start_req, 
                sizeof(HANDOVER_PROFILE_HANDOVER_START_REQ_T), 
                HANDOVER_PROFILE_PROTOCOL_MSG_TIMEOUT_MSEC));
}

/*!
    \brief Send handover protocol cancel indication message to peer.

    \return HANDOVER_PROFILE_STATUS_SUCCESS: If successfully sent the message to peer.
            HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT: Timed out waiting for space in the sink.

*/
handover_profile_status_t HandoverProfile_SendHandoverCancelInd(void)
{
    HANDOVER_PROFILE_HANDOVER_CANCEL_IND_T   cncl_ind;

    memset(&cncl_ind, 0, sizeof(HANDOVER_PROFILE_HANDOVER_CANCEL_IND_T));
    cncl_ind.opcode=HANDOVER_PROTOCOL_CANCEL_IND;

    return (handoverProfile_SendHandoverMsg(&cncl_ind, 
                sizeof(HANDOVER_PROFILE_HANDOVER_CANCEL_IND_T), 
                HANDOVER_PROFILE_PROTOCOL_MSG_TIMEOUT_MSEC));
}

/*! 
    \brief Wait for handover start confirm message from peer.

    \return handover_profile_status_t Returns,
            1. HANDOVER_PROFILE_STATUS_SUCCESS if handover start confirm message received.
            2. HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT if timeout waiting for the message.
            3. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if the cofirmation status is not success
            4. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if any other failure occured.

*/
handover_profile_status_t HandoverProfile_ProcessProtocolStartCfm(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    uint16 size = 0;
    const uint8* buf = NULL;

    /* Block on peer stream to receive HANDOVER_START_CFM */
    if((buf = HandoverProfile_BlockReadSource(ho_inst->link_source, &size, HANDOVER_PROFILE_PROTOCOL_MSG_TIMEOUT_MSEC)) == NULL)
    {
        DEBUG_LOG("HandoverProfile_ProcessProtocolStartCfm timedout waiting for HANDOVER_START_CFM.");
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }

    if(!handoverProfile_IsHandoverStartCfm(buf) || size < sizeof(HANDOVER_PROFILE_HANDOVER_START_CFM_T))
    {
        DEBUG_LOG("HandoverProfile_ProcessProtocolStartCfm did not receive HANDOVER_START_CFM. Msg Opcode=0x%x", buf[0]);
        HandoverProfile_DumpData(buf, size);
        SourceDrop(ho_inst->link_source, size);
        return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
    }

    /* Verify session ID and the status in Handover start confirmation message */
    if(!handoverProfile_IsHandoverStartCfmSessionId(buf, ho_inst->session_id) || !handoverProfile_IsHandoverStartCfmStatusSuccess(buf))
    {
        DEBUG_LOG("HandoverProfile_ProcessProtocolStartCfm status was not success. Session id=%u, cfm=0x%x", buf[1], buf[2]);
        HandoverProfile_DumpData(buf, size);
        SourceDrop(ho_inst->link_source, size);
        return HANDOVER_PROFILE_STATUS_HANDOVER_VETOED;
    }

    SourceDrop(ho_inst->link_source, size);

    DEBUG_LOG("HandoverProfile_ProcessProtocolStartCfm HANDOVER_START_CFM received");
    return HANDOVER_PROFILE_STATUS_SUCCESS;
}

/*! 
    \brief Wait for unmarshal P1 confirm message from peer.

    \return handover_profile_status_t Returns,
            1. HANDOVER_PROFILE_STATUS_SUCCESS if handover start confirm message received.
            2. HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT if timeout waiting for the message.
            3. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if the cofirmation status is not success
            4. HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE if any other failure occured.

*/
handover_profile_status_t HandoverProfile_ProcessProtocolUnmarshalP1Cfm(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    uint16 size = 0;
    const uint8* buf = NULL;

    /* Block on peer stream to receive HANDOVER_START_CFM */
    if((buf = HandoverProfile_BlockReadSource(ho_inst->link_source, &size, HANDOVER_PROFILE_PROTOCOL_MSG_TIMEOUT_MSEC)) == NULL)
    {
        DEBUG_LOG("HandoverProfile_ProcessProtocolUnmarshalP1Cfm timedout waiting for HANDOVER_PROTOCOL_UNMARSHAL_P1_CFM.");
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }

    if(!handoverProfile_IsHandoverUnmarshalP1Cfm(buf) || size < sizeof(HANDOVER_PROFILE_HANDOVER_UNMARSHAL_P1_CFM_T))
    {
        DEBUG_LOG("HandoverProfile_ProcessProtocolUnmarshalP1Cfm did not receive HANDOVER_PROTOCOL_UNMARSHAL_P1_CFM.");
        HandoverProfile_DumpData(buf, size);
        SourceDrop(ho_inst->link_source, size);
        return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
    }

    SourceDrop(ho_inst->link_source, size);

    DEBUG_LOG("HandoverProfile_ProcessProtocolUnmarshalP1Cfm HANDOVER_PROTOCOL_UNMARSHAL_P1_CFM received");
    return HANDOVER_PROFILE_STATUS_SUCCESS;
}

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
    uint32 timeout)
{
    *size = 0;
    timeout = VmGetClock() + timeout;
    do
    {
        if((*size=SourceBoundary(src)) != 0)
        {
            /* Got some data */
            return SourceMap(src);
        }
    } while(VmGetClock() < timeout);

    /* Timed out */
    return NULL;
}

/*! 
    \brief Process the message received from the Primary/Secondary handover
           l2cap connection.

    \param[in] mmd    Message type received when more data has arrived at the
                      Source.

*/
void HandoverProfile_ProcessHandoverMessage(const MessageMoreData *mmd)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    uint16 size = 0, consumed = 0;

    if (mmd->source != ho_inst->link_source)
    {
        DEBUG_LOG("handoverProfile_ProcessHandoverMessage MMD received from unknown link");
        Panic();
    }

    size = SourceBoundary(ho_inst->link_source);

    while(size != 0)
    {
        const uint8 *data = SourceMap(ho_inst->link_source);
        uint8 opcode = data[HANDOVER_DATA_HEADER_OFFSET];
        const uint8 *read_ptr = data;
        bool unmarshal_complete = FALSE, block = TRUE;
        consumed = 0;

        consumed += HANDOVER_DATA_HEADER_LEN;
        read_ptr += HANDOVER_DATA_HEADER_LEN;

        DEBUG_LOG("handoverProfile_ProcessHandoverMessage size=%d, opcode=0x%x", size, opcode);

        switch (opcode)
        {
            case HANDOVER_PROTOCOL_START_REQ:
                /* Request for power performance */
                appPowerPerformanceProfileRequest();
                block = handoverProfile_ProcessProtocolStartReq(data, size, &consumed);
                ho_inst->acl_handle = MirrorProfile_GetMirrorAclHandle();
                DEBUG_LOG("handoverProfile_ProcessHandoverMessage acl handle=0x%x", ho_inst->acl_handle);
                break;

            case HANDOVER_PROTOCOL_CANCEL_IND:
                handoverProfile_ProcessProtocolCancelInd();
                block = FALSE;
                break;

            case HANDOVER_MARSHAL_DATA:
            {
                /* Unmarshal the data */
                unmarshal_complete = handoverProfile_ProcessMarshalData(read_ptr, size - HANDOVER_DATA_HEADER_LEN, &consumed);
                break;
            }

            case HANDOVER_PROTOCOL_UNMARSHAL_COMPLETE_IND:
            case HANDOVER_PROTOCOL_START_CFM:
            case HANDOVER_PROTOCOL_UNMARSHAL_P1_CFM:
                /* Do nothing */
                break;

            default:
                DEBUG_LOG("handoverProfile_ProcessHandoverMessage Unexpected data. Panic!!");
                HandoverProfile_DumpData(data, size);
                Panic();
                break;
        }

        SourceDrop(ho_inst->link_source, size);

        if(!block)
        {
            /* Relinquish power performance */
            appPowerPerformanceProfileRelinquish();
            /* Blocked wait not required */
            return;
        }

        /* Unmarshaling is complete */
        if(unmarshal_complete)
        {
            DEBUG_LOG("handoverProfile_ProcessHandoverMessage marshaling is complete");
            bdaddr bd_addr_peer;

            /* For A2DP mirroring, the earliest the new primary bud may receive
               data from the handset is after the buds re-enter sniff mode. This
               means the P1 commit can be deferred in this mode until after the
               enter sniff command has been sent to the controller */
            if (!MirrorProfile_IsA2dpActive())
            {
                /* Commit P1 clients */
                HandoverProfile_CommitP1Clients(!ho_inst->is_primary);
            }

            HandoverPioSet();
            /* Commit P0/BTSS clients */
            if(!AclHandoverCommit(ho_inst->acl_handle))
            {
                DEBUG_LOG("HandoverProfile_Handover AclHandoverCommit() failed at Secondary, acl handle=0x%x. Panic!!", ho_inst->acl_handle);
                Panic();
            }
            HandoverPioClr();

            if (MirrorProfile_IsA2dpActive())
            {
                /* Need to commit the mirror profile so it knows about the change
                   in role so the peer link policy can be updated correctly */
                mirror_handover_if.pFnCommit(&ho_inst ->tp_handset_addr, TRUE);
            }

            /* The new primary re-enters sniff mode */
            MirrorProfile_UpdatePeerLinkPolicy(lp_sniff);

            if (MirrorProfile_IsA2dpActive())
            {
                /* Commit P1 clients */
                HandoverProfile_CommitP1Clients(!ho_inst->is_primary);
            }

            HandoverPioSet();

            /* Complete P1 clients*/
            HandoverProfile_CompleteP1Clients(!ho_inst->is_primary);

            if (!MirrorProfile_WaitForPeerLinkMode(lp_sniff, HANDOVER_PROFILE_REENTER_SNIFF_TIMEOUT_MSEC))
            {
                DEBUG_LOG("handoverProfile_ProcessHandoverMessage timeout waiting to re-enter sniff mode");
            }

            HandoverPioClr();

            /* Relinquish power performance */
            appPowerPerformanceProfileRelinquish();

            if(!SinkClose(ho_inst->marshal_sink))
            {
                DEBUG_LOG("handoverProfile_ProcessHandoverMessage Failed to close marshal sink");
            }

            ho_inst->marshal_sink = 0;
            ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_IDLE;
            ho_inst->is_primary = TRUE;

            /* Update the new peer address */
            if(appDeviceGetPeerBdAddr(&bd_addr_peer))
            {
                memcpy(&ho_inst->peer_addr, &bd_addr_peer, sizeof(bdaddr));
            }

            DEBUG_LOG("handoverProfile_ProcessHandoverMessage Handover Complete. I am new Primary.");
            return;
        }
        size = 0;

        /* Wait for more data in case of HANDOVER_PROTOCOL_START_REQ and HANDOVER_MARSHAL_DATA */
        if(HandoverProfile_BlockReadSource(ho_inst->link_source, &size, HANDOVER_PROFILE_PROTOCOL_MSG_TIMEOUT_MSEC) == NULL)
        {
            if(opcode == HANDOVER_MARSHAL_DATA)
            {
                if(ho_inst->marshal_state == HANDOVER_PROFILE_MARSHAL_STATE_P0_MARSHALLING)
                {
                    DEBUG_LOG("handoverProfile_ProcessHandoverMessage Marshal data, timedout waiting for P0 marshal data. Panic!!");
                    Panic();
                }
                else if(ho_inst->marshal_state == HANDOVER_PROFILE_MARSHAL_STATE_P1_MARSHALLING)
                {
                    DEBUG_LOG("handoverProfile_ProcessHandoverMessage timedout waiting for P1 marshal data. Cleanup P1 clients");
                    ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_IDLE;
                    /* Call abort to free up any P1 unmarshalled data */
                    HandoverProfile_AbortP1Clients();
                    /* Relinquish power performance */
                    appPowerPerformanceProfileRelinquish();
                }
            }
            return;
        }
    }
}

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
handover_profile_status_t HandoverProfile_MarshalP0Clients(const tp_bdaddr *bd_addr)
{
    Source marshal_src=0;
    uint8 *write_ptr=NULL, *data_len_header_offset=NULL;
    const uint8 *src_addr=NULL;
    uint16 p0_data_len;
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    bool p0_data_available = TRUE, packet_full = FALSE;

    ho_inst->sink_written = 0;

    DEBUG_LOG("HandoverProfile_MarshalP0Clients");

    marshal_src = StreamAclMarshalSource(bd_addr);
    if(marshal_src == 0)
    {
        DEBUG_LOG("HandoverProfile_MarshalP0Clients StreamAclMarshalSource failed!!");
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }

    /* Wait until P0 data marshaling is complete */
    while(p0_data_available)
    {
        uint16 write_len = 0;

        /* Check if P0 has written data to marshal source stream. */
        if((p0_data_len=SourceSize(marshal_src)) != 0)
        {
            DEBUG_LOG("HandoverProfile_MarshalP0Clients SourceSize returned p0_data_len=%d", p0_data_len);

            src_addr = SourceMap(marshal_src);
            if(src_addr == NULL)
            {
                DEBUG_LOG("HandoverProfile_MarshalP0Clients SourceMap failed. Panic");
                Panic();
            }

            /* Claim MTU size in the sink and add P0 header */
            if(write_ptr == NULL)
            {
                write_ptr = handoverProfile_ClaimSink(ho_inst->link_sink,
                                HANDOVER_PROFILE_L2CAP_MTU_SIZE,
                                HANDOVER_PROFILE_P0_MARSHAL_TIMEOUT_MSEC);

                if(write_ptr ==  NULL)
                {
                    DEBUG_LOG("HandoverProfile_MarshalP0Clients handoverProfile_ClaimSink failed!!");
                    return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
                }

                /* Add Header (HANDOVER_MARSHAL_DATA|HANDOVER_MARSHAL_P0_TAG) */
                *write_ptr++ = HANDOVER_MARSHAL_DATA;
                *write_ptr++ = HANDOVER_MARSHAL_P0_TAG;

                /* Point to Data Len field */
                data_len_header_offset = write_ptr;
                /* Point to P0 data for marshal data to be written */
                write_ptr += HANDOVER_PROFILE_P0_DATA_LEN;
                ho_inst->sink_written = HANDOVER_PROFILE_MARSHAL_HEADER_LEN;
            }

            /* P0 data is more than available sink buffer.
               Write P0 data into available buffer and flush */
            if(p0_data_len >= (HANDOVER_PROFILE_L2CAP_MTU_SIZE - ho_inst->sink_written))
            {
                write_len = HANDOVER_PROFILE_L2CAP_MTU_SIZE - ho_inst->sink_written;
                /* Packet length reached mark for flushing the packet */
                packet_full = TRUE;
            }
            else
            {
                /* Write the P0 data and wait for more P0 data */
                write_len = p0_data_len;
            }

            DEBUG_LOG("HandoverProfile_MarshalP0Clients Marshaling P0 write_len=%d bytes", write_len);
            memmove(write_ptr, src_addr, write_len);
            write_ptr += write_len;
            ho_inst->sink_written += write_len;
            SourceDrop(marshal_src, write_len);
        }
        /* No more P0 data */
        else
        {
            DEBUG_LOG("HandoverProfile_MarshalP0Clients P0 done");
            p0_data_available = FALSE;
            packet_full = TRUE;
        }

        /* Update P0 client length field, if P0 data is written to stream */
        if(write_len)
        {
            /* Append P0 Client Length field */
            CONVERT_FROM_UINT16(data_len_header_offset,
                ho_inst->sink_written - HANDOVER_PROFILE_MARSHAL_HEADER_LEN);
        }

        /* Packet length has reached hence flush the packet */
        if(packet_full && p0_data_available)
        {
            packet_full = FALSE;
            DEBUG_LOG("HandoverProfile_MarshalP0Clients Sending packet to peer ");
            if(SinkFlush(ho_inst->link_sink, ho_inst->sink_written) == 0)
            {
                DEBUG_LOG("HandoverProfile_MarshalP0Clients Due to SinkFlush failure, unable to send packet to peer ");
                SourceClose(marshal_src);
                return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
            }
            ho_inst->sink_written = 0;
            write_ptr = NULL;
        }
    }

    SourceClose(marshal_src);

    /* Marshaling all data are done. Check if we have enough space (1 byte) in the current packet
       1. Add end of marshal tag (Client ID=0xFF) else
       2. Claim more space(2 bytes) for writing end of marshal tag (Marshal(0x80) | MarshalEnd(0xFF)) and
       3. Flush */
    if((HANDOVER_PROFILE_L2CAP_MTU_SIZE - ho_inst->sink_written) >= HANDOVER_PROFILE_P0_P1_TAG_LEN)
    {
        /* Add Client ID = 0xFF to mark the end of Marshal data */
        *write_ptr++ = HANDOVER_MARSHAL_END_TAG;
        ho_inst->sink_written++;
    }
    else
    {
        /* Claim more space to write the end of Marshal data tag */
        if(SinkFlush(ho_inst->link_sink, ho_inst->sink_written) == 0)
        {
            DEBUG_LOG("HandoverProfile_MarshalP0Clients Unable to flush the data");
            return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
        }

        ho_inst->sink_written = 0;
        DEBUG_LOG("HandoverProfile_MarshalP0Clients handoverProfile_ClaimSink again for sending marshal end tag");

        /* Claim at least HANDOVER_PROFILE_MARSHAL_P1_HEADER_LEN in the sink */
        write_ptr = handoverProfile_ClaimSink(ho_inst->link_sink,
                        HANDOVER_PROFILE_MARSHAL_P1_HEADER_LEN,
                        HANDOVER_PROFILE_P0_MARSHAL_TIMEOUT_MSEC);

        if(write_ptr == NULL)
        {
            DEBUG_LOG("HandoverProfile_MarshalP0Clients handoverProfile_ClaimSink failed/timedout!!");
            return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
        }
        /* Write Marshal(0x80) + End of marshal tag (0xFF) */
        *write_ptr++ = HANDOVER_MARSHAL_DATA;
        *write_ptr++ = HANDOVER_MARSHAL_END_TAG;
        ho_inst->sink_written = HANDOVER_PROFILE_MARSHAL_P1_HEADER_LEN;
    }

    if(SinkFlush(ho_inst->link_sink, ho_inst->sink_written) == 0)
    {
        DEBUG_LOG("HandoverProfile_MarshalP0Clients Unable to flush end of data tag");
        return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
    }

    ho_inst->sink_written = 0;
    return HANDOVER_PROFILE_STATUS_SUCCESS;
}


/*! 
    \brief Marshal all P1 clients data and send to to peer.

    Start P1 client data in a new packet followed by end-of-marshal tag.

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
handover_profile_status_t HandoverProfile_MarshalP1Clients(const tp_bdaddr *bd_addr)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    uint8 *dest_addr=NULL, *write_ptr=NULL, client_id=0;
    uint16 client_len = 0;

    /* Marshal P1 clients */
    while(client_id < ho_inst->num_clients)
    {
        /* Claim MTU size in the sink and add marshal header */
        if(write_ptr == NULL)
        {
            ho_inst->sink_written = 0;
            client_len = 0;

            /* Claim MTU size in the sink */
            dest_addr = handoverProfile_ClaimSink(ho_inst->link_sink, 
                            HANDOVER_PROFILE_L2CAP_MTU_SIZE,
                            HANDOVER_PROFILE_P0_MARSHAL_TIMEOUT_MSEC);
            write_ptr = dest_addr;

            if(dest_addr == NULL)
            {
                DEBUG_LOG("HandoverProfile_MarshalP1Clients handoverProfile_ClaimSink failed/timedout!!");
                return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
            }

            /* Append P1 Header (HANDOVER_MARSHAL_DATA) */
            *write_ptr++ = HANDOVER_MARSHAL_DATA;
            ho_inst->sink_written++;
        }

        if(((ho_inst->sink_written + HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN) < HANDOVER_PROFILE_L2CAP_MTU_SIZE) &&
             ho_inst->ho_clients[client_id]->pFnMarshal(bd_addr,
                                                        /* Offset after CLIENT_ID + DATA_LEN fields */
                                                        write_ptr + HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN,
                                                        (HANDOVER_PROFILE_L2CAP_MTU_SIZE - (ho_inst->sink_written + HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN)),
                                                        &client_len))
        {
            DEBUG_LOG("HandoverProfile_MarshalP1Clients Client Id=%d Marshalled %d bytes", client_id, client_len);
            /* Marshal complete for a client.
               Append client header(Client ID|DATA_LEN) if client has written marshal data */
            if(client_len)
            {
                /* Append P1 CLIENT_ID */
                write_ptr[HANDOVER_PROFILE_P1_CLIENT_ID_OFFSET] = client_id;
                /* Append Marshaled data length */
                CONVERT_FROM_UINT16(write_ptr + HANDOVER_PROFILE_P1_CLIENT_DATA_LEN_OFFSET, client_len);
                ho_inst->sink_written += client_len + HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN;
                /* Increment by client header(CLIENT_ID|DATA_LEN) + client data length */
                write_ptr += HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN + client_len;
            }

            /* Move to next client */
            client_id++;
            client_len=0;
        }
        /* Flush the current packet and check for marshal data again */
        else
        {
            DEBUG_LOG("HandoverProfile_MarshalP1Clients Client Id=%d marshalled %d bytes returned FALSE, flush the packet to claim more space", client_id, client_len);
            /* Update the header if the client has written some data */
            if(client_len)
            {
                /* Append P1 CLIENT_ID */
                write_ptr[HANDOVER_PROFILE_P1_CLIENT_ID_OFFSET] = client_id;
                /* Append Marshaled data length */
                CONVERT_FROM_UINT16(write_ptr + HANDOVER_PROFILE_P1_CLIENT_DATA_LEN_OFFSET, client_len);
                ho_inst->sink_written += client_len + HANDOVER_PROFILE_P1_CLIENT_HEADER_LEN;
            }
            /* Send the current packet to peer */
            if(SinkFlush(ho_inst->link_sink, ho_inst->sink_written) == 0)
            {
                DEBUG_LOG("HandoverProfile_MarshalP1Clients Unable to flush the packet");
                return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
            }

            write_ptr = NULL;
            ho_inst->sink_written = 0;
        }
    }

    /* Marshalling P1 data is done. Check if we have enough space (1 byte) in the current packet
       1. Add end of marshal tag (Client ID=0xFF) else
       2. Claim more space(2 bytes) for writing end of marshal tag (Marshal(0x80) | MarshalEnd(0xFF)) and
       3. Flush */
    if((HANDOVER_PROFILE_L2CAP_MTU_SIZE - ho_inst->sink_written) >= HANDOVER_PROFILE_P0_P1_TAG_LEN)
    {
        DEBUG_LOG("HandoverProfile_MarshalP0Clients marshal end tag");
        /* Add Client ID = 0xFF to mark the end of Marshal data */
        *write_ptr++ = HANDOVER_MARSHAL_END_TAG;
        ho_inst->sink_written++;
    }
    else
    {
        if(SinkFlush(ho_inst->link_sink, ho_inst->sink_written) == 0)
        {
            DEBUG_LOG("HandoverProfile_MarshalP1Clients Unable to flush the packet");
            return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
        }

        ho_inst->sink_written = 0;
        DEBUG_LOG("HandoverProfile_MarshalP0Clients handoverProfile_ClaimSink again for sending marshal end tag");

        /* Claim at least HANDOVER_PROFILE_MARSHAL_P1_HEADER_LEN in the sink */
        write_ptr = handoverProfile_ClaimSink(ho_inst->link_sink,
                        HANDOVER_PROFILE_MARSHAL_P1_HEADER_LEN,
                        HANDOVER_PROFILE_P0_MARSHAL_TIMEOUT_MSEC);

        if(write_ptr == NULL)
        {
            DEBUG_LOG("HandoverProfile_MarshalP0Clients handoverProfile_ClaimSink failed/timedout!!");
            return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
        }
        /* Write Marshal(0x80) + End of marshal tag (0xFF) */
        *write_ptr++ = HANDOVER_MARSHAL_DATA;
        *write_ptr++ = HANDOVER_MARSHAL_END_TAG;
        ho_inst->sink_written = HANDOVER_PROFILE_MARSHAL_P1_HEADER_LEN;
    }

    /* Send the P1 data completely as its cached at secondary */
    if(ho_inst->sink_written)
    {
        DEBUG_LOG("HandoverProfile_MarshalP1Clients Flush the last P1 packet");
        if(SinkFlush(ho_inst->link_sink, ho_inst->sink_written) == 0)
        {
            DEBUG_LOG("HandoverProfile_MarshalP1Clients Unable to flush the last P1 packet");
            return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
        }

        ho_inst->sink_written = 0;
        write_ptr = NULL;
    }

    DEBUG_LOG("HandoverProfile_MarshalP1Clients Marshalling P1 data complete");
    return HANDOVER_PROFILE_STATUS_SUCCESS;
}

/*! 
    \brief Calls commit function of all the P1 clients registered with the
           Handover Profile.
           This function can be called in Primary or Secondary role.

 */
void HandoverProfile_CommitP1Clients(bool is_primary)
{
    uint8 client_index=0;
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    for(client_index = 0; client_index < ho_inst->num_clients; client_index++)
    {
        if(ho_inst->ho_clients[client_index]->pFnCommit != NULL)
        {
            HandoverPioSet();
            ho_inst->ho_clients[client_index]->pFnCommit(&ho_inst ->tp_handset_addr,
                                                         is_primary);
            HandoverPioClr();
        }
    }
}

/*! 
    \brief Calls Complete function of all the P1 clients registered with the
           Handover Profile.
           This function can be called in Primary or Secondary role.

 */
void HandoverProfile_CompleteP1Clients(bool is_primary)
{
    uint8 client_index=0;
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    for(client_index = 0; client_index < ho_inst->num_clients; client_index++)
    {
        if(ho_inst->ho_clients[client_index]->pFnComplete != NULL)
        {
            ho_inst->ho_clients[client_index]->pFnComplete(is_primary);
        }
    }
}

/*! 
    \brief Handles the failure of handover procedure at various stages before
           marshaling of data starts at the Primary by,
           1. Sending HANDOVER_PROTOCOL_CANCEL_IND message to secondary.
           2. Resumes the inbound data flow for the ACL associated with 
              the addr.

    \param[in] addr    Bluetooth address of the handset.

*/
void HandoverProfile_HandlePrepareForMarshalFailure(const tp_bdaddr *addr)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    DEBUG_LOG("HandoverProfile_HandlePrepareForMarshalFailure send Handover Cancel Ind");
    /* Send HANDOVER_CANCEL_IND to peer */
    if(HandoverProfile_SendHandoverCancelInd() != HANDOVER_PROFILE_STATUS_SUCCESS)
    {
        DEBUG_LOG("HandoverProfile_HandlePrepareForMarshalFailure Failed to send Handover Cancel Ind");
    }

    /* Re-enter sniff mode when handover fails */
    MirrorProfile_UpdatePeerLinkPolicy(lp_sniff);

    if(!AclReceiveEnable(addr, TRUE, HANDOVER_PROFILE_ACL_RECEIVE_ENABLE_TIMEOUT_USEC))
    {
        DEBUG_LOG("HandoverProfile_HandlePrepareForMarshalFailure AclReceiveEnable timedout/failed");
    }

    HandoverProfile_AbortP1Clients();

    DEBUG_LOG("HandoverProfile_HandlePrepareForMarshalFailure AclReceiveEnable(TRUE) completed");

    ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_IDLE;
    /* Relinquish power performance */
    appPowerPerformanceProfileRelinquish();
}

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
void HandoverProfile_HandleMarshalFailure(const tp_bdaddr *addr)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    if(HandoverProfile_SendHandoverCancelInd() != HANDOVER_PROFILE_STATUS_SUCCESS)
    {
        DEBUG_LOG("HandoverProfile_HandleMarshalFailure Failed to send Handover Cancel Ind");
    }

    if(!AclHandoverCancel(ho_inst->acl_handle))
    {
        DEBUG_LOG("HandoverProfile_HandleMarshalFailure AclHandoverCancel failed");
    }

    /* Re-enter sniff mode when handover fails */
    MirrorProfile_UpdatePeerLinkPolicy(lp_sniff);

    if(!AclReceiveEnable(addr, TRUE, HANDOVER_PROFILE_ACL_RECEIVE_ENABLE_TIMEOUT_USEC))
    {
        DEBUG_LOG("HandoverProfile_HandleMarshalFailure AclReceiveEnable timedout/failed");
    }

    HandoverProfile_AbortP1Clients();

    ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_IDLE;
    /* Relinquish power performance */
    appPowerPerformanceProfileRelinquish();
}

/* Check if all outbound data has been transmitted */
bool HandoverProfile_IsAclTransmitPending (const tp_bdaddr *addr, uint32 t)
{    
    uint32 timeout = 0;
    bool timedout = TRUE;
    timeout = VmGetClock() + t;
    do
    {
        if(!AclTransmitDataPending(addr))
        {
            timedout = FALSE;
            break;
        }
    } while(VmGetClock() < timeout);

    return timedout;
}

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
handover_profile_status_t HandoverProfile_PrepareForMarshal(const tp_bdaddr *addr)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    uint32 timeout = 0;
    bool timedout = TRUE;
    tp_bdaddr tp_peer_addr;
    acl_rx_processed_status ret;
    acl_handover_prepare_status prepared_status=ACL_HANDOVER_PREPARE_FAILED;
    lp_power_mode mode;

    BdaddrTpFromBredrBdaddr(&tp_peer_addr, &ho_inst->peer_addr);

    HandoverPioSet();
    if(!AclReceiveEnable(addr, FALSE, HANDOVER_PROFILE_ACL_RECEIVE_ENABLE_TIMEOUT_USEC))
    {
        DEBUG_LOG("HandoverProfile_PrepareForMarshal AclReceiveEnable(false) timeout/failed");
        HandoverPioClr();
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }
    HandoverPioClr();

    TimestampEvent(TIMESTAMP_EVENT_PRI_HANDOVER_CRITICAL_SECTION_STARTED);

    if(ACL_RECEIVE_DATA_PROCESSED_COMPLETE != 
        (ret = AclReceivedDataProcessed(addr, HANDOVER_PROFILE_ACL_RECEIVED_DATA_PROCESSED_TIMEOUT_USEC)))
    {
        DEBUG_LOG("HandoverProfile_PrepareForMarshal AclReceivedDataProcessed timeout/failed, ret=0x%x", ret);
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }

    if(HandoverProfile_IsAclTransmitPending(addr, HANDOVER_PROFILE_ACL_TRANSMIT_DATA_PENDING_TIMEOUT_MSEC))
    {
        DEBUG_LOG("HandoverProfile_PrepareForMarshal AclTransmitDataPending timeout/failed");
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }

    HandoverPioSet();
    /* Check if any of P1 clients Vetos */
    if(HandoverProfile_VetoP1Clients())
    {
        HandoverPioClr();
        DEBUG_LOG("HandoverProfile_PrepareForMarshal Vetoed by P1");
        return HANDOVER_PROFILE_STATUS_HANDOVER_VETOED;
    }
    HandoverPioClr();

    /* For A2DP handover the peer link policy may be changed to active mode at
       the same time as the controller prepares for handover. This reduces the
       handover time. For other handover types the link policy must be changed
       before the controller prepares for handover.
    */
    if (MirrorProfile_IsA2dpActive())
    {
        MirrorProfile_UpdatePeerLinkPolicy(lp_active);
    }
    else
    {
        if(!MirrorProfile_UpdatePeerLinkPolicyBlocking(lp_active, HANDOVER_PROFILE_EXIT_SNIFF_TIMEOUT_MSEC))
        {
            DEBUG_LOG("HandoverProfile_PrepareForMarshal Could not exit sniff mode");
            return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
        }
    }

    HandoverPioSet();
    timedout = TRUE;

    if(ConManagerGetPowerMode(addr,&mode))
    {
        if(mode == lp_sniff)
        {
            uint16 sniff_slots;
            ConManagerGetSniffInterval(addr,&sniff_slots);
            timeout = VmGetClock() + (rtime_get_sniff_interval_in_ms(sniff_slots) * HANDOVER_PROFILE_NO_OF_TIMES_SNIFF_INTERVAL);
        }
        else
        {
            timeout = VmGetClock() + HANDOVER_PROFILE_ACL_HANDOVER_PREPARE_TIMEOUT_MSEC;
        }
    }
    else
    {
        HandoverPioClr();
        DEBUG_LOG("HandoverProfile_PrepareForMarshal No connection exists with the Handset");
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }

    do{
        HandoverPioClr();
        /* Check if P0 mirror manager is in a stable state to initiate handover */
        if((ho_inst->acl_handle = AclHandoverPrepare(&ho_inst->tp_handset_addr, (const tp_bdaddr *)&tp_peer_addr)) != 0xFFFF)
        {
            HandoverPioSet();
             /* Wait until AclHandoverPrepared returns ACL_HANDOVER_PREPARE_COMPLETE or ACL_HANDOVER_PREPARE_FAILED */
            while((prepared_status = AclHandoverPrepared(ho_inst->acl_handle)) == ACL_HANDOVER_PREPARE_IN_PROGRESS)
            {
                /* Do nothing */
            }
            HandoverPioClr();

            /* Exit loop if handover prepare completed */
            if(prepared_status == ACL_HANDOVER_PREPARE_COMPLETE)
            {
                timedout = FALSE;
                break;
            }
        }
    } while(VmGetClock() < timeout);

    /* AclHandoverPrepare didn't succeed return failure */
    if(ho_inst->acl_handle == 0xFFFF)
    {
        DEBUG_LOG("HandoverProfile_PrepareForMarshal AclHandoverPrepare failed, ho_inst->acl_handle=0x%x", ho_inst->acl_handle);
        return HANDOVER_PROFILE_STATUS_HANDOVER_VETOED;
    }

    if(timedout)
    {
        DEBUG_LOG("HandoverProfile_PrepareForMarshal Handover timedout");
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }

    /* Wait to exit sniff mode */
    HandoverPioSet();
    if(!MirrorProfile_WaitForPeerLinkMode(lp_active, HANDOVER_PROFILE_EXIT_SNIFF_TIMEOUT_MSEC))
    {
        HandoverPioClr();
        DEBUG_LOG("HandoverProfile_PrepareForMarshal Could not exit sniff mode");
        return HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT;
    }
    HandoverPioClr();

    return HANDOVER_PROFILE_STATUS_SUCCESS;
}
#endif /* INCLUDE_MIRRORING */
