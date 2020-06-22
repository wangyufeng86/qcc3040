/****************************************************************************
Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
  

*/

/*****************************************************************************/
/*!
\defgroup nfc_cl nfc_cl

\brief The NFC software implements BT Static Handover as specified by the
 NFC-Forum's Connection Handover Technical Specification. This module
 implements the 'Handover Selector' role, which is sufficient to support
 the use cases such as 'tap-to-pair', 'tap-to-connect', 'tap-to-disconnect', etc.

 For Static Handover, the NFC device (such as mobile phone or tablet) that
 initiates the BT handover always takes the roles of 'Handover Requestor'
 so the other device (this device) involved in the handover only needs to
 support the 'Handover Selector' role for all the above use cases. The NFC
 software module implements an interface to access the NFC functionality

 - NFC Forum Type2 Tag - that is required to support BT/NFC handover.

 The NFC software module is organised in such a way that it provides
 two different sets of NFC API for the applications that use NFC/BT
 Static Handover. The first set consists of 'NFC connection library'
 API (nfc_cl.c, nfc_cl.h), which presents a high level Handover function calls
 to the application.

 Connection library internally invokes the necessary BT function calls
 to access the BT information required for the Static Handover.

 This API set is likely to be used in cases wherein the existing
 application is modified to implement the appropriate use cases.

 The Alternative API set provides direct access to the NFC functionality
 (such as programing the tag contents and changing the NFC mode, Etc.)
 via function calls. This API set (i.e. nfc_api.c, nfc_api.h) is likely to be
 used wherein the application needs maximum flexibility while using the NFC and
 BT APIs to support the Static Handover use cases.

\ingroup vm_libs

\section nfc_cl_overview NFC CL code overview

The NFC code on P1 is structured as follow:

  -# sink_nfc.c and corresponding header file sink_nfc.h is an EXAMPLE
  illustrating how to call the nfc connection library and the BT connection
  library to implement an NFC BT static handover.

  -# nfc_cl.c and  corresponding hearder file nfc_cl.h is an EXAMPLE code
  illustrating how to an 'NFC connection library'.

  -# nfc.c and corresponding header file nfc.h provides the NFC API to P1.

  -# Finally, the NFC primitives sent to P0 and received from P0 are described
  in nfc_prim.h

  (see diagram below)

<p><tt>
\code{.unparsed}

  ====================[P1]===================    <=IPC=>         ==[P0]===
    MainTask          NfcClTask
  [sink_nfc] <------> [nfc_cl] <------> [nfc] <--- nfc_prim --->

\endcode
</tt></p>

  The nfc primitives are all using the same base so it is possible to register
  the main task as the handler of Rx NFC primitive and ignore the NFC connection.
  library.

  (see diagram below)
<p><tt>
\code{.unparsed}

  ================[P1]=======================    <=IPC=>      ==[P0]===
      MainTask
  [customer nfc handler] <------------> [nfc] <-- nfc_prim -->

\endcode
</tt></p>


\section nfc_cl_sequence NFC CL valid sequences

<p><tt>
\code{.unparsed}

  Use case: Registration for NFC services using the NFC connection library

  ======================[P1]===================== <=IPC=>  =====[P0]======
  [Client]            [NFC_CL]                                  [NFC]
    |                     |                                       |
  NfcClConfigReq()------->|                                       |
    |               NfcCtrlRegister()--->|                        |
    |                     |              |-NFC_CTRL_REGISTER_REQ->|
    |                     |                                       |
    |                     |<----NFC_CTRL_READY_IND----------------|
    |<-NFC_CL_CONFIG_CNF--|                                       |
   [-]                   [-]                                     [-]

\endcode
</tt></p>

  Valid sequences of calls.

  - Configuration Sequence Example 1: initialise the tag with a BT Static Handover
    The TAG is programed with a default BT address, device name, class of
    device and Service Class UUIDs.

    -# call NfcClConfigReq(ClientApp)
    -# Recv NFC_CL_CONFIG_CNF(NFC_CL_READY)
    -# call NfcClEncBtCarInitData (optional if this is the first time)
    -# call NfcClEncBtCarBDAddr
    -# call NfcClEncBtCarLocalName
    -# call NfcClEncBtCarClassOfDevice
    -# call NfcClEncBtCarCompleteList16BitSc
    -# call NfcClWriteChCarriersReq
    -# Recv NFC_CL_WRITE_CH_CARRIERS_CNF

  - Configuration Sequence Example 2:  initialise the tag with a BT Static Handover
    with ONLY the BT address and device name. The TAG is programed with a
    set BT address, device name and NO class of device and NO Service
    Class UUIDs.

    -# call NfcClConfigReq(ClientApp)
    -# Recv NFC_CL_CONFIG_CNF(NFC_CL_READY)
    -# call NfcClEncBtCarInitData (optional if this is the first time)
    -# call NfcClEncBtCarBDAddr
    -# call NfcClEncBtCarLocalName
    -# call NfcClWriteChCarriersReq
    -# Recv NFC_CL_WRITE_CH_CARRIERS_CNF

  - Wake Up Sequence: the NFC subsystem is already configured as a TT2.
    P1 is woken up by the NFC field of a phone.
    -# call NfcClConfigReq(ClientApp)
    -# Recv NFC_CL_CONFIG_CNF(NFC_CL_CONFIGURED)
    -# Recv NFC_CL_TAG_READ_STARTED_IND
    -# The phone is reading the TAG. Now make the BT device connectable in
    the client app and scan for paging request.


<p><tt>
\code{.unparsed}

  -#BT NFC Handover

  Field description           |  M-Mandatory / O-Optional
  ----------------------------|-----------------------------------
  Bluetooth Address           |  M
  Local Name                  |  M
  Class Of Device             |  O
  Complete List 16 Bit        |  O
  Simple Pairing Hash C       |  O
  Simple Pairing Randomizer R |  O

  -#BLE NFC Handover (to be defined)

  Field description           |  M-Mandatory / O-Optional
  ----------------------------|-----------------------------------
  Bluetooth Address           |  M
  Local Name                  |  M
  LE Role                     |  M
  LE Appearance               |  O
  LE Security Manager Value   |  O

\endcode
</tt></p>

@{
*/
#ifndef NFC_CL_H
#define NFC_CL_H

/* PROJECT INCLUDES **********************************************************/
#include "nfc/nfc_prim.h"
#include "message.h"
#include "bdaddr_.h"
#include "library.h"

/* PUBLIC MACRO DEFINITIONS **************************************************/
#if !defined(NFC_CL_MESSAGE_BASE)
#error "Please define in apps NFC_CL_MESSAGE_BASE e.g. /library/library.h"
#endif

/* Message ID for the NFC Connection Library. */
typedef enum nfc_cl_msg_id_enum
{
    NFC_CL_CONFIG_CNF_ID             = NFC_CL_MESSAGE_BASE+0, /*!< NFC CL Hardware is up and running, configuration result */
    NFC_CL_WRITE_CH_CARRIERS_CNF_ID  = NFC_CL_MESSAGE_BASE+1, /*!< NFC CL alternate carriers update result */
    NFC_CL_CARRIER_ON_IND_ID         = NFC_CL_MESSAGE_BASE+2, /*!< NFC CL Field detected by NFC HW */
    NFC_CL_CARRIER_LOSS_IND_ID       = NFC_CL_MESSAGE_BASE+3, /*!< NFC CL Field loss detected by NFC HW */
    NFC_CL_SELECTED_IND_ID           = NFC_CL_MESSAGE_BASE+4, /*!< NFC CL target selected by a NFC reader */
    NFC_CL_TAG_READ_STARTED_IND_ID   = NFC_CL_MESSAGE_BASE+5, /*!< NFC CL Phone is reading the tag */
    NFC_CL_STOP_FAST_PAGESCAN_IND_ID = NFC_CL_MESSAGE_BASE+6, /*!< NFC CL Fast pagescan has stopped -> go back to default scan */
    NFC_CL_DISALLOW_AG2_PAIRING_IND  = NFC_CL_MESSAGE_BASE+7, /*!< NFC CL Connected pagescan has stopped -> go back to default */
    NFC_CL_HANDOVER_CARRIER_IND_ID   = NFC_CL_MESSAGE_BASE+8, /*!< NFC CL Negotiated Channel Handover received carriers */
    NFC_CL_HANDOVER_COMPLETE_IND_ID  = NFC_CL_MESSAGE_BASE+9, /*!< NFC CL Negotiated Channel Handover procedure result */
    NFC_CL_MESSAGE_TOP               = NFC_CL_MESSAGE_BASE+10
} NFC_CL_MSG_ID;

#define IS_VALID_NFC_CL_MSG_ID(id) ( (NFC_CL_MESSAGE_BASE<=(id)) && ((id)<NFC_CL_MESSAGE_TOP) )


#define MAX_LOCAL_NAME_LEN (64)  /**< Maximum BT local name length in bytes */
#define MAX_COMPLETE_LIST_16BIT_SC (16) /**< Maximum BT COMPLETE LIST 16BIT SC length in bytes */
#define CLASS_OF_DEVICE_SIZE (3) /**< Class of device size */
#define SIMPLE_PAIRING_HASH_C_SIZE (16) /**< 16B Simple Pairing Hash C Size */
#define SIMPLE_PAIRING_RANDOMIZER_R_SIZE (16) /**< 16B Simple Pairing Randomizer R Size */
#define LE_APPEARANCE_SIZE (2)  /**< 2B LE Appearance Size */
#define LE_SECURITY_MANAGER_TK_SIZE (16)   /**< 16B Security Tk Manager Size */

/* @brief This macro defines the maximum number of carriers supported by the
 * NFC library for the Static Channel Handover API using TAG
 */
#define NFC_TAG_MAX_CARRIER (1)

typedef enum nfc_cl_car_enum
{
    NONE_CAR_ID,
    BT_CAR_ID,
    LE_CAR_ID
} NFC_CL_CAR;

/*! NFC_CL_STATUS

   NFC_CL_STATUS is used as the NFC Connection Library returned status for the
   initialisation/configuration procedure
 */
typedef enum nfc_cl_status_enum
{
    NFC_CL_READY, /*!< NFC Connection Library is registered and READY to be configured */
    NFC_CL_CONFIGURED, /*!< NFC Connection Library is CONFIGURED */
    NFC_CL_FAIL /*!< NFC Connection Library  Request has failed. */
} NFC_CL_STATUS;

/* NFC CL PRIMITIVES *********************************************************/
/****************************************************************************/
/*!
    @brief NFC Event Handler function prototype definition
*/
typedef struct nfc_cl_config_req_struct
{
    Task nfcClientRecvTask; /*!< main application task.  If NULL no NFC CL messages will be sent. */

    bool send_carrier_on_ind;   /*!< Send NFC_CL_CARRIER_ON_IND to nfcClientRecvTask if TRUE */
    bool send_carrier_loss_ind; /*!< Send NFC_CL_CARRIER_LOSS_IND to nfcClientRecvTask if TRUE */
    bool send_selected_ind;     /*!< Send NFC_CL_SELECTED_IND to nfcClientRecvTask if TRUE */

    NFC_CTRL_CONFIG nfc_config;
} NFC_CL_CONFIG_REQ;

typedef struct nfc_cl_prim_msg_struct NFC_CL_PRIM;

typedef struct nfc_cl_config_cnf_struct
{
    NFC_CL_STATUS nfc_cl_status;
    NFC_CTRL_CONFIG nfc_config;
} NFC_CL_CONFIG_CNF;

/* NFC_CL_CH_CARRIER_IND

   NFC_CL_CH_CARRIER_IND is used to send the received carrier information to
   the application in case of a Negotiated Channel Handover. Data needs to be
   released once it is consumed using NfcClReleaseChCarriers.

   @warning The carrier array pointer is released when NfcClReleaseChCarriers
   is called.
*/
typedef struct nfc_cl_ch_carrier_ind_struct
{
    NFC_STREAM_HANDOVER_TYPE handover_record_type; /*!< received handover record type */
    NFC_CH_CARRIER **carriers; /*!< carrier array pointers to the received data */
    uint8 n_carriers; /*!< number of received carriers */
    uint16 msg_len; /*!< received data length. This can be used to release carrier data */
}NFC_CL_CH_CARRIER_IND;

struct nfc_cl_prim_msg_struct
{
    NFC_VM_STATUS status;
    union
    {
        NFC_CL_CONFIG_CNF config_cnf;
        NFC_CL_CH_CARRIER_IND ch_carrier_ind;
    } m;
};

/* PUBLIC FUNCTION DECLARATIONS **********************************************/

/* BLUETOOTH CARRIER ENCODING FUNCTION DECLARATIONS **************************/
/*****************************************************************************/

/*****************************************************************************/
/*!
    @brief Initialises the BT carrier data that required to setup the NFC
    BT handover.

    Note: This function deletes the BT carrier information stored locally.
*/
extern void NfcClEncBtCarResetData(void);

/*****************************************************************************/
/*!
    @brief Locally sets the Bluetooth Address to be encoded on the BT carrier

    Note: The handover can only be programed when all the required field
    have been set (e.g. BT Address).

    @param local_bdaddr
           - NULL Deletes the BT address set locally, if set previously.
           - Not NULL Stores a local copy of the BD address to be encoded
*/
extern void NfcClEncBtCarBDAddr(const bdaddr *local_bdaddr);

/*****************************************************************************/
/*!
    @brief Locally set the Bluetooth device name to be encoded on the BT
    carrier.

    - Note 1: The handover can only be programed when all the required field
    have been set (e.g. BT Address).
    - Note 2: The name is truncated to MAX_LOCAL_NAME_LEN

    @param new_local_name Contains the COMPLETE LOCAL NAME of the BT device.
        - NULL Deletes the BT complete local name set locally, if set
          previously.
        - Not NULL Stores a local copy of the "new_local_name" to be encoded.
    @param new_size_local_name Size in bytes of "new_local_name"
*/
extern void NfcClEncBtCarLocalName(const uint8 *new_local_name,
                                   uint8 new_size_local_name);

/*****************************************************************************/
/*!
    @brief Localy set the Complete List of 16-bit Service Class UUIDs to be
    encoded on the BT carrier.

    @param new_list_16bit_sc Contains the LIST_16BIT_SC of the BT device.
        - NULL Deletes the BT UUIDs list set locally, if set previously.
        - Not NULL Stores a local copy of the "new_list_16bit_sc" to be
          encoded.
    @param new_size_list_16bit_sc Size in bytes of "new_list_16bit_sc".  It
    should be smaller or equal to MAX_COMPLETE_LIST_16BIT_SC.
*/
extern void NfcClEncBtCarCompleteList16BitSc(const uint8 *new_list_16bit_sc,
                                             uint8 new_size_list_16bit_sc);

/*****************************************************************************/
/*!
    @brief Localy set the Class Of Device to be encoded on the BT
    carrier.

    @param new_class_of_device contains the "Class Of Device" of the BT
    device. It MUST be of size CLASS_OF_DEVICE_SIZE.
        - NULL Deletes the BT class of device set locally, if set previously.
        - Not NULL Stores a local copy of the "new_class_of_device" to be
          encoded.
*/
extern void NfcClEncBtCarClassOfDevice(const uint8 *new_class_of_device);

/*****************************************************************************/
/*!
    @brief Localy set the simple pairing hash C to be encoded on the BT
    carrier.

    !! deliberate loss of @ on param to prevent error caused by commented prototype
    param new_simple_pairing_hash_c contains the simple pairing hash C of the
    BT device. It MUST be of size SIMPLE_PAIRING_HASH_C_SIZE
        - NULL Deletes the BT Simple Pairing hash set locally, if set
          previously.
        - Not NULL Stores a local copy of the "new_simple_pairing_hash_c" to be
          encoded
*/
extern void NfcClEncBtCarSimplePairingHashC(const uint8 *new_simple_pairing_hash_c);

/*****************************************************************************/
/*!
    @brief Localy set the simple pairing hash C to be encoded on the BT
    carrier.

    !! deliberate loss of @ on param to prevent error caused by commented prototype
    param new_simple_pairing_randomizer_r contains the simple pairing hash C of the
    BT device. It MUST be of size SIMPLE_PAIRING_RANDOMIZER_R_SIZE
        - NULL Deletes the BT Simple Pairing random number set locally, if
          set previously.
        - Not NULL Stores a local copy of the "new_simple_pairing_randomizer_r"
          to be encoded
*/
extern void NfcClEncBtCarSimplePairingRandomizerR(const uint8 *new_simple_pairing_randomizer_r);

/* LOW ENERGY CARRIER ENCODING FUNCTION DECLARATIONS *************************/
/*****************************************************************************/

/*****************************************************************************/
/*!
    @brief Initialises the LE carrier data that required to setup the NFC
    LE handover.

    Note: This function deletes the LE carrier information stored locally.
*/
extern void NfcClEncLeCarResetData(void);

/*****************************************************************************/
/*!
    @brief Localy set the appearance to be encoded on the LE carrier.

    @param new_le_appearance contains the LE appearance.
        - NULL Deletes the LE appearance information set locally, if
          set previously.
        - Not NULL Stores a local copy of the "LE appearance" to be encoded
*/
extern void NfcClEncLeCarAppearance(const uint8 *new_le_appearance);

/*****************************************************************************/
/*!
    @brief Locally sets the LE Address to be encoded on the LE carrier

    Note: The handover can only be programed when all the required field
    have been set (e.g. LE Address).

    @param new_le_local_addr Pointer to address. This MUST be the same as the
    BT address for dual  mode devices.
           - NULL Deletes the LE address set locally, if set previously.
           - Not NULL Stores a local copy of the LE address to be encoded.
*/
extern void NfcClEncLeCarLeLocalAddr(const bdaddr *new_le_local_addr);

/*****************************************************************************/
/*!
    @brief Locally set the LE device name to be encoded on the LE
    carrier.

    - Note 1: The handover can only be programed when all the required field
    have been set (e.g. LE Address).
    - Note 2: The name is truncated to MAX_LOCAL_NAME_LEN

    @param new_le_local_name Contains the COMPLETE LOCAL NAME of the BT device.
        - NULL Deletes the LE complete local name set locally, if set
          previously.
        - Not NULL Stores a local copy of the "new_le_local_name" to be
          encoded.
    @param new_size_le_local_name Size in bytes of "new_le_local_name"
*/
extern void NfcClEncLeCarLeLocalName(const uint8 *new_le_local_name,
                                     uint8 new_size_le_local_name);

/*****************************************************************************/
/*!
    @brief Locally set the LE role  to be encoded on the LE carrier.

    @param new_le_role The LE Role data type defines the LE role capabilities
    of the device. This data type shall exist only once. It may be sent by
    an out of band method to a peer device.
    - The LE Role data type size is 1 octet.
           - 0x00 Only Peripheral Role supported
           - 0x01 Only Central Role supported
           - 0x02 Peripheral and Central Role supported,  Peripheral Role
             preferred for connection establishment.
           - 0x03 Peripheral and Central Role supported, Central Role preferred
             for connection establishment

    - Example:
           - 0x03 => LE Role: Central and peripheral capabilities with the
             central role preferred.
*/
extern void NfcClEncLeCarRole(uint8 new_le_role);

/*****************************************************************************/
/*!
    @brief Localy set the security_manager_value to be encoded on the LE
    carrier.

    @param new_le_security_manager_value contains the security manager value of
    the LE device. It MUST be of size LE_SECURITY_MANAGER_TK_SIZE
        - NULL Deletes the LE Security Manager value set locally, if set
          previously.
        - Not NULL Stores a local copy of the "new_le_security_manager_value"
          to be encoded
*/
extern void NfcClEncLeCarLeSecurityManager(const uint8 *new_le_security_manager_value);

/* NFC CL API FUNCTION DECLARATIONS ******************************************/
/*****************************************************************************/

/*****************************************************************************/
/*!
    @brief This function initialises and configures the NFC hardware as a
    Tag Type 2 and to configure the NFC TAG contents for a Bluetooth static
    handover.

    The NFC extracts the BT information from the BT sub-system and
    format this information - BT address and the EIR Data - into the
    expected NDEF format.

    The request is pending until receiving NFC_CL_INIT_CNF. Only one PENDING
    request is supported.

    BT Address
    BT EIR Data
    - Complete Local Name
    - Class of Device
    - Complete List of 16-bit Service Class UUIDs

    @param p_nfc_cl_config_req pointer on the Client initialisation structure

    @warning This function MUST be called first to register to the NFC subsystem.

    @return result of request validation. The request is only executed when the
    return is SUCCESS
*/
extern NFC_VM_STATUS NfcClConfigReq(NFC_CL_CONFIG_REQ *p_nfc_cl_config_req);

/*****************************************************************************/
/*!
    @brief write the channel handover carrier.

    @param first_car first priority carrier (BT or BLE)
    @param second_car second priority carrier (BT, BLE or NONE)
    - NONE disabled, if first_car=BT then second_car must be either BLE or NONE

    @return result of encoding the carriers. The request is only executed when the
    encoding is successful (e.g. return is SUCCESS)
*/
extern NFC_VM_STATUS NfcClWriteChCarriersReq(NFC_CL_CAR first_car,
                                             NFC_CL_CAR second_car);

/*****************************************************************************/
/*!
    @brief release carrier data received from a Negotiated Channel Handover.

    @param ch_carriers pointer to carrier data structure to be released

    @return result of release procedure. The request is only executed when the
    Negotiated Channel Handover is configured
*/
extern NFC_VM_STATUS NfcClReleaseChCarriers(NFC_CL_CH_CARRIER_IND *ch_carriers);

/** @}*/
#endif /* NFC_CL_H */

