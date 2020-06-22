/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_handover.h
\brief      This module implements handover interface (handover_interface_t) and
            acts as an aggregator for all application components which require
            handover.
*/
#ifndef EARBUD_HANDOVER_H_
#define EARBUD_HANDOVER_H_
#ifdef INCLUDE_MIRRORING
#include <app_handover_if.h>
#include <handover_if.h>

extern const handover_interface application_handover_interface;

/*!
    \brief EarbudHandover_Init
    \param[in] init_task    Init Task handle
    \param[out] bool        Result of intialization
*/
bool EarbudHandover_Init(Task init_task);
#else
#define EarbudHandover_Init(tsk) (FALSE)
#endif
#endif // EARBUD_HANDOVER_H_
