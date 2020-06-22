/****************************************************************************
Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
    */

/*!
\ingroup nfc_cl

\brief Implementation of the NFC Connection Library

@{
*/

/** @}*/
#include <vm.h>
#include <ps.h>
#include <stdio.h>
#include <stdlib.h>
#include <sink.h>
#include <source.h>
#include <stream.h>

#if defined(HYDRACORE)
#include "nfc_cl.h" /* itself first */
#include "nfc_api.h"
#include "nfc_tool_stream.h"

#include "hydra_log.h"
#include "hydra_macros.h"
#include "macros.h"
#include "panic.h"

#define NFC_ASSERT_PRECOND(assertion) \
    ((assertion)? (void)0 : Panic())
#endif /* HYDRACORE */

#if defined(HYDRACORE)
/* PRIVATE MACRO DEFINITIONS *************************************************/
#ifdef DEBUG_NFC_CL
#define NFC_CL_DEBUG(x) {printf x;}
#else
#define NFC_CL_DEBUG(x)
#endif

#define NFC_CL_E "NFC_CL:**ERR: "
#define NFC_CL_W "NFC_CL:**WARN: "
#define NFC_CL_I "NFC_CL:I: "
#define NFC_CL_D "NFC_CL:D: "

/* NFC_CL_NOF_CARRIER

   NFC_CL_NOF_CARRIER defines the number of carriers currently supported by the
   NFC Connection Library. Currently, only the Bluetooth carrier is supported
 */
#define NFC_CL_NOF_CARRIER (1)

#define BDADDR_SIZE (6)

#define LE_ADDR_SIZE (7)

/*! EIR Header
    - Length (size=1) set to value length + type length=1
    - Type   (size=1) set to the eir type

<p><tt>
\code{.unparsed}

  Example:

  Offset Content       Length Explanation
  0      0x04            1    len=3+1
  1      0x0d            1    type: class of device
  2      0x14 0x04 0x20  3    value

\endcode
</tt></p>
*/
#define EIR_HDR_SIZE (2)
#define EIR_HDR_TYPE_SIZE (1)

/*!
 * Complete List of 16-bit Service Class UUIDs
 *
 * Example:
 * - len=0x0b
 * - type=0x03
 * - value=0x0b 0x11 0x0c 0x11 0x0d 0x11 0x0e 0x11 0x0f 0x11
 */
#define COMPLETE_LIST_16_BIT_SC     (0x03)

/*!
 * Complete Local Name
 *
 * Example:
 * - len=0x14
 * - type=0x09
 * - value="csra68100-Static-Hs"
 * 'c' , 's' , 'r' , 'a' , '6' , '8' , '1' , '0' ,
 * '0' , '-' , 'S' , 't' , 'a' , 't' , 'i' , 'c' ,
 * '-' , 'H' , 's' ,
 */
#define COMPLETE_LOCAL_NAME         (0x09)

/*!
 * Class of Device
 *
 * Example:
 * - len=0x04
 * - type=0x0d
 * - value=0x14 0x04 0x20
 * -# 0x20: Service Class=Audio:
 * -# 0x04: Major Device Class=Audio/Video
 * -# 0x14: Minor Device Class=HiFi Audio Device
 */
#define CLASS_OF_DEVICE             (0x0d)

/*!
 * 16B Simple Pairing Hash C
 *
 * Example:
 * - len=0x11
 * - type=0x0e
 * - value=01:02:03:04:05:06:07:08:09:10:11:12:13:14:15:16 */
#define SIMPLE_PAIRING_HASH_C       (0x0e)

/*!
 * 16B Simple Pairing Randomizer R
 *
 * Example:
 * - len=0x11
 * - type=0x0f
 * - value=01:02:03:04:05:06:07:08:09:10:11:12:13:14:15:16 */
#define SIMPLE_PAIRING_RANDOMIZER_R (0x0F)

/*!
 *  LE Bluetooth Device Address data type. Bluetooth Device
 *
 * Example:
 *  - Address: 04:68:00:5b:02:00  */
#define LE_BLUETOOTH_DEVICE_ADDRESS (0x1B)

/*!
 * LE Role data type.
 */
#define LE_ROLE_DATA_TYPE           (0x1C)

#define LE_ROLE_INVALID             (0xFF)
#define LE_ROLE_SIZE                (1)
#define LE_ROLE_PERIPHAL_MASK       (0x01)
#define IS_VALID_LE_ROLE(role) (LE_ROLE_INVALID != (role))

/*!
 * LE Role data type.
 *
 * Example:
 * - len=0x03
 * - type=0x19
 * - value=0x00 0x80 Appearance: Generic Computer
 */
#define APPEARANCE_DATA_TYPE        (0x19)

#define SECURITY_MANAGER_TK_VALUE   (0x10)

/*
#define EIR_TYPE_FLAGS              (0x01)
#define INCOMPLETE_LIST_16_BIT_SC   (0x02)
#define INCOMPLETE_LIST_32_BIT_SC   (0x04)
#define COMPLETE_LIST_32_BIT_SC     (0x05)
#define INCOMPLETE_LIST_128_BIT_SC  (0x06)
#define COMPLETE_LIST_128_BIT_SC    (0x07)
#define SHORTENED_LOCAL_NAME        (0x08)
#define SECURITY_MANAGER_OOB_FLAGS  (0x11)
#define SLAVE_CONNECTION_INTERVAL_RANGE (0x12)*/

/*!
 * CONFIG_CH_ENABLED
 *
 * Check if Channel Handover is enabled in input configuration */
#define CONFIG_CH_ENABLED(X) (X.mode == NFC_VM_P2P && \
                              X.ch_service != NFC_VM_LLCP_NONE)

/* PRIVATE TYPE DEFINITIONS **********************************************/
typedef enum nfc_cl_state_enum
{
    NFC_CL_ST_NULL,
    NFC_CL_ST_REGISTERING,
    NFC_CL_ST_REGISTERED,
    NFC_CL_ST_CONFIGURING,
    NFC_CL_ST_CONFIGURED
} NFC_CL_STATE;

/*!
    @brief NFC Event Handler function prototype definition
*/
typedef void (*PFN_NFC_EVT_HDL)(Task task, const NFC_PRIM *nfc_msg);

typedef struct nfc_cl_bt_car_struct
{
    bdaddr *local_bdaddr;

    uint16 size_local_name;
    uint8 *local_name;

    uint16 size_complete_list_16bit_sc;
    uint8 *complete_list_16bit_sc;

    uint8 *class_of_device;

    uint8 *simple_pairing_hash_c;

    uint8 *simple_pairing_randomizer_r;
} NFC_CL_BT_CAR;

typedef struct nfc_cl_le_car_struct
{
    bdaddr *le_local_addr; /* Mandatory */

    uint8 le_role; /* Mandatory */

    uint8 *security_manager_value; /* Optional */

    uint8 *le_appearance; /* Optional */

    uint16 size_le_local_name;
    uint8 *le_local_name; /* Mandatory */

    /* uint8 le_flags; */
} NFC_CL_LE_CAR;

typedef struct nfc_cl_data_struct
{
    Task nfcClientRecvTask;

    PFN_NFC_EVT_HDL carrier_on_ind_hdl;        /*!< Optional handler for NFC_CTRL_CARRIER_ON_IND, if NULL ignore message */
    PFN_NFC_EVT_HDL carrier_loss_ind_hdl;      /*!< Optional handler for NFC NFC_CTRL_CARRIER_LOSS_IND, if NULL ignore message */
    PFN_NFC_EVT_HDL selected_ind_hdl;          /*!< Optional handler for NFC_CTRL_SELECTED_IND, if NULL ignore message */

    TaskData nfcClTaskData;
    TaskData nfcClStreamData;

    NFC_CTRL_CONFIG current_nfc_config;
    NFC_CTRL_CONFIG requested_nfc_config;

    NFC_CL_BT_CAR bt_car;
    NFC_CL_LE_CAR le_car;

    Sink nfc_ch_sink;

} NFC_CL_DATA;


/* PRIVATE VARIABLE DEFINITIONS **********************************************/
/* NFC Connection Library internal data. This is initialised to zero
 * automatically
 */
static NFC_CL_DATA nfcClData;

static NFC_CL_STATE nfc_cl_state;

/* PRIVATE FUNCTION DECLARATION **********************************************/

/*****************************************************************************/
/*!
    @brief Set NFC Connection Library internal state

    @param new_nfc_cl_state new state to be set
*/
static void NfcClSetState(NFC_CL_STATE new_nfc_cl_state);


/*****************************************************************************/
/*!
    @brief return NFC Connection Library internal FSM state

    @return Library internal FSM state
*/
static NFC_CL_STATE NfcClGetState(void);

/*****************************************************************************/
/*!
    @brief sends NFC_CL_INIT_CNF to application

    @param task destination task for message
    @param nfc_cl_status procedure result status
*/
static void NfcClUpdateConfigSendCnf(Task task, NFC_CL_STATUS nfc_cl_status);

/*****************************************************************************/
/*!
    @brief sends NFC_CL_WRITE_CH_CARRIERS_CNF to application

    @param task destination task for message
    @param status procedure result status
*/
static void NfcClDefaultWriteChCarriersCnfHdl(Task task,
                                              NFC_VM_STATUS status);

/*****************************************************************************/
/*!
    @brief sends NFC_CL_TAG_READ_STARTED_IND to application

    @param task destination task for message
    @param nfc_msg message received from NFC stack
*/
static void NfcClDefaultReadStartedIndHdl(Task task, const NFC_PRIM *nfc_msg);

/*****************************************************************************/
/*!
    @brief sends NFC_CL_CARRIER_ON_IND to application

    @param task destination task for message
    @param nfc_msg message received from NFC stack
*/
static void NfcClOptCarrierOnIndHdl(Task task, const NFC_PRIM *nfc_msg);

/*****************************************************************************/
/*!
    @brief sends NFC_CL_CARRIER_LOSS_IND to application

    @param task destination task for message
    @param nfc_msg message received from NFC stack
*/
static void NfcClOptCarrierLossIndHdl(Task task, const NFC_PRIM *nfc_msg);

/*****************************************************************************/
/*!
    @brief sends NFC_CL_SELECTED_IND to application

    @param task destination task for message
    @param nfc_msg message received from NFC stack
*/
static void NfcClOptSelectedIndHdl(Task task, const NFC_PRIM *nfc_msg);

/*****************************************************************************/
/*!
    @brief sends NFC_CL_HANDOVER_COMPLETE_IND to application

    @param task destination task for message
    @param nfc_msg message received from NFC stack
*/
static void NfcClOptSelectedIndHdl(Task task, const NFC_PRIM *nfc_msg);

/*****************************************************************************/
/*!
    @brief sends NFC_CL_HANDOVER_COMPLETE_IND to application

    @param task destination task for message
    @param nfc_msg message received from NFC stack
*/
static void NfcClSendHandoverCompletedInd(Task task, const NFC_PRIM *nfc_msg);


/*****************************************************************************/
/*!
    @brief sends NFC_CL_HANDOVER_CARRIER_IND to application

    @param task destination task for message
    @param status procedure result
    @param carrier_ind received carrier object
*/
static void NfcClSendHandoverCarrierInd(Task task,
                                        NFC_VM_STATUS status,
                                        NFC_CL_CH_CARRIER_IND carrier_ind);

/*****************************************************************************/
/*!
    @brief returns TRUE if the BT data required to set the tag is valid.

    @return TRUE if valid, FALSE otherwise
*/
static bool NfcClEncIsBtCarDataValid(void);

/*****************************************************************************/
/*!
    @brief calculate Bluetooth OOB object size

    @return OOB object size
*/
static uint16 NfcClEncBtCarCalculateOobSize(void);

/*****************************************************************************/
/*!
    @brief returns TRUE if the LE data required to set the tag is valid.

    @return TRUE if valid, FALSE otherwise
*/
static bool NfcClEncIsLeCarDataValid(void);

/*****************************************************************************/
/*!
    @brief calculate LE OOB object size

    @return OOB object size
*/
static uint16 NfcClEncLeCarCalculateOobSize(void);

/*****************************************************************************/
/*!
    @brief handles NFC_CTRL_READY_IND_ID message received from NFC stack.

    @param nfc_msg pointer to the received NFC message
*/
static void NfcClReadyIndHdl(const NFC_PRIM *nfc_msg);

/*****************************************************************************/
/*!
    @brief handles NFC_CTRL_CONFIG_CNF_ID message received from NFC stack.

    @param nfc_msg pointer to the received NFC message
*/
static void NfcClConfigCnfHdl(const NFC_PRIM *nfc_msg);

/*****************************************************************************/
/*!
    @brief handles NFC messages received from NFC stack.

    @param thisTask client data passed to NfcSetRecvTask call
    @param id received message ID
    @param msg pointer to the received NFC message
*/
static void NfcClRecvTask(Task thisTask, MessageId id, Message msg);

/*****************************************************************************/
/*!
    @brief handles NFC TT2 messages received from NFC stack.

    @param thisTask NOT USED (Client data passed to NfcSetRecvTask call)
    @param id NOT USED
    @param msg pointer to the received NFC message
*/
static void NfcClTagHsRecvTask(Task thisTask, MessageId id, Message msg);

/*****************************************************************************/
/*!
    @brief handles NFC Channel Handover messages received from NFC stack.

   @param thisTask  NOT USED (Client data passed to NfcSetRecvTask call)
   @param id received message ID
   @param msg pointer to the received NFC message
*/
static void NfcClCHRecvTask(Task thisTask, MessageId id, Message msg);

/* PRIVATE BT ENCODING FUNCTION DEFINITIONS **********************************/
/*****************************************************************************/
/*!
    @brief initialise Bluetooth service UUID list
*/
static void NfcClEncBtCarInitCompleteList16BitSc(void);

/*****************************************************************************/
/*!
    @brief initialise Bluetooth local name

*/
static void NfcClEncBtCarInitLocalName(void);

/*****************************************************************************/
/*!
    @brief initialise local Bluetooth class of device
*/
static void NfcClEncBtCarInitClassOfDevice(void);

/*****************************************************************************/
/*!
    @brief initialise local SimplePairingHashC
*/
static void NfcClEncBtCarInitSimplePairingHashC(void);

/*****************************************************************************/
/*!
    @brief initialise local SimplePairingRandomizerR
*/
static void NfcClEncBtCarInitSimplePairingRandomizerR(void);

/*****************************************************************************/
/*!
    @brief initialise local Bluetooth address
*/
static void NfcClEncBtCarInitLocalBdaddr(void);

/* PRIVATE LE ENCODING FUNCTION DEFINITIONS **********************************/
/*****************************************************************************/
/*!
    @brief initialise Bluetooth Low Energy role
*/
static void NfcClEncLeCarInitLeRole(void);

/*****************************************************************************/
/*!
    @brief initialise local Bluetooth Low Energy address
*/
static void NfcClEncLeCarInitLeLocalAddr(void);

/*****************************************************************************/
/*!
    @brief initialise Bluetooth Low Energy local name
*/
static void NfcClEncLeCarInitLeLocalName(void);

/*****************************************************************************/
/*!
    @brief initialise Bluetooth Low Energy appearance
*/
static void NfcClEncLeCarInitAppearance(void);

/*****************************************************************************/
/*!
    @brief initialise local SecurityManager
*/
static void NfcClEncLeCarInitLeSecurityManager(void);

/*!
    @brief NfcClMessageSend is a simple function call implementation in our
    development branch.

    @warning It must be replaced by a MACRO (i.e. call to MessageSend function)
    in the ADK branch.
*/
#define NfcClMessageSend(task, id, message) MessageSend(task, id, message)

/* NFC UID *******************************************************************/
static const uint8 NFC_UID_EX2[DOUBLE_NFCID] = { /* Size 4, 7 or 10*/
    0x00, /* Use UID0 = 0x00 --> Generic manufacturer */
    0x12,
    0x34,
    0x56, /* Don't use 0x88 for UID3 */
    0x70,
    0xab,
    0xba
};
/* To be replaced by NFC UID in MIB */
#define PSKEY_BDADDR   0x0001
#define LAP_MSW_OFF 0
#define LAP_LSW_OFF 1
#define UAP_OFF 2
#define NAP_OFF 3
#define BDADDR_WORD_SIZE (4U)
/*!
    @brief generates an Unique ID for NFC tag derived from the Bluetooth address
    stored in the pskeys.  When the BT address is successfully read, it is
    configured on the NFC TAG. Otherwise a default UID is used when the PSkeys
    is not set.
*/
static void NfcClSendTagWriteUidReqPrim(void)
{
    uint8 nfc_uid[DOUBLE_NFCID];
    uint16 *bd_addr_data = (uint16 *) PanicUnlessMalloc(BDADDR_WORD_SIZE*sizeof(uint16));

    if(BDADDR_WORD_SIZE == PsFullRetrieve(PSKEY_BDADDR, bd_addr_data, BDADDR_WORD_SIZE))
    {
        nfc_uid[0] = 0; /* Generic manufacturer */
        /* ### At the time of writing (i.e. 29/03/2017)
         * The UID could be anything as it is only used in the anticollision and not used after the
         * the TAG has been selected.
         *
         * In practice the NFC anticollision with multiple static TAGs does not work very well due to
         * the difference in coupling between the reader and tags.  They have to be the same size and
         * not aligned.
         * Most readers don't use the NCI_DISCOVERY_IND to identify multiple static TAGs at the time
         * writing so if an NFC credit card (or a static TAG) is left on top of the NFC antenna of
         * the speaker it is most likely that NFC WON'T WORK.
         *
         * So the same last 6UID bytes could be reused. In this instance the last 6Bytes will be set in
         * the same byte order as the BT address stored for a BT handover for debugging the handover. */
        nfc_uid[1] = (uint8)((bd_addr_data[NAP_OFF] >> 8) & 0xFF); /*lint --e{415} --e{416} */
        nfc_uid[2] = (uint8)((bd_addr_data[NAP_OFF]) & 0xFF); /*lint --e{415} --e{416} */
        nfc_uid[3] = (uint8)((bd_addr_data[UAP_OFF]) & 0xFF); /*lint --e{415} --e{416} */
        nfc_uid[4] = (uint8)((bd_addr_data[LAP_MSW_OFF]) & 0xFF); /*lint --e{415} --e{416} */
        nfc_uid[5] = (uint8)((bd_addr_data[LAP_LSW_OFF] >> 8) & 0xFF); /*lint --e{415} --e{416} */
        nfc_uid[6] = (uint8)((bd_addr_data[LAP_LSW_OFF]) & 0xFF); /*lint --e{415} --e{416} */
        if (CASCADE_TAG==nfc_uid[3])
        {
            nfc_uid[3] = 0;
        }
        NFC_CL_DEBUG((NFC_CL_I"Generate NFC UID 00 %4.4x %2.2x %4.4x %2.2x \n",
                      bd_addr_data[NAP_OFF],
                      bd_addr_data[UAP_OFF],
                      bd_addr_data[LAP_LSW_OFF],
                      bd_addr_data[LAP_MSW_OFF]));
        NfcSendTagWriteUidReqPrim(nfc_uid, sizeof(nfc_uid));
    }
    else
    {
        NFC_CL_DEBUG((NFC_CL_W"unable to generate NFC UID => use fixed UID\n"));
        NfcSendTagWriteUidReqPrim(NFC_UID_EX2, sizeof(NFC_UID_EX2));
    }
    free(bd_addr_data);
}

static void NfcClSetState(NFC_CL_STATE new_nfc_cl_state)
{
    NFC_CL_DEBUG((NFC_CL_I"NfcClTag move from state:%d to state:%d\n",
                  nfc_cl_state, new_nfc_cl_state));
    nfc_cl_state = new_nfc_cl_state;
}

static NFC_CL_STATE NfcClGetState(void)
{
    return nfc_cl_state;
}

static void NfcClUpdateConfigSendCnf(Task task,
                                     NFC_CL_STATUS nfc_cl_status)
{
    NFC_CL_PRIM *p_nfc_cl = PanicUnlessMalloc(sizeof(*p_nfc_cl));
    switch (nfc_cl_status)
    {
    case NFC_CL_READY: /* fallthrough */
    case NFC_CL_CONFIGURED:
        nfcClData.current_nfc_config = nfcClData.requested_nfc_config;
        break;
    default:
        break;
    }
    /* The VM_STATUS is not used for this primitive */
    p_nfc_cl->status =(NFC_CL_FAIL==nfc_cl_status) ? NFC_VM_FAIL : NFC_VM_SUCCESS;

    p_nfc_cl->m.config_cnf.nfc_cl_status = nfc_cl_status;
    p_nfc_cl->m.config_cnf.nfc_config = nfcClData.current_nfc_config; /* memcpy of the structure */
    NfcClMessageSend(task, NFC_CL_CONFIG_CNF_ID, p_nfc_cl);
}

static void NfcClDefaultWriteChCarriersCnfHdl(Task task, NFC_VM_STATUS status)
{
    NFC_CL_MSG_ID cl_msg_id = NFC_CL_WRITE_CH_CARRIERS_CNF_ID;
    NFC_CL_PRIM *p_nfc_cl = PanicUnlessMalloc(sizeof(*p_nfc_cl));

    p_nfc_cl->status = status;
    NfcClMessageSend(task, cl_msg_id, p_nfc_cl);
}

static void NfcClDefaultReadStartedIndHdl(Task task, const NFC_PRIM *nfc_msg)
{
    UNUSED(nfc_msg);
    NfcClMessageSend(task, NFC_CL_TAG_READ_STARTED_IND_ID, NULL);
}

static void NfcClOptCarrierOnIndHdl(Task task, const NFC_PRIM *nfc_msg)
{
    UNUSED(nfc_msg);
    NfcClMessageSend(task, NFC_CL_CARRIER_ON_IND_ID, NULL);
}

static void NfcClOptCarrierLossIndHdl(Task task, const NFC_PRIM *nfc_msg)
{
    UNUSED(nfc_msg);
    NfcClMessageSend(task, NFC_CL_CARRIER_LOSS_IND_ID, NULL);
}

static void NfcClOptSelectedIndHdl(Task task, const NFC_PRIM *nfc_msg)
{
    UNUSED(nfc_msg);
    NfcClMessageSend(task, NFC_CL_SELECTED_IND_ID, NULL);
}

static void NfcClSendHandoverCompletedInd(Task task, const NFC_PRIM *nfc_msg)
{
    NFC_CL_MSG_ID cl_msg_id = NFC_CL_HANDOVER_COMPLETE_IND_ID;
    NFC_CL_PRIM *p_nfc_cl = PanicUnlessMalloc(sizeof(*p_nfc_cl));

    p_nfc_cl->status = nfc_msg->m.ch_handover_complete_ind.status;
    NfcClMessageSend(task, cl_msg_id, p_nfc_cl);
}

static void NfcClSendHandoverCarrierInd(Task task,
                                        NFC_VM_STATUS status,
                                        NFC_CL_CH_CARRIER_IND carrier_ind)
{
    NFC_CL_MSG_ID cl_msg_id = NFC_CL_HANDOVER_CARRIER_IND_ID;
    NFC_CL_PRIM *p_nfc_cl = PanicUnlessMalloc(sizeof(*p_nfc_cl));

    p_nfc_cl->status = status;
    p_nfc_cl->m.ch_carrier_ind = carrier_ind;
    NfcClMessageSend(task, cl_msg_id, p_nfc_cl);
}

static bool NfcClEncIsBtCarDataValid(void)
{
    return (IS_VALID_PTR(nfcClData.bt_car.local_name) &&
            0 != nfcClData.bt_car.size_local_name &&
            IS_VALID_PTR(nfcClData.bt_car.local_bdaddr));
}

static bool NfcClEncIsLeCarDataValid(void)
{
    return (IS_VALID_PTR(nfcClData.le_car.le_local_name) &&
            0 != nfcClData.le_car.size_le_local_name &&
            IS_VALID_PTR(nfcClData.le_car.le_local_addr) &&
            IS_VALID_LE_ROLE(nfcClData.le_car.le_role));
}

static uint16 NfcClEncBtCarCalculateOobSize(void)
{
    unsigned int bt_oob_size;

    bt_oob_size = 2 + BDADDR_SIZE;

    bt_oob_size = bt_oob_size + EIR_HDR_SIZE + nfcClData.bt_car.size_local_name;

    if (IS_VALID_PTR(nfcClData.bt_car.simple_pairing_hash_c))
    {
        bt_oob_size = bt_oob_size + EIR_HDR_SIZE + SIMPLE_PAIRING_HASH_C_SIZE;
    }

    if (IS_VALID_PTR(nfcClData.bt_car.simple_pairing_randomizer_r))
    {
        bt_oob_size = bt_oob_size + EIR_HDR_SIZE + SIMPLE_PAIRING_RANDOMIZER_R_SIZE;
    }

    if (IS_VALID_PTR(nfcClData.bt_car.complete_list_16bit_sc) &&
        0 < nfcClData.bt_car.size_complete_list_16bit_sc)
    {
        bt_oob_size = bt_oob_size + EIR_HDR_SIZE + nfcClData.bt_car.size_complete_list_16bit_sc;
    }

    if (IS_VALID_PTR(nfcClData.bt_car.class_of_device))
    {
        bt_oob_size = bt_oob_size + EIR_HDR_SIZE + CLASS_OF_DEVICE_SIZE;
    }
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncBtCarCalculateOobSize:%d\n", bt_oob_size));
    return (uint16)bt_oob_size;
}

static uint16 NfcClEncLeCarCalculateOobSize(void)
{
    unsigned int le_oob_size;

    le_oob_size = LE_ADDR_SIZE + EIR_HDR_SIZE;

    if (IS_VALID_PTR(nfcClData.le_car.security_manager_value))
    {
        le_oob_size = le_oob_size + EIR_HDR_SIZE + LE_SECURITY_MANAGER_TK_SIZE;
    }

    if (IS_VALID_PTR(nfcClData.le_car.le_appearance))
    {
        le_oob_size = le_oob_size + EIR_HDR_SIZE + LE_APPEARANCE_SIZE;
    }

    le_oob_size = le_oob_size + EIR_HDR_SIZE + nfcClData.le_car.size_le_local_name;

    le_oob_size = le_oob_size + EIR_HDR_SIZE + LE_ROLE_SIZE;
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncLeCarCalculateOobSize:%d\n", le_oob_size));
    return (uint16)le_oob_size;
}

/** Initialise NFC Negotiated Channel Handover stream object and store received
 *  sink. Currently, the stream module needs a dummy claim after the sink
 *  creation. This could be removed for future versions of the firmware
 */
static bool NfcClCHSinkOpen(void)
{
    /* CH module configured. Retrieve sink and source */
    if (nfcClData.nfc_ch_sink == NULL)
    {
        nfcClData.nfc_ch_sink = StreamNfcSink();
        NFC_CL_DEBUG((NFC_CL_I"sink new %d\n", nfcClData.nfc_ch_sink));

        if (nfcClData.nfc_ch_sink == NULL)
        {
            NFC_CL_DEBUG((NFC_CL_W"unable to retrieve CH Sink\n"));
            /* Configuration unsuccessful */
            return FALSE;
        }
        else
        {
            /* Set receiver task */
            (void)MessageStreamTaskFromSink(nfcClData.nfc_ch_sink,
                                            &nfcClData.nfcClStreamData);
            /* Configuration successful */
            return TRUE;
        }
    }
    else
    {
        NFC_CL_DEBUG((NFC_CL_W"NfcClCHSinkOpen but sink already open\n"));
        return FALSE;
    }
}

/** Compare the current NFC stack configuration with the one requested from the
 *  application. Leave the configuration unchanged in case of a match or ask
 *  for a reconfiguration otherwise. In case TT2 configuration is maintained
 *  (e.g. after waking from sleep), the NFC Connection Library state is set as
 *  configured, as the Tag information is maintained
 */
static void NfcClReadyIndHdl(const NFC_PRIM *nfc_msg)
{
    nfcClData.current_nfc_config = nfc_msg->m.ready_ind.current_config;

    if (NFC_VM_SUCCESS != nfc_msg->m.ready_ind.status)
    {
        NFC_CL_DEBUG((NFC_CL_W"NfcClReadyIndHdl HW Disabled (status=%d)\n", nfc_msg->m.ready_ind.status));
        NfcClSetState(NFC_CL_ST_REGISTERED);
        NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                 NFC_CL_FAIL);
        return;
    }

    /* Already configured: this is the wake on NFC field case. */
    if (nfc_msg->m.ready_ind.current_config.mode == nfcClData.requested_nfc_config.mode)
    {
        switch (nfc_msg->m.ready_ind.current_config.mode)
        {
        case NFC_VM_TT2:
            if(TRUE==nfc_msg->m.ready_ind.is_tt2_empty)
            {
                /* This case is to catch the following scenario
                 --- REGISTER_REQ -->
                 <-- READY_IND ----
                 --- CONFIG_REQ(TT2) --->
                 Reset any time before the WRITE_NDEF is send to TT2
                 -- WRITE_NDEF_REQ -->
                */
                NFC_CL_DEBUG((NFC_CL_W"NfcClReadyIndHdl Empty TAG memory\n"));
                NfcClSetState(NFC_CL_ST_CONFIGURING);
                NfcClSendTagWriteUidReqPrim(); /* Resume failed configuration */
            }
            /* Nothing to do. */
            else
            {
                /* Tag already configured. Tell the application that Tag
                 * information is already stored
                 */
                NfcClSetState(NFC_CL_ST_CONFIGURED);
                NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                         NFC_CL_CONFIGURED);
            }
            break;
        case NFC_VM_P2P:
            {
                bool ch_configured = CONFIG_CH_ENABLED(nfcClData.requested_nfc_config) &&
                   (NfcClCHSinkOpen() == TRUE);
                /* Check if configuration is for Channel Handover */
                if (ch_configured) /* Everything is configured for CH */
                {
                    NfcClSetState(NFC_CL_ST_CONFIGURED);
                    /* P2P stack initialised correctly. Wait for carrier configuration */
                    NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                             NFC_CL_READY);
                }
                /* else if snep_configured -- possible extension */
                else
                {
                    NfcClSetState(NFC_CL_ST_REGISTERED);
                    /* CH module configuration failed */
                    NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                             NFC_CL_FAIL);
                }
            }
            break;
        case NFC_VM_NONE:/* fallthrough */
        default:
            NfcClSetState(NFC_CL_ST_REGISTERED);
            /* Requested NFC stack configuration is none */
            NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                     NFC_CL_READY);
            break;
        }
    }
    /* Configuration or reconfiguration case*/
    else
    {
        /* Another configuration was running previously so reconfigure as
         * per the requested configuration. */
        NfcClSetState(NFC_CL_ST_CONFIGURING);
        NfcSendCtrlConfigReqPrim(nfcClData.requested_nfc_config.mode,
                                 nfcClData.requested_nfc_config.ch_service,
                                 nfcClData.requested_nfc_config.snep_service);
    }
}

/** Check received configuration confirmation against current state. Set the
 *  NFC CL state as configured or registered depending on the requested
 *  configuration. Open the Sink for NFC Negotiated Channel Handover
 */
static void NfcClConfigCnfHdl(const NFC_PRIM *nfc_msg)
{
    NFC_CL_STATE cur_state = NfcClGetState();

    switch (cur_state)
    {
    case NFC_CL_ST_CONFIGURING:
        /* Check if configuration was successful */
        if (nfc_msg->m.config_cnf.status == NFC_VM_SUCCESS)
        {
            switch (nfcClData.requested_nfc_config.mode)
            {
            case NFC_VM_TT2:
                NfcClSendTagWriteUidReqPrim();
                break;
            case NFC_VM_P2P:
                {
                    bool ch_configured = CONFIG_CH_ENABLED(nfcClData.requested_nfc_config) &&
                       (NfcClCHSinkOpen() == TRUE);
                    /* Check if configuration is for Channel Handover */

                    /* NFC_CL_CONFIG_SEQ_CMPL_IND(Success)*/
                    if (ch_configured) /* Everything is configured for CH */
                    {
                        NfcClSetState(NFC_CL_ST_CONFIGURED);
                        NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                                 NFC_CL_READY);
                    } /* else if snep_configured -- possible extension */
                    else
                    {
                        NfcClSetState(NFC_CL_ST_REGISTERED);
                        NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                                 NFC_CL_FAIL);
                    }
                }
                break;
            default: /* NONE */
                NfcClSetState(NFC_CL_ST_REGISTERED);
                NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                         NFC_CL_READY);
                break;
            }
        }
        else
        {
            NfcClSetState(NFC_CL_ST_REGISTERED);
            NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                     NFC_CL_FAIL);
        }
        break;

    default:
        /* This is unexpected... */
        NFC_CL_DEBUG((NFC_CL_E"NfcClConfigCnfHdl called in state %d\n", cur_state));

        NfcClSetState(NFC_CL_ST_REGISTERED);

        /* Send error to application */
        NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                 NFC_CL_FAIL);
        NFC_ASSERT_PRECOND(0);
        break;
    }
}

/** Receive and process messages from NFC stack. TT2 and CH messages are
 *  forwarded to the respective message handler
 */
static void NfcClRecvTask(Task thisTask, MessageId id, Message msg)
{
    const NFC_PRIM *nfc_msg = (const NFC_PRIM *)msg;
    NFC_VM_MSG_ID msg_id;
    NFC_ASSERT_PRECOND(thisTask == &nfcClData.nfcClTaskData);
    NFC_ASSERT_PRECOND(NFC_MESSAGE_BASE == (MessageId)(id & NFC_MESSAGE_BASE_MASK));
    NFC_ASSERT_PRECOND(TRUE == NfcMessageValid(id, msg));

    msg_id = (NFC_VM_MSG_ID)id;
    UNUSED(msg_id);
    NFC_CL_DEBUG((NFC_CL_I"NfcClRecvTask (msg id=%x)\n", msg_id));
    switch (id)
    {
    case NFC_CTRL_READY_IND_ID:
        NFC_CL_DEBUG((NFC_CL_D"Rx NFC_CTRL_READY_IND in state:%d (mode:%d status=%d)\n",
                      NfcClGetState(), nfc_msg->m.ready_ind.current_config.mode, nfc_msg->m.ready_ind.status));
        NfcClReadyIndHdl(nfc_msg);
        break;
    case NFC_CTRL_CONFIG_CNF_ID:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_CTRL_CONFIG_CNF in state:%d (status=%d)\n",
                      NfcClGetState(), nfc_msg->m.config_cnf.status));
        NfcClConfigCnfHdl(nfc_msg);
        break;
    case NFC_CTRL_CARRIER_ON_IND_ID:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_CTRL_CARRIER_ON_IND in state:%d\n",
                      NfcClGetState()));
        /* Note: these may come before the configuration completed */
        if (IS_VALID_PTR(nfcClData.carrier_on_ind_hdl))
        {
            nfcClData.carrier_on_ind_hdl(nfcClData.nfcClientRecvTask, nfc_msg);
        }
        break;
    case NFC_CTRL_CARRIER_LOSS_IND_ID:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_CTRL_CARRIER_LOSS_IND in state:%d\n",
                      NfcClGetState()));
        /* Note: these may come before the configuration completed */
        if (IS_VALID_PTR(nfcClData.carrier_loss_ind_hdl))
        {
            nfcClData.carrier_loss_ind_hdl(nfcClData.nfcClientRecvTask, nfc_msg);
        }
        break;
    case NFC_CTRL_SELECTED_IND_ID:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_CTRL_SELECTED_IND in state:%d (service=%d)\n",
                      NfcClGetState(), nfc_msg->m.selected_ind.service_type));
        /* Note: these may come before the configuration completed */
        if (IS_VALID_PTR(nfcClData.selected_ind_hdl))
        {
            nfcClData.selected_ind_hdl(nfcClData.nfcClientRecvTask, nfc_msg);
        }
        break;
    case NFC_CTRL_RESET_CNF_ID:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_CTRL_RESET_CNF in state:%d (status=%d)\n",
                      NfcClGetState(), nfc_msg->m.reset_cnf.status));
        NfcClSetState(NFC_CL_ST_REGISTERED);
        break;
    case NFC_TAG_WRITE_UID_CNF_ID: /* fallthrough */
    case NFC_TAG_WRITE_CH_CARRIERS_CNF_ID: /* fallthrough */
    case NFC_TAG_READ_STARTED_IND_ID: /* fallthrough */
    case NFC_TAG_WRITE_NDEF_CNF_ID:
        NfcClTagHsRecvTask(thisTask, id, msg);
        break;
    case NFC_CH_MSG_HANDOVER_COMPLETE_IND: /* fallthrough */
    case NFC_CH_MSG_CARRIER_CONFIG_CNF:
        /* Call CH receiver task */
        NfcClCHRecvTask(thisTask, id, msg);
        break;
    default:
        NFC_CL_DEBUG((NFC_CL_W"ID 0x%x unhandled NFC CL msg in state:%d\n",
                      NfcClGetState(), msg_id));
        break;
    }
    /* NOTE: nfc_msg is freed automatically in the IPC framework */
}

static void NfcClTagHsRecvTask(Task thisTask, MessageId id, Message msg)
{
    const NFC_PRIM *nfc_msg = (const NFC_PRIM *)msg;
    NFC_VM_MSG_ID msg_id;

    UNUSED(thisTask);

    msg_id = (NFC_VM_MSG_ID)id;

    /* Be sure we are in TT2 configuration */
    NFC_ASSERT_PRECOND((NFC_VM_TT2 == nfcClData.current_nfc_config.mode) ||
                       (NFC_VM_TT2 == nfcClData.requested_nfc_config.mode && NFC_TAG_WRITE_UID_CNF_ID == msg_id));

    NFC_CL_DEBUG((NFC_CL_I"ID 0x%x NfcClTagHsRecvTask\n", msg_id));
    switch (msg_id)
    {
    case NFC_TAG_WRITE_UID_CNF_ID:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_TAG_WRITE_UID_CNF in state:%d (status=%d)\n",
                      NfcClGetState(), nfc_msg->m.write_uid_cnf.status));
        if (nfc_msg->m.write_uid_cnf.status == NFC_VM_SUCCESS)
        {
            /* NFC_CL_CONFIG_SEQ_CMPL_IND(Success)*/
            NfcClSetState(NFC_CL_ST_CONFIGURED);
            NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                     NFC_CL_READY);

        }
        else
        {
            /* NFC_CL_CONFIG_SEQ_CMPL_IND(FAIL)*/
            NfcClSetState(NFC_CL_ST_REGISTERED);
            NfcClUpdateConfigSendCnf(nfcClData.nfcClientRecvTask,
                                     NFC_CL_FAIL);
        }
        break;

    case NFC_TAG_WRITE_CH_CARRIERS_CNF_ID:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_TAG_WRITE_CH_CARRIERS_CNF in state:%d (status=%d)\n",
                      NfcClGetState(), nfc_msg->m.write_carriers_cnf.status));
        NfcClDefaultWriteChCarriersCnfHdl(nfcClData.nfcClientRecvTask,
                                          nfc_msg->m.write_carriers_cnf.status);
        break;
    case NFC_TAG_READ_STARTED_IND_ID:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_TAG_READ_STARTED_IND in state:%d\n",
                      NfcClGetState()));
        switch (NfcClGetState())
        {
        case NFC_CL_ST_CONFIGURED:
            NfcClDefaultReadStartedIndHdl(nfcClData.nfcClientRecvTask, nfc_msg);
            break;
        default:
            break;
        }
        break;
    case NFC_TAG_WRITE_NDEF_CNF_ID:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_TAG_WRITE_NDEF_CNF in state:%d (status=%d)\n",
                      NfcClGetState(), nfc_msg->m.write_ndef_cnf.status));
        break;
    default:
        NFC_ASSERT_PRECOND(0);
        break;
    }
}

static void NfcClCHRecvTask(Task thisTask, MessageId id, Message msg)
{
    const NFC_PRIM *nfc_msg = (const NFC_PRIM *)msg;

    UNUSED(thisTask);

    NFC_ASSERT_PRECOND(nfcClData.current_nfc_config.mode == NFC_VM_P2P &&
                       nfcClData.current_nfc_config.ch_service != NFC_VM_LLCP_NONE);

    NFC_CL_DEBUG((NFC_CL_I"ID 0x%x NfcClCHRecvTask\n", id));
    switch (id)
    {
    case NFC_CH_MSG_HANDOVER_COMPLETE_IND:
        NfcClSendHandoverCompletedInd(nfcClData.nfcClientRecvTask, nfc_msg);
        break;
    case NFC_CH_MSG_CARRIER_CONFIG_CNF:
        NFC_CL_DEBUG((NFC_CL_I"Rx NFC_CH_MSG_CARRIER_CONFIG_CNF in state:%d (status=%d)\n",
                      NfcClGetState(), nfc_msg->m.ch_carrier_config_cnf.status));
        NfcClDefaultWriteChCarriersCnfHdl(nfcClData.nfcClientRecvTask,
                                          nfc_msg->m.ch_carrier_config_cnf.status);
        break;
    default:
        NFC_ASSERT_PRECOND(0);
        break;
    }
}

/** Receive the messages over the stream interface. Multiple streams can be
 *  received in the same task as they can be identified by the source ID.
 *  For Negotiated Channel Handover, the content of the message is processed
 *  and the relative carrier object is sent to the application
 */
static void NfcClStreamHdlTask(Task thisTask, MessageId id, Message msg)
{
    Source nfc_source;
    uint16 msg_len;

    NFC_ASSERT_PRECOND(nfcClData.current_nfc_config.mode == NFC_VM_P2P);
    NFC_ASSERT_PRECOND(thisTask == &nfcClData.nfcClStreamData);

    switch (id)
    {
    case MESSAGE_MORE_DATA:
        NFC_CL_DEBUG((NFC_CL_I"got message more data\n"));

        nfc_source = ((const MessageMoreData *)msg)->source;

        if (nfc_source == NULL)
        {
            NFC_ASSERT_PRECOND(0);
            /*lint -e{527} assert is to be removed for production */
            return;
        }

        msg_len = SourceSize(nfc_source);
        {
            /* Map header */
            const NFC_STREAM_INF *msgHeader = SourceMapHeader(nfc_source);
            uint8 n_carriers;

            NFC_ASSERT_PRECOND(msgHeader != NULL);

            /* Check sender stream */
            if (StreamSinkFromSource(nfc_source) == nfcClData.nfc_ch_sink &&
                msgHeader->stream_id == nfc_stream_type_ch)
            {
                /* This is from the Channel Handover module */
                const NFC_CH_CARRIER **carrierArray =
                   NfcUnpackCarriers(SourceMap(nfc_source), msg_len,
                                     &n_carriers);

                if (carrierArray != NULL)
                {
                    /* Send indication to application */
                    NFC_CL_CH_CARRIER_IND ch_carrier_ind;

                    /* Allocate carrier memory and fill carrier array pointer */
                    ch_carrier_ind.carriers =
                       PanicUnlessMalloc(n_carriers * sizeof(carrierArray));
                    memcpy(ch_carrier_ind.carriers,
                           carrierArray,
                           n_carriers * sizeof(carrierArray));
                    /* Release original pointer array now */
                    free(carrierArray);

                    /* Set number of carriers and record type */
                    ch_carrier_ind.msg_len = msg_len;
                    ch_carrier_ind.n_carriers = n_carriers;
                    ch_carrier_ind.handover_record_type =
                       msgHeader->m.nfc_ch_info.handover_record_type;

                    /* Send message to application */
                    NfcClSendHandoverCarrierInd(nfcClData.nfcClientRecvTask,
                                                NFC_VM_SUCCESS,
                                                ch_carrier_ind);
                }
                else
                {
                    /* Something went wrong in carrier unpack */
                    NFC_CL_DEBUG((NFC_CL_W"received CH carrier data corrupted\n"));
                    SourceDrop(nfc_source, msg_len);
                }
            }
            else
            {
                NFC_CL_DEBUG((NFC_CL_W"stream source %d not recognised, message dropped\n",
                              nfc_source));
                SourceDrop(nfc_source, msg_len);
            }
        }
        break;
    default:
        NFC_CL_DEBUG((NFC_CL_W"NfcClStreamHdlTask: unhandled message %d received\n", id));
        break;
    }
}

/* BLUETOOTH CARRIER ENCODING PRIVATE FUNCTION DEFINITIONS *******************/
static void NfcClEncBtCarInitCompleteList16BitSc(void)
{
    free(nfcClData.bt_car.complete_list_16bit_sc);
    nfcClData.bt_car.complete_list_16bit_sc = NULL;
    nfcClData.bt_car.size_complete_list_16bit_sc = 0;
}

static void NfcClEncBtCarInitLocalName(void)
{
    free(nfcClData.bt_car.local_name);
    nfcClData.bt_car.local_name = NULL;
    nfcClData.bt_car.size_local_name = 0;
}

static void NfcClEncBtCarInitClassOfDevice(void)
{
    free(nfcClData.bt_car.class_of_device);
    nfcClData.bt_car.class_of_device = NULL;
}

static void NfcClEncBtCarInitSimplePairingHashC(void)
{
    free(nfcClData.bt_car.simple_pairing_hash_c);
    nfcClData.bt_car.simple_pairing_hash_c = NULL;
}

static void NfcClEncBtCarInitSimplePairingRandomizerR(void)
{
    free(nfcClData.bt_car.simple_pairing_randomizer_r);
    nfcClData.bt_car.simple_pairing_randomizer_r = NULL;
}

static void NfcClEncBtCarInitLocalBdaddr(void)
{
    free(nfcClData.bt_car.local_bdaddr);
    nfcClData.bt_car.local_bdaddr = NULL;
}

/* LE CARRIER ENCODING PRIVATE FUNCTION DEFINITIONS **************************/
static void NfcClEncLeCarInitLeRole(void)
{
    nfcClData.le_car.le_role = LE_ROLE_INVALID;
}

static void NfcClEncLeCarInitLeLocalAddr(void)
{
    free(nfcClData.le_car.le_local_addr);
    nfcClData.le_car.le_local_addr = NULL;
}

static void NfcClEncLeCarInitLeLocalName(void)
{
    free(nfcClData.le_car.le_local_name);
    nfcClData.le_car.le_local_name = NULL;
    nfcClData.le_car.size_le_local_name = 0;
}

static void NfcClEncLeCarInitAppearance(void)
{
    free(nfcClData.le_car.le_appearance);
    nfcClData.le_car.le_appearance = NULL;
}

static void NfcClEncLeCarInitLeSecurityManager(void)
{
    free(nfcClData.le_car.security_manager_value);
    nfcClData.le_car.security_manager_value = NULL;
}

/* BLUETOOTH CARRIER ENCODING PUBLIC FUNCTION DEFINITIONS ********************/
void NfcClEncBtCarResetData(void)
{
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncBtCarInitData\n"));
    NfcClEncBtCarInitLocalName();
    NfcClEncBtCarInitLocalBdaddr();
    NfcClEncBtCarInitCompleteList16BitSc();
    NfcClEncBtCarInitClassOfDevice();
    NfcClEncBtCarInitSimplePairingHashC();
    NfcClEncBtCarInitSimplePairingRandomizerR();
}

void NfcClEncBtCarClassOfDevice(const uint8 *new_class_of_device)
{
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncBtCarClassOfDevice\n"));
    NfcClEncBtCarInitClassOfDevice();
    if (IS_VALID_PTR(new_class_of_device))
    {
        nfcClData.bt_car.class_of_device = PanicUnlessMalloc(CLASS_OF_DEVICE_SIZE);
        memcpy(nfcClData.bt_car.class_of_device, new_class_of_device, CLASS_OF_DEVICE_SIZE);
    }
}

void NfcClEncBtCarBDAddr(const bdaddr *new_local_bdaddr)
{
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncBtCarBDAddr\n"));
    NfcClEncBtCarInitLocalBdaddr();
    if (IS_VALID_PTR(new_local_bdaddr))
    {
        nfcClData.bt_car.local_bdaddr = PanicUnlessMalloc(sizeof(*new_local_bdaddr));
        *nfcClData.bt_car.local_bdaddr = *new_local_bdaddr;
    }
}

void NfcClEncBtCarLocalName(const uint8 *new_local_name, uint8 new_size_local_name)
{
    NfcClEncBtCarInitLocalName();
    if (IS_VALID_PTR(new_local_name) && 0 < new_size_local_name)
    {
        nfcClData.bt_car.size_local_name = MIN(new_size_local_name, MAX_LOCAL_NAME_LEN);
        nfcClData.bt_car.local_name = PanicUnlessMalloc(nfcClData.bt_car.size_local_name);
        memcpy(nfcClData.bt_car.local_name, new_local_name, nfcClData.bt_car.size_local_name);
        NFC_CL_DEBUG((NFC_CL_I"NfcClEncBtCarLocalName:%d\n",
                      nfcClData.bt_car.size_local_name));
    }
}

void NfcClEncBtCarCompleteList16BitSc(const uint8 *new_complete_list_16bit_sc,
                                      uint8 new_size_complete_list_16bit_sc)
{
    NfcClEncBtCarInitCompleteList16BitSc();
    if (IS_VALID_PTR(new_complete_list_16bit_sc) && 0 < new_size_complete_list_16bit_sc)
    {
        nfcClData.bt_car.size_complete_list_16bit_sc = MIN(new_size_complete_list_16bit_sc, MAX_COMPLETE_LIST_16BIT_SC);
        nfcClData.bt_car.complete_list_16bit_sc = PanicUnlessMalloc(nfcClData.bt_car.size_complete_list_16bit_sc);
        memcpy(nfcClData.bt_car.complete_list_16bit_sc,
               new_complete_list_16bit_sc,
               nfcClData.bt_car.size_complete_list_16bit_sc);
        NFC_CL_DEBUG((NFC_CL_I"NfcClEncBtCarCompleteList16BitSc:%d\n",
                      nfcClData.bt_car.size_complete_list_16bit_sc));
    }
}

void NfcClEncBtCarSimplePairingHashC(const uint8 *new_simple_pairing_hash_c)
{
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncBtCarSimplePairingHashC\n"));
    NfcClEncBtCarInitSimplePairingHashC();
    if (IS_VALID_PTR(new_simple_pairing_hash_c))
    {
        nfcClData.bt_car.simple_pairing_hash_c = PanicUnlessMalloc(SIMPLE_PAIRING_HASH_C_SIZE);
        memcpy(nfcClData.bt_car.simple_pairing_hash_c, new_simple_pairing_hash_c, SIMPLE_PAIRING_HASH_C_SIZE);
    }
}

void NfcClEncBtCarSimplePairingRandomizerR(const uint8 *new_simple_pairing_randomizer_r)
{
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncBtCarSimplePairingRandomizerR\n"));
    NfcClEncBtCarInitSimplePairingRandomizerR();
    if (IS_VALID_PTR(new_simple_pairing_randomizer_r))
    {
        nfcClData.bt_car.simple_pairing_randomizer_r = PanicUnlessMalloc(SIMPLE_PAIRING_RANDOMIZER_R_SIZE);
        memcpy(nfcClData.bt_car.simple_pairing_randomizer_r, new_simple_pairing_randomizer_r, SIMPLE_PAIRING_RANDOMIZER_R_SIZE);
    }
}
/* LE CARRIER ENCODING PUBLIC FUNCTION DEFINITIONS ****************************/
void NfcClEncLeCarResetData(void)
{
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncLeCarResetData\n"));
    NfcClEncLeCarInitLeLocalName();
    NfcClEncLeCarInitLeLocalAddr();
    NfcClEncLeCarInitLeRole();
    NfcClEncLeCarInitAppearance();
    NfcClEncLeCarInitLeSecurityManager();
}

void NfcClEncLeCarRole(uint8 new_le_role)
{
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncLeCarRole\n"));
    NfcClEncLeCarInitLeRole();
    nfcClData.le_car.le_role = new_le_role;
}

void NfcClEncLeCarLeLocalAddr(const bdaddr *new_le_local_addr)
{
    NFC_CL_DEBUG((NFC_CL_I"NfcClEncLeCarLeLocalAddr\n"));
    NfcClEncLeCarInitLeLocalAddr();
    if (IS_VALID_PTR(new_le_local_addr))
    {
        nfcClData.le_car.le_local_addr = PanicUnlessMalloc(sizeof(*new_le_local_addr));
        *nfcClData.le_car.le_local_addr = *new_le_local_addr;
    }
}

void NfcClEncLeCarLeLocalName(const uint8 *new_le_local_name, uint8 new_size_le_local_name)
{
    NfcClEncLeCarInitLeLocalName();
    if (IS_VALID_PTR(new_le_local_name) && 0 < new_size_le_local_name)
    {
        nfcClData.le_car.size_le_local_name = MIN(new_size_le_local_name, MAX_LOCAL_NAME_LEN);
        nfcClData.le_car.le_local_name = PanicUnlessMalloc(nfcClData.le_car.size_le_local_name);
        memcpy(nfcClData.le_car.le_local_name, new_le_local_name, nfcClData.le_car.size_le_local_name);
        NFC_CL_DEBUG((NFC_CL_I"NfcClEncLeCarLeLocalName:%d\n",
                    nfcClData.le_car.size_le_local_name));
    }
}

void NfcClEncLeCarAppearance(const uint8 *new_le_appearance)
{
    NfcClEncLeCarInitLeLocalAddr();
    if (IS_VALID_PTR(new_le_appearance))
    {
        nfcClData.le_car.le_appearance = PanicUnlessMalloc(LE_APPEARANCE_SIZE);
        memcpy(nfcClData.le_car.le_appearance, new_le_appearance, LE_APPEARANCE_SIZE);
        NFC_CL_DEBUG((NFC_CL_I"NfcClEncLeCarAppearance\n"));
    }
}

void NfcClEncLeCarLeSecurityManager(const uint8 *new_le_security_manager_value)
{
    NfcClEncLeCarInitLeSecurityManager();
    if (IS_VALID_PTR(new_le_security_manager_value))
    {
        nfcClData.le_car.security_manager_value = PanicUnlessMalloc(LE_SECURITY_MANAGER_TK_SIZE);
        memcpy(nfcClData.le_car.security_manager_value, new_le_security_manager_value, LE_SECURITY_MANAGER_TK_SIZE);
        NFC_CL_DEBUG((NFC_CL_I"NfcClEncLeCarLeSecurityManager"));
    }
}

static uint8* NfcClBtCarEncodeOob(uint8 *bt_oob, uint16 bt_oob_size)
{
    NFC_ASSERT_PRECOND(IS_VALID_PTR(bt_oob));

    *bt_oob++ = (uint8)(bt_oob_size & 0xFF); /*lint --e{415} --e{416} */
    *bt_oob++ = 0; /*lint --e{415} --e{416} */
    /* Write the BD addr */
    *bt_oob++ = (uint8)((nfcClData.bt_car.local_bdaddr->lap) & 0xFF); /*lint --e{415} --e{416} */
    *bt_oob++ = (uint8)(((nfcClData.bt_car.local_bdaddr->lap) >> 8) & 0xFF); /*lint --e{415} --e{416} */
    *bt_oob++ = (uint8)(((nfcClData.bt_car.local_bdaddr->lap) >> 16) & 0xFF); /*lint --e{415} --e{416} */
    *bt_oob++ = (uint8)((nfcClData.bt_car.local_bdaddr->uap)); /*lint --e{415} --e{416} */
    *bt_oob++ = (uint8)((nfcClData.bt_car.local_bdaddr->nap) & 0xFF); /*lint --e{415} --e{416} */
    *bt_oob++ = (uint8)((nfcClData.bt_car.local_bdaddr->nap) >> 8); /*lint --e{415} --e{416} */

    /* Write the length */
    *bt_oob++ = (uint8)(nfcClData.bt_car.size_local_name + EIR_HDR_TYPE_SIZE); /*lint --e{415} --e{416} */

    /* Write the type */
    *bt_oob++ = (uint8)COMPLETE_LOCAL_NAME; /*lint --e{415} --e{416} */

    /* Write the device name */
    memcpy(/*lint --e{669} */bt_oob,
           nfcClData.bt_car.local_name,
           nfcClData.bt_car.size_local_name);
    bt_oob += nfcClData.bt_car.size_local_name; /*lint --e{415} --e{416} */

    if (IS_VALID_PTR(nfcClData.bt_car.class_of_device))
    {
        /* Write the length */
        *bt_oob++ = (uint8)(CLASS_OF_DEVICE_SIZE + EIR_HDR_TYPE_SIZE);
        /* Write the type */
        *bt_oob++ = (uint8)CLASS_OF_DEVICE;
        memcpy(/*lint --e{669} --e{662} */bt_oob, /* safe because of assert */
               nfcClData.bt_car.class_of_device,
               CLASS_OF_DEVICE_SIZE);
        bt_oob += CLASS_OF_DEVICE_SIZE; /*lint --e{415} --e{416} */
    }

    if (IS_VALID_PTR(nfcClData.bt_car.simple_pairing_hash_c))
    {
        /* Write the length */
        *bt_oob++ = (uint8)(SIMPLE_PAIRING_HASH_C_SIZE + EIR_HDR_TYPE_SIZE);
        /* Write the type */
        *bt_oob++ = (uint8)SIMPLE_PAIRING_HASH_C;
        memcpy(/*lint --e{669} --e{662} */bt_oob, /* safe because of assert */
               nfcClData.bt_car.simple_pairing_hash_c,
               SIMPLE_PAIRING_HASH_C_SIZE);
        bt_oob += SIMPLE_PAIRING_HASH_C_SIZE; /*lint --e{415} --e{416} */
    }

    if (IS_VALID_PTR(nfcClData.bt_car.simple_pairing_randomizer_r))
    {
        /* Write the length */
        *bt_oob++ = (uint8)(SIMPLE_PAIRING_RANDOMIZER_R_SIZE + EIR_HDR_TYPE_SIZE);
        /* Write the type */
        *bt_oob++ = (uint8)SIMPLE_PAIRING_RANDOMIZER_R;
        memcpy(/*lint --e{669} --e{662} */bt_oob, /* safe because of assert */
               nfcClData.bt_car.simple_pairing_randomizer_r,
               SIMPLE_PAIRING_RANDOMIZER_R_SIZE);
        bt_oob += SIMPLE_PAIRING_RANDOMIZER_R_SIZE; /*lint --e{415} --e{416} */
    }

    /* Must be after Class of device */
    if (IS_VALID_PTR(nfcClData.bt_car.complete_list_16bit_sc) &&
        0 < nfcClData.bt_car.size_complete_list_16bit_sc)
    {
        /* Write the length */
        *bt_oob++ = (uint8)(nfcClData.bt_car.size_complete_list_16bit_sc + EIR_HDR_TYPE_SIZE); /*lint --e{415} --e{416} */

        /* Write the type */
        *bt_oob++ = (uint8)COMPLETE_LIST_16_BIT_SC; /*lint --e{415} --e{416} */

        /* Write the list of 16bit UUID */
        memcpy(/*lint --e{669} --e{662} */bt_oob, /* safe because of assert */
               nfcClData.bt_car.complete_list_16bit_sc,
               nfcClData.bt_car.size_complete_list_16bit_sc);
        /* uncomment if more element are added */
        /*bt_oob += nfcClData.bt_car.size_complete_list_16bit_sc; */ /*lint --e{415} --e{416} */
    }
    return bt_oob;
}

static uint8* NfcClLeCarEncodeOob(uint8 *le_oob)
{
    NFC_ASSERT_PRECOND(IS_VALID_PTR(le_oob));

    /* Write the length */
    *le_oob++ = (uint8)(LE_ADDR_SIZE + EIR_HDR_TYPE_SIZE); /*lint --e{415} --e{416} */
    /* Write the type */
    *le_oob++ = (uint8)(LE_BLUETOOTH_DEVICE_ADDRESS); /*lint --e{415} --e{416} */
    /* Write the BD addr */
    *le_oob++ = (uint8)((nfcClData.le_car.le_local_addr->lap) & 0xFF); /*lint --e{415} --e{416} */
    *le_oob++ = (uint8)(((nfcClData.le_car.le_local_addr->lap) >> 8) & 0xFF); /*lint --e{415} --e{416} */
    *le_oob++ = (uint8)(((nfcClData.le_car.le_local_addr->lap) >> 16) & 0xFF); /*lint --e{415} --e{416} */
    *le_oob++ = (uint8)((nfcClData.le_car.le_local_addr->uap)); /*lint --e{415} --e{416} */
    *le_oob++ = (uint8)((nfcClData.le_car.le_local_addr->nap) & 0xFF); /*lint --e{415} --e{416} */
    *le_oob++ = (uint8)((nfcClData.le_car.le_local_addr->nap) >> 8); /*lint --e{415} --e{416} */
    *le_oob++ = (0x00==(LE_ROLE_PERIPHAL_MASK&nfcClData.le_car.le_role)) ? 0x01 : 0x00;

    /* Write the length */
    *le_oob++ = (uint8)(LE_ROLE_SIZE + EIR_HDR_TYPE_SIZE);
    /* Write the type */
    *le_oob++ = (uint8)(LE_ROLE_DATA_TYPE); /*lint --e{415} --e{416} */
    *le_oob++ = (uint8)(nfcClData.le_car.le_role); /*lint --e{415} --e{416} */

    if (IS_VALID_PTR(nfcClData.le_car.security_manager_value))
    {
        /* Write the length */
        *le_oob++ = (uint8)(LE_SECURITY_MANAGER_TK_SIZE + EIR_HDR_TYPE_SIZE);
        /* Write the type */
        *le_oob++ = (uint8)(SECURITY_MANAGER_TK_VALUE);
        memcpy(/*lint --e{669} --e{662} */le_oob, /* safe because of assert */
               nfcClData.le_car.security_manager_value,
               LE_SECURITY_MANAGER_TK_SIZE);
        le_oob += LE_SECURITY_MANAGER_TK_SIZE; /*lint --e{415} --e{416} */
    }

    if (IS_VALID_PTR(nfcClData.le_car.le_appearance))
    {
        /* Write the length */
        *le_oob++ = (uint8)(LE_APPEARANCE_SIZE + EIR_HDR_TYPE_SIZE);
        /* Write the type */
        *le_oob++ = (uint8)(APPEARANCE_DATA_TYPE);
        memcpy(/*lint --e{669} --e{662} */le_oob, /* safe because of assert */
               nfcClData.le_car.le_appearance,
               LE_APPEARANCE_SIZE);
        le_oob += LE_APPEARANCE_SIZE; /*lint --e{415} --e{416} */
    }

    /* Write the length */
    *le_oob++ = (uint8)(nfcClData.le_car.size_le_local_name + EIR_HDR_TYPE_SIZE); /*lint --e{415} --e{416} */
    /* Write the type */
    *le_oob++ = (uint8)(COMPLETE_LOCAL_NAME); /*lint --e{415} --e{416} */
    /* Write the device name */
    memcpy(/*lint --e{669} */le_oob,
           nfcClData.le_car.le_local_name,
           nfcClData.le_car.size_le_local_name);
    le_oob += nfcClData.le_car.size_le_local_name; /*lint --e{415} --e{416} */

    return le_oob;
}

/* NFC CL API FUNCTION DEFINITIONS *******************************************/
/*****************************************************************************/
/*!

  \section nfc_cl_fsm_overview NFC CL FSM overview

<p><tt>
\code{.unparsed}

  [Client]  <--->   [NFC_CL_FSM] <----->  [NFC]

  NFC CL FSM main events handling

                         STATES ->|   NULL  | REGISTERED | CONFIGURING | CONFIGURED |
                 [Client EVENTS]  ---------------------------------------------------
                    CONFIG_REQ_EV |    1    |     1      | Ignored     |     1      |
         WRITE_CH_CARRIERS_REQ_EV | Ignored | Ignored    | Ignored     |     2      |
                [Internal EVENTS] ---------------------------------------------------
           CONFIG_SEQ_CMPL_IND_EV |  Error  |   Error    |     3       |   Error    |
                    [NFC EVENTS]  ---------------------------------------------------
            NFC_CTRL_READY_IND_EV |    5    |      5     |     5       |     5      |
           NFC_CTRL_CONFIG_CNF_EV |  Error  |   Error    |     6       |   Error    |

  - (1) The CONFIG_REQ is only handled stable states like NULL, REGISTERED and
        CONFIGURED states.
  - (2) NfcClWriteChCarriersReq is only handled in CONFIGURED state.
  - (3) The CONFIG_SEQ_CMPL_IND_EV indicates the end of a configuration or a reconfiguration.
        The FSM moves to CONFIGURED for SUCCESS or REGISTERED in all the other cases
  - (5) NfcClReadyIndHdl
        if this is a new or a different configuration the FSM moves to CONFIGURING
        if this is a wake up from DORMANT the FSM moves to CONFIGURED
        else the FSM moves to REGISTERED in the remaining other cases.
  - (6) NfcClConfigCnfHdl
        The FSM remains in CONFIGURING if there is more to be done or if it is
        completed see CONFIG_SEQ_CMPL_IND_EV.

\endcode
</tt></p>
*/

/*****************************************************************************/
/*!
    @brief validate the WriteChCarriers request

    @param first_car first priority carrier (BT or BLE)
    @param second_car second priority carrier (BT, BLE or NONE)

    @return NFC_VM_SUCCESS if it is valid
*/
static NFC_VM_STATUS NfcClWriteChCarriersReqValidation(NFC_CL_CAR first_car,
                                                       NFC_CL_CAR second_car)
{
    /* Validate WriteChCarriersReq */
    /*if (NONE_CAR_ID == first_car)*/  /*OK Erase the carriers */
    if (NONE_CAR_ID != second_car)
    {
        NFC_CL_DEBUG((NFC_CL_W"NfcClWriteChCarriersReq(Ignored) Only one carrier supported\n"));
        return NFC_VM_UNSUPPORTED;
    }
    if ((BT_CAR_ID == first_car) &&
        (FALSE == NfcClEncIsBtCarDataValid()))
    {
        NFC_CL_DEBUG((NFC_CL_W"NfcClWriteChCarriersReq(Ignored) BtDataValid:%d\n",
                    NfcClEncIsBtCarDataValid()));
        return NFC_VM_WRONG_NDEF_FORMAT;
    }
    if ((LE_CAR_ID == first_car) &&
        (FALSE == NfcClEncIsLeCarDataValid()))
    {
        NFC_CL_DEBUG((NFC_CL_W"NfcClWriteChCarriersReq(Ignored) LeDataValid:%d\n",
                    NfcClEncIsLeCarDataValid()));
        return NFC_VM_WRONG_NDEF_FORMAT;
    }
    if (NFC_VM_TT2 == nfcClData.current_nfc_config.mode)
    {
        if ((BT_CAR_ID == first_car) &&
            (MAX_CARRIERS_SIZE < NfcClEncBtCarCalculateOobSize()))
        {
            NFC_CL_DEBUG((NFC_CL_W"NfcClWriteChCarriersReq(Ignored) Tag size too small (BT):%d\n",
                        NfcClEncBtCarCalculateOobSize()));
            return NFC_VM_WRONG_NDEF_FORMAT;

        }
        if ((LE_CAR_ID == first_car) &&
            (MAX_CARRIERS_SIZE < NfcClEncLeCarCalculateOobSize()))
        {
            NFC_CL_DEBUG((NFC_CL_W"NfcClWriteChCarriersReq(Ignored) Tag size too small (LE):%d\n",
                        NfcClEncLeCarCalculateOobSize()));
            return NFC_VM_WRONG_NDEF_FORMAT;
        }
    }
    return NFC_VM_SUCCESS;
}

/*****************************************************************************/
/*!
    @note Send carrier object to its destination in the NFC stack. The destination
    could be Tag module in case of Static Channel Handover or CH module in
    case of Negotiated Channel Handover. Build carrier object from received
    information.
*/
NFC_VM_STATUS NfcClWriteChCarriersReq(NFC_CL_CAR first_car,
                                      NFC_CL_CAR second_car)
{
    NFC_VM_STATUS vm_status;

    vm_status = NfcClWriteChCarriersReqValidation(first_car,second_car);
    if (NFC_VM_SUCCESS!=vm_status)
    {
        return vm_status;
    }
    switch (NfcClGetState())
    {
    case NFC_CL_ST_CONFIGURED:
        {
            uint8 *oob;
            uint16 oob_size = 0;
            uint16 stream_msg_len = 0;
            uint16 prev_sink_claimed;
            NFC_CH_CARRIER **carriers;

            NFC_CL_DEBUG((NFC_CL_I"NfcClWriteChCarriersReq(Updating)\n"));
            switch(first_car)
            {
            case BT_CAR_ID:
                oob_size = NfcClEncBtCarCalculateOobSize();
                break;
            case LE_CAR_ID:
                oob_size = NfcClEncLeCarCalculateOobSize();
                break;
            case NONE_CAR_ID:
                oob_size = 0;
                break;
            default:
                NFC_ASSERT_PRECOND(0);
                break;
            }
            /* Allocate carrier pointers array */
            carriers = PanicUnlessMalloc(sizeof(*carriers) * NFC_CL_NOF_CARRIER);
            memset(carriers, 0, sizeof(*carriers) * NFC_CL_NOF_CARRIER);
            if (CONFIG_CH_ENABLED(nfcClData.current_nfc_config))
            {
                /* Send data to NFC Negotiated Channel Handover. Calculate
                 * message length and retrieve data pointer
                 */
                stream_msg_len = (uint16)(oob_size +
                                          (uint16)sizeof(NFC_CH_CARRIER_HEADER) +
                                          NFC_STREAM_CONTENT_SIZE);

                NFC_ASSERT_PRECOND(nfcClData.nfc_ch_sink);

                prev_sink_claimed =
                        SinkClaim(nfcClData.nfc_ch_sink, stream_msg_len);
                /* Check if sink configuration is coherent and previously
                 * claimed size is not invalid
                 */
                if (nfcClData.nfc_ch_sink != NULL &&
                    prev_sink_claimed != 0xFFFF)
                {
                    NFC_STREAM_CONTENT *sinkCarrier = prev_sink_claimed +
                       (NFC_STREAM_CONTENT *)SinkMap(nfcClData.nfc_ch_sink);
                    sinkCarrier->n_carriers = NFC_CL_NOF_CARRIER;
                    /* Check received pointer */
                    if (sinkCarrier != NULL)
                    {
                        /* Get carrier object pointer */
                        carriers[0] =
                           (NFC_CH_CARRIER *)sinkCarrier->carrier_info;
                    }
                    else
                    {
                        NFC_CL_DEBUG((NFC_CL_W"NFC CH sink map failed\n"));
                        free(carriers);
                        return NFC_VM_FAIL;
                    }
                }
                else
                {
                    NFC_CL_DEBUG((NFC_CL_W"NFC CH sink claim failed\n"));
                    free(carriers);
                    return NFC_VM_FAIL;
                }
            }
            else
            {
                /* Send data to NFC TT2 Channel Handover. Allocate carrier data
                 * memory. This will contain both the carrier message header
                 * and the carrier information data. */
                carriers[0] = PanicUnlessMalloc(sizeof(NFC_CH_CARRIER_HEADER) +  oob_size);
            }
            NFC_ASSERT_PRECOND(carriers[0] != NULL);
            /* Fill state and control information */
            carriers[0]->carrier_header.cps = PS_ACTIVE;
            switch(first_car)
            {
            case BT_CAR_ID:
                carriers[0]->carrier_header.carrier_tech = BT_EP_OOB;
            CONVERT_FROM_UINT16(carriers[0]->carrier_header.data_length, oob_size);
                /* Assign carrier data pointer to oob and fill it. Write the
                 * payload length */
                oob = carriers[0]->data;
                oob = NfcClBtCarEncodeOob(oob, oob_size);
                break;
            case LE_CAR_ID:
                carriers[0]->carrier_header.carrier_tech = BT_LE_OOB;
                CONVERT_FROM_UINT16(carriers[0]->carrier_header.data_length, oob_size);
                /* Assign carrier data pointer to oob and fill it. Write the
                 * payload length */
                oob = carriers[0]->data;
                oob = NfcClLeCarEncodeOob(oob);
                break;
            case NONE_CAR_ID:
                carriers[0]->carrier_header.carrier_tech = 0;
                oob_size = 0;
                break;
            default:
                NFC_ASSERT_PRECOND(0);
                break;
            }
            NFC_CL_DEBUG((NFC_CL_I"Created carrier object with CPS %d, tech %d, len %d\n",
                          carriers[0]->carrier_header.cps,
                          carriers[0]->carrier_header.carrier_tech,
                          oob_size));

            if (CONFIG_CH_ENABLED(nfcClData.current_nfc_config))
            {
                /* Use Sink API to flush carrier data */
                (void)NfcFlushCHData(nfcClData.nfc_ch_sink, stream_msg_len,
                                     nfc_stream_handover_config);
                free(carriers);
            }
            else
            {
                /* Use Tag API to send carrier */
                NfcSendTagWriteChCarriersReqPrim(NFC_CL_NOF_CARRIER,
                                                 carriers);
                /* Note: carriers is freed on P0 for the TAG configuration */
            }

            switch(first_car)
            {
            case BT_CAR_ID:
                NfcClEncBtCarResetData();
                break;
            case LE_CAR_ID:
                NfcClEncLeCarResetData();
                break;
            case NONE_CAR_ID:
                break;
            default:
                NFC_ASSERT_PRECOND(0);
                break;
            }
            vm_status = NFC_VM_SUCCESS;
        }
        break;

    case NFC_CL_ST_NULL: /* fallthrough */
    case NFC_CL_ST_REGISTERING: /* fallthrough */
    case NFC_CL_ST_REGISTERED: /* fallthrough */
    case NFC_CL_ST_CONFIGURING: /* fallthrough */
    default:
        NFC_CL_DEBUG((NFC_CL_W"NfcClWriteChCarriersReq(Ignored) in state:%d\n",
                      NfcClGetState()));
        vm_status = NFC_VM_WRONG_STATE;
        break;
    }
    return vm_status;
}


/*****************************************************************************/
/*!
    @note Drop data of msg_len from the Negotiated Channel Handover stream and
    free carrier array memory
*/
NFC_VM_STATUS NfcClReleaseChCarriers(NFC_CL_CH_CARRIER_IND *ch_carriers)
{
    NFC_VM_STATUS vm_status;
    Source nfc_source;

    /* Check if NFC Negotiated Channel Handover is enabled and sink is
     * available
     */
    if (CONFIG_CH_ENABLED(nfcClData.current_nfc_config)
        && nfcClData.nfc_ch_sink != NULL && ch_carriers != NULL)
    {
        /* Retrieve NFC source from sink */
        nfc_source = StreamSourceFromSink(nfcClData.nfc_ch_sink);
        if (nfc_source != NULL)
        {
            SourceDrop(nfc_source, ch_carriers->msg_len);
            ch_carriers->msg_len = 0;
            /* Free allocated memory */
            free(ch_carriers->carriers);
            ch_carriers->carriers = NULL;
            vm_status = NFC_VM_SUCCESS;
        }
        else
        {
            NFC_CL_DEBUG((NFC_CL_W"NfcClReleaseChCarriers can't NFC CH Source\n"));
            vm_status = NFC_VM_FAIL;
        }
    }
    else
    {
        NFC_CL_DEBUG((NFC_CL_W"NfcClReleaseChCarriers can't release carriers for CH\n"));
        vm_status = NFC_VM_FAIL;
    }

    return vm_status;
}

static void NfcClConfigReqCommonHdl(NFC_CL_CONFIG_REQ *p_nfc_cl_config_req)
{
    NFC_ASSERT_PRECOND(IS_VALID_PTR(p_nfc_cl_config_req));

    nfcClData.nfcClientRecvTask = p_nfc_cl_config_req->nfcClientRecvTask;

    /** Reset the current NFC CL static configuration. If a
     *  configuration needs some data reset, check if it is enabled
     *  and perform the reset. This is executed before the reset
     *  command is sent to the NFC stack
     */
    if (CONFIG_CH_ENABLED(nfcClData.current_nfc_config))
    {
        /* Reset Negotiated Channel Handover configuration */
        if (SinkClose(nfcClData.nfc_ch_sink) != TRUE)
        {
            NFC_CL_DEBUG((NFC_CL_W"NfcClConfigReset error closing CH sink\n"));
        }
        nfcClData.nfc_ch_sink = NULL;
    }
    /* Add other configurations here... */
    /* Reset callback pointer for NFC_CTRL_CARRIER_ON_IND_ID,
     * NFC_CTRL_CARRIER_LOSS_IND_ID and NFC_CTRL_SELECTED_IND_ID
     */
    nfcClData.carrier_on_ind_hdl = NULL;
    nfcClData.carrier_loss_ind_hdl = NULL;
    nfcClData.selected_ind_hdl = NULL;
    /* Reset pending carrier data */
    NfcClEncBtCarResetData();

    /** Now configure the NFC CL static configuration. */
    if (p_nfc_cl_config_req->send_carrier_on_ind)
    {
        nfcClData.carrier_on_ind_hdl = NfcClOptCarrierOnIndHdl;
    }
    if (p_nfc_cl_config_req->send_carrier_loss_ind)
    {
        nfcClData.carrier_loss_ind_hdl = NfcClOptCarrierLossIndHdl;
    }
    if (p_nfc_cl_config_req->send_selected_ind)
    {
        nfcClData.selected_ind_hdl = NfcClOptSelectedIndHdl;
    }
    nfcClData.nfcClTaskData.handler = NfcClRecvTask;
    nfcClData.nfcClStreamData.handler = NfcClStreamHdlTask;
    nfcClData.requested_nfc_config = p_nfc_cl_config_req->nfc_config;
    NfcSetRecvTask(&nfcClData.nfcClTaskData);
}

/*****************************************************************************/
/*!
    @brief validate the requested configuration. Current supported
    configurations:

<p><tt>
\code{.unparsed}
    ----------------------------------------
    |  Mode  | Channel Handover |   SNEP   |
    ----------------------------------------
    |  NONE  |     Disabled     | Disabled |
    |   TT2  |     Disabled     | Disabled |
    |   P2P  |      Server      | Disabled |
    ----------------------------------------
\endcode
</tt></p>

    @param p_nfc_cl_config_req pointer on the Client initialisation structure.
    @return NFC_VM_SUCCESS if the request is valid.

*/
static NFC_VM_STATUS NfcClConfigReqValidation(NFC_CL_CONFIG_REQ *p_nfc_cl_config_req)
{
    if (!IS_VALID_PTR(p_nfc_cl_config_req))
    {
        return NFC_VM_WRONG_ARGS;
    }
    if (!(p_nfc_cl_config_req->nfc_config.mode == NFC_VM_NONE &&
          p_nfc_cl_config_req->nfc_config.ch_service == NFC_VM_LLCP_NONE &&
          p_nfc_cl_config_req->nfc_config.snep_service == NFC_VM_LLCP_NONE) &&

        !(p_nfc_cl_config_req->nfc_config.mode == NFC_VM_TT2 &&
          p_nfc_cl_config_req->nfc_config.ch_service == NFC_VM_LLCP_NONE &&
          p_nfc_cl_config_req->nfc_config.snep_service == NFC_VM_LLCP_NONE) &&

        !(p_nfc_cl_config_req->nfc_config.mode == NFC_VM_P2P &&
          p_nfc_cl_config_req->nfc_config.ch_service != NFC_VM_LLCP_NONE &&
          p_nfc_cl_config_req->nfc_config.snep_service == NFC_VM_LLCP_NONE))
    {
        /* Received configuration currently unsupported. Return error */
        return NFC_VM_UNSUPPORTED;
    }
    return NFC_VM_SUCCESS;
}

NFC_VM_STATUS NfcClConfigReq(NFC_CL_CONFIG_REQ *p_nfc_cl_config_req)
{
    NFC_VM_STATUS vm_status;

    NFC_CL_DEBUG((NFC_CL_I"NfcClConfigReq\n"));
    vm_status = NfcClConfigReqValidation(p_nfc_cl_config_req);
    if (NFC_VM_SUCCESS != vm_status)
    {
        return vm_status;
    }

    switch (NfcClGetState())
    {
    case NFC_CL_ST_NULL:
        NfcClConfigReqCommonHdl(p_nfc_cl_config_req);
        /* Register Connection Library with NFC stack */
        NfcClSetState(NFC_CL_ST_REGISTERING);
        NfcSendCtrlRegisterReqPrim();
        vm_status = NFC_VM_SUCCESS;
        break;
    case NFC_CL_ST_REGISTERED: /* fallthrough */
    case NFC_CL_ST_CONFIGURED:
        NfcClConfigReqCommonHdl(p_nfc_cl_config_req);
        /* Connection Library already registered. Another configuration was
         * running previously so ask for new configuration */
        NfcClSetState(NFC_CL_ST_CONFIGURING);
        NfcSendCtrlConfigReqPrim(nfcClData.requested_nfc_config.mode,
                             nfcClData.requested_nfc_config.ch_service,
                             nfcClData.requested_nfc_config.snep_service);
        vm_status = NFC_VM_SUCCESS;
        break;
    case NFC_CL_ST_REGISTERING: /* fallthrough */
    case NFC_CL_ST_CONFIGURING: /* fallthrough */
    default:
        vm_status = NFC_VM_WRONG_STATE;
        break;
    }
    return vm_status;
}
#endif /* HYDRACORE */
