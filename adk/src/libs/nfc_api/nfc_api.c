/****************************************************************************
Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.
    */

/*!
\ingroup nfc_api

\brief Implementation of the NFC API Library 

@{
*/

/** @}*/
#include <vm.h>
#include <stdio.h>
#include <stdlib.h>
#include <message.h>
#include <sink.h>

#if defined(HYDRACORE)
#include "nfc_api.h" /* itself first */
#include "hydra_log.h"
#include "hydra_macros.h"
#include "panic.h"
#include "nfc.h"
#endif /* HYDRACORE */

#if defined(HYDRACORE)
/* PRIVATE MACRO DEFINITIONS *************************************************/
#ifdef DEBUG_NFC_API
    #define NFC_API_DEBUG(x) {printf x;}
#else
    #define NFC_API_DEBUG(x)
#endif

/* PRIVATE FUNCTION DEFINITIONS **********************************************/

/* PUBLIC FUNCTION DEFINITIONS ***********************************************/
void NfcSetRecvTask(TaskData *nfcTask)
{
    NFC_API_DEBUG(("NFC_PRIM_I:NfcSetRecvTask\n"));
    (void)MessageNfcTask(nfcTask);
}

void NfcSendCtrlRegisterReqPrim(void)
{
    NFC_PRIM *mv = PanicUnlessMalloc(sizeof(*mv));
    mv->id = (uint16)NFC_CTRL_REGISTER_REQ_ID;
    NFC_API_DEBUG(("NFC_PRIM_I:ID 0x%x (NFC_CTRL_REGISTER_REQ)\n", mv->id));
    NfcSendPrim(mv);
}

void NfcSendCtrlResetReqPrim(void)
{
    NFC_PRIM *mv = PanicUnlessMalloc(sizeof(*mv));
    mv->id = (uint16)NFC_CTRL_RESET_REQ_ID;
    NFC_API_DEBUG(("NFC_PRIM_I:ID 0x%x (NFC_CTRL_RESET_REQ)\n", mv->id));
    NfcSendPrim(mv);
}

void NfcSendCtrlConfigReqPrim(NFC_VM_MODE nfc_mode,
                              NFC_VM_LLCP_SERVICE_TYPE ch_service,
                              NFC_VM_LLCP_SERVICE_TYPE snep_service)
{
    NFC_PRIM *mv = PanicUnlessMalloc(sizeof(*mv));
    mv->id = (uint16)NFC_CTRL_CONFIG_REQ_ID;
    mv->m.config_req.mode = nfc_mode;
    mv->m.config_req.ch_service = ch_service;
    mv->m.config_req.snep_service = snep_service;
    NFC_API_DEBUG(("NFC_PRIM_I:ID 0x%x (NFC_CTRL_CONFIG_REQ) (mode=%d CH-C/S=%d SNEP-C/S=%d)\n",
                   mv->id,
                   mv->m.config_req.mode,
                   mv->m.config_req.ch_service,
                   mv->m.config_req.snep_service));
    NfcSendPrim(mv);
}

void NfcSendTagReadUidReqPrim(void)
{
    NFC_PRIM *mv = PanicUnlessMalloc(sizeof(*mv));
    mv->id = (uint16)NFC_TAG_READ_UID_REQ_ID;
    NFC_API_DEBUG(("NFC_PRIM_I:ID 0x%x (NFC_TAG_READ_UID_REQ)\n", mv->id));
    NfcSendPrim(mv);
}

void NfcSendTagWriteUidReqPrim(const uint8 *nfcid, uint8 nfcid_size)
{
    NFC_PRIM *mv = PanicUnlessMalloc(sizeof(*mv));
    mv->id = (uint16)NFC_TAG_WRITE_UID_REQ_ID;
    if(IS_VALID_PTR(nfcid) && IS_VALID_NFCID_SZ(nfcid_size))
    {
        if(0 != nfcid_size)
        {
            memcpy(mv->m.write_uid_req.nfcid,
                   nfcid,
                   MIN(nfcid_size, MAX_NFCID_SIZE));
        }
        mv->m.write_uid_req.nfcid_size = (uint8)MIN(nfcid_size, MAX_NFCID_SIZE);
    }
    else
    {
        NFC_API_DEBUG(("NFC_PRIM_E:(INVALID nfcid_size=%d use 0,4,7,10 or INVALID nfcid=0x%x)\n",
                       nfcid_size, nfcid));
        mv->m.write_uid_req.nfcid_size = 0;
    }
    NFC_API_DEBUG(("NFC_PRIM_I:ID 0x%x (NFC_TAG_WRITE_UID_REQ)(nfcid_size=%d)\n",
                   mv->id, mv->m.write_uid_req.nfcid_size));
    NfcSendPrim(mv);
}

void NfcSendTagWriteNdefReqPrim(const uint8 *ndef, uint8 ndef_size)
{
    NFC_PRIM *mv;
    if(IS_VALID_PTR(ndef) &&
       ndef_size != 0 &&
       TT2_MAX_NDEF >= ndef_size)
    {
        mv = PanicUnlessMalloc(sizeof(*mv) + MIN(TT2_MAX_NDEF, ndef_size));
        mv->m.write_ndef_req.ndef_size = MIN(TT2_MAX_NDEF, ndef_size);
        memcpy(mv->m.write_ndef_req.ndef, ndef, mv->m.write_ndef_req.ndef_size);
    }
    else
    {
        mv = PanicUnlessMalloc(sizeof(*mv));
        mv->m.write_ndef_req.ndef_size = 0;
        NFC_API_DEBUG(("NFC_PRIM_E:(INVALID ndef_size=%d or INVALID ndef=0x%x)\n",
                       ndef_size, ndef));
    }
    mv->id = (uint16)NFC_TAG_WRITE_NDEF_REQ_ID;
    NFC_API_DEBUG(("NFC_PRIM_I:ID 0x%x (NFC_TAG_WRITE_NDEF_REQ)(ndef_size=%d)\n",
                   mv->id,mv->m.write_ndef_req.ndef_size));
    NfcSendPrim(mv);
}

void NfcSendTagReadNdefReqPrim(void)
{
    NFC_PRIM *mv = PanicUnlessMalloc(sizeof(*mv));
    mv->id = (uint16)NFC_TAG_READ_NDEF_REQ_ID;
    NFC_API_DEBUG(("NFC_PRIM_I:ID 0x%x (NFC_TAG_READ_NDEF_REQ)\n",
                   mv->id));
    NfcSendPrim(mv);
}

void NfcSendTagWriteChCarriersReqPrim(uint8 n_carriers,
                                      NFC_CH_CARRIER **carriers)
{
    NFC_PRIM *mv;
    mv = PanicUnlessMalloc(sizeof(*mv));
    mv->id = (uint16)NFC_TAG_WRITE_CH_CARRIERS_REQ_ID;
    mv->m.write_carriers_req.n_carriers = n_carriers;
    mv->m.write_carriers_req.carriers = carriers;
    NFC_API_DEBUG(("NFC_PRIM_I:ID 0x%x (NFC_TAG_WRITE_CARRIERS_REQ)(n_carriers:%d)\n",
                   mv->id, n_carriers));
    NfcSendPrim(mv);
}

bool NfcMessageValid(MessageId id, Message msg)
{
    bool valid;
    /* Message without body */
    /* Other must have a body not NULL body */
    if(IS_VALID_PTR(msg))
    {
        valid = TRUE;
    }
    else if(NFC_CTRL_CARRIER_ON_IND_ID  == id ||
            NFC_CTRL_CARRIER_LOSS_IND_ID == id ||
            NFC_TAG_READ_STARTED_IND_ID == id)
    {
        valid = TRUE;
    }
    else
    {
        valid = FALSE;
    }
    return valid;
}

bool NfcFlushCHData(Sink sink, uint16 msg_len,
                            NFC_STREAM_HANDOVER_TYPE handover_type)
    {
    NFC_STREAM_INF meta_inf;

    /* Fill metadata information */
    meta_inf.stream_id = nfc_stream_type_ch;
    meta_inf.m.nfc_ch_info.handover_record_type = handover_type;

    /* Flush data */
    return SinkFlushHeader(sink, msg_len, (const void*)&meta_inf,
                    sizeof(meta_inf));
}
#endif /* HYDRACORE */

