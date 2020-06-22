/*******************************************************************************
Copyright (c) 2015-2020 Qualcomm Technologies International, Ltd.


FILE NAME
    anc.c

DESCRIPTION
    ANC VM Library API functions.
*/

#include "anc.h"
#include "anc_sm.h"
#include "anc_data.h"
#include "anc_debug.h"
#include "anc_config_read.h"

#include <stdlib.h>

/******************************************************************************/
bool AncInit(anc_mic_params_t *mic_params, anc_mode_t init_mode)
{
    anc_state_event_initialise_args_t args;
    anc_state_event_t event = {anc_state_event_initialise, NULL};

    args.mic_params = mic_params;
    args.mode = init_mode;
    event.args = &args;

    return ancStateMachineHandleEvent(event);
}

/******************************************************************************/
#ifdef HOSTED_TEST_ENVIRONMENT
bool AncLibraryTestReset(void)
{
    return ancDataDeinitialise();
}
#endif

/******************************************************************************/
bool AncEnable(bool enable)
{
    anc_state_event_t event = {anc_state_event_disable, NULL};

    if(enable)
    {
        event.id = anc_state_event_enable;
    }

    return ancStateMachineHandleEvent(event);
}

/******************************************************************************/
bool AncSetMode(anc_mode_t mode)
{
    anc_state_event_set_mode_args_t args;
    anc_state_event_t event = {anc_state_event_set_mode, NULL};

    /* Assign args to the set mode event */
    args.mode = mode;
    event.args = &args;

    return ancStateMachineHandleEvent(event);
}

/******************************************************************************/
bool AncIsEnabled(void)
{
    /* Get current state to determine if ANC is enabled */
    anc_state state = ancDataGetState();

    /* If library has not been initialised then it is invalid to call this function */
    ANC_ASSERT(state != anc_state_uninitialised);

    /* ANC is enabled in any state greater than anc_state_disabled, which allows
       the above assert to be compiled out if needed and this function still 
       behave as expected. */
    return (state > anc_state_disabled) ? TRUE : FALSE;
}

/******************************************************************************/

anc_mic_params_t * AncGetAncMicParams(void)
{
    return ancDataGetMicParams();
}
/******************************************************************************/
bool AncConfigureFFAPathGain(audio_anc_instance instance, uint8 gain)
{
    anc_state_event_set_path_gain_args_t args;
    anc_state_event_t event = {anc_state_event_set_filter_path_gain, NULL};

    /* Assign args to the set filter path gain event */
    args.instance = instance;
    args.path = AUDIO_ANC_PATH_ID_FFA;
    args.gain = gain;
    event.args = &args;

    return ancStateMachineHandleEvent(event);
}

/******************************************************************************/
bool AncConfigureFFBPathGain(audio_anc_instance instance, uint8 gain)
{
    anc_state_event_set_path_gain_args_t args;
    anc_state_event_t event = {anc_state_event_set_filter_path_gain, NULL};

    /* Assign args to the set filter path gain event */
    args.instance = instance;
    args.path = AUDIO_ANC_PATH_ID_FFB;
    args.gain = gain;
    event.args = &args;

    return ancStateMachineHandleEvent(event);
}

/******************************************************************************/
bool AncConfigureFBPathGain(audio_anc_instance instance, uint8 gain)
{
    anc_state_event_set_path_gain_args_t args;
    anc_state_event_t event = {anc_state_event_set_filter_path_gain, NULL};

    /* Assign args to the set filter path gain event */
    args.instance = instance;
    args.path = AUDIO_ANC_PATH_ID_FB;
    args.gain = gain;
    event.args = &args;

    return ancStateMachineHandleEvent(event);
}

/******************************************************************************/
bool AncReadFineGain(audio_anc_path_id gain_path, uint8 *gain)
{
    return ancReadFineGain(ancDataGetMode(), gain_path, gain);
}

/******************************************************************************/
bool AncWriteFineGain(audio_anc_path_id gain_path, uint8 gain)
{
    anc_state_event_write_gain_args_t args;
    anc_state_event_t event = {anc_state_event_write_fine_gain, NULL};

    /* Assign args to the event */
    args.path = gain_path;
    args.gain = gain;
    event.args = &args;

    return ancStateMachineHandleEvent(event);
}


void AncReadCoarseGainFromInstance(audio_anc_instance inst,audio_anc_path_id gain_path, uint8 *gain)
{
    if (gain != NULL)
    {
        ancReadGainFromInst(inst, gain_path, FALSE, gain);
    }
}

/******************************************************************************/
void AncReadFineGainFromInstance(audio_anc_instance inst,audio_anc_path_id gain_path, uint8 *gain)
{
    if (gain != NULL)
    {
        ancReadGainFromInst(inst, gain_path, TRUE, gain);
    }
}
