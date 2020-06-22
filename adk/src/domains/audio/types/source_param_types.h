/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Datatypes common between audio and voice sources
*/

#ifndef SOURCE_PARAM_TYPES_H_
#define SOURCE_PARAM_TYPES_H_

typedef enum
{
    source_type_voice,
    source_type_audio,
} audio_source_type_t;

typedef struct
{
    unsigned data_length;
    void * data;
} source_defined_params_t;

#endif /* SOURCE_PARAM_TYPES_H_ */
