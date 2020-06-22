/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */


#ifndef GATT_ROLE_SELECTION_CLIENT_DEBUG_H_
#define GATT_ROLE_SELECTION_CLIENT_DEBUG_H_

#include <panic.h>

#undef DISABLE_LOG

/* Macro used to generate debug version of this library */
#ifdef GATT_ROLE_SELECTION_CLIENT_DEBUG_LIB
#undef DISABLE_LOG

#define GATT_ROLE_SELECTION_CLIENT_DEBUG_PANIC()  Panic()
#else
#define DISABLE_LOG
#define GATT_ROLE_SELECTION_CLIENT_DEBUG_PANIC()  ((void)0)
#endif

#include "logging.h"

#define GATT_ROLE_SELECTION_CLIENT_DEBUG DEBUG_LOG

#endif /* GATT_ROLE_SELECTION_CLIENT_DEBUG_H_ */
