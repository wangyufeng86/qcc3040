/*!
    \copyright Copyright (c) 2019 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version 
    \file
    \brief The state_proxy marshal type declarations and handover header file.
*/

#ifndef _STATE_PROXY_HANDOVER_H__
#define _STATE_PROXY_HANDOVER_H__

#include <csrtypes.h>
#include <marshal_common.h>
#include <app/marshal/marshal_if.h>

extern const marshal_type_descriptor_t marshal_type_descriptor_state_proxy_task_data_t;


#define STATE_PROXY_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(state_proxy_task_data_t)

#endif /* _STATE_PROXY_HANDOVER_H__ */

