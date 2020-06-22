/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_config.h
\brief      Configuration defintions and stubs for Active Noise Cancellation (ANC). 
*/

#ifndef ANC_CONFIG_H_
#define ANC_CONFIG_H_

#ifdef ENABLE_ANC

#include <anc.h>

/*
 * There is no config manager setup yet, so hard-code the default value
 * by referring ..module_configurations/anc_def.xml
 */

typedef struct {
    unsigned short feed_back_right_mic:4;
    unsigned short feed_back_left_mic:4;
    unsigned short feed_forward_right_mic:4;
    unsigned short feed_forward_left_mic:4;
} anc_mic_params_r_config_t;

#define ANC_READONLY_CONFIG_BLK_ID 1155

typedef struct {
    anc_mic_params_r_config_t anc_mic_params_r_config;
    unsigned short num_anc_modes:8;
} anc_readonly_config_def_t;

#define ANC_WRITEABLE_CONFIG_BLK_ID 1162

typedef struct {
    unsigned short persist_initial_mode:1;
    unsigned short persist_initial_state:1;
    unsigned short initial_anc_state:1;
    unsigned short initial_anc_mode:4;
} anc_writeable_config_def_t;

#define EVENTS_USR_MESSAGE_BASE (0x4000)

/*This enum is ANC specific user events and should be removed once peer service layer is provided  */
typedef enum EventsTag
{
 /* USER EVENTS */
    
/*0x4000*/    EventInvalid = EVENTS_USR_MESSAGE_BASE,

/*0x40E4*/    EventUsrAncOn,
/*0x40E5*/    EventUsrAncOff,
/*0x40E6*/    EventUsrAncToggleOnOff,
/*0x40E7*/    EventUsrAncMode1,
/*0x40E8*/    EventUsrAncMode2,
/*0x40E9*/    EventUsrAncNextMode,
/*0x40FF*/    EventUsrEnterAncTuningMode,

/*0x4101*/    EventUsrAncMode3,
/*0x4102*/    EventUsrAncMode4,
/*0x4103*/    EventUsrAncMode5,
/*0x4104*/    EventUsrAncMode6,
/*0x4105*/    EventUsrAncMode7,
/*0x4106*/    EventUsrAncMode8,
/*0x4107*/    EventUsrAncMode9,
/*0x4108*/    EventUsrAncMode10,

/* User events list cannot go past 0x43FF */
              EventUsrLast,
} Events_t;


extern anc_readonly_config_def_t anc_readonly_config;

uint16 ancConfigManagerGetReadOnlyConfig(uint16 config_id, const void **data);
void ancConfigManagerReleaseConfig(uint16 config_id);
uint16 ancConfigManagerGetWriteableConfig(uint16 config_id, void **data, uint16 size);
void ancConfigManagerUpdateWriteableConfig(uint16 config_id);

#ifdef ANC_PEER_SUPPORT
bool ancPeerProcessEvent(MessageId id);
bool ancPeerIsLinkMaster(void);
void ancPeerSendAncState (void);
void ancPeerSendAncMode (void);
#endif /* ANC_PEER_SUPPORT*/

#endif /* ENABLE_ANC */
#endif /* ANC_CONFIG_H_ */
