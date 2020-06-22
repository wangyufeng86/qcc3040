/*!

 %%fullcopyright(2017)

\file   vendor_specific_hci.h

\brief  This file contains HCI specific type definitions for Qualcomm Vendor
        Specific commands.

\verbatim
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *   W   W    A    RRRR   N   N  IIIII  N   N   GGG
 *   W   W   A A   R   R  N   N    I    N   N  G   G
 *   W   W  A   A  R   R  NN  N    I    NN  N  G
 *   W   W  A   A  RRRR   N N N    I    N N N  G GGG
 *   W W W  AAAAA  R R    N  NN    I    N  NN  G   G
 *   W W W  A   A  R  R   N   N    I    N   N  G   G
 *    W W   A   A  R   R  N   N  IIIII  N   N   GGGG
 *
 *  This header file uses structures for defining the messages, but does
 *  nothing regarding the guaranteeing of packing of those structures.
 *
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
\endverbatim
*/


#ifndef BLUESTACK_VENDOR__HCI_H    /* Once is enough */
#define BLUESTACK_VENDOR__HCI_H

/*****************************************************************************
*  We expect this file to be included *only* by hci.h. This file is for 
*  vendor specific commands only and is actually a part of hci.h. We have
*  separated these commands in order to keep hci.h clean.
*****************************************************************************/

typedef uint8_t hci_sub_op_code_t;            /* used for Qualcomm commands  */

/******************************************************************************
  OP CODE defines - ULP
 ******************************************************************************/
#define HCI_ULP_QVS_BT_LOWER_TESTER              ((hci_op_code_t)HCI_MANUFACTURER_EXTENSION| 0x018)

 /******************************************************************************
 OP CODE defines - QCT commands
 ******************************************************************************/
#define OCF_MASK                                 ((hci_ocf_t)HCI_OPCODE_MASK)
/* B-289548 These defines are duplicated for now exclude from firmware builds */
/******************************************************************************
   HCI_COMMAND, Argument Length Definitions.
 *****************************************************************************/
#define HCI_LT_DATA_LENGTH_LEN                   ((uint8_t)  9)
#define HCI_LT_PHY_TEST_MODE_LEN                 ((uint8_t)  4)

 /******************************************************************************
 QLM and QLL constants.
 *****************************************************************************/

#define QBCE_QLMP_FEATURE_SIZE                   ((uint8_t)  16)
#define QBCE_QLL_FEATURE_SIZE                    ((uint8_t)  16)
#define QBCE_EVENT_MASK_SIZE                     ((uint8_t)  8)
#define QBCE_MAX_BD_ADDR_NUM                     ((uint8_t)  16)

/******************************************************************************
   HCI_COMMAND_COMPLETE, Argument Length Definitions (Full length)
   Should consist of: nhcp (1) + opcode (2) + return parameters
                    : = 3 + return parameters from spec (incl. status)
   When an argument length is dependant on the number of elements in the array
   the defined length contains the constant parameter lengths only. The full
   array length must be calculated.
*****************************************************************************/
#define  HCI_ULP_QVS_BT_LOWER_TESTER_ARG_LEN                                        ((uint8_t)  5)

/******************************************************************************* 
    Qualcomm Vendor Specific command sub-opcodes 
 ******************************************************************************/
#define SUB_OPCODE_QCT_NVM_ACCESS_GET                                               ((uint8_t) 0x00)
#define SUB_OPCODE_QCT_NVM_ACCESS_SET                                               ((uint8_t) 0x01)
#define SUB_OPCODE_QCT_NVM_HCI_RESET                                                ((uint8_t) 0x03)
#define SUB_OPCODE_QCT_NVM_EXT_ACCESS_GET                                           ((uint8_t) 0x05)
#define SUB_OPCODE_QCT_NVM_EXT_ACCESS_SET                                           ((uint8_t) 0x06)
#define SUB_OPCODE_QCT_EDL_GETAPPVER_REQ                                            ((uint8_t) 0x06)
#define SUB_OPCODE_QCT_EDL_GET_BUILD_INFO                                           ((uint8_t) 0x20)
#define SUB_OPCODE_QCT_EDL_POKE16_REQ                                               ((uint8_t) 0x0A)
#define SUB_OPCODE_QCT_EDL_PEEK16_REQ                                               ((uint8_t) 0x0B)
#define SUB_OPCODE_LT_DATA_LENGTH                                                   ((uint8_t) 0x0B)
#define SUB_OPCODE_LT_PHY_TEST_MODE                                                 ((uint8_t) 0x0C)
#define SUB_OPCODE_QCT_EDL_POKE32_REQ                                               ((uint8_t) 0x0C)
#define SUB_OPCODE_QCT_EDL_PEEK32_REQ                                               ((uint8_t) 0x0D)
#define COMMAND_STATUS_PATCH_VER_REQ                                                ((uint8_t) 0x19) /* Effectively a sub-opcode */
#define SUB_OPCODE_QCT_EDL_PATCH_GETVER                                             COMMAND_STATUS_PATCH_VER_REQ
#define SUB_OPCODE_QCT_QBCE_READ_LOCAL_QLM_SUPPORTED_FEATURES                       ((uint8_t) 0x09)
#define SUB_OPCODE_QCT_QBCE_READ_REMOTE_QLM_SUPPORTED_FEATURES                      ((uint8_t) 0x0A)
#define SUB_OPCODE_QCT_QBCE_READ_LOCAL_QLL_SUPPORTED_FEATURES                       ((uint8_t) 0x0B)
#define SUB_OPCODE_QCT_QBCE_READ_REMOTE_QLL_SUPPORTED_FEATURES                      ((uint8_t) 0x0C)
#define SUB_OPCODE_QCT_QBCE_SET_QLM_EVENT_MASK                                      ((uint8_t) 0x0F)
#define SUB_OPCODE_QCT_QBCE_SET_QLL_EVENT_MASK                                      ((uint8_t) 0x10)
#define SUB_OPCODE_QCT_QBCE_WRITE_SECURE_CONNECTIONS_HOST_SUPPORT_OVERRIDE          ((uint8_t) 0x11)
#define SUB_OPCODE_QCT_QBCE_READ_SECURE_CONNECTIONS_HOST_SUPPORT_OVERRIDE           ((uint8_t) 0x12)
#define SUB_OPCODE_QCT_QBCE_READ_SECURE_CONNECTIONS_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDRS  ((uint8_t) 0x13)
#define COMMAND_STATUS_TLV_DOWNLOAD_REQ                                             ((uint8_t) 0x1E) /* Effectively a sub-opcode */
#define SUB_OPCODE_QCT_TLV_DOWNLOAD_REQ                                             COMMAND_STATUS_TLV_DOWNLOAD_REQ
#define SUB_OPCODE_QCT_TWSS_SHADOW_CREATE_CONNECTION                                ((uint8_t) 0x00)
#define SUB_OPCODE_QCT_TWSS_SHADOW_DISCONNECT                                       ((uint8_t) 0x01)
#define SUB_OPCODE_QCT_TWSS_PREPARE_HANDOVER                                        ((uint8_t) 0x02)
#define SUB_OPCODE_QCT_TWSS_SET_SLAVE_BD_ADDR                                       ((uint8_t) 0x03)
#define SUB_OPCODE_QCT_TWSS_COMPLETE_HANDOVER                                       ((uint8_t) 0x04)
#define SUB_OPCODE_QCT_TWSS_SECONDARY_SHADOW_SNIFFER_DATA_ENABLE                    ((uint8_t) 0x05)
#define SUB_OPCODE_QCT_TWSS_HANDOVER_SYNCHRONOUS_CONNECTION                         ((uint8_t) 0x06)
#define SUB_OPCODE_QCT_TWSS_RELIABLE_ACL_SHADOW_ENABLE                              ((uint8_t) 0x07)
#define SUB_OPCODE_QCT_TWSS_RELIABLE_ACL_SHADOW_REQUEST_REPLY                       ((uint8_t) 0x08)
#define SUB_OPCODE_QCT_TWSS_RELAY_DEBUG_SUPPORT                                     ((uint8_t) 0x09)
#define SUB_OPCODE_QCT_TWSS_APPLY_BTSS_DATA                                         ((uint8_t) 0x0A)
#define SUB_OPCODE_QCT_TWSS_READ_RELAY_DEBUG_COUNTERS                               ((uint8_t) 0x0B)
#define SUB_OPCODE_QCT_TX_CONTROL_ENABLE_TX                                         ((uint8_t) 0x07)
#define SUB_OPCODE_QCT_TX_CONTROL_DISABLE_TX                                        ((uint8_t) 0x08)
#define SUB_OPCODE_QCT_TX_CONTROL_SET_DEFAULT_POWER                                 ((uint8_t) 0x09)
#define SUB_OPCODE_QCT_TX_CONTROL_GET_DEFAULT_POWER                                 ((uint8_t) 0x0A)
#define SUB_OPCODE_QCT_TX_CONTROL_SET_MAXIMUM_POWER                                 ((uint8_t) 0x0B)
#define SUB_OPCODE_QCT_TX_CONTROL_GET_MAXIMUM_POWER                                 ((uint8_t) 0x0C)
#define SUB_OPCODE_QCT_TX_CONTROL_TX_POWER_MESSAGES_ENABLE                          ((uint8_t) 0x0D)
#define SUB_OPCODE_HCI_VS_BLUETOOTH_READ_RSSI_LINEAR                                ((uint8_t) 0x01)
#define SUB_OPCODE_HCI_VS_BLUETOOTH_L2CAP_FLOW                                      ((uint8_t) 0x0C)
#define SUB_OPCODE_HCI_VS_BLUETOOTH_READ_LINK_AWAY_TIME                             ((uint8_t) 0x0D)
#define SUB_OPCODE_HCI_VS_BLUETOOTH_SET_BD_ADDR                                     ((uint8_t) 0x0E)
#define SUB_OPCODE_HCI_VS_DEBUG_GET_BLE_WHITELIST_FREE_SPACE                        ((uint8_t) 0x0E)
#define SUB_OPCODE_HCI_VS_DEBUG_OPCODE_GET_ESCO_STATS                               ((uint8_t) 0x0F)
#define SUB_OPCODE_HCI_VS_DEBUG_OPCODE_GET_DEBUG_SLOT_COUNTERS                      ((uint8_t) 0x10)
#define SUB_OPCODE_HCI_VS_DEBUG_OPCODE_GET_DEBUG_ADV_TIMES                          ((uint8_t) 0x11)
#define SUB_OPCODE_HCI_VS_DEBUG_OPCODE_GET_DEBUG_CONN_TIMES                         ((uint8_t) 0x12)
#define SUB_OPCODE_HCI_VS_DEBUG_OPCODE_GET_RX_EVENT_LOG                             ((uint8_t) 0x13)
#define SUB_OPCODE_HCI_VS_DEBUG_OPCODE_GET_TX_EVENT_LOG                             ((uint8_t) 0x14)
#define SUB_OPCODE_HCI_VS_DEBUG_OPCODE_GET_BUILD_NAME                               ((uint8_t) 0x15)
#define SUB_OPCODE_HCI_VS_DEBUG_OPCODE_GET_DEEP_SLEEP_TIME                          ((uint8_t) 0x16)
#define SUB_OPCODE_HCI_VS_DEBUG_SET_LE_ENABLE_ZERO_SLAVE_LATENCY                    ((uint8_t) 0x17)
#define SUB_OPCODE_HCI_VS_BCAUD_CDA_CSB_SET_TX_AFH_MAP                              ((uint8_t) 0x00)
#define SUB_OPCODE_HCI_VS_BCAUD_CDA_CSB_SET_RX_AFH_MAP                              ((uint8_t) 0x01)
#define SUB_OPCODE_HCI_VS_BCAUD_CDA_ENABLE_CSB_NEW_MAP_EV                           ((uint8_t) 0x02)
#define SUB_OPCODE_HCI_VS_PROD_TEST_STATS                                           ((uint8_t) 0x02)
#define SUB_OPCODE_HCI_VS_PROD_TEST_STOP                                            ((uint8_t) 0x03)
#define SUB_OPCODE_HCI_VS_PROD_TEST_TX                                              ((uint8_t) 0x04)
#define SUB_OPCODE_HCI_VS_PROD_TEST_TXC_ONLY                                        ((uint8_t) 0x05)
#define SUB_OPCODE_HCI_VS_PROD_TEST_RX_BURST                                        ((uint8_t) 0x06)
#define SUB_OPCODE_HCI_VS_PROD_TEST_RX_BERT                                         ((uint8_t) 0x07)
#define SUB_OPCODE_HCI_VS_PROD_TEST_RXC                                             ((uint8_t) 0x1E)

 /*******************************************************************************
 Qualcomm Vendor Specific event codes
 ******************************************************************************/
#define QCT_EV_NVM_DOWNLOAD                                                         ((hci_event_code_t) 0x00)
#define QCT_EV_NVM_ACCESS                                                           ((hci_event_code_t) 0x0B)
#define QCT_EV_TWS                                                                  ((hci_event_code_t) 0x0E)
#define QCT_EV_QBC                                                                  ((hci_event_code_t) 0x51)
#define QCT_EV_BCAUD_CDA_CSB_AFH_MAP_AVAILABLE                                      ((hci_event_code_t) 0x54)
#define QCT_EV_LINK_STATUS_CMD                                                      ((hci_event_code_t) 0x0F)
#define QCT_EV_SET_BAUDRATE_RESPONSE                                                ((hci_event_code_t) 0x92)

 /*******************************************************************************
 Qualcomm Vendor Specific event sub opcodes
 ******************************************************************************/
#define QCT_EV_EDL_STATUS_RESPONSE                                                  ((uint8_t) 0x00)
#define QCT_EV_EDL_UPLOAD_RESPONSE                                                  ((uint8_t) 0x01)
#define QCT_EV_EDL_APPVER_RESPONSE                                                  ((uint8_t) 0x02)
#define QCT_EV_PATCH_VER_RESPONSE                                                   ((uint8_t) 0x02)
#define QCT_EV_TLV_DOWNLOAD_RESPONSE                                                ((uint8_t) 0x04)
#define QCT_EV_EDL_BLDVER_RESPONSE                                                  ((uint8_t) 0x05)
#define QCT_EV_INBOUND_DATA_CLEAR_EVENT                                             ((uint8_t) 0x00)
#define QCT_EV_QBCE_READ_REMOTE_SUPPORTED_QLMP_FEATURES_COMPLETE                    ((uint8_t) 0x05)
#define QCT_EV_QBCE_READ_REMOTE_QLL_FEATURES_COMPLETE                               ((uint8_t) 0x06)
#define QCT_EV_QBCE_QLM_CONNECTION_COMPLETE                                         ((uint8_t) 0x08)
#define QCT_EV_QBCE_QLL_CONNECTION_COMPLETE                                         ((uint8_t) 0x09)
#define QCT_EV_TWS_ACL_CONNECTION_COMPLETE                                          ((uint8_t) 0x00)
#define QCT_EV_TWS_SHADOW_DISCONNECTION_COMPLETE                                    ((uint8_t) 0x01)
#define QCT_EV_TWS_BD_ADDR_CHANGED                                                  ((uint8_t) 0x02)
#define QCT_EV_TWS_PREPARE_HANDOVER_COMPLETE                                        ((uint8_t) 0x03)
#define QCT_EV_TWS_ESCO_CONNECTION_COMPLETE                                         ((uint8_t) 0x04)
#define QCT_EV_TWS_ESCO_CONNECTION_CHANGED                                          ((uint8_t) 0x05)
#define QCT_EV_TWS_HANDOVER_SYNCHRONOUS_CONNECTION_COMPLETE                         ((uint8_t) 0x06)
#define QCT_EV_TWS_RELIABLE_ACL_REQUESTED                                           ((uint8_t) 0x07)
#define QCT_EV_TWS_RELIABLE_ACL_COMPLETE                                            ((uint8_t) 0x08)
#define QCT_EV_TWS_RELIABLE_ACL_PACKETS_FLUSHED                                     ((uint8_t) 0x09)
#define QCT_EV_TWS_BTSS_DATA                                                        ((uint8_t) 0x0A)
#define QCT_EV_TWS_SHADOW_ACL_CLOCK_SYNC                                            ((uint8_t) 0x0B)                      

/******************************************************************************* 
    MACROS
 ******************************************************************************/
/* Used by the Lower Tester PHY Test Mode command */

#ifdef INSTALL_ULP_MULTI_PHY
extern uint16_t global_phy_test_mode;
extern uint16_t global_phy_m2s;
extern uint16_t global_phy_s2m;
#endif

/* Used by the HCI Qualcomm Lower Tester PHY Test Mode command */

typedef enum {
/* Used by the Slave Resp of PHY Update Procedure. When set to 1, it forces the 
Slave to send an LL_UNKNOWN_PHY PDU to the other end. */
    DEBUG_ULP_2LE_UNKN_RSP = 1,
/* Used by the Master Init/Resp of PHY Update Procedure and also by Slave 
Resp of PHY Update procedure. When set to 1, it forces the Master/Slave to *NOT* 
send an LL_PHY_UPDATE_REQ/LL_PHY_RSP PDU to the other end. 
This means that the other end will timeout.*/
    DEBUG_ULP_2LE_TIMEOUT = 2,
/* Flag used by the Master or Slave Init of PHY Update Procedure. 
When set to 1, it will block the initiation of the PHY Exchange. */
    DEBUG_ULP_2LE_BLOCK_INIT = 4,
/* Used to force the instant of an LL_PHY_UPDATE_REQ PDU to be in the past 
(well actually zero). This is used for testing the remote device response to 
receiving a past instant. Setting to 1 will force the update instant to be zero.*/
    DEBUG_ULP_2LE_INSTANT_PAST = 8,
/* Used by the Master Init or Resp of PHY Update Procedure. When set to 1, it 
forces the Master to use m2s_phy and s2m_phy in the LL_PHY_UPDATE_REQ.*/
    DEBUG_ULP_2LE_FORCE_UPDATE_REQ = 16,
/* Used by Master or Slave of an LE link. When used by Master or Slave
initiator it forces a PHY_REQ with the values given in global_phy_m2s and 
global_phy_s2m. The debug command must be followed by an LE Set PHY command 
whose TX_PHY and RX_PHY parameters will be ignored in the PHY update procedure.
When used by Slave responder it forces a PHY_RSP with the values given in 
global_phy_m2s and global_phy_s2m.*/
    DEBUG_ULP_2LE_FORCE_REQ_RSP = 32
} DEBUG_ULP_2LE_PHY_TEST_MODES;


#ifdef LMP_DEBUG
#define IS_PHY_TEST_MODE_BIT_ON(x)  (global_phy_test_mode & (x))
#else
#define IS_PHY_TEST_MODE_BIT_ON(x)  (FALSE)
#endif

/*----------------------------------------------------------------------------*
 * PURPOSE
 *     HCI Qualcomm Lower Tester Data Length command
 *
 *----------------------------------------------------------------------------*/

typedef struct
{
    uint8_t                  override;
    uint16_t                 max_tx_octets;
    uint16_t                 max_rx_octets;
    uint16_t                 max_tx_time;
    uint16_t                 max_rx_time;
} HCI_LT_DATA_LENGTH_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *     HCI Qualcomm Lower Tester PHY Test Mode command
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    uint8_t                  enable;
    uint8_t                  phy_test_mode;
    uint8_t                  phy_m2s;
    uint8_t                  phy_s2m;
} HCI_LT_PHY_TEST_MODE_T;


/*----------------------------------------------------------------------------*
 * PURPOSE
 *     HCI Qualcomm Vendor Specific command OpCode = 0xFC18
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
  hci_sub_op_code_t          sub_opcode;
} HCI_ULP_QVS_BT_LOWER_TESTER_RET_T;


typedef union
{
    HCI_LT_DATA_LENGTH_T     hci_lt_data_length;
    HCI_LT_PHY_TEST_MODE_T   hci_lt_phy_test_mode;
} HCI_QVS_COMMAND_T;

typedef struct
{
    HCI_COMMAND_COMMON_T     common;
    hci_sub_op_code_t        sub_opcode;
    HCI_QVS_COMMAND_T        command;
} HCI_ULP_QVS_BT_LOWER_TESTER_T;

/*----------------------------------------------------------------------------*
* PURPOSE
*     HCI Qualcomm Vendor Specific command OpCode = HCI_VS_WR_BT_ADDR
*
*----------------------------------------------------------------------------*/

typedef struct {
    HCI_COMMAND_COMMON_T           common;
    BD_ADDR_T                      device_address;
} HCI_VS_WR_BT_ADDR_T;

typedef struct
{
    uint8_t  status;
} HCI_VS_WR_BT_ADDR_RET_T;


typedef struct
{
    uint8_t                  status;
    hci_sub_op_code_t        sub_opcode;
} QCT_COMMAND_COMPLETE_COMMON_T;

#endif
