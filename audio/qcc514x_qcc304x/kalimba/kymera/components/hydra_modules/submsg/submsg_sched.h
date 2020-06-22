/*****************************************************************************
*
* Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/****************************************************************************
FILE
   submsg_sched.h  -  scheduler header for submsg config

LEGAL
   CONFIDENTIAL
   Copyright (C) Cambridge Silicon Radio Ltd 2010-2010. All rights reserved.

CONTAINS

DESCRIPTION
   Header file for submsg background
*/

#ifndef SUBMSG_SCHED_H
#define SUBMSG_SCHED_H

#define SUBMSG_SCHED_TASK(m)

#define SUBMSG_BG_INT(m)                        \
    BG_INT(m, (submsg_tx, submsg_tx_bg))        \
    BG_INT(m, (submsg_rx, submsg_rx_bg))

#endif /* SUBMSG_SCHED_H */
