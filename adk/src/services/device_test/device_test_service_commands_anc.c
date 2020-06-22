/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the auth commands for device test service
*/

#include "device_test_service.h"
#include "device_test_service_auth.h"
#include "device_test_parse.h"

#include <anc.h>
#include <anc_state_manager.h>
#include <logging.h>

#include <stdio.h>

#ifdef INCLUDE_DEVICE_TEST_SERVICE

/*! \name Range of ANC mode value supported in the AT command.

    These are mapped onto the equivalent anc_mode_x enumerated
    type.
 */
/*! @{ */
#define MIN_AT_ANC_MODE     1
#define MAX_AT_ANC_MODE     10
/* @} */

/*! \name Mapping of gain_path parameters in the AT commands to
    defines used by the ANC library
 */
/*! @{ */
#define FFA_PATH    1   /*!< Feed Forward path A (left) */
#define FFB_PATH    2   /*!< Feed Forward path B (right) */
#define FB_PATH     3   /*!< Feed Backwards path */
/*! @} */

/*! Response message sent to the read fine gain command */
#define ANC_READ_FINE_GAIN_RESPONSE "+ANCREADFINEGAIN:"
/*! Maximum length of the response message, allowing for an 8
    bit value (max '255' - 3 digits) */
#define FULL_FINE_GAIN_RESPONSE_LEN (sizeof(ANC_READ_FINE_GAIN_RESPONSE) + 3)


/*! Map the ANC mode supplied in the AT command to a mode understood by libraries 

    \param requested_mode   The mode value from the AT command
    \param[out] anc_mode    Pointer to location to take the library mode value

    \return TRUE if the value passed was legal, and value could be mapped. 
            FALSE otherwise
*/
static bool deviceTestService_AncMapMode(uint16 requested_mode, anc_mode_t* anc_mode)
{
    if (MIN_AT_ANC_MODE <= requested_mode && requested_mode <= MAX_AT_ANC_MODE)
    {
        *anc_mode = (anc_mode_t)(requested_mode - MIN_AT_ANC_MODE + anc_mode_1);
        return TRUE;
    }
    DEBUG_LOG_WARN("deviceTestService_AncMapMode Attempt to map a bad ANC mode:%d",requested_mode);
    return FALSE;
}

/*! Map the gain path supplied in the AT command to a mode understood by libraries 

    \param requested_gain_path   The gain_path value from the AT command
    \param[out] gain_path        Pointer to location to take the library gain_path value

    \return TRUE if the value passed was legal, and value could be mapped. 
            FALSE otherwise
*/
static bool deviceTestService_AncMapGainPath(uint16 requested_gain_path, 
                                           audio_anc_path_id *gain_path)
{
    switch (requested_gain_path)
    {
        case FFA_PATH:
            *gain_path = AUDIO_ANC_PATH_ID_FFA;
            break;

        case FFB_PATH:
            *gain_path = AUDIO_ANC_PATH_ID_FFB;
            break;

        case FB_PATH:
            *gain_path = AUDIO_ANC_PATH_ID_FB;
            break;

        default:
            DEBUG_LOG_WARN("deviceTestService_AncMapGainPath. Attempt to map a bad gain path:%d", requested_gain_path);
            return FALSE;
    }

    return TRUE;
}


static bool deviceTestService_AncCommandsAllowed(void)
{
    return DeviceTestService_CommandsAllowed() && AncStateManager_IsSupported();
}


/*! Handle the AT+ANCENABLE command

    Handle the enable, making sure to send either an "OK" or "ERROR" response.

    \note ANC Enable needs the audio sub system to be started, which can
            cause a delay before the OK response is sent.

    \param task Identifier for the task that issued the command. This should be
                passed when responding to the command.
    \param[in] ancEnable Structure supplied by the AT command parser, providing
                    the parameters passed in the AT command (the requested mode to
                    enable)
*/
void DeviceTestServiceCommand_HandleAncEnable(Task task, 
                const struct DeviceTestServiceCommand_HandleAncEnable *ancEnable)
{
    anc_mode_t anc_mode;
    uint8 gain;

    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleAncEnable. mode:%d", ancEnable->mode);

    if (   !deviceTestService_AncCommandsAllowed()
        || !deviceTestService_AncMapMode(ancEnable->mode, &anc_mode))
    {
        DeviceTestService_CommandResponseError(task);
        return;
    }

    if (!AncStateManager_IsEnabled())
    {
        AncStateManager_Enable();
    }

    AncSetMode(anc_mode);

    /* The return value from AncSetMode does not indicate whether the mode
       set was successful, so attempt a read - which will return ERROR 
       if the mode was invalid mode */
    DeviceTestService_CommandResponseOkOrError(task, 
                            AncReadFineGain(AUDIO_ANC_PATH_ID_FFA, &gain));
}


/*! Handle the AT+ANCDISABLE command

    Handle the disable, making sure to send either an "OK" or "ERROR" response.

    \param task Identifier for the task that issued the command. This should be
                passed when responding to the command.
*/
void DeviceTestServiceCommand_HandleAncDisable(Task task)
{
    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleAncDisable");
    
    if (!deviceTestService_AncCommandsAllowed())
    {
        DeviceTestService_CommandResponseError(task);
    }

    if (AncStateManager_IsEnabled())
    {
        AncStateManager_Disable();
    }

    DeviceTestService_CommandResponseOk(task);
}


/*! Handle the AT+ANCSETFINEGAIN command

    Handle the command, making sure to send either an "OK" or "ERROR" response.

    Parameters are checked, any mistakes causing an error response.

    \param task Identifier for the task that issued the command. This should be
                passed when responding to the command.
    \param[in] ancSetFineGain Structure supplied by the AT command parser, providing
                              the parameters passed in the AT command (the requested 
                              mode, gain path and gain value)
*/
void DeviceTestServiceCommand_HandleAncSetFineGain(Task task, 
                    const struct DeviceTestServiceCommand_HandleAncSetFineGain *ancSetFineGain)
{
    anc_mode_t anc_mode;
    audio_anc_path_id gain_path;
    bool status = FALSE;

    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleAncSetFindGain  mode:%u Path:%u Gain:%u ",
                     ancSetFineGain->mode,
                     ancSetFineGain->gainpath, ancSetFineGain->gainvalue);

    if (   !deviceTestService_AncCommandsAllowed()
        || !AncStateManager_IsEnabled()
        || !deviceTestService_AncMapMode(ancSetFineGain->mode, &anc_mode)
        || !deviceTestService_AncMapGainPath(ancSetFineGain->gainpath, &gain_path)
        || ancSetFineGain->gainvalue > 0xFF)
    {
        DeviceTestService_CommandResponseError(task);
        return;
    }

    /*Set gain for currently set mode in DSP*/
    switch(gain_path)
    {
        case FFA_PATH:
            status = AncConfigureFFAPathGain(AUDIO_ANC_INSTANCE_0,
                                             (uint8)ancSetFineGain->gainvalue);
            break;

        case FFB_PATH:
            status = AncConfigureFFBPathGain(AUDIO_ANC_INSTANCE_0,
                                             (uint8)ancSetFineGain->gainvalue);
            break;

        case FB_PATH:
            status = AncConfigureFBPathGain(AUDIO_ANC_INSTANCE_0,
                                            (uint8)ancSetFineGain->gainvalue);
            break;

        default:
            /* Panic here as the mapping should have ensured a valid gain_path
               A Panic would indicate a new gain_path should be supported but 
               has not been implemented here */
            Panic();
            break;
    }

    DeviceTestService_CommandResponseOkOrError(task, status);
}


/*! Handle the AT+ANCREADFINEGAIN command

    Handle the command, making sure to send either an "OK" or "ERROR" response.

    Parameters are checked, any mistakes causing an error response.

    \param task Identifier for the task that issued the command. This should be
                passed when responding to the command.
    \param[in] ancReadFineGain Structure supplied by the AT command parser, providing
                               the parameters passed in the AT command (the requested 
                               mode and gain path)
*/
void DeviceTestServiceCommand_HandleAncReadFineGain(Task task, 
                const struct DeviceTestServiceCommand_HandleAncReadFineGain *ancReadFineGain)
{
    anc_mode_t anc_mode;
    audio_anc_path_id gain_path;
    uint8 gain=0;
    char response[FULL_FINE_GAIN_RESPONSE_LEN];

    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleAncReadFineGain mode:%u Path:%u ",
                     ancReadFineGain->mode, ancReadFineGain->gainpath);

    if (   !deviceTestService_AncCommandsAllowed()
        || !AncStateManager_IsEnabled()
        || !deviceTestService_AncMapMode(ancReadFineGain->mode, &anc_mode)
        || !deviceTestService_AncMapGainPath(ancReadFineGain->gainpath, &gain_path)
        || !AncReadFineGain(gain_path, &gain))
    {
        DeviceTestService_CommandResponseError(task);
        return;
    }

    /* We don't have a handy function for itoa and it is not commonly
       needed. sprintf is available, and is an embedded one so relatively
       efficient */
    sprintf(response, ANC_READ_FINE_GAIN_RESPONSE "%d", gain);

    DeviceTestService_CommandResponse(task, response, FULL_FINE_GAIN_RESPONSE_LEN);
    DeviceTestService_CommandResponseOk(task);
}


/*! Handle the AT+ANCWRITEFINEGAIN command

    Handle the command, making sure to send either an "OK" or "ERROR" response.

    Parameters are checked, any mistakes causing an error response.

    \param task Identifier for the task that issued the command. This should be
                passed when responding to the command.
    \param[in] ancWriteFineGain Structure supplied by the AT command parser, providing
                                the parameters passed in the AT command (the requested 
                                mode, gain path and gain value)
*/
void DeviceTestServiceCommand_HandleAncWriteFineGain(Task task, 
                const struct DeviceTestServiceCommand_HandleAncWriteFineGain *ancWriteFineGain)
{
    anc_mode_t anc_mode;
    audio_anc_path_id gain_path;

    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleAncWriteFindGain  mode:%u Path:%u Gain:%u ",
                     ancWriteFineGain->mode,
                     ancWriteFineGain->gainpath, ancWriteFineGain->gainvalue);

    if (   !deviceTestService_AncCommandsAllowed()
        || !AncStateManager_IsEnabled()
        || !deviceTestService_AncMapMode(ancWriteFineGain->mode, &anc_mode)
        || !deviceTestService_AncMapGainPath(ancWriteFineGain->gainpath, &gain_path)
        || ancWriteFineGain->gainvalue > 0xFF
        || !AncWriteFineGain(gain_path, ancWriteFineGain->gainvalue ))
    {
        DeviceTestService_CommandResponseError(task);
        return;
    }

    DeviceTestService_CommandResponseOk(task);
}

#else /* INCLUDE_DEVICE_TEST_SERVICE */

/* Stub command included if the device test service is not included.
   Allows initial compile and link. Should be remmoved in final link as functions unused */
void DeviceTestServiceCommand_HandleAncEnable(Task task, 
                const struct DeviceTestServiceCommand_HandleAncEnable *ancEnable)
{
    UNUSED(task);
    UNUSED(ancEnable);
}

/* Stub command included if the device test service is not included.
   Allows initial compile and link. Should be remmoved in final link as functions unused */
void DeviceTestServiceCommand_HandleAncDisable(Task task)
{
    UNUSED(task);
}

/* Stub command included if the device test service is not included.
   Allows initial compile and link. Should be remmoved in final link as functions unused */
void DeviceTestServiceCommand_HandleAncSetFineGain(Task task, 
                    const struct DeviceTestServiceCommand_HandleAncSetFineGain *ancSetFineGain)
{
    UNUSED(task);
    UNUSED(ancSetFineGain);
}

/* Stub command included if the device test service is not included.
   Allows initial compile and link. Should be remmoved in final link as functions unused */
void DeviceTestServiceCommand_HandleAncReadFineGain(Task task, 
                const struct DeviceTestServiceCommand_HandleAncReadFineGain *ancReadFineGain)
{
    UNUSED(task);
    UNUSED(ancReadFineGain);
}


/* Stub command included if the device test service is not included.
   Allows initial compile and link. Should be remmoved in final link as functions unused */
void DeviceTestServiceCommand_HandleAncWriteFineGain(Task task, 
                const struct DeviceTestServiceCommand_HandleAncWriteFineGain *ancWriteFineGain)
{
    UNUSED(task);
    UNUSED(ancWriteFineGain);
}

#endif /* INCLUDE_DEVICE_TEST_SERVICE */
