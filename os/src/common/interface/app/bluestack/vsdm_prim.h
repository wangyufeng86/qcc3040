/*!

Copyright (c) 2020 Qualcomm Technologies International, Ltd.
  %%version

\file   vsdm_prim.h

\brief  Vendor specific Device Manager

        The functionalities exposed in this file are Qualcomm proprietary.

        Vendor Specific Device Manager provides application interface to 
        perform vendor specific functionalities.
*/
#ifndef _VSDM_PRIM_H_
#define _VSDM_PRIM_H_

#include "hci.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
    \name Response/result error and status codes

    \{
*/
/*! Operation was successful */
#define VSDM_RESULT_SUCCESS                      0x0000
#define VSDM_RESULT_INVALID_PARAM                0x0001
#define VSDM_RESULT_INPROGRESS                   0x0002
#define VSDM_RESULT_FAIL                         0x0003

#define VSDM_MAX_NO_OF_COMPIDS     (4)
#define VSDM_QLM_SUPP_FET_SIZE     (16)

/*! \name Bluestack primitive segmentation and numbering

    \brief VSDM primitives occupy the number space from
    VSDM_PRIM_BASE to (VSDM_PRIM_BASE | 0x00FF).

    \{ */
#define VSDM_PRIM_DOWN           (VSDM_PRIM_BASE)
#define VSDM_PRIM_UP             (VSDM_PRIM_BASE | 0x0080)
#define VSDM_PRIM_MAX            (VSDM_PRIM_BASE | 0x00FF)

typedef enum vsdm_prim_tag
{
    /* downstream primitives */
    ENUM_VSDM_REGISTER_REQ = VSDM_PRIM_DOWN,
    ENUM_VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ,
    ENUM_VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ,
    ENUM_VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ,
    ENUM_VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ,

    /* upstream primitives */
    ENUM_VSDM_REGISTER_CFM = VSDM_PRIM_UP,
    ENUM_VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM,
    ENUM_VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM,
    ENUM_VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM,
    ENUM_VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM,
    ENUM_VSDM_QLM_CONNECTION_COMPLETE_IND,
    ENUM_VSDM_QCM_PHY_CHANGE_IND

} VSDM_PRIM_T;

/* downstream primitives */
#define VSDM_REGISTER_REQ                           ((vsdm_prim_t)ENUM_VSDM_REGISTER_REQ)
#define VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ       ((vsdm_prim_t)ENUM_VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ)
#define VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ      ((vsdm_prim_t)ENUM_VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ)
#define VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ    ((vsdm_prim_t)ENUM_VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ)
#define VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ     ((vsdm_prim_t)ENUM_VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ)

/* upstream primitives */
#define VSDM_REGISTER_CFM                       ((vsdm_prim_t)ENUM_VSDM_REGISTER_CFM)
#define VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM   ((vsdm_prim_t)ENUM_VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM)
#define VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM  ((vsdm_prim_t)ENUM_VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM)
#define VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM    ((vsdm_prim_t)ENUM_VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM)
#define VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM     ((vsdm_prim_t)ENUM_VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM)
#define VSDM_QLM_CONNECTION_COMPLETE_IND            ((vsdm_prim_t)ENUM_VSDM_QLM_CONNECTION_COMPLETE_IND)
#define VSDM_QCM_PHY_CHANGE_IND                     ((vsdm_prim_t)ENUM_VSDM_QCM_PHY_CHANGE_IND)

/*! \} */

/*! \brief Types for VSDM */
typedef uint16_t                vsdm_prim_t;
typedef uint16_t                vsdm_result_t;

/**
 * Type definition used to specify phy type.
 */
typedef uint8_t vsdm_phy_type_t;

#define PHY_TYPE_BREDR       ((vsdm_phy_type_t)0x00)
#define PHY_TYPE_QHS         ((vsdm_phy_type_t)0x01)

/**
 * Type definition used to specify source type.
 */
typedef uint8_t vsdm_source_type_t;

#define SOURCE_TYPE_LOCAL   ((vsdm_source_type_t)0x00)
#define SOURCE_TYPE_REMOTE  ((vsdm_source_type_t)0x01)

/*! \brief Register the VSDM subsystem request

    Before any VSDM operations can be performed the VSDM subsystem shall
    be registered and a destination phandle for upstream application
    primitives shall also be registered.
*/
typedef struct
{
    vsdm_prim_t         type;           /*!< Always VSDM_REGISTER_REQ */
    phandle_t           phandle;        /*!< Destination phandle */
} VSDM_REGISTER_REQ_T;

typedef struct
{
    vsdm_prim_t         type;           /*!< Always VSDM_REGISTER_CFM */
    phandle_t           phandle;        /*!< Destination phandle */
    vsdm_result_t       result;         /*!< Result code - uses VSDM_RESULT range */
} VSDM_REGISTER_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Read local supported QLM features command
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;               /* Always VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ */
    phandle_t           phandle;            /* destination phandle */
} VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Read local supported QLM features complete
 *     
 *      QLMP feature bit mask is 16 octets and is represented as follows :
 *      qlmp_supp_features[0], Bit 0 -> Split ACL (LSB)
 *      qlmp_supp_features[0], Bit 1 -> TWM eSCO
 *      qlmp_supp_features[0], Bit 2 -> eSCO DTX
 *      qlmp_supp_features[0], Bit 3 -> Reserved
 *      qlmp_supp_features[0], Bit 4 -> QHS Classic Mode including QHS-P2 packet support
 *      qlmp_supp_features[0], Bit 5 -> QHS-P3 packet support
 *      qlmp_supp_features[0], Bit 6 -> QHS-P4 packet support
 *      qlmp_supp_features[0], Bit 7 -> QHS-P5 packet support
 *      qlmp_supp_features[1], Bit 0 -> QHS-P6 packet support
 *      qlmp_supp_features[1], Bit 1 -> Real Time Soft Combining
 *      qlmp_supp_features[1], Bit 2 -> QHS Classic Mode eSCO packets without MIC
 *      qlmp_supp_features[1], Bit 3 -> QHS Classic Mode Separate ACL and eSCO Nonces
 *      qlmp_supp_features[1], Bit 4 -> ACL mirroring
 *      qlmp_supp_features[1], Bit 5 -> eSCO mirroring
 *      qlmp_supp_features[1], Bit 6 -> CSB Burst Mode
 *      qlmp_supp_features[1], Bit 7 -> Non-DM1 Encapsulated Payloads
 *      qlmp_supp_features[2], Bit 0 -> ACL Handover
 *      qlmp_supp_features[2], Bit 1 -> Reserved
 *      qlmp_supp_features[2], Bit 2 -> eSCO Handover
 *      qlmp_supp_features[2], Bit 3 -> TWM Mirroring Fast Handover
 *      qlmp_supp_features[2], Bit 4 -> 1.5 Slot QHS Packets
 *      qlmp_supp_features[2], Bit 5 -> Broadcast Relay
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;                                       /*!< Always VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM */
    phandle_t           phandle;                                    /*!< destination phandle */
    hci_return_t        status;                                     /*!< status */
    uint8_t             qlmp_supp_features[VSDM_QLM_SUPP_FET_SIZE]; /*!< QLMP supported features */
} VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Read remote supported QLM features command
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t             type;           /* Always VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ */
    phandle_t               phandle;        /* destination phandle */
    hci_connection_handle_t handle;         /* connection handle */
    BD_ADDR_T               bd_addr;        /* Bluetooth device address */
} VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Notification of remote supported QLM features
 *     
 *      QLMP feature bit mask is 16 octets and is represented as follows :
 *      qlmp_supp_features[0], Bit 0 -> Split ACL (LSB)
 *      qlmp_supp_features[0], Bit 1 -> TWM eSCO
 *      qlmp_supp_features[0], Bit 2 -> eSCO DTX
 *      qlmp_supp_features[0], Bit 3 -> Reserved
 *      qlmp_supp_features[0], Bit 4 -> QHS Classic Mode including QHS-P2 packet support
 *      qlmp_supp_features[0], Bit 5 -> QHS-P3 packet support
 *      qlmp_supp_features[0], Bit 6 -> QHS-P4 packet support
 *      qlmp_supp_features[0], Bit 7 -> QHS-P5 packet support
 *      qlmp_supp_features[1], Bit 0 -> QHS-P6 packet support
 *      qlmp_supp_features[1], Bit 1 -> Real Time Soft Combining
 *      qlmp_supp_features[1], Bit 2 -> QHS Classic Mode eSCO packets without MIC
 *      qlmp_supp_features[1], Bit 3 -> QHS Classic Mode Separate ACL and eSCO Nonces
 *      qlmp_supp_features[1], Bit 4 -> ACL mirroring
 *      qlmp_supp_features[1], Bit 5 -> eSCO mirroring
 *      qlmp_supp_features[1], Bit 6 -> CSB Burst Mode
 *      qlmp_supp_features[1], Bit 7 -> Non-DM1 Encapsulated Payloads
 *      qlmp_supp_features[2], Bit 0 -> ACL Handover
 *      qlmp_supp_features[2], Bit 1 -> Reserved
 *      qlmp_supp_features[2], Bit 2 -> eSCO Handover
 *      qlmp_supp_features[2], Bit 3 -> TWM Mirroring Fast Handover
 *      qlmp_supp_features[2], Bit 4 -> 1.5 Slot QHS Packets
 *      qlmp_supp_features[2], Bit 5 -> Broadcast Relay
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;                                        /*!< Always VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM */
    phandle_t           phandle;                                     /*!< destination phandle */
    hci_return_t        status;                                      /*!< Success or failure */
    BD_ADDR_T           bd_addr;                                     /*!< Bluetooth device address */
    uint8_t             qlmp_supp_features[VSDM_QLM_SUPP_FET_SIZE];  /*!< QLMP supported features */
} VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Indication of QLMP Connection Establishment.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t             type;      /* Always VSDM_QLM_CONNECTION_COMPLETE_IND */
    phandle_t               phandle;   /* destination phandle */
    hci_connection_handle_t handle;    /* QLM Connection handle */
    BD_ADDR_T               bd_addr;   /* Bluetooth device address */
    hci_return_t            status;    /* 0 if QLM connection completed successfully, otherwise error */
} VSDM_QLM_CONNECTION_COMPLETE_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Indication of QCM PHY change to indicate the controller has changed the
 *      the PHY used on a normal ACL connection or a mirrored ACL connection.
 *
 *----------------------------------------------------------------------------*/
 typedef struct
{
    vsdm_prim_t             type;      /*!< Always VSDM_QCM_PHY_CHANGE_IND */
    phandle_t               phandle;   /*!< destination phandle */
    hci_connection_handle_t handle;    /*!< QLM Connection handle */
    BD_ADDR_T               bd_addr;   /*!< Bluetooth device address */
    vsdm_phy_type_t         phy;       /*!< Type of phy, either BR/EDR or QHS */
    vsdm_source_type_t      source;    /*!< Type of source, either local or remote */
    hci_return_t            status;    /*!< 0 if PHY changed successfully, otherwise error */
} VSDM_QCM_PHY_CHANGE_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 * This command writes an array of compID, min_lmpVersion, and min_lmpSubVersion
 * parameters to be used by the Controller. After the LMP version sequence the
 * controller determines whether the compID, lmpVersion, and lmpSubVersion are
 * valid compared to the array written by this command. If the compID matches
 * and the lmpVersion and lmpSubVersion of the remote device is greater than
 * the values stored then the controller forces the SC_host_support LMP feature
 * bit to 'Enabled' in the LMP feature sequence. By default the controller can 
 * be configured to indicate that host does not support SC. Based on the parameters 
 * provided here, if the remote device qualifies then the controller would overide 
 * the SC bit to indicated host SC support to the remote device. This can as well 
 * be overridden for an individual device using DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ 
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t   type;                                      /* Always VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ */
    phandle_t     phandle;                                   /* destination phandle */
    uint8_t       num_compIDs;                               /* Number of compIDs */
    uint16_t      compID[VSDM_MAX_NO_OF_COMPIDS];            /* compIDs to apply host mode override values */
    uint8_t       min_lmpVersion[VSDM_MAX_NO_OF_COMPIDS];    /*!< min_lmpVersion associated with compIDs */
    uint16_t      min_lmpSubVersion[VSDM_MAX_NO_OF_COMPIDS]; /*!< min_lmpSubVersion associated with compIDs */
} VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t     type;       /* Always VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM */
    phandle_t       phandle;    /* destination phandle */
    hci_return_t    status;     /* status of write secure connections override */
} VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This command reads the array of compID, lmpVersion, and lmpSubVersion parameters
 * used by the controller to determine whether to override the SC_host_support LMP
 * feature bit.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t     type;       /* VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ */
    phandle_t       phandle;    /* destination phandle */
} VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t   type;                                      /* Always VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM */
    phandle_t     phandle;                                   /* destination phandle */
    hci_return_t  status;                                    /* status of read secure connections QC override */
    uint8_t       num_compIDs;                               /* Number of compIDs */
    uint16_t      compID[VSDM_MAX_NO_OF_COMPIDS];            /* compIDs to apply host mode override values */
    uint8_t       min_lmpVersion[VSDM_MAX_NO_OF_COMPIDS];    /*!< min_lmpVersion associated with compIDs */
    uint16_t      min_lmpSubVersion[VSDM_MAX_NO_OF_COMPIDS]; /*!< min_lmpSubVersion associated with compIDs */
} VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM_T;

/*! \brief Union of the primitives */
typedef union
{
    /* Shared */
    vsdm_prim_t                        type;

    /* Downstream */
    VSDM_REGISTER_REQ_T                          vsdm_register_req;
    VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ_T      vsdm_read_local_qlm_supp_features_req;
    VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ_T     vsdm_read_remote_qlm_supp_features_req;
    VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ_T       vsdm_write_sc_host_supp_override_req;
    VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ_T        vsdm_read_sc_host_supp_override_req;

    /* Upstream */
    VSDM_REGISTER_CFM_T                          vsdm_register_cfm;
    VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM_T      vsdm_read_local_qlm_supp_features_cfm;
    VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM_T     vsdm_read_remote_qlm_supp_features_cfm;
    VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM_T       vsdm_write_sc_host_supp_override_cfm;
    VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM_T        vsdm_read_sc_host_supp_override_cfm;
    VSDM_QLM_CONNECTION_COMPLETE_IND_T           vsdm_qlm_connection_complete_ind;
    VSDM_QCM_PHY_CHANGE_IND_T                    vsdm_qcm_phy_change_ind;

} VSDM_UPRIM_T;

#ifdef __cplusplus
}
#endif

#endif /* _VSDM_PRIM_H_ */
