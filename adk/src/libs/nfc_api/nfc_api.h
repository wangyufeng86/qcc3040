/****************************************************************************
Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd. 
  
 
*/

/*****************************************************************************/
/*!

\defgroup nfc_api nfc_api

\ingroup nfc_cl

\brief  This file provides the functions required to send NFC primitives 
    to NFC subsystem. .i.e. the API between the VM application and the NFC
    stack on P0.


\section nfc_sequence NFC valid sequences

  - NFC Service Registration sequence
<p><tt>
\code{.unparsed}
  =============[P1]================       <=IPC=>        =====[P0]=====
  NfcSetRecvTask(RxTaskHdl)->
                        [RxTaskHdl]                          [NFC]
                             |                                 |
  NfcSendCtrlRegisterReqPrim()                                 |
                             |--NFC_CTRL_REGISTER_REQ--------->|
                             |<-NFC_CTRL_READY_IND-------------|
                            [-]                               [-]
\endcode
</tt></p>

  - Configuration or re-configuration sequence
<p><tt>
\code{.unparsed}
  =============[P1]================       <=IPC=>        =====[P0]=====
                          [RxTaskHdl]                        [NFC]
                             |                                 |
  NfcSendCtrlConfigReqPrim(TT2/NONE)                           |
                             |--NFC_CTRL_CONFIG_REQ----------->|
                             |<-NFC_CTRL_CONFIG_CNF------------|
                            [-]                               [-]
\endcode
</tt></p>

  - NFC HO Tag Emulation - BT Static Handover sequence
<p><tt>
\code{.unparsed}
  =============[P1]================       <=IPC=>        =====[P0]=====
                          [RxTaskHdl]                        [NFC]
                             |                                 |
  NfcSendCtrlConfigReqPrim(TT2)  
                             |--NFC_CTRL_CONFIG_REQ(TT2)------>|
                             |<-NFC_CTRL_CONFIG_CNF------------|
  NfcSendTagWriteUidReqPrim(0x00aabbccddeeff)  00=Generic      |
                             |--NFC_TAG_WRITE_UID_REQ--------->|
                             |<-NFC_TAG_WRITE_UID_CNF----------|
  NfcSendTagWriteChCarriersReqPrim(...)                        |
                             |--NFC_TAG_WRITE_CH_CARRIERS_REQ->|
                             |         (BT carrier)            |
                             |<-NFC_TAG_WRITE_CH_CARRIERS_CNF--|
                            [-]                               [-]
\endcode
</tt></p>

  - NFC HO Tag Emulation - Custom Application (e.g. URL or App link) sequence
<p><tt>
\code{.unparsed}
  =============[P1]===================== <=IPC=>  =====[P0]======
                          [RxTaskHdl]                        [NFC]
                             |                                 |
  NfcSendCtrlConfigReqPrim(TT2)                                |
                             |--NFC_CTRL_CONFIG_REQ(TT2)------>|
                             |<-NFC_CTRL_CONFIG_CNF------------|
  NfcSendTagWriteUidReqPrim(0x00aabbccddeeff)  00=Generic      |
                             |--NFC_TAG_WRITE_UID_REQ--------->|
                             |<-NFC_TAG_WRITE_UID_CNF----------|
  NfcSendTagWriteNdefReqPrim(...)                              |
                             |--NFC_TAG_WRITE_NDEF_REQ-------->|
                                      (NDEF Message)
                             |<-NFC_TAG_WRITE_NDEF_CNF---------|
                            [-]                               [-]
\endcode
</tt></p>

  - NFC HO Tag Emulation - DeRegistration sequence
<p><tt>
\code{.unparsed}
  =============[P1]===================== <=IPC=>  =====[P0]======
                          [RxTaskHdl]                        [NFC]
                             |                                 |
  NfcSendCtrlConfigReqPrim(NONE)                               |
                             |--NFC_CTRL_CONFIG_REQ(NONE)----->|
                             |<-NFC_CTRL_CONFIG_CNF------------|
                            [-]                               [-]
  No more NFC indications will be sent.
\endcode
</tt></p>

  - Wake up on NFC sequence \n
  NOTE: NFC_CTRL_CARRIER_ON_IND, NFC_CTRL_CARRIER_LOSS_IND signals are
        filtered after the initial Rx signal.  The filter is hardcoded
        to 500ms.
<p><tt>
\code{.unparsed}
  =============[P1]===================== <=IPC=>  =====[P0]======
                          [RxTaskHdl]                        [NFC]
                             |                                 |
                             |                                 |  USER BRING THE
                             |<-NFC_CTRL_CARRIER_ON_IND--------|  NFC DEVICE TOGETHER
                             |<-NFC_CTRL_SELECTED_IND(TT2)-----|
                             |<-NFC_TAG_READ_STARTED_IND-------|
                             |                                 |
                             |<-NFC_CTRL_CARRIER_LOSS_IND------|  USER MOVES AWAY THE 
                             |                                 |  NFC DEVICES
                            [-]                               [-]
\endcode
</tt></p>
@{
*/
#ifndef NFC_API_H
#define NFC_API_H

/* PROJECT INCLUDES **********************************************************/
#include "message.h"
#include "nfc/nfc_prim.h"

/* PUBLIC MACRO DEFINITIONS **************************************************/
/*****************************************************************************/
/*!
    @brief This macro wraps the NFC library call with the one contained in NFC
    shared code
 */
#define NfcUnpackCarriers(X, Y, Z) nfc_stream_read_carriers(X, Y, Z)


/* PUBLIC FUNCTION DECLARATIONS **********************************************/

/*****************************************************************************/
/*!
    @brief Sets the NFC Recv Task

    @param nfcTask Data passed to the NFC Recv task. 

    The nfcTask task will be called for every NFC message received from the 
    NFC subsystem located on P0.

    nfcTask is usually set by default to NFC_CL task as the NFC API messages 
    are handled by default in NFC_CL task.

    @warning if NFC_CL task is overridden by the main task then 
@#define     NFC_MESSAGE_BASE                     0x7E00
    SHOULD NOT be use by other base.
*/
extern void NfcSetRecvTask(TaskData *nfcTask);

/*****************************************************************************/
/*!
    @brief Tests if the message is a valid NFC rx message from P0.  This 
    function is used for debugging.

    @param id is of the received message 
    @param msg body of the message.

    @return TRUE if it is a valid NFC messge from P0, FALSE otherwise
*/
extern bool NfcMessageValid(MessageId id, Message msg);

/*****************************************************************************/
/*!
    @brief Sends a NFC_CTRL_REGISTER_REQ primitive to the NFC subsystem.

    After this message has been sent the NFC task handler will be able to receive
    NFC messages according to its configuration.
    For instance:
    - config
        -# NONE only a single message (e.g. NFC_CTRL_READY_IND) can be received.
        -# TT2 NFC_CTRL_CARRIER_ON_IND, NFC_CTRL_CARRIER_LOSS_IND ...
*/
extern void NfcSendCtrlRegisterReqPrim(void);

/*****************************************************************************/
/*!
    @brief Sends a NFC_CTRL_RESET_REQ primitive to the NFC subsystem.

    The NFC Hardware will be in disabled state as a result of this call. To 
    enable NFC (see NfcSendConfigCtrlReqPrim)
*/
extern void NfcSendCtrlResetReqPrim(void);

/*****************************************************************************/
/*!
    @brief sends a NFC_CTRL_CONFIG_REQ primitive to the NFC subsystem.

    @param nfc_mode
        - NFC_VM_NONE NFC hardware is disabled (same as a ctrl_reset_req)
        - NFC_VM_P2P NFC hardware is configured in Peer to Peer mode (NOT Available
          in this release)
        - NFC_VM_TT2 NFC hardware is configured as Tag Type 2
    @param ch_service NOT Available in this release
    @param snep_service NOT Available in this release

*/
extern void NfcSendCtrlConfigReqPrim(NFC_VM_MODE nfc_mode,
                                     NFC_VM_LLCP_SERVICE_TYPE ch_service,
                                     NFC_VM_LLCP_SERVICE_TYPE snep_service);

/*****************************************************************************/
/*!
    @brief Sends a NFC_TAG_READ_UID_REQ primitive to the NFC subsystem.
*/
extern void NfcSendTagReadUidReqPrim(void);

/*****************************************************************************/
/*!
    @brief Sends a NFC_TAG_WRITE_UID_REQ primitive to the NFC subsystem.

    @param nfcid pointer the nfc uid to initialise the tag
    @param nfcid_size: 4, 7 or 10
*/
extern void NfcSendTagWriteUidReqPrim(const uint8 *nfcid, uint8 nfcid_size);

/*****************************************************************************/
/*!
    @brief Sends a NFC_TAG_WRITE_NDEF_REQ primitive to the NFC subsystem.

    @param ndef pointer to the NDEF message to store on the TAG
    @param ndef_size Size in byte of "ndef". It must be <= TT2_MAX_NDEF
*/
extern void NfcSendTagWriteNdefReqPrim(const uint8 *ndef, uint8 ndef_size);

/*****************************************************************************/
/*!
    @brief Sends a NFC_TAG_READ_NDEF_REQ primitive to the NFC subsystem.
*/
extern void NfcSendTagReadNdefReqPrim(void);

/*****************************************************************************/
/*!
    @brief Sends a NFC_TAG_WRITE_CH_CARRIERS_REQ primitive to the NFC subsystem.

    @param n_carriers - Number of carriers to be written to TAG for NFC Channel
    Handover.
    @param carriers - array of pointers to NFC carrier objects
*/
extern void NfcSendTagWriteChCarriersReqPrim(uint8 n_carriers,
                                             NFC_CH_CARRIER **carriers);

/*****************************************************************************/
/*!
    @brief flush the data in the sink and send it to the NFC Channel
    Handover module in the NFC subsystem.
 
    @param sink - sink ready to be flushed
    @param msg_len - size of data to be flushed
    @param handover_type - the handover record type to be sent
 
    @return TRUE if the operation is successful
*/
extern bool NfcFlushCHData(Sink sink, uint16 msg_len,
                           NFC_STREAM_HANDOVER_TYPE handover_type);

/** @}*/

#endif /* NFC_API_H */
