/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_sm_marshal_defs.c
\brief      Marshal type definitions for Earbud application.
*/

/* component includes */
#include "earbud_sm_marshal_defs.h"

/* framework includes */
#include <marshal_common.h>

/* system includes */
#include <marshal.h>

/*! earbud_sm_empty payload message marshalling type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_req_dfu_active_when_in_case_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_req_dfu_active_when_in_case_t);

const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_req_dfu_active_when_out_case_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_req_dfu_active_when_out_case_t);

const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_req_factory_reset_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_req_factory_reset_t);

const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_ind_dfu_ready_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_ind_dfu_ready_t);

const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_ind_dfu_start_timeout_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_ind_dfu_start_timeout_t);

const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_req_stereo_audio_mix_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_req_stereo_audio_mix_t);

const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_req_mono_audio_mix_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_req_mono_audio_mix_t);

const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_req_delete_handsets_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_req_delete_handsets_t);

const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_ind_mru_handset_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_ind_mru_handset_t);

const marshal_type_descriptor_t marshal_type_descriptor_earbud_sm_req_delete_handset_if_full_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(earbud_sm_req_delete_handset_if_full_t);

/*! X-Macro generate earbud SM marshal type descriptor set that can be passed to a (un)marshaller
 *  to initialise it.
 *  */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const earbud_sm_marshal_type_descriptors[] = {
    MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
