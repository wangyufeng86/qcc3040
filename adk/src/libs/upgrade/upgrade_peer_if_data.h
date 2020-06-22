/****************************************************************************
Copyright (c) 2004 - 2018 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_host_if_data.h
    
DESCRIPTION
    Interface to the module which handles protocol message communications
    with the host.
    
    A set of functions for building and sending protocol message from the
    device to the host.
    
    A generic function for handling incoming protocol messages from the host,
    building an internal message, and passing it to the host interface client.
    Currently that client is the upgrade state machine.
*/
#ifndef UPGRADE_PEER_IF_DATA_H_
#define UPGRADE_PEER_IF_DATA_H_

#include <message.h>
#include "upgrade_msg_host.h"

/*!
    Definition the current protocol version in use.
 */
#define PROTOCOL_CURRENT_VERSION    PROTOCOL_VERSION_3

/*!
    @brief TODO
    @return void
 */
void UpgradePeerIFDataSendShortMsg(UpgradeMsgHost message);

/*!
    @brief TODO
    @return void
 */
void UpgradePeerIFDataSendSyncCfm(uint16 status, uint32 id);

/*!
    @brief TODO
    @return void
 */
void UpgradePeerIFDataSendStartCfm(uint16 status, uint16 batteryLevel);

/*!
    @brief TODO
    @return void
 */
void UpgradePeerIFDataSendBytesReq(uint32 numBytes, uint32 startOffset);

/*!
    @brief TODO
    @return void
 */
void UpgradePeerIFDataSendErrorInd(uint16 errorCode);

/*!
    @brief TODO
    @return void
 */
void UpgradePeerIFDataSendIsCsrValidDoneCfm(uint16 backOffTime);

#endif /* UPGRADE_PEER_IF_DATA_H_ */
