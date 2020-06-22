/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup secure_basic_passthrough
 * \file  secure_basic_passthrough_struct.h
 * \ingroup capabilities
 *
 * Secure basic passthrough operator header file containing type definitions
 * shared between C and asm. <br>
 *
 */

#ifndef THIRD_PARTY_LICENSING_EXAMPLE_H
#define THIRD_PARTY_LICENSING_EXAMPLE_H

#include "opmgr/opmgr_for_ops.h"

#ifdef LEGACY_LICENSING
/* Setup and start license check */
extern void basic_passthrough_retrieve_license_req(OPERATOR_DATA *op_data);

/* Full license check */
extern bool secure_basic_passthrough_license_verified(OPERATOR_DATA *op_data);

#else  
typedef struct SECURE_BASIC_PASSTHROUGH_LICENSING SECURE_BASIC_PASSTHROUGH_LICENSING;

/* Setup and start license check */
extern bool basic_passthrough_retrieve_license_req(OPERATOR_DATA *op_data, CAP_ID capid);

/* Full license check */
extern bool secure_basic_passthrough_license_verified(SECURE_BASIC_PASSTHROUGH_LICENSING *lic);

/* Release and free license key */
void basic_passthrough_release_license(void);
#endif

/* Quick license check, use bit set by previous full license check */
extern bool secure_basic_passthrough_license_ok(OPERATOR_DATA *op_data);

#endif /* THIRD_PARTY_LICENSING_EXAMPLE */
