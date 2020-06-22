// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef _ASF200_LIB_H
#define _ASF200_LIB_H

// -----------------------------------------------------------------------------
// ASF external constants
// -----------------------------------------------------------------------------
.CONST $asf200.BIN_SKIPPED          1;
.CONST $asf200.NUM_PROC             63;
.CONST $asf200.SCRATCH_SIZE_DM1     $asf200.NUM_PROC * 10;
.CONST $asf200.SCRATCH_SIZE_DM2     $asf200.NUM_PROC * 10;
.CONST $asf200.RV_SIZE.2MIC         $asf200.NUM_PROC * 2 * (1*2 + 2*1); // float * (real + complex)
.CONST $asf200.RV_SIZE.3MIC_EXTRA   $asf200.NUM_PROC * 2 * (1*1 + 2*2);

.CONST $asf200.ffv.SCRATCH_Z3       $asf200.NUM_PROC * 6 - 1;
.CONST $asf200.ffv.SCRATCH_SIZE_DM1 $asf200.NUM_PROC * 8;               // up to T2R
.CONST $asf200.ffv.SCRATCH_SIZE_DM2 $asf200.NUM_PROC * 8;               // up to T2I
.CONST $asf200.ffv.DATA1_SIZE.3MIC  $asf200.NUM_PROC * (4*2 + 3 + 2);   // Phi(1:2)(1:4) + CC(1:3) + CC_PWR(1,3)
.CONST $asf200.ffv.DATA2_SIZE.3MIC  $asf200.NUM_PROC * (4*2 + 3 + 1);   // Phi(1:2)(1:4) + CC(1:3) + CC_PWR(2)


// -----------------------------------------------------------------------------
// ASF data object structure
// -----------------------------------------------------------------------------

// (Z0)(real/imag/BExp)
.CONST $asf200.Z0_FIELD                   MK1 * 0;

// (Z1)(real/imag/BExp)
.CONST $asf200.Z1_FIELD                   MK1 * 1;

// (Z2)(real/imag/BExp)
.CONST $asf200.Z2_FIELD                   MK1 * 2;

// (Z3)(real/imag/BExp)
.CONST $asf200.Z3_FIELD                   MK1 * 3;

// Variant Flag Pointer
.CONST $asf200.VARIANT_FIELD              MK1 * 4;

// Pointer to Parameters
.CONST $asf200.PARAM_FIELD                MK1 * 5;

// Pointer to DM1 scratch block
.CONST $asf200.SCRATCH_DM1_FIELD          MK1 * 6;

// Pointer to DM2 scratch block
.CONST $asf200.SCRATCH_DM2_FIELD          MK1 * 7;

// Pointer to RV array
.CONST $asf200.PTR_RV_FIELD               MK1 * 8;

// Beam Former Data Object
.CONST $asf200.BF_OBJ_FIELD               MK1 * 9;

// Pointer to ASF const table
.CONST $asf200.MODE_FIELD                 MK1 * 10;

// .............................................................................
// Internal fields
// .............................................................................

// WNR Data Object
.CONST $asf200.WNR_OBJ_FIELD              MK1 + $asf200.MODE_FIELD;

// COH Filter Data Object
.CONST $asf200.COH_OBJ_FIELD              MK1 + $asf200.WNR_OBJ_FIELD;

// BF process function pointer
.CONST $asf200.BF_FUNC_FIELD              MK1 + $asf200.COH_OBJ_FIELD;

// BF Scratch Allocation function pointer
.CONST $asf200.SALLOC_FUNC_FIELD          MK1 + $asf200.BF_FUNC_FIELD;

// Module Bypass Flag
.CONST $asf200.BYPASS_FLAG_PREP_FIELD     MK1 + $asf200.SALLOC_FUNC_FIELD;
.CONST $asf200.BYPASS_FLAG_BF_FIELD       MK1 + $asf200.BYPASS_FLAG_PREP_FIELD;
.CONST $asf200.WIND_FIELD                 MK1 + $asf200.BYPASS_FLAG_BF_FIELD;

// Configuration/Parameter
.CONST $asf200.NUM_MIC_FIELD              MK1 + $asf200.WIND_FIELD;
.CONST $asf200.FFTLEN_FIELD               MK1 + $asf200.NUM_MIC_FIELD;
.CONST $asf200.NUM_HIGH_BAND_FIELD        MK1 + $asf200.FFTLEN_FIELD;
.CONST $asf200.ALFA_P_FIELD               MK1 + $asf200.NUM_HIGH_BAND_FIELD;

// CC limiter
.CONST $asf200.CC_LIMITER_FIELD           MK1 + $asf200.ALFA_P_FIELD;
.CONST $asf200.CC_R_MIN_FIELD             MK1 + $asf200.CC_LIMITER_FIELD;
.CONST $asf200.CC_R_MAX_FIELD             MK1 + $asf200.CC_R_MIN_FIELD;
.CONST $asf200.CC_I_MIN_FIELD             MK1 + $asf200.CC_R_MAX_FIELD;
.CONST $asf200.CC_I_MAX_FIELD             MK1 + $asf200.CC_I_MIN_FIELD;

// data pointers
.CONST $asf200.RV2R_FIELD                 MK1 + $asf200.CC_I_MAX_FIELD;
.CONST $asf200.RV2I_FIELD                 MK1 + $asf200.RV2R_FIELD;
.CONST $asf200.RV3_FIELD                  MK1 + $asf200.RV2I_FIELD;
.CONST $asf200.X0_REAL_FIELD              MK1 + $asf200.RV3_FIELD;
.CONST $asf200.X0_IMAG_FIELD              MK1 + $asf200.X0_REAL_FIELD;
.CONST $asf200.X1_REAL_FIELD              MK1 + $asf200.X0_IMAG_FIELD;
.CONST $asf200.X1_IMAG_FIELD              MK1 + $asf200.X1_REAL_FIELD;

// scratch pointer
.CONST $asf200.COH_AUX1_REAL_FIELD        MK1 + $asf200.X1_IMAG_FIELD;
.CONST $asf200.COH_AUX1_IMAG_FIELD        MK1 + $asf200.COH_AUX1_REAL_FIELD;
.CONST $asf200.COH_AUX2_REAL_FIELD        MK1 + $asf200.COH_AUX1_IMAG_FIELD;
.CONST $asf200.COH_AUX2_IMAG_FIELD        MK1 + $asf200.COH_AUX2_REAL_FIELD;
.CONST $asf200.COH_AUX3_REAL_FIELD        MK1 + $asf200.COH_AUX2_IMAG_FIELD;
.CONST $asf200.COH_AUX3_IMAG_FIELD        MK1 + $asf200.COH_AUX3_REAL_FIELD;
.CONST $asf200.Z0_REAL_FIELD              MK1 + $asf200.COH_AUX3_IMAG_FIELD;
.CONST $asf200.Z0_IMAG_FIELD              MK1 + $asf200.Z0_REAL_FIELD;
.CONST $asf200.Z1_REAL_FIELD              MK1 + $asf200.Z0_IMAG_FIELD;
.CONST $asf200.Z1_IMAG_FIELD              MK1 + $asf200.Z1_REAL_FIELD;
.CONST $asf200.Z2_REAL_FIELD              MK1 + $asf200.Z1_IMAG_FIELD;
.CONST $asf200.Z2_IMAG_FIELD              MK1 + $asf200.Z2_REAL_FIELD;
.CONST $asf200.T1_REAL_FIELD              MK1 + $asf200.Z2_IMAG_FIELD;
.CONST $asf200.T1_IMAG_FIELD              MK1 + $asf200.T1_REAL_FIELD;
.CONST $asf200.T2_REAL_FIELD              MK1 + $asf200.T1_IMAG_FIELD;
.CONST $asf200.T2_IMAG_FIELD              MK1 + $asf200.T2_REAL_FIELD;
.CONST $asf200.T3_REAL_FIELD              MK1 + $asf200.T2_IMAG_FIELD;
.CONST $asf200.T3_IMAG_FIELD              MK1 + $asf200.T3_REAL_FIELD;
.CONST $asf200.T4_REAL_FIELD              MK1 + $asf200.T3_IMAG_FIELD;
.CONST $asf200.T4_IMAG_FIELD              MK1 + $asf200.T4_REAL_FIELD;

// 3mic fields
.CONST $asf200.X2_REAL_FIELD              MK1 + $asf200.T4_IMAG_FIELD;
.CONST $asf200.X2_IMAG_FIELD              MK1 + $asf200.X2_REAL_FIELD;
.CONST $asf200.RV4R_FIELD                 MK1 + $asf200.X2_IMAG_FIELD;
.CONST $asf200.RV4I_FIELD                 MK1 + $asf200.RV4R_FIELD;
.CONST $asf200.RV5R_FIELD                 MK1 + $asf200.RV4I_FIELD;
.CONST $asf200.RV5I_FIELD                 MK1 + $asf200.RV5R_FIELD;
.CONST $asf200.RV6_FIELD                  MK1 + $asf200.RV5I_FIELD;

// ASF200 data structure size
.CONST $asf200.ASF_END_FIELD                    $asf200.RV6_FIELD;
.CONST $asf200.STRUC_SIZE                 1 + ($asf200.ASF_END_FIELD >> LOG2_ADDR_PER_WORD);



// -----------------------------------------------------------------------------
// ASF parameter structure
// -----------------------------------------------------------------------------

// Beam0 Aggrssiveness, Q1.F
.CONST $asf200.param.Beam0_Aggr                 MK1 * 0;

// Microphone distance Parameters (in meter), Q1.F
.CONST $asf200.param.Element_D                  MK1 * 1;

// Speech Degration Factor for Target Capture Beam. Range is 0.1 to 1.0, Q1.F
.CONST $asf200.param.Beam0_Beta                 MK1 * 2;

// Direction of arrival of target 0, Integer
.CONST $asf200.param.DOA0                       MK1 * 3;

// Direction of arrival of target 1, Integer
.CONST $asf200.param.DOA1                       MK1 * 4;

// Wind Gain Aggressiveness. Q1.F
// The larger the value the more noise is reduced. 0: no wind reduction.
.CONST $asf200.wnr.param.GAIN_AGGR_FIELD        MK1 * 5;

// Silence Threshold, in log2 dB. Q8.F
// Signal will be treated as non-wind silence if power is below this threshold.
.CONST $asf200.wnr.param.WNR_WIND_POW_TH_FIELD  MK1 * 6;

// WNR detection hold time in seconds, Q7.F
.CONST $asf200.wnr.param.HOLD_FIELD             MK1 * 7;

// Wind Noise Reduction Phase Threshold. Range is 0 to 1.0, Q1.F
.CONST $asf200.wnr.param.THRESH_PHASE_FIELD     MK1 * 8;

// Wind Noise Reduction Coherence Threshold. Range is 0 to 1.0, Q1.F
.CONST $asf200.wnr.param.THRESH_COHERENCE_FIELD MK1 * 9;



// -----------------------------------------------------------------------------
// ASF200 COH FILTER data object structure
// -----------------------------------------------------------------------------

// COH_G
.CONST $asf200.coh.G_FIELD                      MK1 * 0;
// COH_Cos0
.CONST $asf200.coh.COS_FIELD                    MK1 * 1;
// COH_Sin0
.CONST $asf200.coh.SIN_FIELD                    MK1 * 2;
// function pointer - process
.CONST $asf200.coh.FUNC_PROC                    MK1 * 3;

.CONST $asf200.coh.STRUC_SIZE                   1 + ($asf200.coh.FUNC_PROC >> LOG2_ADDR_PER_WORD);


// -----------------------------------------------------------------------------
// ASF200 WNR data object structure
// -----------------------------------------------------------------------------

// Pointer to wind flag variable
.CONST $asf200.wnr.WIND_FLAG_PTR                MK1 * 0;

// Power adjustment
.CONST $asf200.wnr.POWER_ADJUST_FIELD           MK1 * 1;

// WNR G array (G0/G1)
.CONST $asf200.wnr.G_FIELD                      MK1 * 2;

// WNR G1 array
.CONST $asf200.wnr.G1_FIELD                     MK1 * 3;

// function pointer - process
.CONST $asf200.wnr.FUNC_PROC                    MK1 * 4;

// WNR Object internal fields
.CONST $asf200.wnr.PHS_FACTOR_EXP_FIELD         MK1 + $asf200.wnr.FUNC_PROC;
.CONST $asf200.wnr.PHS_FACTOR_LB_FIELD          MK1 + $asf200.wnr.PHS_FACTOR_EXP_FIELD;
.CONST $asf200.wnr.PHS_FACTOR_TR_FIELD          MK1 + $asf200.wnr.PHS_FACTOR_LB_FIELD;
.CONST $asf200.wnr.PHS_FACTOR_HB_FIELD          MK1 + $asf200.wnr.PHS_FACTOR_TR_FIELD;
.CONST $asf200.wnr.MEAN_PWR_FIELD               MK1 + $asf200.wnr.PHS_FACTOR_HB_FIELD;
.CONST $asf200.wnr.MEAN_G_FIELD                 MK1 + $asf200.wnr.MEAN_PWR_FIELD;
.CONST $asf200.wnr.MEAN_COH_AUX_FIELD           MK1 + $asf200.wnr.MEAN_G_FIELD;
.CONST $asf200.wnr.COH_ATK_FIELD                MK1 + $asf200.wnr.MEAN_COH_AUX_FIELD;
.CONST $asf200.wnr.COH_DEC_FIELD                MK1 + $asf200.wnr.COH_ATK_FIELD;
.CONST $asf200.wnr.DETECT_FLAG_FIELD            MK1 + $asf200.wnr.COH_DEC_FIELD;
.CONST $asf200.wnr.COHERENCE_FIELD              MK1 + $asf200.wnr.DETECT_FLAG_FIELD;
.CONST $asf200.wnr.THRESH_SILENCE_FIELD         MK1 + $asf200.wnr.COHERENCE_FIELD;
.CONST $asf200.wnr.STRUC_SIZE                   1 + ($asf200.wnr.THRESH_SILENCE_FIELD >> LOG2_ADDR_PER_WORD);


// -----------------------------------------------------------------------------
// ASF200 BF data object structure
// -----------------------------------------------------------------------------

// function pointer - process
.CONST $asf200.bf.FUNC_INIT                     MK1 * 0;

// -----------------------------------------------------------------------------
// ASF200 BF ULA data object structure
// -----------------------------------------------------------------------------

// function pointer - process
.CONST $asf200.ula.END                          MK1 * 1;

.CONST $asf200.ula.STRUC_SIZE                   1 + ($asf200.ula.END >> LOG2_ADDR_PER_WORD);

// -----------------------------------------------------------------------------
// ASF200 BF FAR FIELD VOICE data object structure
// -----------------------------------------------------------------------------

// function pointer - process
.CONST $asf200.ffv.Z3                           MK1 * 1;
.CONST $asf200.ffv.DATA_REAL                    MK1 * 2;
.CONST $asf200.ffv.DATA_IMAG                    MK1 * 3;
.CONST $asf200.ffv.PTR_DVAD0                    MK1 * 4;
.CONST $asf200.ffv.PTR_DVAD1                    MK1 * 5;
.CONST $asf200.ffv.PTR_DVAD2                    MK1 * 6;
.CONST $asf200.ffv.PTR_DVAD3                    MK1 * 7;

// internal data
.CONST $asf200.ffv.PHI_REAL_11                  MK1 + $asf200.ffv.PTR_DVAD3;
.CONST $asf200.ffv.PHI_IMAG_11                  MK1 + $asf200.ffv.PHI_REAL_11;
.CONST $asf200.ffv.PHI_REAL_12                  MK1 + $asf200.ffv.PHI_IMAG_11;
.CONST $asf200.ffv.PHI_IMAG_12                  MK1 + $asf200.ffv.PHI_REAL_12;
.CONST $asf200.ffv.PHI_REAL_21                  MK1 + $asf200.ffv.PHI_IMAG_12;
.CONST $asf200.ffv.PHI_IMAG_21                  MK1 + $asf200.ffv.PHI_REAL_21;
.CONST $asf200.ffv.PHI_REAL_22                  MK1 + $asf200.ffv.PHI_IMAG_21;
.CONST $asf200.ffv.PHI_IMAG_22                  MK1 + $asf200.ffv.PHI_REAL_22;
.CONST $asf200.ffv.PHI_REAL_31                  MK1 + $asf200.ffv.PHI_IMAG_22;
.CONST $asf200.ffv.PHI_IMAG_31                  MK1 + $asf200.ffv.PHI_REAL_31;
.CONST $asf200.ffv.PHI_REAL_32                  MK1 + $asf200.ffv.PHI_IMAG_31;
.CONST $asf200.ffv.PHI_IMAG_32                  MK1 + $asf200.ffv.PHI_REAL_32;
.CONST $asf200.ffv.PHI_REAL_41                  MK1 + $asf200.ffv.PHI_IMAG_32;
.CONST $asf200.ffv.PHI_IMAG_41                  MK1 + $asf200.ffv.PHI_REAL_41;
.CONST $asf200.ffv.PHI_REAL_42                  MK1 + $asf200.ffv.PHI_IMAG_41;
.CONST $asf200.ffv.PHI_IMAG_42                  MK1 + $asf200.ffv.PHI_REAL_42;
.CONST $asf200.ffv.C12C23_D13_REAL              MK1 + $asf200.ffv.PHI_IMAG_42;
.CONST $asf200.ffv.C12C23_D13_IMAG              MK1 + $asf200.ffv.C12C23_D13_REAL;
.CONST $asf200.ffv.C13J12_D23_REAL              MK1 + $asf200.ffv.C12C23_D13_IMAG;
.CONST $asf200.ffv.C13J12_D23_IMAG              MK1 + $asf200.ffv.C13J12_D23_REAL;
.CONST $asf200.ffv.C13J23_D12_REAL              MK1 + $asf200.ffv.C13J12_D23_IMAG;
.CONST $asf200.ffv.C13J23_D12_IMAG              MK1 + $asf200.ffv.C13J23_D12_REAL;
.CONST $asf200.ffv.CC1_POWER                    MK1 + $asf200.ffv.C13J23_D12_IMAG;
.CONST $asf200.ffv.CC2_POWER                    MK1 + $asf200.ffv.CC1_POWER;
.CONST $asf200.ffv.CC3_POWER                    MK1 + $asf200.ffv.CC2_POWER;

// scratch
.CONST $asf200.ffv.Z3_REAL                      MK1 + $asf200.ffv.CC3_POWER;
.CONST $asf200.ffv.Z3_IMAG                      MK1 + $asf200.ffv.Z3_REAL;

// reserved
.CONST $asf200.ffv.RESERVED_0                   MK1 + $asf200.ffv.Z3_IMAG;
.CONST $asf200.ffv.STRUC_SIZE                   1 + ($asf200.ffv.RESERVED_0 >> LOG2_ADDR_PER_WORD);

#endif   // _ASF200_LIB_H
