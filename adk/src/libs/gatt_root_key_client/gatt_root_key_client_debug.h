/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */


#ifndef GATT_ROOT_KEY_CLIENT_DEBUG_H_
#define GATT_ROOT_KEY_CLIENT_DEBUG_H_

/* Macro used to generate debug version of this library */
#ifdef GATT_ROOT_KEY_CLIENT_DEBUG_LIB
#undef DISABLE_LOG
#else
#define DISABLE_LOG
#endif

#include "logging.h"

#define GATT_ROOT_KEY_CLIENT_DEBUG DEBUG_LOG

#endif /* GATT_ROOT_KEY_CLIENT_DEBUG_H_ */
