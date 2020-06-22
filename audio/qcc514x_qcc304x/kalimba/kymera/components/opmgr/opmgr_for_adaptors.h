/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_for_adaptors.h
 * \ingroup opmgr
 *
 * Operator Manager header file used by adaptor(s). <br>
 *
 */

#ifndef _OPMGR_FOR_ADAPTORS_H_
#define _OPMGR_FOR_ADAPTORS_H_

#include "types.h"
#include "cap_id_prim.h"

extern bool opmgr_get_max_sources_and_sinks_for_cap_id(CAP_ID capid, uint16 *max_sources, uint16 *max_sinks);

#ifdef PROFILER_ON
/**
 * Function to get the operator mips usage.
 * @param op_id Operatora ID
 * @return Returns the operator mips usage in permille. -1 if the operator doesn't exist.
 */
extern unsigned opmgr_get_operator_mips(unsigned op_id);
#endif /* PROFILER_ON */

#endif /* _OPMGR_FOR_ADAPTORS_H_ */

