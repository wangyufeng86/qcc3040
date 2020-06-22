/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_connection.h
\brief      Handling of connection manager events in the state proxy.
*/

#ifndef STATE_PROXY_CONNECTION_H
#define STATE_PROXY_CONNECTION_H

/* framework includes */
#include <connection_manager.h>

/*! \brief Get connection for initial state message. */
void stateProxy_GetInitialConnectionState(void);

/*! \brief Handle local connection events.
    \param[in] ind Connection indication event message.
*/
void stateProxy_HandleConManagerConnectInd(const CON_MANAGER_TP_CONNECT_IND_T* ind);

/*! \brief Handle local disconnection events.
    \param[in] ind Disconnection indication event message.
*/
void stateProxy_HandleConManagerDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T* ind);

/*! \brief Handle remote connection events.
    \param[in] ind Connection indication event message.
*/
void stateProxy_HandleRemoteConManagerConnectInd(const CON_MANAGER_TP_CONNECT_IND_T* ind);

/*! \brief Handle remote disconnection events.
    \param[in] ind Disconnection indication event message.
*/
void stateProxy_HandleRemoteConManagerDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T* ind);

/*! \brief Get the connection instance.
    \param data The local/remote state proxy data.
    \param bd_addr The bdaddr for which to get the connection instance.
*/
state_proxy_connection_t* stateProxy_GetConnection(state_proxy_data_t *data, const tp_bdaddr *bd_addr);

/*! \brief Get the peer connection instance.
    \param data The local/remote state proxy data.
    \return The peer connection.
*/
state_proxy_connection_t* stateProxy_GetPeerConnection(state_proxy_data_t* data);

/*! \brief Get the first empty connection instance.
    \param data The local/remote state proxy data.
*/
state_proxy_connection_t* stateProxy_GetEmptyConnection(state_proxy_data_t *data);

/*! \brief Query is there are any local connections */
bool stateProxy_AnyLocalConnections(void);

/*! \brief Update the RSSI for a connection.
    \param data The local/remote state proxy data.
    \param bd_addr The bdaddr for which to update the RSSI.
    \param rssi The new RSSI value.
    \return TRUE if the RSSI was updated. */
bool stateProxy_ConnectionUpdateRssi(state_proxy_data_t *data, const tp_bdaddr *bd_addr, int8 rssi);

/*! \brief Update the link quality for a connection.
    \param data The local/remote state proxy data.
    \param bd_addr The bdaddr for which to update the link quality.
    \param link_quality The new link quality value.
    \return TRUE if the link quality was updated. */
bool stateProxy_ConnectionUpdateLinkQuality(state_proxy_data_t *data, const tp_bdaddr *bd_addr, uint8 link_quality);

#endif /* STATE_PROXY_CONNECTION_H */
