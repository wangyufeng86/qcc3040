/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       ui_inputs.h
\brief     Contains definitions required by ui config table.


*/

#ifndef _UI_INPUTS_H_
#define _UI_INPUTS_H_

#include "domain_message.h"

#define FOREACH_UI_INPUT(UI_INPUT) \
    UI_INPUT(ui_input_voice_call_hang_up = UI_INPUTS_TELEPHONY_MESSAGE_BASE)   \
    UI_INPUT(ui_input_voice_call_accept)  \
    UI_INPUT(ui_input_hfp_transfer_to_ag)   \
    UI_INPUT(ui_input_hfp_transfer_to_headset)  \
    UI_INPUT(ui_input_voice_call_reject)  \
    UI_INPUT(ui_input_hfp_voice_dial) \
    UI_INPUT(ui_input_toggle_play_pause = UI_INPUTS_MEDIA_PLAYER_MESSAGE_BASE)  \
    UI_INPUT(ui_input_play)  \
    UI_INPUT(ui_input_pause)  \
    UI_INPUT(ui_input_stop_av_connection)  \
    UI_INPUT(ui_input_av_forward)  \
    UI_INPUT(ui_input_av_backward)  \
    UI_INPUT(ui_input_av_fast_forward_start) \
    UI_INPUT(ui_input_fast_forward_stop) \
    UI_INPUT(ui_input_av_rewind_start) \
    UI_INPUT(ui_input_rewind_stop) \
    UI_INPUT(ui_input_sco_forward_call_hang_up = UI_INPUTS_PEER_MESSAGE_BASE) \
    UI_INPUT(ui_input_sco_forward_call_accept) \
    UI_INPUT(ui_input_sco_voice_call_reject) \
    UI_INPUT(ui_input_factory_reset_request = UI_INPUTS_DEVICE_STATE_MESSAGE_BASE) \
    UI_INPUT(ui_input_dfu_active_when_in_case_request) \
    UI_INPUT(ui_input_force_reset) \
    UI_INPUT(ui_input_production_test_mode_request) \
    UI_INPUT(ui_input_hfp_volume_stop = UI_INPUTS_VOLUME_MESSAGE_BASE) \
    UI_INPUT(ui_input_av_volume_down_remote_stop) \
    UI_INPUT(ui_input_av_volume_up_remote_stop) \
    UI_INPUT(ui_input_hfp_mute_toggle) \
    UI_INPUT(ui_input_hfp_volume_down_start) \
    UI_INPUT(ui_input_hfp_volume_up_start) \
    UI_INPUT(ui_input_sco_fwd_volume_down_start) \
    UI_INPUT(ui_input_sco_fwd_volume_up_start) \
    UI_INPUT(ui_input_av_volume_down_start) \
    UI_INPUT(ui_input_av_volume_up_start) \
    UI_INPUT(ui_input_sco_fwd_volume_stop) \
    UI_INPUT(ui_input_sm_pair_handset = UI_INPUTS_HANDSET_MESSAGE_BASE) \
    UI_INPUT(ui_input_sm_delete_handsets) \
    UI_INPUT(ui_input_connect_handset) \
    UI_INPUT(ui_input_sm_power_on) \
    UI_INPUT(ui_input_sm_power_off) \
    UI_INPUT(ui_input_anc_on = UI_INPUTS_AUDIO_CURATION_MESSAGE_BASE) \
    UI_INPUT(ui_input_anc_off) \
    UI_INPUT(ui_input_anc_toggle_on_off) \
    UI_INPUT(ui_input_anc_set_mode_1) \
    UI_INPUT(ui_input_anc_set_mode_2) \
    UI_INPUT(ui_input_anc_set_mode_3) \
    UI_INPUT(ui_input_anc_set_mode_4) \
    UI_INPUT(ui_input_anc_set_mode_5) \
    UI_INPUT(ui_input_anc_set_mode_6) \
    UI_INPUT(ui_input_anc_set_mode_7) \
    UI_INPUT(ui_input_anc_set_mode_8) \
    UI_INPUT(ui_input_anc_set_mode_9) \
    UI_INPUT(ui_input_anc_set_mode_10) \
    UI_INPUT(ui_input_anc_set_next_mode) \
    UI_INPUT(ui_input_anc_enter_tuning_mode) \
    UI_INPUT(ui_input_anc_exit_tuning_mode) \
    UI_INPUT(ui_input_anc_set_leakthrough_gain) \
    UI_INPUT(ui_input_leakthrough_on) \
    UI_INPUT(ui_input_leakthrough_off) \
    UI_INPUT(ui_input_leakthrough_toggle_on_off) \
    UI_INPUT(ui_input_leakthrough_set_mode_1) \
    UI_INPUT(ui_input_leakthrough_set_mode_2) \
    UI_INPUT(ui_input_leakthrough_set_mode_3) \
    UI_INPUT(ui_input_leakthrough_set_next_mode) \
    UI_INPUT(ui_input_va_1 = UI_INPUTS_VOICE_UI_MESSAGE_BASE) \
    UI_INPUT(ui_input_va_2) \
    UI_INPUT(ui_input_va_3) \
    UI_INPUT(ui_input_va_4) \
    UI_INPUT(ui_input_va_5) \
    UI_INPUT(ui_input_va_6) \
    UI_INPUT(ui_input_invalid = UI_INPUTS_BOUNDS_CHECK_MESSAGE_BASE) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum UI_INPUT_ENUM {
    FOREACH_UI_INPUT(GENERATE_ENUM)
};

/*! The first UI input, not in the enum, as that causes ui_input_string_debug
    test to fail */
#define ui_input_first UI_INPUTS_TELEPHONY_MESSAGE_BASE

/*! Macro to test if a message id is a UI input message */
#define isMessageUiInput(msg_id) (ui_input_first <= (msg_id) && (msg_id) < ui_input_invalid)

typedef enum UI_INPUT_ENUM ui_input_t;

/*! \brief ui providers */
typedef enum
{
    ui_provider_hfp,
    ui_provider_scofwd,
    ui_provider_device,
    ui_provider_media_player,
    ui_provider_av,
    ui_provider_indicator,
    ui_provider_phy_state,
    ui_provider_app_sm,
    ui_provider_power,
    ui_provider_audio_curation,
    ui_provider_voice_ui,
    ui_provider_peer_pairing,
    ui_provider_handset_pairing,
    ui_providers_max
} ui_providers_t;

#endif /* _UI_INPUTS_H_ */






