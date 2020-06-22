/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file kip_msg_adaptor_for_kip_mgr.h
 * \ingroup kip
 *
 * Private interface shared between the KIP adaptor and the KIP manager.
 */

#ifndef KIP_MSG_ADAPTOR_FOR_KIP_MGR_H
#define KIP_MSG_ADAPTOR_FOR_KIP_MGR_H

#include "types.h"
#include "ipc/ipc_kip.h"

/**
 * \brief  Helper function for sending variable length key-value system messages to P1
 *
 * \return True/False (see kip_adaptor_send_system_key_value_pairs).
 */
extern bool kip_send_system_key_value_pairs_to_adaptor(void);

/*
 * Functions registered as received message notification handler.
 * One for P0, to receive KIP response messages from secondary processor.
 * One for Px, to receive KIP request messages from P0.
 */
extern void kip_adaptor_msg_handler_p0(uint16 channel_id, ipc_msg* msg);
extern void kip_adaptor_msg_handler_px(uint16 channel_id, ipc_msg* msg);

#endif /* KIP_MSG_ADAPTOR_FOR_KIP_MGR_H */
