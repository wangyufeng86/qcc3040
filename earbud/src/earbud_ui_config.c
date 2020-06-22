/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       earbud_ui_config.c
\brief      ui configuration table

    This file contains ui configuration table which maps different logical inputs to
    corresponding ui inputs based upon ui provider contexts.
*/

#include "earbud_ui_config.h"
#include "ui.h"

#if defined(HAVE_9_BUTTONS)
#include "9_buttons.h"
#elif defined(HAVE_7_BUTTONS)
#include "7_buttons.h"
#elif defined(HAVE_6_BUTTONS)
#include "6_buttons.h"
#elif defined(HAVE_4_BUTTONS)
#include "4_buttons.h"
#elif defined(HAVE_2_BUTTONS)
#include "2_button.h"
#elif defined(HAVE_1_BUTTON)
#include "1_button.h"
#endif

/* Needed for UI contexts - transitional; when table is code generated these can be anonymised
 * unsigned ints and these includes can be removed. */
#include "av.h"
#include "hfp_profile.h"
#include "bt_device.h"
#include "scofwd_profile.h"
#include "earbud_sm.h"
#include "power_manager.h"
#include "voice_ui.h"
#include "audio_curation.h"

#include <macros.h>

#ifdef INCLUDE_CAPSENSE
const touchEventConfig touch_event_table [] =
{
    {
        SINGLE_PRESS,
        CAP_SENSE_SINGLE_PRESS
    },
    {
        DOUBLE_PRESS,
        CAP_SENSE_DOUBLE_PRESS
    },    
    {
        DOUBLE_PRESS_HOLD,
        CAP_SENSE_DOUBLE_PRESS_HOLD
    },
    {
        TRIPLE_PRESS,
        CAP_SENSE_TRIPLE_PRESS
    },    
    {
        TRIPLE_PRESS_HOLD,
        CAP_SENSE_TRIPLE_PRESS_HOLD
    },  
    {
        FOUR_PRESS_HOLD,
        CAP_SENSE_FOUR_PRESS_HOLD
    },
    {
        FIVE_PRESS,
        CAP_SENSE_FIVE_PRESS
    },
    {
        FIVE_PRESS_HOLD,
        CAP_SENSE_FIVE_PRESS_HOLD
    },
    {
        SIX_PRESS,
        CAP_SENSE_SIX_PRESS
    },
    {
        SEVEN_PRESS,
        CAP_SENSE_SEVEN_PRESS
    },
    {
        EIGHT_PRESS,
        CAP_SENSE_EIGHT_PRESS
    },
    {
        NINE_PRESS,
        CAP_SENSE_NINE_PRESS
    },
    {
        LONG_PRESS,
        CAP_SENSE_LONG_PRESS
    },
    {
        VERY_LONG_PRESS,
        CAP_SENSE_VERY_LONG_PRESS
    },
    {
        VERY_VERY_LONG_PRESS,
        CAP_SENSE_VERY_VERY_LONG_PRESS
    },
    {
        VERY_VERY_VERY_LONG_PRESS,
        CAP_SENSE_VERY_VERY_VERY_LONG_PRESS
    },
    {
        SLIDE_UP,
        CAP_SENSE_SLIDE_UP
    },
    {
        SLIDE_DOWN,
        CAP_SENSE_SLIDE_DOWN
    },
    {
        SLIDE_LEFT,
        CAP_SENSE_SLIDE_UP
    },
    {
        SLIDE_RIGHT,
        CAP_SENSE_SLIDE_DOWN
    },
    {
        HAND_COVER,
        CAP_SENSE_HAND_COVER
    },
    {
        HAND_COVER_RELEASE,
        CAP_SENSE_HAND_COVER_RELEASE
    },
};
#endif

/*! \brief ui config table*/
const ui_config_table_content_t earbud_ui_config_table[] =
{
    {APP_LEAKTHROUGH_TOGGLE_ON_OFF,    ui_provider_audio_curation,  context_leakthrough_disabled,         ui_input_leakthrough_toggle_on_off            },
    {APP_LEAKTHROUGH_TOGGLE_ON_OFF,    ui_provider_audio_curation,  context_leakthrough_enabled,          ui_input_leakthrough_toggle_on_off            },
    
    {APP_ANC_ENABLE,                   ui_provider_audio_curation,  context_anc_disabled,                 ui_input_anc_on                               },
    {APP_ANC_DISABLE,                  ui_provider_audio_curation,  context_anc_enabled,                  ui_input_anc_off                              },
    {APP_ANC_TOGGLE_ON_OFF,            ui_provider_audio_curation,  context_anc_disabled,                 ui_input_anc_toggle_on_off                    },
    {APP_ANC_TOGGLE_ON_OFF,            ui_provider_audio_curation,  context_anc_enabled,                  ui_input_anc_toggle_on_off                    },
    {APP_ANC_SET_NEXT_MODE,            ui_provider_audio_curation,  context_anc_enabled,                  ui_input_anc_set_next_mode                    },
    {APP_MFB_BUTTON_SINGLE_CLICK,      ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_voice_call_hang_up                   },
    {APP_MFB_BUTTON_SINGLE_CLICK,      ui_provider_hfp,             context_hfp_voice_call_sco_inactive,  ui_input_voice_call_hang_up                   },
    {APP_MFB_BUTTON_SINGLE_CLICK,      ui_provider_hfp,             context_hfp_voice_call_outgoing,      ui_input_voice_call_hang_up                   },
    {APP_MFB_BUTTON_SINGLE_CLICK,      ui_provider_hfp,             context_hfp_voice_call_incoming,      ui_input_voice_call_accept                    },
    {APP_MFB_BUTTON_SINGLE_CLICK,      ui_provider_media_player,    context_av_is_streaming,              ui_input_toggle_play_pause                    },
    {APP_MFB_BUTTON_SINGLE_CLICK,      ui_provider_media_player,    context_av_connected,                 ui_input_toggle_play_pause                    },
    {APP_MFB_BUTTON_SINGLE_CLICK,      ui_provider_device,          context_handset_not_connected,        ui_input_connect_handset                      },

    {APP_MFB_BUTTON_HELD_RELEASE_1SEC, ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_transfer_to_ag                   },
    {APP_MFB_BUTTON_HELD_RELEASE_1SEC, ui_provider_hfp,             context_hfp_voice_call_sco_inactive,  ui_input_hfp_transfer_to_headset              },
    {APP_MFB_BUTTON_HELD_RELEASE_1SEC, ui_provider_hfp,             context_hfp_voice_call_incoming,      ui_input_voice_call_reject                    },
    {APP_MFB_BUTTON_HELD_RELEASE_1SEC, ui_provider_media_player,    context_av_is_streaming,              ui_input_stop_av_connection                   },

#ifdef PRODUCTION_TEST_MODE
    {APP_MFB_BUTTON_HELD_RELEASE_3SEC, ui_provider_phy_state,       context_phy_state_out_of_case,        ui_input_production_test_mode_request         },
#endif

    {APP_BUTTON_DFU,                   ui_provider_phy_state,       context_phy_state_out_of_case,        ui_input_dfu_active_when_in_case_request      },
    {APP_BUTTON_FACTORY_RESET,         ui_provider_phy_state,       context_phy_state_out_of_case,        ui_input_factory_reset_request                },

#ifdef QCC3020_FF_ENTRY_LEVEL_AA
    {APP_BUTTON_FORCE_RESET,           ui_provider_phy_state,       context_phy_state_out_of_case,        ui_input_force_reset                          },
#endif

    {APP_MFB_BUTTON_HELD_RELEASE_6SEC, ui_provider_phy_state,       context_phy_state_out_of_case,        ui_input_sm_pair_handset                      },
    {APP_MFB_BUTTON_HELD_RELEASE_8SEC, ui_provider_phy_state,       context_phy_state_out_of_case,        ui_input_sm_delete_handsets                   },

    {APP_VA_BUTTON_DOWN,              ui_provider_voice_ui,        context_voice_ui_default,              ui_input_va_1                                 },
    {APP_VA_BUTTON_SINGLE_CLICK,      ui_provider_voice_ui,        context_voice_ui_default,              ui_input_va_3                                 },
    {APP_VA_BUTTON_DOUBLE_CLICK,      ui_provider_voice_ui,        context_voice_ui_default,              ui_input_va_4                                 },
    {APP_VA_BUTTON_HELD_1SEC,         ui_provider_voice_ui,        context_voice_ui_default,              ui_input_va_5                                 },
    {APP_VA_BUTTON_RELEASE,           ui_provider_voice_ui,        context_voice_ui_default,              ui_input_va_6                                 },
    
#if defined(HAVE_4_BUTTONS) || defined(HAVE_6_BUTTONS) || defined(HAVE_7_BUTTONS) || defined(HAVE_9_BUTTONS)
    {APP_LEAKTHROUGH_ENABLE,          ui_provider_audio_curation,  context_leakthrough_disabled,         ui_input_leakthrough_on                       },
    {APP_LEAKTHROUGH_DISABLE,         ui_provider_audio_curation,  context_leakthrough_enabled,          ui_input_leakthrough_off                      },
    {APP_LEAKTHROUGH_SET_NEXT_MODE,   ui_provider_audio_curation,  context_leakthrough_enabled,          ui_input_leakthrough_set_next_mode            },

    {APP_BUTTON_VOLUME_UP_DOWN,       ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_volume_stop                      },
    {APP_BUTTON_VOLUME_UP_DOWN,       ui_provider_media_player,    context_av_is_streaming,              ui_input_av_volume_down_remote_stop           },
    {APP_BUTTON_VOLUME_UP_DOWN,       ui_provider_hfp,             context_hfp_connected,                ui_input_hfp_volume_stop                      },
    {APP_BUTTON_VOLUME_UP_DOWN,       ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_mute_toggle                      },

    {APP_BUTTON_VOLUME_DOWN,          ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_volume_down_start                },
    {APP_BUTTON_VOLUME_DOWN,          ui_provider_media_player,    context_av_is_streaming,              ui_input_av_volume_down_start                 },
    {APP_BUTTON_VOLUME_DOWN,          ui_provider_hfp,             context_hfp_connected,                ui_input_hfp_volume_down_start                },

    {APP_BUTTON_VOLUME_UP,            ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_volume_up_start                  },
    {APP_BUTTON_VOLUME_UP,            ui_provider_media_player,    context_av_is_streaming,              ui_input_av_volume_up_start                   },
    {APP_BUTTON_VOLUME_UP,            ui_provider_hfp,             context_hfp_connected,                ui_input_hfp_volume_up_start                  },

    {APP_BUTTON_VOLUME_DOWN_RELEASE,  ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_volume_stop                      },
    {APP_BUTTON_VOLUME_DOWN_RELEASE,  ui_provider_media_player,    context_av_is_streaming,              ui_input_av_volume_down_remote_stop           },
    {APP_BUTTON_VOLUME_DOWN_RELEASE,  ui_provider_hfp,             context_hfp_connected,                ui_input_hfp_volume_stop                      },

    {APP_BUTTON_VOLUME_UP_RELEASE,    ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_volume_stop                      },
    {APP_BUTTON_VOLUME_UP_RELEASE,    ui_provider_media_player,    context_av_is_streaming,              ui_input_av_volume_up_remote_stop             },
    {APP_BUTTON_VOLUME_UP_RELEASE,    ui_provider_hfp,             context_hfp_connected,                ui_input_hfp_volume_stop                      },
#endif /* HAVE_4_BUTTONS || HAVE_6_BUTTONS || HAVE_7_BUTTONS || HAVE_9_BUTTONS */

#if defined(HAVE_6_BUTTONS) || defined(HAVE_7_BUTTONS) || defined(HAVE_9_BUTTONS)
    {APP_BUTTON_FORWARD,              ui_provider_media_player,    context_av_is_streaming,              ui_input_av_forward                           },
    {APP_BUTTON_FORWARD_HELD,         ui_provider_media_player,    context_av_is_streaming,              ui_input_av_fast_forward_start                },
    {APP_BUTTON_FORWARD_HELD_RELEASE, ui_provider_media_player,    context_av_is_streaming,              ui_input_fast_forward_stop                    },
    {APP_BUTTON_BACKWARD,             ui_provider_media_player,    context_av_is_streaming,              ui_input_av_backward                          },
    {APP_BUTTON_BACKWARD_HELD,        ui_provider_media_player,    context_av_is_streaming,              ui_input_av_rewind_start                      },
    {APP_BUTTON_BACKWARD_HELD_RELEASE,ui_provider_media_player,    context_av_is_streaming,              ui_input_rewind_stop                          },
#endif /* HAVE_6_BUTTONS || HAVE_7_BUTTONS || HAVE_9_BUTTONS */
#ifdef INCLUDE_CAPSENSE
    {CAP_SENSE_DOUBLE_PRESS,          ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_voice_call_hang_up                   },
    {CAP_SENSE_DOUBLE_PRESS,          ui_provider_hfp,             context_hfp_voice_call_sco_inactive,  ui_input_voice_call_hang_up                   },
    {CAP_SENSE_DOUBLE_PRESS,          ui_provider_hfp,             context_hfp_voice_call_outgoing,      ui_input_voice_call_hang_up                   },
    {CAP_SENSE_DOUBLE_PRESS,          ui_provider_hfp,             context_hfp_voice_call_incoming,      ui_input_voice_call_accept                    },
    {CAP_SENSE_DOUBLE_PRESS,          ui_provider_media_player,    context_av_is_streaming,              ui_input_toggle_play_pause                    },
    {CAP_SENSE_DOUBLE_PRESS,          ui_provider_media_player,    context_av_connected,                 ui_input_toggle_play_pause                    },
    {CAP_SENSE_DOUBLE_PRESS,          ui_provider_device,          context_handset_not_connected,        ui_input_connect_handset                      },

    {CAP_SENSE_SLIDE_DOWN,            ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_volume_down_start                },
    {CAP_SENSE_SLIDE_DOWN,            ui_provider_media_player,    context_av_is_streaming,              ui_input_av_volume_down_start                 },
    {CAP_SENSE_SLIDE_DOWN,            ui_provider_hfp,             context_hfp_connected,                ui_input_hfp_volume_down_start                },

    {CAP_SENSE_SLIDE_UP,              ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_volume_up_start                  },
    {CAP_SENSE_SLIDE_UP,              ui_provider_media_player,    context_av_is_streaming,              ui_input_av_volume_up_start                   },
    {CAP_SENSE_SLIDE_UP,              ui_provider_hfp,             context_hfp_connected,                ui_input_hfp_volume_up_start                  },

    {CAP_SENSE_TRIPLE_PRESS,          ui_provider_hfp,             context_hfp_voice_call_sco_active,    ui_input_hfp_volume_stop                      },
    {CAP_SENSE_TRIPLE_PRESS,          ui_provider_media_player,    context_av_is_streaming,              ui_input_av_volume_up_remote_stop             },
    {CAP_SENSE_TRIPLE_PRESS,          ui_provider_hfp,             context_hfp_connected,                ui_input_hfp_volume_stop                      },

#ifdef ENABLE_ANC    
    {CAP_SENSE_FIVE_PRESS,            ui_provider_audio_curation,  context_anc_disabled,                 ui_input_anc_toggle_on_off                    },
    {CAP_SENSE_FIVE_PRESS,            ui_provider_audio_curation,  context_anc_enabled,                  ui_input_anc_toggle_on_off                    },
    {CAP_SENSE_SIX_PRESS,             ui_provider_audio_curation,  context_anc_enabled,                  ui_input_anc_set_next_mode                    },
#endif
    {CAP_SENSE_SEVEN_PRESS,           ui_provider_phy_state,       context_phy_state_out_of_case,        ui_input_sm_pair_handset                      },
    {CAP_SENSE_EIGHT_PRESS,           ui_provider_phy_state,       context_phy_state_out_of_case,        ui_input_sm_delete_handsets                   },

    {CAP_SENSE_FOUR_PRESS_HOLD,       ui_provider_phy_state,       context_phy_state_out_of_case,        ui_input_dfu_active_when_in_case_request      },
    {CAP_SENSE_VERY_VERY_VERY_LONG_PRESS,  ui_provider_phy_state,  context_phy_state_out_of_case,        ui_input_factory_reset_request                },
#endif
};

const ui_config_table_content_t* EarbudUi_GetConfigTable(unsigned* table_length)
{
    *table_length = ARRAY_DIM(earbud_ui_config_table);
    return earbud_ui_config_table;
}

#ifdef INCLUDE_CAPSENSE
const touchEventConfig* EarbudUi_GetCapsenseEventTable(unsigned* table_length)
{
    *table_length = ARRAY_DIM(touch_event_table);
    return touch_event_table;
}
#endif
