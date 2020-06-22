/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.

@file    gatt_proxy_stream.h
@brief   Proxy data from client to server using streams
*/

#ifndef GATT_PROXY_STREAM_
#define GATT_PROXY_STREAM_

#include <csrtypes.h>

struct gatt_proxy_stream_tag;

typedef struct gatt_proxy_stream_tag* gatt_proxy_stream_t;

/*!
    @brief Deinit gatt_proxy_stream library
*/
void GattProxyStreamReset(void);

/*!
    @brief Create a stream to proxy notifications from client handle to server handle
*/
gatt_proxy_stream_t GattProxyStreamNew(uint16 cid, uint16 client_handle, uint16 server_handle);

/*!
    @brief Destroy a proxy stream
*/
void GattProxyStreamDestroy(gatt_proxy_stream_t proxy);

#endif /* GATT_PROXY_STREAM_ */
