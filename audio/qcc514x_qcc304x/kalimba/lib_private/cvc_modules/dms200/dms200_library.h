// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef DMS200_LIB_H
#define DMS200_LIB_H

// -----------------------------------------------------------------------------
// OMS270/OMS280 version number
//    v.0.0.1: initial version
//    v.1.0.0: wideband
//    v.1.1.0: wnr
//    v.1.2.0: wideband resource reduction
//    v.2.0.0: PBP
//    v.2.1.0: PBP/LINEAR
//    v.3.0.0: OMS280 hd
//
// DMS200 version number
//    ver 1.0.0 - DMS100 : from OMS270 v.2.1.0
//    ver 2.0.0 - DMS200 : hd + va
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Minimum search
// -----------------------------------------------------------------------------
.CONST $dms200.MS_DUR                           Qfmt_(0.256, 7);
.CONST $dms200.MIN_SEARCH_LENGTH                34;
.CONST $dms200.MIN_SEARCH_LENGTH_UWB_FB         51;
.CONST $oms280.MAX_MIN_SEARCH_LENGTH            68;
.CONST $oms280.MAX_MIN_SEARCH_LENGTH_UWB_FB     102;

.CONST $dms200.MS_DUR.VA                        Qfmt_(1.0, 7);
.CONST $dms200.MIN_SEARCH_LENGTH.VA             133;
.CONST $dms200.MIN_SEARCH_LENGTH.VA_UWB         200;

// -----------------------------------------------------------------------------
// OMS280 PBP mode external constants
// -----------------------------------------------------------------------------
.CONST $oms280.PBP.DIM_NB                       25;
.CONST $oms280.PBP.DIM_WB                       30;
.CONST $oms280.PBP.DIM_SWB                      35;
.CONST $oms280.PBP.NUM_LINEAR_BAND              15;
.CONST $oms280.PBP.MS_BAND                      2;

.CONST $M.oms280.QUE_LENGTH                     ($oms280.MAX_MIN_SEARCH_LENGTH * $oms280.PBP.MS_BAND);
.CONST $M.oms280.QUE_LENGTH_UWB_FB              ($oms280.MAX_MIN_SEARCH_LENGTH_UWB_FB * $oms280.PBP.MS_BAND);
.CONST $M.oms280.narrow_band.STATE_LENGTH       ($oms280.PBP.DIM_NB * 4 + 2 * $oms280.PBP.MS_BAND);   // 104
.CONST $M.oms280.wide_band.STATE_LENGTH         ($oms280.PBP.DIM_WB * 4 + 2 * $oms280.PBP.MS_BAND);   // 124
.CONST $M.oms280.swb.STATE_LENGTH               ($oms280.PBP.DIM_SWB * 4 + 2 * $oms280.PBP.MS_BAND);  // 144
.CONST $M.oms280.narrow_band.SCRATCH_LENGTH     MAX( 65, $oms280.PBP.DIM_NB * 5);       // 125 or 65
.CONST $M.oms280.wide_band.SCRATCH_LENGTH       MAX(129, $oms280.PBP.DIM_WB * 5);       // 150 or 129
.CONST $M.oms280.swb.SCRATCH_LENGTH             MAX(257, $oms280.PBP.DIM_SWB * 5);      // 175 or 257

// -----------------------------------------------------------------------------
// OMS280 Linear mode external constants
// -----------------------------------------------------------------------------
.CONST $oms280.linear.DIM                       65;
.CONST $oms280.linear.narrow_band.MS_BAND       2;
.CONST $oms280.linear.wide_band.MS_BAND         3;
.CONST $oms280.linear.swb.MS_BAND               3;

.CONST $oms280.linear.narrow_band.QUE_LENGTH    ($M.oms280.QUE_LENGTH);
.CONST $oms280.linear.wide_band.QUE_LENGTH      ($oms280.MAX_MIN_SEARCH_LENGTH * $oms280.linear.wide_band.MS_BAND);
.CONST $oms280.linear.uwb.QUE_LENGTH            ($oms280.MAX_MIN_SEARCH_LENGTH_UWB_FB * $oms280.linear.wide_band.MS_BAND);
.CONST $oms280.linear.narrow_band.STATE_LENGTH  ($oms280.linear.DIM * 4 + 2 * $oms280.linear.narrow_band.MS_BAND);
.CONST $oms280.linear.wide_band.STATE_LENGTH    ($oms280.linear.DIM * 4 + 2 * $oms280.linear.wide_band.MS_BAND);
.CONST $oms280.linear.SCRATCH_LENGTH            ($oms280.linear.DIM * 5);   // 325

// -----------------------------------------------------------------------------
// DMS200 external constants
// -----------------------------------------------------------------------------
.CONST $dms200.DIM                              65;
.CONST $dms200.MS_BAND                          8;
.CONST $dms200.SS_BAND.PBP_NB                   5;
.CONST $dms200.SS_BAND.PBP_WB                   6;
.CONST $dms200.SS_BAND.LINEAR                   8;
#define $dms200.MAX_SS_BAND                     $dms200.SS_BAND.LINEAR

.CONST $dms200.QUE_LENGTH                       ($dms200.MIN_SEARCH_LENGTH  * $dms200.MS_BAND);
.CONST $dms200.QUE_LENGTH_UWB_FB                ($dms200.MIN_SEARCH_LENGTH_UWB_FB  * $dms200.MS_BAND);
.CONST $dms200.STATE_LENGTH                     ($dms200.DIM * 8 + $dms200.MS_BAND * 2);
.CONST $dms200.SCRATCH_LENGTH                   ($dms200.DIM * 8);
.CONST $dms200.ATH_STATE_LENGTH                 ($dms200.SS_BAND.LINEAR * 5);

// -----------------------------------------------------------------------------
// DMS200 PBP mode external constants
// -----------------------------------------------------------------------------
.CONST $dms200.pbp.narrow_band.DIM              25;
.CONST $dms200.pbp.narrow_band.MS_BAND          4;

.CONST $dms200.pbp.wide_band.DIM                30;
.CONST $dms200.pbp.wide_band.MS_BAND            5;

.CONST $dms200.pbp.swb.DIM                      35;
.CONST $dms200.pbp.swb.MS_BAND                  6;

.CONST $dms200.pbp.narrow_band.SCRATCH_LENGTH   (7 * $dms200.pbp.narrow_band.DIM + $dms200.pbp.narrow_band.MS_BAND + $dms200.MAX_SS_BAND * 5 + 2 * ( 65-2-$oms280.PBP.NUM_LINEAR_BAND));
.CONST $dms200.pbp.wide_band.SCRATCH_LENGTH     (7 * $dms200.pbp.wide_band.DIM   + $dms200.pbp.wide_band.MS_BAND   + $dms200.MAX_SS_BAND * 5 + 2 * (129-2-$oms280.PBP.NUM_LINEAR_BAND));
.CONST $dms200.pbp.swb.SCRATCH_LENGTH           (7 * $dms200.pbp.swb.DIM   + $dms200.pbp.swb.MS_BAND   + $dms200.MAX_SS_BAND * 5 + 2 * (257-2-$oms280.PBP.NUM_LINEAR_BAND));

.CONST $dms200.pbp.QUE_LENGTH_NB                ($dms200.MIN_SEARCH_LENGTH  * $dms200.pbp.narrow_band.MS_BAND);
.CONST $dms200.pbp.QUE_LENGTH_WB                ($dms200.MIN_SEARCH_LENGTH  * $dms200.pbp.wide_band.MS_BAND);
.CONST $dms200.pbp.QUE_LENGTH_UWB               ($dms200.MIN_SEARCH_LENGTH_UWB_FB * $dms200.pbp.wide_band.MS_BAND);
.CONST $dms200.pbp.QUE_LENGTH_SWB               ($dms200.MIN_SEARCH_LENGTH * $dms200.pbp.swb.MS_BAND); 
.CONST $dms200.pbp.QUE_LENGTH_FB                ($dms200.MIN_SEARCH_LENGTH_UWB_FB * $dms200.pbp.swb.MS_BAND); 

.CONST $dms200.pbp.QUE_LENGTH.VA_NB             ($dms200.MIN_SEARCH_LENGTH.VA  * $dms200.pbp.narrow_band.MS_BAND);
.CONST $dms200.pbp.QUE_LENGTH.VA_WB             ($dms200.MIN_SEARCH_LENGTH.VA  * $dms200.pbp.wide_band.MS_BAND);
.CONST $dms200.pbp.QUE_LENGTH.VA_UWB            ($dms200.MIN_SEARCH_LENGTH.VA_UWB * $dms200.pbp.wide_band.MS_BAND);
.CONST $dms200.pbp.QUE_LENGTH.VA_SWB            ($dms200.MIN_SEARCH_LENGTH.VA * $dms200.pbp.swb.MS_BAND);
.CONST $dms200.pbp.QUE_LENGTH.VA_FB             ($dms200.MIN_SEARCH_LENGTH.VA_UWB * $dms200.pbp.swb.MS_BAND);

.CONST $dms200.pbp.STATE_LENGTH                 ($dms200.pbp.wide_band.DIM  * 8 + $dms200.MS_BAND * 2);
.CONST $dms200.pbp.STATE_LENGTH_SWB_FB          ($dms200.pbp.swb.DIM * 8 + $dms200.MS_BAND * 2);


// -----------------------------------------------------------------------------
// OMS280/DMS200 DATA STRUCTURE
// -----------------------------------------------------------------------------

// Pointer to output channel (Y)(real/imag/BExp)
.CONST $M.oms280.Y_FIELD                     0*ADDR_PER_WORD;

// Pointer to mono (left) channel (X)(real/imag/BExp)
.CONST $M.oms280.X_FIELD                     1*ADDR_PER_WORD;

// Pointer to cvc_variant
.CONST $M.oms280.PTR_VARIANT_FIELD           2*ADDR_PER_WORD;

// OMS mode object (wideband/narrowband)
.CONST $M.oms280.PTR_MODE_FIELD              3*ADDR_PER_WORD;

// Pointer to DMS Parameter
.CONST $M.oms280.PARAM_FIELD                 4*ADDR_PER_WORD;

// Pointer to cvclib_table
.CONST $M.oms280.PBP_TABLE_FIELD             5*ADDR_PER_WORD;

// Pointer to internal circular buffer, of size '$M.oms280.QUE_LENGTH'
.CONST $M.oms280.LPX_QUEUE_START_FIELD       6*ADDR_PER_WORD;

// Pointer to G, external interpolated
.CONST $M.oms280.PTR_G_FIELD                 7*ADDR_PER_WORD;
.CONST $dms200.PTR_G_FIELD                   $M.oms280.PTR_G_FIELD;

// Pointer to LpX_nz, external interpolated
.CONST $M.oms280.PTR_LPXNZ_FIELD             8*ADDR_PER_WORD;

// Pointer to internal state memory
.CONST $M.oms280.PTR_STATE_FIELD             9*ADDR_PER_WORD;

// Pointer to scratch memory
.CONST $M.oms280.PTR_SCRATCH_FIELD           10*ADDR_PER_WORD;

// Harmonicity Threshould, CVC parameter
.CONST $M.oms280.HARM_THRESH_FIELD           11*ADDR_PER_WORD;

// Minimun search duration(OMS/DMS internal process)
.CONST $M.oms280.MIN_SEARCH_TIME_FIELD       12*ADDR_PER_WORD;

// Flag to enable LSA (OMS internal process)
.CONST $M.oms280.MMSE_LSA_ON_FIELD           13*ADDR_PER_WORD;

// Pointer to Tone flag, where 0 indicates a tone
.CONST $M.oms280.PTR_TONE_FLAG_FIELD         14*ADDR_PER_WORD;

// Pointer to DMS data object (non OMS portion)
.CONST $dms200.DMS_OBJ_FIELD                 15*ADDR_PER_WORD;

// Pointer to output channel (Y)(real/imag/BExp)
.CONST $dms200.Y_VA_FIELD                    16*ADDR_PER_WORD;

// Pointer to NS Power Parameter
.CONST $dms200.PTR_NS_POWER_FIELD            17*ADDR_PER_WORD;

// -----------------------------------------------------------------------------
// Variants
// -----------------------------------------------------------------------------
.CONST $dms200.numbins                       ADDR_PER_WORD + $dms200.PTR_NS_POWER_FIELD;
.CONST $dms200.DMS_dim                       ADDR_PER_WORD + $dms200.numbins;
.CONST $dms200.MS_bands                      ADDR_PER_WORD + $dms200.DMS_dim;
.CONST $dms200.linear_uwb_mode               ADDR_PER_WORD + $dms200.MS_bands;
.CONST $M.oms280.ALFA_NZ_FIELD               ADDR_PER_WORD + $dms200.linear_uwb_mode;
.CONST $M.oms280.ALFA_PS1_FIELD              ADDR_PER_WORD + $M.oms280.ALFA_NZ_FIELD;
.CONST $M.oms280.LALFA_PS_FIELD              ADDR_PER_WORD + $M.oms280.ALFA_PS1_FIELD;  
.CONST $M.oms280.LALFA_PS1_FIELD             ADDR_PER_WORD + $M.oms280.LALFA_PS_FIELD;
.CONST $M.oms280.VAD_noise_Th                ADDR_PER_WORD + $M.oms280.LALFA_PS1_FIELD;
.CONST $M.oms280.VAD_speech_Th               ADDR_PER_WORD + $M.oms280.VAD_noise_Th;
.CONST $M.oms280.N_lift_span_ratio           ADDR_PER_WORD + $M.oms280.VAD_speech_Th;
.CONST $M.oms280.F_IDX_K2000                 ADDR_PER_WORD + $M.oms280.N_lift_span_ratio;
.CONST $M.oms280.F_IDX_LIKE                  ADDR_PER_WORD + $M.oms280.F_IDX_K2000;

// -----------------------------------------------------------------------------
// Internal, OMS/DMS common fileds
// -----------------------------------------------------------------------------
.CONST $M.oms280.PTR_HARM_VALUE_FIELD        ADDR_PER_WORD + $M.oms280.F_IDX_LIKE;
.CONST $M.oms280.WNR_ENABLED_FIELD           ADDR_PER_WORD + $M.oms280.PTR_HARM_VALUE_FIELD;

.CONST $M.oms280.WIND_FIELD                  ADDR_PER_WORD + $M.oms280.WNR_ENABLED_FIELD;
.CONST $M.oms280.VOICED_FIELD                ADDR_PER_WORD + $M.oms280.WIND_FIELD;

.CONST $M.oms280.LTILT_FIELD                 ADDR_PER_WORD + $M.oms280.VOICED_FIELD;
.CONST $M.oms280.MIN_SEARCH_COUNT_FIELD      ADDR_PER_WORD + $M.oms280.LTILT_FIELD;
.CONST $M.oms280.MIN_SEARCH_LENGTH_FIELD     ADDR_PER_WORD + $M.oms280.MIN_SEARCH_COUNT_FIELD;
.CONST $M.oms280.PTR_LPXS_FIELD              ADDR_PER_WORD + $M.oms280.MIN_SEARCH_LENGTH_FIELD;
.CONST $M.oms280.PTR_LPY_FIELD               ADDR_PER_WORD + $M.oms280.PTR_LPXS_FIELD;
.CONST $M.oms280.VOICED_COUNTER_FIELD        ADDR_PER_WORD + $M.oms280.PTR_LPY_FIELD;
.CONST $M.oms280.PTR_LPX_MIN_FIELD           ADDR_PER_WORD + $M.oms280.VOICED_COUNTER_FIELD;
.CONST $M.oms280.INITIALISED_FIELD           ADDR_PER_WORD + $M.oms280.PTR_LPX_MIN_FIELD;
.CONST $M.oms280.LIKE_MEAN_FIELD             ADDR_PER_WORD + $M.oms280.INITIALISED_FIELD;
.CONST $M.oms280.TEMP_FIELD                  ADDR_PER_WORD + $M.oms280.LIKE_MEAN_FIELD;
.CONST $M.oms280.TEMP1_FIELD                 ADDR_PER_WORD + $M.oms280.TEMP_FIELD;

// internal G/LPXNZ
// G/LpX_nz (OMS) -> G_G/MS_LpN (DMS)
.CONST $M.oms280.G_G_FIELD                   ADDR_PER_WORD + $M.oms280.TEMP1_FIELD;
.CONST $M.oms280.MS_LPN_FIELD                ADDR_PER_WORD + $M.oms280.G_G_FIELD;
.CONST $M.oms280.PTR_LPX_QUEUE_FIELD         ADDR_PER_WORD + $M.oms280.MS_LPN_FIELD;
.CONST $M.oms280.PTR_LPN_FIELD               ADDR_PER_WORD + $M.oms280.PTR_LPX_QUEUE_FIELD;

// Scratch pointer fields
.CONST $M.oms280.SCRATCH_LPXT_FIELD          ADDR_PER_WORD + $M.oms280.PTR_LPN_FIELD;
.CONST $M.oms280.SCRATCH_LIKE_FIELD          ADDR_PER_WORD + $M.oms280.SCRATCH_LPXT_FIELD;
.CONST $M.oms280.SCRATCH_NZLIFT_FIELD        ADDR_PER_WORD + $M.oms280.SCRATCH_LIKE_FIELD;
.CONST $M.oms280.SCRATCH_LPNZLIFT_FIELD      ADDR_PER_WORD + $M.oms280.SCRATCH_NZLIFT_FIELD;
.CONST $M.oms280.SCRATCH_LTILT_FIELD         ADDR_PER_WORD + $M.oms280.SCRATCH_LPNZLIFT_FIELD;
.CONST $dms200.SCRATCH_LRATIO_FIELD          ADDR_PER_WORD + $M.oms280.SCRATCH_LTILT_FIELD;

.CONST $M.oms280.BYPASS_FIELD                ADDR_PER_WORD + $dms200.SCRATCH_LRATIO_FIELD;   

.CONST $M.oms280.STRUC_SIZE                  1 + ($M.oms280.BYPASS_FIELD >> LOG2_ADDR_PER_WORD);

// -----------------------------------------------------------------------------
// DMS internal
// -----------------------------------------------------------------------------
.CONST $dms200.SCRATCH_TLRATIO_FIELD         ADDR_PER_WORD + $M.oms280.BYPASS_FIELD;
.CONST $dms200.SCRATCH_LPDT_FIELD            ADDR_PER_WORD + $dms200.SCRATCH_TLRATIO_FIELD;
.CONST $dms200.LPDS_FIELD                    ADDR_PER_WORD + $dms200.SCRATCH_LPDT_FIELD;
.CONST $dms200.LPS_FLOOR_FIELD               ADDR_PER_WORD + $dms200.LPDS_FIELD;
.CONST $dms200.LPN_FIELD                     ADDR_PER_WORD + $dms200.LPS_FLOOR_FIELD;
.CONST $dms200.VAD_T_LIKE_FIELD              ADDR_PER_WORD + $dms200.LPN_FIELD;
.CONST $dms200.SNR_FIELD                     ADDR_PER_WORD + $dms200.VAD_T_LIKE_FIELD;
.CONST $dms200.SNR_ALFA_FIELD                ADDR_PER_WORD + $dms200.SNR_FIELD;
.CONST $dms200.ALFA_LIKE_FIELD               ADDR_PER_WORD + $dms200.SNR_ALFA_FIELD; 

// Feature Bypass Flags
.CONST $dms200.BYPASS_VAD_S_FIELD            ADDR_PER_WORD + $dms200.ALFA_LIKE_FIELD;
.CONST $dms200.BYPASS_SPP_FIELD              ADDR_PER_WORD + $dms200.BYPASS_VAD_S_FIELD;
.CONST $dms200.BYPASS_GSMOOTH_FIELD          ADDR_PER_WORD + $dms200.BYPASS_SPP_FIELD;
.CONST $dms200.BYPASS_NFLOOR_FIELD           ADDR_PER_WORD + $dms200.BYPASS_GSMOOTH_FIELD;
.CONST $dms200.BYPASS_NLIFT_FIELD            ADDR_PER_WORD + $dms200.BYPASS_NFLOOR_FIELD;
.CONST $dms200.BYPASS_AUTO_TH_FIELD          ADDR_PER_WORD + $dms200.BYPASS_NLIFT_FIELD; 
.CONST $dms200.GSCHEME_FIELD                 ADDR_PER_WORD + $dms200.BYPASS_AUTO_TH_FIELD;

// Internal States
.CONST $dms200.VAD_S_VOICED_FIELD            ADDR_PER_WORD + $dms200.GSCHEME_FIELD;
.CONST $dms200.VAD_S_COUNT_FIELD             ADDR_PER_WORD + $dms200.VAD_S_VOICED_FIELD;
.CONST $dms200.VAD_S_LIKE_MEAN_FIELD         ADDR_PER_WORD + $dms200.VAD_S_COUNT_FIELD;
.CONST $dms200.VAD_S_BINL_FIELD              ADDR_PER_WORD + $dms200.VAD_S_LIKE_MEAN_FIELD;
.CONST $dms200.VAD_S_LIKE_MN_FIELD           ADDR_PER_WORD + $dms200.VAD_S_BINL_FIELD;
.CONST $dms200.VAD_S_SNR_MN_FIELD            ADDR_PER_WORD + $dms200.VAD_S_LIKE_MN_FIELD;
.CONST $dms200.VAD_S_NOISE_TH_FIELD          ADDR_PER_WORD + $dms200.VAD_S_SNR_MN_FIELD;
.CONST $dms200.VAD_S_SPEECH_TH_FIELD         ADDR_PER_WORD + $dms200.VAD_S_NOISE_TH_FIELD;
.CONST $dms200.VAD_S_BAND_LIKE_MN_FIELD      ADDR_PER_WORD + $dms200.VAD_S_SPEECH_TH_FIELD;
.CONST $dms200.VAD_S_BAND_NOISE_TH_FIELD     ADDR_PER_WORD + $dms200.VAD_S_BAND_LIKE_MN_FIELD;
.CONST $dms200.VAD_S_BAND_SPEECH_TH_FIELD    ADDR_PER_WORD + $dms200.VAD_S_BAND_NOISE_TH_FIELD;
.CONST $dms200.VAD_S_BAND_COUNT_FIELD        ADDR_PER_WORD + $dms200.VAD_S_BAND_SPEECH_TH_FIELD;
.CONST $dms200.SS_BANDS_START_FIELD          ADDR_PER_WORD + $dms200.VAD_S_BAND_COUNT_FIELD; 
.CONST $dms200.SS_BANDS_FIELD                ADDR_PER_WORD + $dms200.SS_BANDS_START_FIELD; 
.CONST $dms200.noise_lock_flag               ADDR_PER_WORD + $dms200.SS_BANDS_FIELD;

.CONST $dms200.VAD_VOICED_FIELD              ADDR_PER_WORD + $dms200.noise_lock_flag;
.CONST $dms200.VAD_COUNT_FIELD               ADDR_PER_WORD + $dms200.VAD_VOICED_FIELD;
.CONST $dms200.VAD_LIKE_MEAN_FIELD           ADDR_PER_WORD + $dms200.VAD_COUNT_FIELD;
.CONST $dms200.VAD_SPEECH_ON_FIELD           ADDR_PER_WORD + $dms200.VAD_LIKE_MEAN_FIELD;
.CONST $dms200.DMS_MODE_FIELD                ADDR_PER_WORD + $dms200.VAD_SPEECH_ON_FIELD;
.CONST $dms200.SNR_MN_FIELD                  ADDR_PER_WORD + $dms200.DMS_MODE_FIELD;  
.CONST $dms200.VOICED_SNR_FIELD              ADDR_PER_WORD + $dms200.SNR_MN_FIELD;
.CONST $dms200.NSN_AGGRT_FIELD               ADDR_PER_WORD + $dms200.VOICED_SNR_FIELD;

.CONST $dms200.STRUC_SIZE                    1 + ($dms200.NSN_AGGRT_FIELD >> LOG2_ADDR_PER_WORD);


// -----------------------------------------------------------------------------
// DMS data object
// -----------------------------------------------------------------------------

// Pointer to external DMS mode
.CONST $dms200.dms.PTR_MIC_MODE_FIELD           0;

// Pointer to parameter of DOA0
.CONST $dms200.dms.PTR_DOA0_FIELD               MK1 * 1;

// Pointer to parameter of Power Adjustment
// In log2 dB in Q8.16 format
.CONST $dms200.dms.PTR_POWR_ADJUST_FIELD        MK1 * 2;

// Pointer to second (right) channel (D1)(real/imag/BExp)
.CONST $dms200.dms.D1_FIELD                     MK1 * 3;

// Pointer to LRatio_interpolated array
.CONST $dms200.dms.AUTO_TH_STATE_FIELD          MK1 * 4;

// Pointer to SPP array
.CONST $dms200.dms.SPP_FIELD                    MK1 * 5;

// Pointer to LRatio_interpolated array
.CONST $dms200.dms.LRATIO_INTERPOLATED_FIELD    MK1 * 6;

// DMS master mode: 0 - dual mic, 1 - 1mic
.CONST $dms200.dms.MASTER_DMS_MODE_FIELD        MK1 * 7;

.CONST $dms200.dms.STRUC_SIZE                   1 + ($dms200.dms.MASTER_DMS_MODE_FIELD >> LOG2_ADDR_PER_WORD);


// -----------------------------------------------------------------------------
// WNR data object
// -----------------------------------------------------------------------------

// Pointer to WNR user parameter object
.CONST $M.oms280.wnr.PTR_WNR_PARAM_FIELD           0*ADDR_PER_WORD;

// Pointer to receive path VAD flag
.CONST $M.oms280.wnr.PTR_RCVVAD_FLAG_FIELD         1*ADDR_PER_WORD;

// Pointer to send path VAD flag
.CONST $M.oms280.wnr.PTR_SNDVAD_FLAG_FIELD         2*ADDR_PER_WORD;

// Pointer to external wind_flag variable
.CONST $M.oms280.wnr.PTR_WIND_FLAG_FIELD           3*ADDR_PER_WORD;

// Internal fields
.CONST $M.oms280.wnr.SND_VAD_COUNT_FIELD           4*ADDR_PER_WORD;
.CONST $M.oms280.wnr.HOLD_FIELD                    5*ADDR_PER_WORD;
.CONST $M.oms280.wnr.MAX_ROLLOFF_BIN               6*ADDR_PER_WORD;
.CONST $M.oms280.wnr.POWER_THRES_FIELD             7*ADDR_PER_WORD;
.CONST $M.oms280.wnr.POWER_LEVEL_FIELD             8*ADDR_PER_WORD;
.CONST $M.oms280.wnr.COUNT_FIELD                   9*ADDR_PER_WORD;
// The following 3 fields need to be consecutive and in order
.CONST $M.oms280.wnr.HIGH_BIN_FIELD               10*ADDR_PER_WORD;
.CONST $M.oms280.wnr.LOW_BIN_FIELD                11*ADDR_PER_WORD;
.CONST $M.oms280.wnr.ROLLOFF_IDX_FIELD            12*ADDR_PER_WORD;
.CONST $M.oms280.wnr.FIX_ROLLOFF_BIN              13*ADDR_PER_WORD;
.CONST $M.oms280.wnr.FUNC_WIND_DETECT_FIELD       14*ADDR_PER_WORD;
.CONST $M.oms280.wnr.FUNC_WIND_REDUCTION_FIELD    15*ADDR_PER_WORD;
.CONST $M.oms280.wnr.FUNC_WIND_POWER_FIELD        16*ADDR_PER_WORD;
.CONST $M.oms280.wnr.STRUC_SIZE                   17;


// -----------------------------------------------------------------------------
// WNR control parameter block offset
// -----------------------------------------------------------------------------

// WNR aggressiveness, default 1.0, Q1.F
.CONST $M.oms280.param.WNR_AGRESSIVENESS_FIELD     0*ADDR_PER_WORD;

// WNR power threshold (dB in log2), below which signal is seen as non-wind, Q8.F
.CONST $M.oms280.param.WNR_POWER_THRESHOLD_FIELD   1*ADDR_PER_WORD;

// WNR detection hold time in seconds, Q7.F
.CONST $M.oms280.param.WNR_HOLD_FIELD              2*ADDR_PER_WORD;


// -----------------------------------------------------------------------------
// DMS200 Parameter Structure
// -----------------------------------------------------------------------------

// DMS aggressiveness, default 1.0, Q1.F
.CONST $dms200.param.AGRESSIVENESS_FIELD           0*ADDR_PER_WORD;

// Residual Noise floor (in log2 dB), Q8.F
.CONST $dms200.param.RESIDUAL_NOISE_FIELD          1*ADDR_PER_WORD;

// Non-Stationary Noise Suppression Aggresiveness, Q1.F
.CONST $dms200.param.NSN_AGGR_FIELD                2*ADDR_PER_WORD;

// VA DMS aggressiveness, Q1.F
.CONST $dms200.param.VA_AGRESSIVENESS_FIELD        3*ADDR_PER_WORD;

#endif // DMS200_LIB_H
