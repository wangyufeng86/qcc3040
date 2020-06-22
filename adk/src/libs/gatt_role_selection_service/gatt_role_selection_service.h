/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file

@brief  Header file for the GATT role selection service

        This file provides documentation for types that are used for the 
        service and so common to the client and the server.
*/

#ifndef GATT_ROLE_SELECTION_SERVICE_H_
#define GATT_ROLE_SELECTION_SERVICE_H_

#include <csrtypes.h>
#include <message.h>

#include <library.h>

#include "gatt_manager.h"


/*! Mirroring states known by the Role Selection Service */
typedef enum
{
    GrssMirrorStateIdle,
    GrssMirrorStateActingPrimary,
    GrssMirrorStateCandidate,
    GrssMirrorStateConnectedPrimary,
    GrssMirrorStateRecoverPhone,
    GrssMirrorStateRecoverPrimary,
    GrssMirrorStateRecoverSecondary,
    GrssMirrorStatePrimary,
    GrssMirrorStateSecondary,
    GrssMirrorStateWannabePrimary,
    GrssMirrorStateTwsPlus,

        /*! Internal state that should never be sent over GATT */
    GrssMirrorStateUnknown,
} GattRoleSelectionServiceMirroringState;


/*! Type for the figure of merit used by the role selection 

    \note A score of 0 is considered invalid. #GRSS_FIGURE_OF_MERIT_INVALID
*/
typedef uint16 grss_figure_of_merit_t;

#define GRSS_FIGURE_OF_MERIT_INVALID 0


/*! Opcodes for the control point of the service. */
typedef enum
{
    GrssOpcodeBecomePrimary = 1,
    GrssOpcodeBecomeSecondary,
} GattRoleSelectionServiceControlOpCode;


    /* Definitions for the Mirroring State PDU */
/*! Size of the mirror state PDU returned */
#define GRSS_SIZE_MIRROR_STATE_PDU_OCTETS       1

    /* Definitions for the Figure Of Merit PDU */
/*! Size of the figure of merit PDU returned */
#define GRSS_SIZE_FIGURE_OF_MERIT_PDU_OCTETS    2


    /* Definitions for the Control PDUs */

#define GRSS_CONTROL_PRI_SEC_OFFSET_OPCODE      0

/*! Size of the command to transition to primary or secondary */
#define GRSS_SIZE_CONTROL_PRI_SEC_PDU_OCTETS    1


/*! Size of the message setting the client configuration (for notfications) */
#define GRSS_CLIENT_CONFIG_OCTET_SIZE           2


#endif /* GATT_ROLE_SELECTION_SERVICE_H_ */
