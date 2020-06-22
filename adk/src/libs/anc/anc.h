/* Copyright (c) 2015-2020 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file   anc.h
@brief  Header file for the ANC library.

        This file provides documentation for the ANC library API.
*/

#ifndef ANC_H_
#define ANC_H_

#include <audio_plugin_common.h>
#include <audio_plugin_music_variants.h>

#include <csrtypes.h>
#include <app/audio/audio_if.h>

/*! @brief The ANC Mode of operation when ANC functionality is enabled.

    Used by the VM application to set different ANC modes and also get the
    current mode of operation.
 */
typedef enum
{
    anc_mode_1,
    anc_mode_2,
    anc_mode_3,
    anc_mode_4,
    anc_mode_5,
    anc_mode_6,
    anc_mode_7,
    anc_mode_8,
    anc_mode_9,
    anc_mode_10
} anc_mode_t;

typedef enum
{
    all_disabled                    = 0x00,
    feed_forward_left               = 0x01,
    feed_forward_right              = 0x02,
    feed_back_left                  = 0x04,
    feed_back_right                 = 0x08,

    feed_forward_mode               = (feed_forward_left | feed_forward_right),
    feed_forward_mode_left_only     = feed_forward_left,
    feed_forward_mode_right_only    = feed_forward_right,

    feed_back_mode                  = (feed_back_left | feed_back_right),
    feed_back_mode_left_only        = feed_back_left,
    feed_back_mode_right_only       = feed_back_right,

    hybrid_mode                     = (feed_forward_left | feed_forward_right | feed_back_left | feed_back_right),
    hybrid_mode_left_only           = (feed_forward_left | feed_back_left),
    hybrid_mode_right_only          = (feed_forward_right | feed_back_right)
} anc_path_enable;


/*! @brief The ANC Microphones to be used by the library.

    Used by the VM application to provide the ANC library with all the required
    information about the microphones to use.
 */
typedef struct
{
    audio_mic_params feed_forward_left;
    audio_mic_params feed_forward_right;
    audio_mic_params feed_back_left;
    audio_mic_params feed_back_right;
    anc_path_enable enabled_mics;
} anc_mic_params_t;

/*!
    @brief Initialise the ANC library. This function must be called, and return
           indicating success, before any of the other library API functions can
           be called.

    @param mic_params The microphones to use for ANC functionality.
    @param init_mode The ANC mode at initialisation
    @param init_gain The gain at initialisation

    @return TRUE indicating the library was successfully initialised otherwise FALSE.
*/
bool AncInit(anc_mic_params_t *mic_params, anc_mode_t init_mode);

#ifdef HOSTED_TEST_ENVIRONMENT
/*!
    @brief free the memory allocated for the ANC library data. 

    @return TRUE indicating the memory allocated for library was successfully freed doesn't return FALSE.  
*/    
bool AncLibraryTestReset(void);
#endif
             
/*!
    @brief Enable or Disable the ANC functionality. If enabled then the ANC will
           start operating in the last set ANC mode. To ensure no audio artefacts,
           the ANC functionality should not be enabled or disabled while audio is
           being routed to the DACs.

    @param mic_params The microphones to use for ANC functionality.

    @return TRUE indicating ANC was successfully enabled or disabled otherwise FALSE.
*/
bool AncEnable(bool enable);


/*!
    @brief Query the current state of the ANC functionality.

    @return TRUE if ANC is currently enabled otherwise FALSE.
*/
bool AncIsEnabled(void);


/*!
    @brief Set the ANC operating mode. To ensure no audio artefacts, the ANC mode
           should not be changed while audio is being routed to the DACs.

    @param anc_mode_t The ANC mode to set.

    @return TRUE indicating the ANC mode was successfully changed otherwise FALSE.
*/
bool AncSetMode(anc_mode_t mode);

/*!
    @brief Returns the current ANC mic config.

    @return Pointer to current anc_mic_params_t stored in the ANC library
*/
anc_mic_params_t * AncGetAncMicParams(void);

/*!
    @brief Set the ANC filter path FFA gain. If the ANC functionality is enabled then
           the ANC filter path FFA gain will be applied immediately otherwise it will be ignored.

    @param instance The audio ANC hardware instance number.
    @param gain The ANC filter path FFA gain to set.

    @return TRUE indicating the ANC filter path gain was successfully changed
            otherwise FALSE.
*/
bool AncConfigureFFAPathGain(audio_anc_instance instance, uint8 gain);

/*!
    @brief Set the ANC filter path FFB gain. If the ANC functionality is enabled then
           the ANC filter path FFB gain will be applied immediately otherwise it will be ignored.

    @param instance The audio ANC hardware instance number.
    @param gain The ANC filter path FFB gain to set.

    @return TRUE indicating the ANC filter path gain was successfully changed
            otherwise FALSE.
*/
bool AncConfigureFFBPathGain(audio_anc_instance instance, uint8 gain);

/*!
    @brief Set the ANC filter path FB gain. If the ANC functionality is enabled then
           the ANC filter path FB gain will be applied immediately otherwise it will be ignored.

    @param instance The audio ANC hardware instance number.
    @param gain The ANC filter path FB gain to set.

    @return TRUE indicating the ANC filter path gain was successfully changed
            otherwise FALSE.
*/
bool AncConfigureFBPathGain(audio_anc_instance instance, uint8 gain);

/*!
    @brief Read the ANC fine gain from the Audio PS key for current mode and specified gain path

    ADVISORY : THIS API IS RECOMMENDED TO BE USED DURING PRODUCTION TUNING/CALIBRATION ONLY. 

    @param gain_path    The gain path to read gain from
    @param *gain        Pointer to gain value

    @return TRUE indicating the ANC gain was successfully read otherwise FALSE.
*/
bool AncReadFineGain(audio_anc_path_id gain_path, uint8 *gain);

/*!
    @brief Write the ANC fine gain to the Audio PS key for current mode and specified gain path

    ADVISORY : THIS API IS RECOMMENDED TO BE USED DURING PRODUCTION TUNING/CALIBRATION ONLY. 
    There is a risk to the product in detuning pre-set values if used in other scenarios. 
    
    @param gain_path    The gain path to write the gain to
    @param gain         The ANC fine gain value to be updated

    @return TRUE indicating the ANC gain was successfully changed otherwise FALSE.
*/
bool AncWriteFineGain(audio_anc_path_id gain_path, uint8 gain);

/*!
    @brief Read the ANC coarse gain for specificed path from the particular ANC HW instance

    @param inst     The ANC Hardware instance to get the coefficients from
    @param gain_path    The gain path to read gain from
    @param *gain        Pointer to gain value

*/
void AncReadCoarseGainFromInstance(audio_anc_instance inst, audio_anc_path_id gain_path, uint8 *gain);

/*!
    @brief Read the ANC fine gain for specificed path from the particular ANC HW instance

    @param inst     The ANC Hardware instance to get the coefficients from
    @param gain_path    The gain path to read gain from
    @param *gain        Pointer to gain value

*/
void AncReadFineGainFromInstance(audio_anc_instance inst, audio_anc_path_id gain_path, uint8 *gain);

#endif
