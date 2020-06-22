/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup cvsd
 * \file  cvsd_struct.h
 * \ingroup capabilities
 *
 */

#ifndef CVSD_STRUCT_H
#define CVSD_STRUCT_H

//*****************************************************************************
//  Include Files
//*****************************************************************************
//#include "buffer/cbuffer_c.h"                 // type declaration of 'tCbuffer'

// Filter state variables
typedef struct sCvsdAp2_t
{
	int yAP1;				// State allpass 1
	int yAP2;				// State allpass 2
	int xAP1;				// input allpass 1
	int xAP2;				// input allpass 2
} sCvsdAp2_t;

// CVSD user interface data
typedef struct sCvsdUi_t
{
	int mode;									// dummy for the moment
} sCvsdUi_t;


// CVSD state variables + user interface
typedef struct sCvsdState_t
{
	// CVSD decoder state
    int                 accu;					// accu
	int                 stepsize;				// stepsize
	int					bitHistory;				// bit history
	sCvsdAp2_t			fStage[4];				// filter memory for decimation / interpolation
	sCvsdUi_t			ui;						// user interface
} sCvsdState_t;

#endif /* CVSD_STRUCT_H */
