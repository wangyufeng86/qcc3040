// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
//
// %%version
//
// *****************************************************************************

#ifndef AEC520_LIB_H_INCLUDED
#define AEC520_LIB_H_INCLUDED

// *****************************************************************************
// AEC algorithm matlab version 510
//
// VERSION HISTORY:
//    1.0.0 - initial version aec500
//    2.0.0 - aec510: run-time HS/HF/CE configurable
//    3.0.0 - aec510: hdv
//    3.1.0 - aec520: hdv + va
// *****************************************************************************

// -----------------------------------------------------------------------------
// AEC520 external user constants
// -----------------------------------------------------------------------------
#define $aec520.HF_FLAG_HS    0
#define $aec520.HF_FLAG_HF    1
#define $aec520.HF_FLAG_CE    2
#define $aec520.HF_FLAG_VA    3

.CONST $aec520.Num_Primary_Taps              2;
.CONST $aec520.Num_Auxillary_Taps            0;

.CONST $aec520_HF.Num_Auxillary_Taps         3;
.CONST $aec520_HF.Num_Primary_Taps           8;

.CONST $aec520_HF_UWB.Num_Auxillary_Taps     4;
.CONST $aec520_HF_UWB.Num_Primary_Taps       12;
.CONST $aec520.HS_UWB.Num_Primary_Taps       3;

.CONST $aec520.RER_DIM                       64;
.CONST $aec520.RER_DIM.UWB                   43;

// -----------------------------------------------------------------------------
// AEC520 user parameter structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECPARAMETEROBJECT

// @DOC_FIELD_TEXT Reference delay in seconds
// @DOC_FIELD_FORMAT Q7.17 (arch4: Q7.25)
.CONST $aec520.Parameter.REF_DELAY                    0*ADDR_PER_WORD;
// @DOC_FIELD_TEXT AEC tail-length in seconds
// @DOC_FIELD_FORMAT Q7.17 (arch4: Q7.25)
.CONST $aec520.Parameter.TAIL_LENGTH                  1*ADDR_PER_WORD;
// @DOC_FIELD_TEXT CND gain, default 1.0 in Q3.21 (CVC parameter)
// @DOC_FIELD_FORMAT Q3.21 (arch4: Q3.29)
.CONST $aec520.Parameter.OFFSET_CNG_Q_ADJUST          2*ADDR_PER_WORD;
// @DOC_FIELD_TEXT, Comfort noise color selection -1=wht,0=brn,1=pnk,2=blu,3=pur (CVC parameter)
// @DOC_FIELD_FORMAT Flag
.CONST $aec520.Parameter.OFFSET_CNG_NOISE_COLOR       3*ADDR_PER_WORD;
// @DOC_FIELD_TEXT DTC aggressiveness, default 0.5 (Q1.23) (CVC parameter)
// @DOC_FIELD_FORMAT Q1.23 (arch4: Q1.31)
.CONST $aec520.Parameter.OFFSET_DTC_AGRESSIVENESS     4*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Maximum Power Margin, default 2.5 in (Q8.16) (CVC parameter for handsfree)
// @DOC_FIELD_FORMAT Q8.16 (arch4: Q8.24)
.CONST $aec520.Parameter.OFFSET_MAX_LPWR_MARGIN       5*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Flag for repeating AEC filtering
// @DOC_FIELD_TEXT Set to '1' for handsfree
// @DOC_FIELD_TEXT Set to '0' for headset (CVC parameter)
// @DOC_FIELD_FORMAT Flag
.CONST $aec520.Parameter.OFFSET_ENABLE_AEC_REUSE      6*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Reference Power Threshold. Default set to '$aec520.AEC_L2Px_HB' (Q8.16)
// @DOC_FIELD_TEXT CVC parameter for handsfree
// @DOC_FIELD_FORMAT Q8.16 (arch4: Q8.24)
.CONST $aec520.Parameter.OFFSET_AEC_REF_LPWR_HB       7*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Handsfree only. RER Adaptation. Default 0 (Handsfree CVC parameter)
// @DOC_FIELD_FORMAT Q1.23 (arch4: Q1.31)
.CONST $aec520.Parameter.OFFSET_RER_ADAPT             8*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Handsfree only. RER aggresiveness. Default 0x200000 (Q6.18) (Handsfree CVC parameter)
// @DOC_FIELD_FORMAT Q6.18 (arch4: Q6.26)
.CONST $aec520.Parameter.OFFSET_RER_AGGRESSIVENESS    9*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Handsfree only. RER power. Default 2 (Handsfree CVC parameter)
// @DOC_FIELD_FORMAT Integer
.CONST $aec520.Parameter.OFFSET_RER_POWER             10*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Threshold for DTC decision
// @DOC_FIELD_FORMAT Q8.16
.CONST $aec520.Parameter.OFFSET_L2TH_RERDT_OFF        11*ADDR_PER_WORD;
// @DOC_FIELD_TEXT RERDT aggressiveness
// @DOC_FIELD_FORMAT Q6.18
.CONST $aec520.Parameter.OFFSET_RERDT_ADJUST          12*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Handsfree only. RERDT power.
// @DOC_FIELD_FORMAT Integer
.CONST $aec520.Parameter.OFFSET_RERDT_POWER           13*ADDR_PER_WORD;
// AEC LMS filter upper frequency
.CONST $aec520.Parameter.OFFSET_LMS_FREQ              14*ADDR_PER_WORD;
// HDV mode gain control, Integer: 0 - 5
.CONST $aec520.Parameter.OFFSET_HDV_GAIN_CNTRL        15*ADDR_PER_WORD;
// Multi-channel AEC LRM mode, Integer, 0: LRM off, 1: LRM on
.CONST $aec520.Parameter.OFFSET_AEC_LRM_MODE          16*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Pre-AEC noise estimator control
// @DOC_FIELD_FORMAT Q1.N
.CONST $aec520.Parameter.OFFSET_AEC_NS_CNTRL          17*ADDR_PER_WORD;

// @END  DATA_OBJECT AECPARAMETEROBJECT


// -----------------------------------------------------------------------------
// AEC520 data object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECDATAOBJECT

// @DOC_FIELD_TEXT AEC table:
// @DOC_FIELD_TEXT    - $aec520.const     (NB/WB/SWB)
// @DOC_FIELD_TEXT    - $aec520.const_uwb (UWB/FB)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.PTR_CONST_FIELD               0*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC Parameters
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.PARAM_FIELD                   1*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to Variant Flag
// @DOC_FIELD_FORMAT Flag Pointer
.CONST $aec520.VARIANT_FIELD                 2*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Maximum Primary Filter Taps
// @DOC_FIELD_FORMAT Integer
.CONST $aec520.MAX_FILTER_LENGTH_FIELD       3*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Handsfree flag
// @DOC_FIELD_TEXT Headset use case if cleared
// @DOC_FIELD_TEXT Handsfree use case if set
// @DOC_FIELD_FORMAT Flag
.CONST $aec520.HF_FLAG_FIELD                 4*ADDR_PER_WORD;

// @DOC_FIELD_TEXT DTC Enhancement Flag
// @DOC_FIELD_FORMAT Integer
.CONST $aec520.FLAG_DTC_ENH                  5*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to pre-AEC OMS G
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.OMS_G_FIELD                   6*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to pre-AEC OMS MS_LpN
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.OMS_LPN_FIELD                 7*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC reference input stream
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.X_STREAM_FIELD                8*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC reference delayed stream
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.X_STREAM_DELAY_FIELD          9*ADDR_PER_WORD;


// @DOC_FIELD_TEXT Pointer to AEC reference channel X (real/imag/BExp)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.X_FIELD                       10*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to real part of receive buffer X_buf, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.XBUF_REAL_FIELD               11*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to imaginary part of receive buffer X_buf, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.XBUF_IMAG_FIELD               12*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to BExp_X_buf (internal array, permanant), size of 'Num_Primary_Taps+1'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.XBUF_BEXP_FIELD               13*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to left channel FBC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.PTR_FBC_OBJ_FIELD             14*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC (left) channel D (real/imag/BExp)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.D_FIELD                       15*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to real part of Ga, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.GA_REAL_FIELD                 16*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to imaginary part of Ga, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.GA_IMAG_FIELD                 17*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to BExp_Ga (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.GA_BEXP_FIELD                 18*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to second channel AEC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.DM_OBJ_FIELD                  19*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to LPwrX0 (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.LPWRX0_FIELD                  20*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to LPwrX1 (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.LPWRX1_FIELD                  21*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to RatFE (internal array, permanant), size of RER_dim
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.RATFE_FIELD                   22*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to imaginary part of Gr (RER internal complex array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.RER_GR_IMAG_FIELD             23*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to real part of Gr (RER internal complex array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.RER_GR_REAL_FIELD             24*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to SqGr (RER internal real array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.RER_SQGR_FIELD                25*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to L2absGr (RER internal real array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.RER_L2ABSGR_FIELD             26*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to LPwrD (RER internal real array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.RER_LPWRD_FIELD               27*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to LpZ_nz (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.CNG_LPZNZ_FIELD               28*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to currently selected CNG noise shaping table (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer Q2.22
.CONST $aec520.CNG_CUR_NZ_TABLE_FIELD        29*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to L_adaptA (internal array, scratch in DM1), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.SCRPTR_LADAPTA_FIELD          30*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to a scratch memory in DM2 with size of '2*$M.CVC.Num_FFT_Freq_Bins + 1'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.SCRPTR_EXP_MTS_ADAPT_FIELD    31*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to Attenuation (internal array, scratch in DM1), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.SCRPTR_ATTENUATION_FIELD      32*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to W_ri (RER internal interleaved complex array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.SCRPTR_W_RI_FIELD             33*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to L_adaptR (RER internal real array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.SCRPTR_LADAPTR_FIELD          34*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to DTC_lin array, size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.SCRPTR_DTC_LIN_FIELD          35*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Scratch pointer to channel structure T
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.SCRPTR_T_FIELD                36*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Scratch pointer to channel structure ET
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.ET_FIELD                      37*ADDR_PER_WORD;

// @DOC_FIELD_TEXT DTC status array for each frequency bins, scratch
// @DOC_FIELD_TEXT Size of Number of FFT bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.SCRPTR_RERDT_DTC_FIELD        38*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to real part of Gb, size of 'RER_dim*Num_Auxillary_Taps'
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.GB_REAL_FIELD                 39*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to imaginary part of Gb, size of 'RER_dim*Num_Auxillary_Taps'
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.GB_IMAG_FIELD                 40*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to BExp_Gb (internal array, permanant), size of RER_dim
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.GB_BEXP_FIELD                 41*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to L_RatSqG (internal array, permanant), size of RER_dim
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.L_RATSQG_FIELD                42*ADDR_PER_WORD;

// Internal AEC Data - Variables

// Time
.CONST $aec520.FRAME_SIZE_FIELD              $aec520.L_RATSQG_FIELD + ADDR_PER_WORD;
.CONST $aec520.REF_DELAY_FIELD               $aec520.FRAME_SIZE_FIELD + ADDR_PER_WORD;
.CONST $aec520.TAIL_LENGTH                   $aec520.REF_DELAY_FIELD + ADDR_PER_WORD;

// Frequency
.CONST $aec520.LMS_MAX_FREQ                  $aec520.TAIL_LENGTH + ADDR_PER_WORD;

// PREP
.CONST $aec520.G_OMS_IN2_FIELD               $aec520.LMS_MAX_FREQ + ADDR_PER_WORD;
.CONST $aec520.L2PXT_FIELD                   $aec520.G_OMS_IN2_FIELD + ADDR_PER_WORD;
.CONST $aec520.L2PDT_FIELD                   $aec520.L2PXT_FIELD + ADDR_PER_WORD;

// DTC
.CONST $aec520.L_MUA_FIELD                   $aec520.L2PDT_FIELD + ADDR_PER_WORD;
.CONST $aec520.L_MUB_FIELD                   $aec520.L_MUA_FIELD + ADDR_PER_WORD;
.CONST $aec520.L_DTC_HFREQ_FEF_FIELD         $aec520.L_MUB_FIELD + ADDR_PER_WORD;
.CONST $aec520.DTC_AVG_FIELD                 $aec520.L_DTC_HFREQ_FEF_FIELD + ADDR_PER_WORD;
.CONST $aec520.DTC_PROB_FIELD                $aec520.DTC_AVG_FIELD + ADDR_PER_WORD;
.CONST $aec520.DTC_AVGRFE_FIELD              $aec520.DTC_PROB_FIELD + ADDR_PER_WORD;
.CONST $aec520.DTC_STDRFE_FIELD              $aec520.DTC_AVGRFE_FIELD + ADDR_PER_WORD;
.CONST $aec520.mn_L_RatSqGt                  $aec520.DTC_STDRFE_FIELD + ADDR_PER_WORD;

.CONST $aec520.OFFSET_L_RatSqG               $aec520.mn_L_RatSqGt + ADDR_PER_WORD;
.CONST $aec520.OFFSET_dL2PxFB                $aec520.OFFSET_L_RatSqG + ADDR_PER_WORD;
.CONST $aec520.OFFSET_L2Pxt0                 $aec520.OFFSET_dL2PxFB + ADDR_PER_WORD;
.CONST $aec520.DTC_dLpX                      $aec520.OFFSET_L2Pxt0 + ADDR_PER_WORD;
.CONST $aec520.DTC_LpXt_curr                 $aec520.DTC_dLpX + ADDR_PER_WORD;
.CONST $aec520.DTC_LpXt_prev                 $aec520.DTC_LpXt_curr + ADDR_PER_WORD;

.CONST $aec520.OFFSET_tt_dtc                 $aec520.DTC_LpXt_prev + ADDR_PER_WORD;
.CONST $aec520.OFFSET_ct_init                $aec520.OFFSET_tt_dtc + ADDR_PER_WORD;
.CONST $aec520.OFFSET_ct_Px                  $aec520.OFFSET_ct_init + ADDR_PER_WORD;
.CONST $aec520.OFFSET_tt_cng                 $aec520.OFFSET_ct_Px + ADDR_PER_WORD;

// RERDT
.CONST $aec520.OFFSET_LPXFB_RERDT            $aec520.OFFSET_tt_cng + ADDR_PER_WORD;
.CONST $aec520.RERDT_DTC_ACTIVE_FIELD        $aec520.OFFSET_LPXFB_RERDT + ADDR_PER_WORD;

// RER variables
.CONST $aec520.RER_AGGR_FIELD                $aec520.RERDT_DTC_ACTIVE_FIELD + ADDR_PER_WORD;
.CONST $aec520.RER_BEXP_FIELD                $aec520.RER_AGGR_FIELD + ADDR_PER_WORD;
.CONST $aec520.RER_E_FIELD                   $aec520.RER_BEXP_FIELD + ADDR_PER_WORD;
.CONST $aec520.RER_L2PET_FIELD               $aec520.RER_E_FIELD + ADDR_PER_WORD;
.CONST $aec520.RER_SQGR_REVERED_FIELD        $aec520.RER_L2PET_FIELD + ADDR_PER_WORD;
.CONST $aec520.RER_HBGAIN_FIELD              $aec520.RER_SQGR_REVERED_FIELD + ADDR_PER_WORD;
.CONST $aec520.OFFSET_PXRS_FIELD             $aec520.RER_HBGAIN_FIELD + ADDR_PER_WORD;
.CONST $aec520.OFFSET_PXRD_FIELD             $aec520.OFFSET_PXRS_FIELD + ADDR_PER_WORD;
.CONST $aec520.OFFSET_PDRS_FIELD             $aec520.OFFSET_PXRD_FIELD + ADDR_PER_WORD;
.CONST $aec520.OFFSET_PDRD_FIELD             $aec520.OFFSET_PDRS_FIELD + ADDR_PER_WORD;
.CONST $aec520.GROFFSET_FIELD                $aec520.OFFSET_PDRD_FIELD + ADDR_PER_WORD;

// CNG
.CONST $aec520.OFFSET_OMS_AGGRESSIVENESS     $aec520.GROFFSET_FIELD + ADDR_PER_WORD;
.CONST $aec520.OFFSET_CNG_offset             $aec520.OFFSET_OMS_AGGRESSIVENESS + ADDR_PER_WORD;
.CONST $aec520.CNG_OMS_G_FIELD               $aec520.OFFSET_CNG_offset + ADDR_PER_WORD;
        
// HD
.CONST $aec520.OFFSET_AEC_COUPLING           $aec520.CNG_OMS_G_FIELD + ADDR_PER_WORD;
.CONST $aec520.OFFSET_HD_L_AECgain           $aec520.OFFSET_AEC_COUPLING + ADDR_PER_WORD;

// scratch variables
.CONST $aec520.LPWRX_MARGIN_FIELD            $aec520.OFFSET_HD_L_AECgain + ADDR_PER_WORD;
.CONST $aec520.MN_PWRX_DIFF_FIELD            $aec520.LPWRX_MARGIN_FIELD + ADDR_PER_WORD;
.CONST $aec520.OFFSET_TEMP_FIELD             $aec520.MN_PWRX_DIFF_FIELD + ADDR_PER_WORD;

// NB/WB/UWB/SWB/FB
.CONST $aec520.OFFSET_NUM_FREQ_BINS          $aec520.OFFSET_TEMP_FIELD + ADDR_PER_WORD;
.CONST $aec520.LMS_dim                       $aec520.OFFSET_NUM_FREQ_BINS + ADDR_PER_WORD;
.CONST $aec520.RER_dim                       $aec520.LMS_dim + ADDR_PER_WORD;
.CONST $aec520.AEC_hold_dL2Px                $aec520.RER_dim + ADDR_PER_WORD;
.CONST $aec520.AEC_hold_adapt                $aec520.AEC_hold_dL2Px + ADDR_PER_WORD;
.CONST $aec520.AEC_kL_FBpwr                  $aec520.AEC_hold_adapt + ADDR_PER_WORD;
.CONST $aec520.AEC_kH_FBpwr                  $aec520.AEC_kL_FBpwr + ADDR_PER_WORD;
.CONST $aec520.DTC_kL_ref                    $aec520.AEC_kH_FBpwr + ADDR_PER_WORD;
.CONST $aec520.DTC_kH_ref                    $aec520.DTC_kL_ref + ADDR_PER_WORD;
.CONST $aec520.DTC_scale_dLpX                $aec520.DTC_kH_ref + ADDR_PER_WORD;
.CONST $aec520.L_dtc                         $aec520.DTC_scale_dLpX + ADDR_PER_WORD;
.CONST $aec520.alfaSQG                       $aec520.L_dtc + ADDR_PER_WORD;
.CONST $aec520.kL_rerdt                      $aec520.alfaSQG + ADDR_PER_WORD;
.CONST $aec520.kH_rerdt                      $aec520.kL_rerdt + ADDR_PER_WORD;
.CONST $aec520.alfa_rerdt                    $aec520.kH_rerdt + ADDR_PER_WORD;
.CONST $aec520.kL_GrTilt                     $aec520.alfa_rerdt + ADDR_PER_WORD;
.CONST $aec520.kH_GrTilt                     $aec520.kL_GrTilt + ADDR_PER_WORD;
.CONST $aec520.RER_kL_ref                    $aec520.kH_GrTilt + ADDR_PER_WORD;
.CONST $aec520.RER_kH_ref                    $aec520.RER_kL_ref + ADDR_PER_WORD;
.CONST $aec520.alfaL2P_Rs                    $aec520.RER_kH_ref + ADDR_PER_WORD;
.CONST $aec520.inv_L_L2P_Rd                  $aec520.alfaL2P_Rs + ADDR_PER_WORD;
.CONST $aec520.alfaCNG                       $aec520.inv_L_L2P_Rd + ADDR_PER_WORD;
.CONST $aec520.LB_cng                        $aec520.alfaCNG + ADDR_PER_WORD;
.CONST $aec520.HB_cng                        $aec520.LB_cng + ADDR_PER_WORD;
.CONST $aec520.HD_kL_Gain                    $aec520.HB_cng + ADDR_PER_WORD;
.CONST $aec520.HD_kH_Gain                    $aec520.HD_kL_Gain + ADDR_PER_WORD;
.CONST $aec520.HD_alfa_Gain                  $aec520.HD_kH_Gain + ADDR_PER_WORD;

// HS/HF
.CONST $aec520.OFFSET_NUM_PRIMARY_TAPS       $aec520.HD_alfa_Gain + ADDR_PER_WORD;
.CONST $aec520.OFFSET_NUM_AUXILLARY_TAPS     $aec520.OFFSET_NUM_PRIMARY_TAPS + ADDR_PER_WORD;
.CONST $aec520.OFFSET_AEC_L_MUA_ON           $aec520.OFFSET_NUM_AUXILLARY_TAPS + ADDR_PER_WORD;
.CONST $aec520.OFFSET_AEC_L_MUB_ON           $aec520.OFFSET_AEC_L_MUA_ON + ADDR_PER_WORD;
.CONST $aec520.OFFSET_AEC_ALFA_A             $aec520.OFFSET_AEC_L_MUB_ON + ADDR_PER_WORD;
.CONST $aec520.OFFSET_AEC_L_ALFA_A           $aec520.OFFSET_AEC_ALFA_A + ADDR_PER_WORD;

// Sub-module control
.CONST $aec520.FLAG_AEC_HDMODE_FIELD         $aec520.OFFSET_AEC_L_ALFA_A + ADDR_PER_WORD;
.CONST $aec520.FLAG_BYPASS_CNG_FIELD         $aec520.FLAG_AEC_HDMODE_FIELD    + ADDR_PER_WORD;
.CONST $aec520.FLAG_BYPASS_RER_FIELD         $aec520.FLAG_BYPASS_CNG_FIELD + ADDR_PER_WORD;
.CONST $aec520.FLAG_BYPASS_RERDT_FIELD       $aec520.FLAG_BYPASS_RER_FIELD + ADDR_PER_WORD;
.CONST $aec520.FLAG_BYPASS_FBC_FIELD         $aec520.FLAG_BYPASS_RERDT_FIELD + ADDR_PER_WORD;

.CONST $aec520.STRUCT_SIZE                  ($aec520.FLAG_BYPASS_FBC_FIELD >> LOG2_ADDR_PER_WORD) + 1;

// @END  DATA_OBJECT AECDATAOBJECT


// -----------------------------------------------------------------------------
// AEC520 FNLMS channel data object structure
// -----------------------------------------------------------------------------

// Pointer to AEC FNLMS channel (D)(real/imag/BExp)
.CONST $aec520.fnlms.D_FIELD                 0*ADDR_PER_WORD;

// Pointer to real part of LMS G, size of 'LMS_dim*N_Taps'
.CONST $aec520.fnlms.G_REAL_FIELD            1*ADDR_PER_WORD;

// Pointer to imaginary part of LMS G, size of 'LMS_dim*N_Taps'
.CONST $aec520.fnlms.G_IMAG_FIELD            2*ADDR_PER_WORD;

// Pointer to BExp_G (internal array, permanant), size of 'LMS_dim'
.CONST $aec520.fnlms.G_BEXP_FIELD            3*ADDR_PER_WORD;

// Pointer to FBC object
.CONST $aec520.fnlms.PTR_FBC_OBJ_FIELD       4*ADDR_PER_WORD;

// Start of next channel
// If 0, no more channel, otherwise repeat fnlsm structure
.CONST $aec520.fnlms.LRM_NEXT_FIELD          1*ADDR_PER_WORD;
.CONST $aec520.fnlms.NEXT_FIELD              5*ADDR_PER_WORD;


// -----------------------------------------------------------------------------
// AEC520 dual microphone (second channel) data object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECDM_DATAOBJECT

// @DOC_FIELD_TEXT Pointer to external microphone mode
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.dm.PTR_MIC_MODE_FIELD         0*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to rightt channel FBC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.dm.FLAG_AEC_LRM_FIELD         1*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC (right) channel (D1)(real/imag/BExp)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.dm.D1_FIELD                   2*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to real part of Ga1, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.dm.GA1_REAL_FIELD             3*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to imaginary part of Ga1, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.dm.GA1_IMAG_FIELD             4*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to BExp_Ga1 (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.dm.GA1_BEXP_FIELD             5*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to rightt channel FBC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.dm.PTR_FBC1_OBJ_FIELD         6*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC next (3rd) channel start
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.dm.NEXT_CHANNEL_FIELD         7*ADDR_PER_WORD;

// @END  DATA_OBJECT AECDM_DATAOBJECT

// -----------------------------------------------------------------------------
// AEC520 FDNLP/VSM sub-module object structure
// -----------------------------------------------------------------------------
// FDNLP - (Handsfree)
.CONST $aec520.fdnlp.OFFSET_VSM_HB              0*ADDR_PER_WORD;
.CONST $aec520.fdnlp.OFFSET_VSM_LB              $aec520.fdnlp.OFFSET_VSM_HB + ADDR_PER_WORD;
.CONST $aec520.fdnlp.OFFSET_VSM_MAX_ATT         $aec520.fdnlp.OFFSET_VSM_LB + ADDR_PER_WORD;
.CONST $aec520.fdnlp.OFFSET_FDNLP_HB            $aec520.fdnlp.OFFSET_VSM_MAX_ATT + ADDR_PER_WORD;
.CONST $aec520.fdnlp.OFFSET_FDNLP_LB            $aec520.fdnlp.OFFSET_FDNLP_HB + ADDR_PER_WORD;
.CONST $aec520.fdnlp.OFFSET_FDNLP_MB            $aec520.fdnlp.OFFSET_FDNLP_LB + ADDR_PER_WORD;
.CONST $aec520.fdnlp.OFFSET_FDNLP_NBINS         $aec520.fdnlp.OFFSET_FDNLP_MB + ADDR_PER_WORD;
.CONST $aec520.fdnlp.OFFSET_FDNLP_ATT           $aec520.fdnlp.OFFSET_FDNLP_NBINS + ADDR_PER_WORD;
.CONST $aec520.fdnlp.OFFSET_FDNLP_ATT_THRESH    $aec520.fdnlp.OFFSET_FDNLP_ATT + ADDR_PER_WORD;
.CONST $aec520.fdnlp.OFFSET_FDNLP_ECHO_THRESH   $aec520.fdnlp.OFFSET_FDNLP_ATT_THRESH + ADDR_PER_WORD;
.CONST $aec520.fdnlp.STRUCT_SIZE                ($aec520.fdnlp.OFFSET_FDNLP_ECHO_THRESH >> LOG2_ADDR_PER_WORD) + 1;


// -----------------------------------------------------------------------------
// AEC520 NLP user parameter structure
// -----------------------------------------------------------------------------

.CONST $aec520.nlp.Parameter.OFFSET_HD_THRESH_GAIN           0*ADDR_PER_WORD;
.CONST $aec520.nlp.Parameter.OFFSET_TIER2_THRESH             1*ADDR_PER_WORD;
.CONST $aec520.nlp.Parameter.OFFSET_TIER0_CONFIG             -1;
.CONST $aec520.nlp.Parameter.OFFSET_TIER1_CONFIG             $aec520.nlp.Parameter.OFFSET_TIER2_THRESH + ADDR_PER_WORD;
.CONST $aec520.nlp.Parameter.OFFSET_TIER2_CONFIG             $aec520.nlp.Parameter.OFFSET_TIER1_CONFIG + $aec520.fdnlp.STRUCT_SIZE*ADDR_PER_WORD;

.CONST $aec520.nlp.Parameter.HF_OBJECT_SIZE                  ($aec520.nlp.Parameter.OFFSET_TIER2_CONFIG >> LOG2_ADDR_PER_WORD) + $aec520.fdnlp.STRUCT_SIZE;
.CONST $aec520.nlp.Parameter.HS_OBJECT_SIZE                  2;


// -----------------------------------------------------------------------------
// AEC520 NLP object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT NLPDATAOBJECT

// @DOC_FIELD_TEXT Pointer to AEC master object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.nlp.AEC_OBJ_PTR                  0*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to Non-Linear Processing Parameters
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.nlp.OFFSET_PARAM_PTR             1*ADDR_PER_WORD;

// FDNLP - VSM
// @DOC_FIELD_TEXT Pointer to current system call state flag
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.nlp.OFFSET_CALLSTATE_PTR         2*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to receive path signal VAD flag
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.nlp.OFFSET_PTR_RCV_DETECT        3*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to Attenuation, same array as used in AEC main object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.nlp.OFFSET_SCRPTR_Attenuation    4*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to scratch memory with size of Num_FFT_Freq_Bins + RER_dim
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.nlp.OFFSET_SCRPTR                5*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Function pointer for FDNLP
// @DOC_FIELD_TEXT To enable: set '$aec520.FdnlpProcess'
// @DOC_FIELD_TEXT To disable: set '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.nlp.FDNLP_FUNCPTR                6*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Function pointer for VSM
// @DOC_FIELD_TEXT To enable: set '$aec520.VsmProcess'
// @DOC_FIELD_TEXT To disable: set '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec520.nlp.VSM_FUNCPTR                  7*ADDR_PER_WORD;

// SP.  Internal FNDLP Data
.CONST $aec520.nlp.OFFSET_PTR_RatFE             $aec520.nlp.VSM_FUNCPTR + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_SCRPTR_absGr          $aec520.nlp.OFFSET_PTR_RatFE + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_SCRPTR_temp           $aec520.nlp.OFFSET_SCRPTR_absGr + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_CUR_TIER              $aec520.nlp.OFFSET_SCRPTR_temp + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_PTR_CUR_CONFIG        $aec520.nlp.OFFSET_CUR_TIER + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_hd_ct_hold            $aec520.nlp.OFFSET_PTR_CUR_CONFIG + $aec520.fdnlp.STRUCT_SIZE*ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_hd_att                $aec520.nlp.OFFSET_hd_ct_hold + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_G_vsm                 $aec520.nlp.OFFSET_hd_att + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_fdnlp_cont_test       $aec520.nlp.OFFSET_G_vsm + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_mean_len              $aec520.nlp.OFFSET_fdnlp_cont_test + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_avg_RatFE             $aec520.nlp.OFFSET_mean_len + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_Vad_ct_burst          $aec520.nlp.OFFSET_avg_RatFE + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_Vad_ct_hang           $aec520.nlp.OFFSET_Vad_ct_burst + ADDR_PER_WORD; // must follow ct_burst

.CONST $aec520.nlp.FLAG_BYPASS_HD_FIELD         $aec520.nlp.OFFSET_Vad_ct_hang + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_HC_TIER_STATE         $aec520.nlp.FLAG_BYPASS_HD_FIELD + ADDR_PER_WORD;
.CONST $aec520.nlp.FLAG_HD_MODE_FIELD           $aec520.nlp.OFFSET_HC_TIER_STATE + ADDR_PER_WORD;
.CONST $aec520.nlp.RER_dim                      $aec520.nlp.FLAG_HD_MODE_FIELD + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_NUM_FREQ_BINS         $aec520.nlp.RER_dim + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_D_REAL_PTR            $aec520.nlp.OFFSET_NUM_FREQ_BINS + ADDR_PER_WORD;
.CONST $aec520.nlp.OFFSET_D_IMAG_PTR            $aec520.nlp.OFFSET_D_REAL_PTR + ADDR_PER_WORD;

// NB/WB/UWB/SWB/FB
.CONST $aec520.nlp.VAD_Th_hang                  $aec520.nlp.OFFSET_D_IMAG_PTR + ADDR_PER_WORD;
.CONST $aec520.nlp.VAD_Th_burst                 $aec520.nlp.VAD_Th_hang + ADDR_PER_WORD;
.CONST $aec520.nlp.HD_Th_hold                   $aec520.nlp.VAD_Th_burst + ADDR_PER_WORD;
.CONST $aec520.nlp.FDNLP_Th_cont                $aec520.nlp.HD_Th_hold + ADDR_PER_WORD;

.CONST $aec520.nlp.STRUCT_SIZE                  ($aec520.nlp.FDNLP_Th_cont >> LOG2_ADDR_PER_WORD) + 1;

// @END  DATA_OBJECT NLPDATAOBJECT


// -----------------------------------------------------------------------------
// FBC data object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECFBC_DATAOBJECT

.CONST $aec520.fbc.STREAM_D_FIELD               0*ADDR_PER_WORD;
.CONST $aec520.fbc.PTR_VADX_FIELD               1*ADDR_PER_WORD;
.CONST $aec520.fbc.G_A_FIELD                    2*ADDR_PER_WORD;
.CONST $aec520.fbc.G_B_FIELD                    3*ADDR_PER_WORD;
.CONST $aec520.fbc.PERD_FIELD                   4*ADDR_PER_WORD;
.CONST $aec520.fbc.NIBBLE_FIELD                 5*ADDR_PER_WORD;
.CONST $aec520.fbc.HPF_STREAM_FIELD             6*ADDR_PER_WORD;
.CONST $aec520.fbc.HPF_FILTER_FIELD             7*ADDR_PER_WORD;

// Internal fields
.CONST $aec520.fbc.FILTER_LENGTH_FIELD          8*ADDR_PER_WORD;
.CONST $aec520.fbc.ALFA_ERLE_FIELD              $aec520.fbc.FILTER_LENGTH_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.HD_TS_FIELD                  $aec520.fbc.ALFA_ERLE_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.SIL_TS_FIELD                 $aec520.fbc.HD_TS_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.G_A_AMP_FIELD                $aec520.fbc.SIL_TS_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.G_B_AMP_FIELD                $aec520.fbc.G_A_AMP_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.X_BUF_PWR_GW_FIELD           $aec520.fbc.G_B_AMP_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.X_BUF_PWR_MSW_FIELD          $aec520.fbc.X_BUF_PWR_GW_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.X_BUF_PWR_LSW_FIELD          $aec520.fbc.X_BUF_PWR_MSW_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.MU_MANTISA_FIELD             $aec520.fbc.X_BUF_PWR_LSW_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.MU_EXP_FIELD                 $aec520.fbc.MU_MANTISA_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.MU2_MANTISA_FIELD            $aec520.fbc.MU_EXP_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.MU2_EXP_FIELD                $aec520.fbc.MU2_MANTISA_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.DERLE_AMP_FIELD              $aec520.fbc.MU2_EXP_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.DERLE_FIL_FIELD              $aec520.fbc.DERLE_AMP_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.ERLE_FOLD_FIELD              $aec520.fbc.DERLE_FIL_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.HD_FOLD_FIELD                $aec520.fbc.ERLE_FOLD_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.HD_FLAG_FIELD                $aec520.fbc.HD_FOLD_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.HD_CNTR_FIELD                $aec520.fbc.HD_FLAG_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.TH_CNTR_FIELD                $aec520.fbc.HD_CNTR_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.SIL_CNTR_FIELD               $aec520.fbc.TH_CNTR_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.DIVERGE_FLAG_FIELD           $aec520.fbc.SIL_CNTR_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.LRAT_0_FIELD                 $aec520.fbc.DIVERGE_FLAG_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.LRAT_1_FIELD                 $aec520.fbc.LRAT_0_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.LRAT_2_FIELD                 $aec520.fbc.LRAT_1_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.L2P_IBUF_D_FIELD             $aec520.fbc.LRAT_2_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.L2P_OBUF_D_FIELD             $aec520.fbc.L2P_IBUF_D_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.L2P_OBUF_D_1_FIELD           $aec520.fbc.L2P_OBUF_D_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.L2P_OBUF_D_2_FIELD           $aec520.fbc.L2P_OBUF_D_1_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.L2_HD_GAIN_FIELD             $aec520.fbc.L2P_OBUF_D_2_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.HD_GAIN_FIELD                $aec520.fbc.L2_HD_GAIN_FIELD + ADDR_PER_WORD;
.CONST $aec520.fbc.IBUF_D_PRE_PWR_FIELD         $aec520.fbc.HD_GAIN_FIELD  + ADDR_PER_WORD;
.CONST $aec520.fbc.OBUF_D_PRE_PWR_FIELD         $aec520.fbc.IBUF_D_PRE_PWR_FIELD  + ADDR_PER_WORD;
.CONST $aec520.fbc.L2P_PREP_FBC_FIELD           $aec520.fbc.OBUF_D_PRE_PWR_FIELD  + ADDR_PER_WORD;
.CONST $aec520.fbc.L2P_PWR_DIFFERENCE_FIELD     $aec520.fbc.L2P_PREP_FBC_FIELD  + ADDR_PER_WORD;        
.CONST $aec520.fbc.STREAM_D_DLY_B_FIELD         MK1 + $aec520.fbc.L2P_PWR_DIFFERENCE_FIELD;
.CONST $aec520.fbc.STREAM_X_DLY_B_FIELD         MK1 + $aec520.fbc.STREAM_D_DLY_B_FIELD;
.CONST $aec520.fbc.STREAM_D_HI_FIELD            MK1 + $aec520.fbc.STREAM_X_DLY_B_FIELD;
.CONST $aec520.fbc.STREAM_X_HI_FIELD            MK1 + $aec520.fbc.STREAM_D_HI_FIELD;
.CONST $aec520.fbc.FLAG_BYPASS_HPF_FIELD        MK1 + $aec520.fbc.STREAM_X_HI_FIELD;
.CONST $aec520.fbc.TEMP0_FIELD                  MK1 + $aec520.fbc.FLAG_BYPASS_HPF_FIELD;
.CONST $aec520.fbc.TEMP1_FIELD                  MK1 + $aec520.fbc.TEMP0_FIELD;
.CONST $aec520.fbc.STRUCT_SIZE                 ($aec520.fbc.TEMP1_FIELD >> LOG2_ADDR_PER_WORD) + 1;

// @END  DATA_OBJECT AECFBC_DATAOBJECT
#endif // AEC520_LIB_H_INCLUDED
