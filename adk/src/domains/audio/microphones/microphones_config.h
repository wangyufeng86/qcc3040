/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Microphone configuration
*/

#ifndef MICROPHONES_CONFIG_H_
#define MICROPHONES_CONFIG_H_

//!@{ @name Parameters for microphone 0 - Left analog MIC */
#define appConfigMic0Bias()                     (BIAS_CONFIG_MIC_BIAS_0)
#define appConfigMic0BiasVoltage()              (3) /* 1.9v */
#define appConfigMic0Pio()                      (0x13)
#define appConfigMic0Gain()                     (0x5)
#define appConfigMic0IsDigital()                (FALSE)
#define appConfigMic0AudioInstance()            (AUDIO_INSTANCE_0)
#define appConfigMic0AudioChannel()             (AUDIO_CHANNEL_A)
//!@}

//!@{ @name Parameters for microphone 1 - Right analog MIC */
#define appConfigMic1Bias()                     (BIAS_CONFIG_MIC_BIAS_0)
#define appConfigMic1BiasVoltage()              (3) /* 1.9v */
#define appConfigMic1Pio()                      (0x16)
#define appConfigMic1Gain()                     (0x5)
#define appConfigMic1IsDigital()                (FALSE)
#define appConfigMic1AudioInstance()            (AUDIO_INSTANCE_0)
#define appConfigMic1AudioChannel()             (AUDIO_CHANNEL_B)
//!@}

#if defined(CORVUS_PG806)

//!@{ @name Parameters for microphone 2 - HBL_L_FB Analog MIC connected to digital MIC ADC */
#define appConfigMic2Bias()                     (BIAS_CONFIG_MIC_BIAS_0)
#define appConfigMic2BiasVoltage()              (3)
#define appConfigMic2Pio()                      (0)
#define appConfigMic2Gain()                     (0)
#define appConfigMic2IsDigital()                (TRUE)
#define appConfigMic2AudioInstance()            (AUDIO_INSTANCE_1)
#define appConfigMic2AudioChannel()             (AUDIO_CHANNEL_B)
//!@}

//!@{ @name Parameters microphone 3 - HBL_L_FF Analog MIC connected to digital MIC ADC */
#define appConfigMic3Bias()                     (BIAS_CONFIG_MIC_BIAS_0)
#define appConfigMic3BiasVoltage()              (3)
#define appConfigMic3Pio()                      (0)
#define appConfigMic3Gain()                     (0)
#define appConfigMic3IsDigital()                (TRUE)
#define appConfigMic3AudioInstance()            (AUDIO_INSTANCE_1)
#define appConfigMic3AudioChannel()             (AUDIO_CHANNEL_A)
//!@}

#elif defined(CORVUS_YD300)

//!@{ @name Parameters for microphone 2 */
#define appConfigMic2Bias()                       (BIAS_CONFIG_MIC_BIAS_0)
#define appConfigMic2BiasVoltage()              (7)
#define appConfigMic2Pio()                      (4)
#define appConfigMic2Gain()                     (0)
#define appConfigMic2IsDigital()                (TRUE)
#define appConfigMic2AudioInstance()            (AUDIO_INSTANCE_1)
#define appConfigMic2AudioChannel()             (AUDIO_CHANNEL_A)
//!@}

//!@{ @name Parameters microphone 3 */
#define appConfigMic3Bias()                     (BIAS_CONFIG_MIC_BIAS_0)
#define appConfigMic3BiasVoltage()              (7)
#define appConfigMic3Pio()                      (4)
#define appConfigMic3Gain()                     (0)
#define appConfigMic3IsDigital()                (TRUE)
#define appConfigMic3AudioInstance()            (AUDIO_INSTANCE_1)
#define appConfigMic3AudioChannel()             (AUDIO_CHANNEL_B)
//!@}


#else

//!@{ @name Parameters for microphone 2 */
#define appConfigMic2Bias()                     (BIAS_CONFIG_PIO)
#ifdef QCC5141_FF_HYBRID_ANC_AA
#define appConfigMic2BiasVoltage()              (3)
#define appConfigMic2Pio()                      (19)
#define appConfigMic2Gain()                     (15)
#else
#define appConfigMic2BiasVoltage()              (0)
#define appConfigMic2Pio()                      (4)
#define appConfigMic2Gain()                     (0)
#endif
#define appConfigMic2IsDigital()                (TRUE)
#define appConfigMic2AudioInstance()            (AUDIO_INSTANCE_1)
#define appConfigMic2AudioChannel()             (AUDIO_CHANNEL_A)
//!@}

//!@{ @name Parameters microphone 3 */
#define appConfigMic3Bias()                     (BIAS_CONFIG_PIO)
#ifdef QCC5141_FF_HYBRID_ANC_AA
#define appConfigMic3BiasVoltage()              (3)
#define appConfigMic3Pio()                      (19)
#define appConfigMic3Gain()                     (15)
#else
#define appConfigMic3BiasVoltage()              (0)
#define appConfigMic3Pio()                      (4)
#define appConfigMic3Gain()                     (0)
#endif
#define appConfigMic3IsDigital()                (TRUE)
#define appConfigMic3AudioInstance()            (AUDIO_INSTANCE_1)
#define appConfigMic3AudioChannel()             (AUDIO_CHANNEL_B)
//!@}

#endif

#endif /* MICROPHONES_CONFIG_H_ */
