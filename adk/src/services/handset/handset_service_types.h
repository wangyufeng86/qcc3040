/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Handset service types to be used within handset_service only
*/

#ifndef HANDSET_SERVICE_TYPES_H_
#define HANDSET_SERVICE_TYPES_H_

#include <bdaddr.h>
#include <logging.h>
#include <task_list.h>

#include "handset_service.h"
#include "handset_service_sm.h"


/*! \{
    Macros for diagnostic output that can be suppressed.
*/
#define HS_LOG         DEBUG_LOG
/*! \} */

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x) PanicFalse(x)


/*! \brief Internal messages for the handset_service */
typedef enum
{
    /*! Request to connect to a handset */
    HANDSET_SERVICE_INTERNAL_CONNECT_REQ = 0,

    /*! Request to disconnect a handset */
    HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ,

    /*! Delivered when an ACL connect request has completed. */
    HANDSET_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE,
} handset_service_internal_msg_t;

typedef struct
{
    /* Address of handset to connect. */
    bdaddr addr;

    /* Mask of profile(s) to connect. */
    handset_service_profile_mask_t profiles;
} HANDSET_SERVICE_INTERNAL_CONNECT_REQ_T;

typedef struct
{
    /* Address of handset to disconnect. */
    bdaddr addr;
} HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ_T;


#endif /* HANDSET_SERVICE_TYPES_H_ */