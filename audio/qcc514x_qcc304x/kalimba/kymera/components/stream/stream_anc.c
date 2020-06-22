/*******************************************************************************
 * Copyright (c) 2009 - 2017 Qualcomm Technologies International, Ltd.
*******************************************************************************/

/**
 * \file  stream_anc.c
 * \ingroup stream
 *
 * Control the ANC audio HW <br>
 * This file contains the stream ANC operator shim functions. <br>
 *
 *  \note This file contains the operator access functions for Active Noise Cancellation (ANC).
 *  It consists of several shim functions that provide an interface between instances of
 *  (downloaded) Kymera capabilities and the ANC HAL layer (without which access would not
 *  be possible).
 *
 */

#if !defined(HAVE_SIDE_TONE_HARDWARE)
    #error "HAVE_SIDE_TONE_HARDWARE must be defined "
           "when INSTALL_UNINTERRUPTABLE_ANC is defined"
#endif /* !defined(HAVE_SIDE_TONE_HARDWARE) */

/*******************************************************************************
Include Files
*/


#include "hal_audio.h"
#include "patch/patch.h"
#include "stream/stream_for_override.h"
#include "stream/stream_for_opmgr.h"
#include "stream/stream_for_ops.h"
#include "stream/stream_for_anc.h"
#include "stream/stream_endpoint.h"
#include "audio_hwm_anc.h"
#if defined(INSTALL_CLK_MGR)
#include "clk_mgr/clk_mgr.h"
#endif

/*******************************************************************************
 * Private macros/consts
 */

#ifdef STREAM_ANC_ENABLE_L5_DBG_MSG

#define STREAM_ANC_L5_DBG_MSG(x)                  L5_DBG_MSG(x)
#define STREAM_ANC_L5_DBG_MSG1(x, a)              L5_DBG_MSG1(x, a)
#define STREAM_ANC_L5_DBG_MSG2(x, a, b)           L5_DBG_MSG2(x, a, b)
#define STREAM_ANC_L5_DBG_MSG3(x, a, b, c)        L5_DBG_MSG3(x, a, b, c)
#define STREAM_ANC_L5_DBG_MSG4(x, a, b, c, d)     L5_DBG_MSG4(x, a, b, c, d)
#define STREAM_ANC_L5_DBG_MSG5(x, a, b, c, d, e)  L5_DBG_MSG5(x, a, b, c, d, e)

#else  /* STREAM_ANC_ENABLE_L5_DBG_MSG */

#define STREAM_ANC_L5_DBG_MSG(x)                  ((void)0)
#define STREAM_ANC_L5_DBG_MSG1(x, a)              ((void)0)
#define STREAM_ANC_L5_DBG_MSG2(x, a, b)           ((void)0)
#define STREAM_ANC_L5_DBG_MSG3(x, a, b, c)        ((void)0)
#define STREAM_ANC_L5_DBG_MSG4(x, a, b, c, d)     ((void)0)
#define STREAM_ANC_L5_DBG_MSG5(x, a, b, c, d, e)  ((void)0)

#endif /* STREAM_ANC_ENABLE_L5_DBG_MSG */

/*******************************************************************************
Private Function Declarations
*/
#if defined(INSTALL_CLK_MGR) && !defined(UNIT_TEST_BUILD)
static inline bool kcodec_is_clocked(void)
{
    return clk_mgr_kcodec_is_clocked();
}
#else
#define kcodec_is_clocked() TRUE
#endif

/*******************************************************************************
Private Function Definitions
*/

/**
 * \brief Derive an ANC filter ID from the source configure key.
 *
 * \param key ANC instance ID (e.g. ANC0, ANC1).
 *
 * \return path_id Path ID that is derived from the key.
 */
static STREAM_ANC_PATH key_to_id(STREAM_CONFIG_KEY key);

/**
 * \brief dummy response callback function.
 *
 */
static bool stream_anc_dummy_callback(CONNECTION_LINK dummy_con_id,
                                      STATUS_KYMERA dummy_status)
{
    return TRUE;
}
/*******************************************************************************
Public Function Definitions
*/

/**
 * \brief Configure an ANC IIR filter (sets the coefficients)
 *
 * \param instance   ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id    ANC input path ID (e.g. FFA, FFB, FB).
 * \param num_coeffs Number of coefficients.
 * \param coeffs     Pointer to an array of IIR coefficients.
 *
 * \return TRUE if successful
 */
bool stream_anc_set_anc_iir_coeffs(STREAM_ANC_INSTANCE instance,
                                   STREAM_ANC_PATH path_id,
                                   unsigned num_coeffs,
                                   const uint16 *coeffs)
{
    patch_fn_shared(stream_anc);

    return audio_hwm_anc_set_anc_iir_filter(instance,
                                            path_id,
                                            (uint16) num_coeffs,
                                            coeffs);
}

/**
 * \brief Select the currently active IIR coefficient set
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param coeff_set coefficient set 0: Foreground, 1: background.
 *
 * \note  Coefficients for the FFA, FFB and FB IIR filters are banked
 *        (LPF shift coefficients are not banked)
 */
void stream_anc_select_active_iir_coeffs(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_BANK coeff_set)
{
    patch_fn_shared(stream_anc);

    audio_hwm_anc_select_active_iir_coeffs(anc_instance, coeff_set);
}



/**
 * \brief Copy the foreground coefficient set to the background coefficient set
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 *
 * \note  Coefficients for the FFA, FFB and FB IIR filters are banked
 *        (LPF shift coefficients are not banked)
 */
void stream_anc_update_background_iir_coeffs(STREAM_ANC_INSTANCE anc_instance)
{
    patch_fn_shared(stream_anc);

    audio_hwm_anc_update_background_iir_coeffs(anc_instance);
}


/**
 * \brief Configure an ANC LPF filter (sets the LPF coefficients)
 *
 * \param instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id  ANC input path ID (e.g. FFA, FFB, FB).
 * \param shift1   Coefficient 1 expressed as a shift.
 * \param shift2   Coefficient 2 expressed as a shift.
 *
 * \return TRUE if successful
 */
bool stream_anc_set_anc_lpf_coeffs(STREAM_ANC_INSTANCE instance,
                                   STREAM_ANC_PATH path_id,
                                   uint16 shift1, uint16 shift2)
{
    patch_fn_shared(stream_anc);

    if (!kcodec_is_clocked())
    {
        return FALSE;
    }

    return audio_hwm_anc_set_anc_lpf_filter(instance, path_id, shift1, shift2);
}

#if !defined(UNIT_TEST_BUILD)

#define STREAM_ANC_CONTROL_EN_0_FBTUNEOUT0_SEL_MASK 0x0001
#define STREAM_ANC_CONTROL_EN_0_FBTUNEOUT1_SEL_MASK 0x0002
#define STREAM_ANC_CONTROL_EN_1_FBTUNEOUT0_SEL_MASK 0x0004
#define STREAM_ANC_CONTROL_EN_1_FBTUNEOUT1_SEL_MASK 0x0008
#define STREAM_ANC_CONTROL_EN_2_FBTUNEOUT0_SEL_MASK 0x0010
#define STREAM_ANC_CONTROL_EN_2_FBTUNEOUT1_SEL_MASK 0x0020
#define STREAM_ANC_CONTROL_EN_3_FBTUNEOUT0_SEL_MASK 0x0040
#define STREAM_ANC_CONTROL_EN_3_FBTUNEOUT1_SEL_MASK 0x0080

#define DEC_CHAIN_0 0
#define DEC_CHAIN_1 1
#define DEC_CHAIN_2 2
#define DEC_CHAIN_3 3

/**
 * \brief Configure ANC tuning options
 *
 * \param instance ANC instance ID (e.g. ANC0, ANC1).
 * \param chain    Which chain must be tuned.
 *
 */
void stream_anc_set_anc_tune(STREAM_ANC_INSTANCE instance,
                             unsigned chain)
{
    uint32 enable;
    uint32 select;

    patch_fn_shared(stream_anc);

    switch (chain)
    {
        case DEC_CHAIN_3:
            select = STREAM_ANC_CONTROL_EN_3_FBTUNEOUT0_SEL_MASK |
                     STREAM_ANC_CONTROL_EN_3_FBTUNEOUT1_SEL_MASK;
            enable = STREAM_ANC_CONTROL_EN_3_FBTUNEOUT1_SEL_MASK;
            break;
        case DEC_CHAIN_2:
            select = STREAM_ANC_CONTROL_EN_2_FBTUNEOUT0_SEL_MASK |
                     STREAM_ANC_CONTROL_EN_2_FBTUNEOUT1_SEL_MASK;
            enable = STREAM_ANC_CONTROL_EN_2_FBTUNEOUT0_SEL_MASK;
            break;
        case DEC_CHAIN_1:
            select = STREAM_ANC_CONTROL_EN_1_FBTUNEOUT0_SEL_MASK |
                     STREAM_ANC_CONTROL_EN_1_FBTUNEOUT1_SEL_MASK;
            enable = STREAM_ANC_CONTROL_EN_1_FBTUNEOUT1_SEL_MASK;
           break;
        case DEC_CHAIN_0:
            select = STREAM_ANC_CONTROL_EN_0_FBTUNEOUT0_SEL_MASK |
                     STREAM_ANC_CONTROL_EN_0_FBTUNEOUT1_SEL_MASK;
            enable = STREAM_ANC_CONTROL_EN_0_FBTUNEOUT0_SEL_MASK;
           break;
        default:
            select = 0;
            enable = 0;
            break;
    }

    enable = enable << 16;
    select = select << 16;
    hal_audio_set_anc_control(instance, enable, select);
}

/**
 * \brief Copy the foreground gain set to the background gain set
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 *
 * \note  Gains for the FFA, FFB, and FB LPF are shadowed
 *        (but gain shifts are not)
 */
void stream_anc_update_background_gains(STREAM_ANC_INSTANCE anc_instance)
{
    patch_fn_shared(stream_anc);

    hal_update_background_gains(anc_instance);
}

/**
 * \brief Retrieve the ANC enable/disable state
 *
 * \param anc_enable_0_ptr Pointer to result (bitfield controlling instance ANC0 signal paths).
 * \param anc_enable_1_ptr Pointer to result (bitfield controlling instance ANC1 signal paths).
 *
 */
void stream_get_anc_enable(uint16 *anc_enable_0_ptr, uint16 *anc_enable_1_ptr)
{
    patch_fn_shared(stream_anc);

    /* Call the HAL layer to determine the ANC enable/disable state */
    hal_get_audio_anc_stream_anc_enable(anc_enable_0_ptr, anc_enable_1_ptr);
}

#if defined(INSTALL_ANC_CLIP_THRESHOLD)
/**
 * \brief Configure the clipping/threshold detection ANC output level
 *
 * \param anc_instance ANC instance
 * \param level threshold level to configure
 *
 * \return none
 */
void stream_anc_set_clip_level(
    STREAM_ANC_INSTANCE anc_instance,
    uint32 level)
{
    patch_fn_shared(stream_anc);

    hal_audio_anc_set_clip_level(
        anc_instance,
        level);
}

/**
 * \brief Enable/disable the ANC output threshold detector
 *
 * \param anc_instance ANC instance
 * \param callback NULL: disable ANC threshold detection
 *                 non-NUll: pointer to function to be called
 *                           on exceeding ANC detection threshold
 *
 * \return none
 */
void stream_anc_detect_enable(
    STREAM_ANC_INSTANCE anc_instance,
    void (*callback)(void))
{
    patch_fn_shared(stream_anc);

    hal_audio_anc_detect_enable(
        anc_instance,
        callback);
}
#endif /* defined(INSTALL_ANC_CLIP_THRESHOLD) */

#endif /* UNIT_TEST_BUILD */
/**
 * \brief Wrapper to enable/disable ANC with license check.
 *
 * \param con_id Feature ID (passed as first parameter to the callback)
 * \param anc_enable_0 Bitfield controlling instance ANC0 signal paths.
 * \param anc_enable_1 Bitfield controlling instance ANC1 signal paths.
 * \param resp_callback callback function pointer for sending the response.
 *
 * \note: Calls the secure ANC interface and supplies a user callback of
 * the form:
 *
 * bool stream_anc_dummy_callback(unsigned dummy_con_id, unsigned dummy_status)
 *
 * This can be used to determine the completion status of the command.
 */
void stream_anc_enable_wrapper(CONNECTION_LINK con_id,
                               uint16 anc_enable_0,
                               uint16 anc_enable_1,
                               bool (*resp_callback)(CONNECTION_LINK con_id,
                                                     STATUS_KYMERA status))
{
    bool (*wrapper_callback)(CONNECTION_LINK con_id, STATUS_KYMERA status);

    patch_fn_shared(stream_anc);

    /* Use a default dummy callback if none supplied */
    wrapper_callback = (resp_callback == NULL) ? stream_anc_dummy_callback : resp_callback;

    /* Call the existing ANC enable wrapper */
    audio_hwm_anc_stream_anc_enable_wrapper(con_id, anc_enable_0, anc_enable_1, wrapper_callback);
}

/**
 * \brief Place-holder function for patching purposes.
 *
 * \param ptr Pointer to function parameters (to be used as required)
 *
 */
void stream_anc_user1(void *ptr)
{
    patch_fn_shared(stream_anc);
}

/**
 * \brief Place-holder function for patching purposes.
 *
 * \param ptr Pointer to function parameters (to be used as required)
 *
 */
void stream_anc_user2(void *ptr)
{
    patch_fn_shared(stream_anc);
}

bool stream_anc_configure_input(ENDPOINT *ep, STREAM_ANC_PATH path)
{
    STREAM_CONFIG_KEY key = STREAM_CONFIG_KEY_STREAM_ANC_INPUT;
    return stream_configure_connected_to_endpoint(ep, key, (uint32) path);
}

bool stream_anc_configure_instance(ENDPOINT *ep, STREAM_ANC_INSTANCE instance)
{
    STREAM_CONFIG_KEY key = STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE;
    return stream_configure_connected_to_endpoint(ep, key, (uint32) instance);
}

/* TODO: proper bitfield description. */
bool stream_anc_configure_control(ENDPOINT *ep, uint32 bitfield)
{
    STREAM_CONFIG_KEY key = STREAM_CONFIG_KEY_STREAM_ANC_CONTROL;
    return stream_configure_connected_to_endpoint(ep, key, (uint32) bitfield);
}

bool stream_anc_configure_gain(ENDPOINT *ep,
                               STREAM_ANC_PATH path,
                               unsigned value)
{
    STREAM_CONFIG_KEY key;
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN;
            break;
        case STREAM_ANC_PATH_FFB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN;
            break;
        case STREAM_ANC_PATH_FB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN;
            break;
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep, key, (uint32) value);
}

bool stream_anc_configure_gain_shift(ENDPOINT *ep,
                                     STREAM_ANC_PATH path,
                                     unsigned value)
{
    STREAM_CONFIG_KEY key;
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN_SHIFT;
            break;
        case STREAM_ANC_PATH_FFB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN_SHIFT;
            break;
        case STREAM_ANC_PATH_FB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN_SHIFT;
            break;
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep, key, (uint32) value);
}

bool stream_anc_configure_dc_filter_enable(ENDPOINT *ep,
                                           STREAM_ANC_PATH path,
                                           unsigned value)
{
    STREAM_CONFIG_KEY key;
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_ENABLE;
            break;
        case STREAM_ANC_PATH_FFB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_ENABLE;
            break;
        case STREAM_ANC_PATH_SM_LPF_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_ENABLE;
            break;
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep, key, (uint32) value);
}

bool stream_anc_configure_dc_filter_shift(ENDPOINT *ep,
                                          STREAM_ANC_PATH path,
                                          unsigned value)
{
    STREAM_CONFIG_KEY key;
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_SHIFT;
            break;
        case STREAM_ANC_PATH_FFB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_SHIFT;
            break;
        case STREAM_ANC_PATH_SM_LPF_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_SHIFT;
            break;
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep, key, (uint32) value);
}

bool stream_anc_configure_dmic_x2_enable(ENDPOINT *ep,
                                         STREAM_ANC_PATH path,
                                         unsigned value)
{
    uint32 ff_dmic_x2_mask;

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            ff_dmic_x2_mask = 
             (STREAM_ANC_CONTROL_DMIC_X2_A_SEL_MASK \
                << STREAM_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT);

            if(value == 1)
            {
               ff_dmic_x2_mask |= STREAM_ANC_CONTROL_DMIC_X2_A_SEL_MASK;
            }

            break;

        case STREAM_ANC_PATH_FFB_ID:
            ff_dmic_x2_mask = 
             (STREAM_ANC_CONTROL_DMIC_X2_B_SEL_MASK \
                << STREAM_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT);

            if(value == 1)
            {
               ff_dmic_x2_mask |= STREAM_ANC_CONTROL_DMIC_X2_B_SEL_MASK;
            }

            break;
        case STREAM_ANC_PATH_SM_LPF_ID:
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep,
                            STREAM_CONFIG_KEY_STREAM_ANC_CONTROL,
                            ff_dmic_x2_mask);
}

unsigned stream_anc_get_filters_coeff_number(STREAM_ANC_PATH path)
{
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            return STREAM_ANC_IIR_FILTER_FFA_NUM_COEFFS;
        case STREAM_ANC_PATH_FFB_ID:
            return STREAM_ANC_IIR_FILTER_FFB_NUM_COEFFS;
        case STREAM_ANC_PATH_SM_LPF_ID:
            return STREAM_ANC_IIR_FILTER_FB_NUM_COEFFS;
        default:
            return 0;
    }
}

/**
 * \brief Perform ANC configuration via stream source configure.
 *
 * \param endpoint Pointer to the endpoint.
 * \param key      Key identifying the entity to configure.
 * \param value    Value to configure.
 *
 * \return TRUE if successful, FALSE if error.
 */
bool stream_anc_source_configure(ENDPOINT *endpoint,
                                 STREAM_CONFIG_KEY key,
                                 uint32 value)
{
    STREAM_ANC_INSTANCE instance_id;
    STREAM_ANC_PATH path_id;
    bool result;
    patch_fn_shared(stream_anc);

    STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
        "Configuring ANC, key=0x%x, value=0x%x",
        key, value);

    /* Check the endpoint pointer */
    if (endpoint == NULL)
    {
        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
            "Endpoint pointer is NULL");

        return FALSE;
    }

    /* ID of instance being configured (or set) */
    instance_id = stream_audio_anc_get_instance_id(endpoint);

    /* Check that this a valid instance (unless the instance is being set) */
    if ((audio_hwm_anc_is_anc_instance_valid(instance_id) == FALSE) &&
       (key != STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE))
    {
        STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
            "Endpoint has an unset/invalid ANC instance ID, key=0x%x, instance_id=0x%x",
            key, instance_id);

        return FALSE;
    }

    path_id = key_to_id(key);

    /* Configure source according to the key */
    switch (key)
    {
        /* Set the ANC instance associated with the endpoint */
        case STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE:
        {
            STREAM_ANC_INSTANCE stream_anc_instance_id = (STREAM_ANC_INSTANCE)value;

            if (audio_hwm_anc_is_anc_instance_valid_or_none(stream_anc_instance_id) == FALSE)
            {
                STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                    "Invalid ANC instance ID, key=0x%x, value=0x%x",
                    key, value);

                return FALSE;
            }

            /* Disable ANC endpoints */
            if (audio_hwm_anc_source_interface_disable(stream_anc_instance_id) == FALSE)
            {
                STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
                    "failed to disable ANC endpoints");

                return FALSE;
            }

#ifdef INSTALL_ANC_STICKY_ENDPOINTS
            /* Close the endpoint if a closure by the ANC is pending */
            if (stream_audio_anc_get_close_pending(endpoint))
            {
                STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
                    "closing source endpoint released by ANC");

                /* Close the endpoint now ANC has finished with it */
                if (!stream_close_endpoint(endpoint))
                {
                    STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
                        "failed to close source endpoint released by ANC");
                    return FALSE;
                }

                STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
                    "source endpoint released by ANC is now closed");

                /* No longer waiting for ANC to close the endpoint */
                stream_audio_anc_set_close_pending(endpoint, FALSE);
            }
#endif /* INSTALL_ANC_STICKY_ENDPOINTS */

            /* Associate/dissociate the source endpoint with the ANC instance ID */
            stream_audio_anc_set_instance_id(endpoint, stream_anc_instance_id);

            return TRUE;
        }

        /* Set the ANC input path associated with the endpoint */
        case STREAM_CONFIG_KEY_STREAM_ANC_INPUT:
        {
            STREAM_ANC_PATH stream_anc_path_id;
            STREAM_DEVICE hardware_type;
            audio_instance device_instance;
            audio_channel channel_number;

            /* Get the device type */
            hardware_type = (STREAM_DEVICE) stream_get_device_type(endpoint);

            /* Get the device instance */
            device_instance = (audio_instance) get_hardware_instance(endpoint);

            /* Get the channel number (note: zeroth channel is channel number 1) */
            channel_number = (audio_channel) (get_hardware_channel(endpoint) + 1);

            /* Check the input endpoint type */
            switch (hardware_type)
            {
                case STREAM_DEVICE_CODEC:
                case STREAM_DEVICE_DIGITAL_MIC:
                    break;

                default:
                    /* Can only support AMIC and DMIC ANC inputs */
                    STREAM_ANC_L5_DBG_MSG3("stream_anc: stream_anc_source_configure(): "
                        "Invalid ANC input endpoint type, key=0x%x, value=0x%x, hardware_type=0x%x",
                        key, value, hardware_type);

                    return FALSE;
            }

            STREAM_ANC_L5_DBG_MSG3("stream_anc: stream_anc_source_configure(): "
                "STREAM_ANC_INPUT, hardware_type=0x%x, device_instance=0x%x, channel_number=0x%x",
                hardware_type, device_instance, channel_number);

            /* Associate or disassociate the endpoint and the ANC input path */
            stream_audio_anc_set_input_path_id(endpoint, path_id);

            stream_anc_path_id = (STREAM_ANC_PATH)value;

            result = audio_hwm_anc_interface_update(instance_id,
                                                    stream_anc_path_id,
                                                    hardware_type,
                                                    device_instance,
                                                    channel_number);
            if (result == FALSE)
            {
                STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                    "Invalid ANC input path, key=0x%x, value=0x%x",
                    key, value);
            }

            return result;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_ENABLE:
        {
            uint16 val = (uint16)value;

            result = audio_hwm_anc_source_config_filter_enable(instance_id, path_id, val);
            if (result == FALSE)
            {
                STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                    "FILTER_ENABLE config FAIL, key=0x%x, value=0x%x",
                    key, value);
            }

            return result;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_SHIFT:
        {
            uint16 val = (uint16)value;

            result = audio_hwm_anc_source_config_filter_shift(instance_id, path_id, val);
            if (result == FALSE)
            {
                STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                    "FILTER_SHIFT config FAIL, key=0x%x, value=0x%x",
                    key, value);
            }

            return result;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN:
        {
            uint16 val = (uint16)value;

            result = audio_hwm_anc_source_config_gain(instance_id, path_id, val);
            if (result == FALSE)
            {
                STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                    "GAIN config FAIL, key=0x%x, value=0x%x",
                    key, value);
            }

            return result;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN_SHIFT:
        {
            uint16 val = (uint16)value;

            result = audio_hwm_anc_source_config_gain_shift(instance_id, path_id, val);
            if (result == FALSE)
            {
                STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                    "GAIN SHIFT config FAIL, key=0x%x, value=0x%x",
                    key, value);
            }

            return result;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_ADAPT_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_ADAPT_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_ADAPT_ENABLE:
        {
            uint16 val = (uint16)value;

            result = audio_hwm_anc_source_config_adapt_enable(instance_id, path_id, val);
            if (result == FALSE)
            {
                STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                    "ADAPT ENABLE config FAIL, key=0x%x, value=0x%x",
                    key, value);
            }

            return result;
        }

        /* Set ANC misc. controls */
        case STREAM_CONFIG_KEY_STREAM_ANC_CONTROL:
        {
            result = audio_hwm_anc_source_set_anc_control(instance_id, value);
            if (result == FALSE)
            {
                STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                    "ANC CONTROL config FAIL, key=0x%x, value=0x%x",
                    key, value);
            }

            return result;
        }

        default:
            STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                "Invalid key, key=0x%x, value=0x%x",
                key, value);

            /* Error invalid key */
            return FALSE;
    }
}

/**
 * \brief Perform ANC configuration via stream sink configure.
 *
 * \param endpoint Pointer to the endpoint.
 * \param key      Key identifying the entity to configure.
 * \param value    Value to configure.
 *
 * \return TRUE if successful, FALSE if error.
 */
bool stream_anc_sink_configure(ENDPOINT *endpoint,
                               STREAM_CONFIG_KEY key,
                               uint32 value)
{
    STREAM_ANC_INSTANCE instance_id;
    STREAM_ANC_INSTANCE stream_anc_instance_id;
    audio_instance device_instance;
    audio_channel channel_number;
    bool result;
    patch_fn_shared(stream_anc);

    STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_sink_configure(): Configuring ANC, key=0x%x, value=0x%x",
        key, value);

    /* Check the endpoint pointer */
    if (endpoint == NULL)
    {
        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): Endpoint pointer is NULL");

        return FALSE;
    }

    /* ID of instance being configured (or set) */
    instance_id = stream_audio_anc_get_instance_id(endpoint);

    /* Check that this a valid instance (unless the instance is being set) */
    if ((audio_hwm_anc_is_anc_instance_valid(instance_id) == FALSE) &&
       (key != STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE))
    {
        STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_sink_configure(): "
            "Endpoint has an unset/invalid ANC instance ID, key=0x%x, instance_id=0x%x",
            key, instance_id);

        return FALSE;
    }

    if (key != STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE)
    {
        STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_sink_configure(): "
            "Invalid key, key=0x%x, value=0x%x",
            key, value);

        return FALSE;
    }

    /* Configure sink according to the key */

    /* Set the ANC instance associated with the endpoint */
    stream_anc_instance_id = (STREAM_ANC_INSTANCE)value;

    if (audio_hwm_anc_is_anc_instance_valid_or_none(stream_anc_instance_id) == FALSE)
    {
        STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_sink_configure(): "
            "Invalid ANC instance ID, key=0x%x, value=0x%x",
            key, value);

        return FALSE;
    }

    /* Get the device instance */
    device_instance = (audio_instance) get_hardware_instance(endpoint);

    /* Get the channel number (note: zeroth channel is channel number 1) */
    channel_number = (audio_channel) (get_hardware_channel(endpoint) + 1);

    result = audio_hwm_anc_sink_interface_update(stream_anc_instance_id,
                                                 device_instance,
                                                 channel_number);
    if (result == FALSE)
    {
        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): "
            "failed to disable ANC endpoints");

        return FALSE;
    }

#ifdef INSTALL_ANC_STICKY_ENDPOINTS
    /* Close the endpoint if a closure by the ANC is pending */
    if (stream_audio_anc_get_close_pending(endpoint))
    {
        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): "
            "closing sink endpoint released by ANC");

        /* Close the endpoint now ANC has finished with it */
        if (!stream_close_endpoint(endpoint))
        {
            STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): "
                "failed to close sink endpoint released by ANC");

            return FALSE;
        }

        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): "
            "sink endpoint released by ANC is now closed");

        /* No longer waiting for ANC to close the endpoint */
        stream_audio_anc_set_close_pending(endpoint, FALSE);
    }
#endif /* INSTALL_ANC_STICKY_ENDPOINTS */

    /* Associate/dissociate the sink endpoint with the ANC instance ID */
    stream_audio_anc_set_instance_id(endpoint, stream_anc_instance_id);

    return TRUE;
}

/**
 * \brief Enables the Sigma-Delta Modulator on the feedback tuning output.
 *        TODO: This probably should be folded into anc_tuning_set_monitor.
 *
 * \param endpoint Pointer to the endpoint.
 *
 * \return TRUE if successful, FALSE if error.
 */
bool stream_anc_enable_sdm(ENDPOINT *endpoint)
{
    uint32 en_and_mask;
    patch_fn_shared(stream_anc);
    en_and_mask = (STREAM_ANC_CONTROL_FB_TUNE_DSM_EN_MASK <<
                   STREAM_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT);
    en_and_mask |= STREAM_ANC_CONTROL_FB_TUNE_DSM_EN_MASK;
    return stream_anc_configure_control(endpoint, en_and_mask);
}

/**
 * \brief Connect Feedback monitor to an IIR filter input.
 *
 * \param endpoint Pointer to the endpoint.
 * \param source   Which IIR filter input to connect.
 *
 * \return TRUE if successful, FALSE if error.
 */
bool stream_anc_connect_feedback_monitor(ENDPOINT *endpoint,
                                         ANC_FBMON_SRC source)
{
    uint32 en_and_mask;
    patch_fn_shared(stream_anc);

    en_and_mask = (STREAM_ANC_CONTROL_FB_ON_FBMON_IS_TRUE_MASK <<
                   STREAM_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT);
    if (source == ANC_FBMON_FB)
    {
        en_and_mask |= STREAM_ANC_CONTROL_FB_ON_FBMON_IS_TRUE_MASK;
    }
    return stream_anc_configure_control(endpoint, en_and_mask);
}

/**
 * \brief Derive an ANC filter ID from the source configure key.
 *
 * \param key ANC instance ID (e.g. ANC0, ANC1).
 *
 * \return path_id Path ID that is derived from the key.
 */
static STREAM_ANC_PATH key_to_id(STREAM_CONFIG_KEY key)
{
    STREAM_ANC_PATH path_id;
    patch_fn_shared(stream_anc);

    /* Act according to the key */
    switch (key)
    {
        /* Configure the ANC DC filter/SM LPF */
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_ADAPT_ENABLE:
            path_id = STREAM_ANC_PATH_FFA_ID;
            break;

        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_ADAPT_ENABLE:
            path_id = STREAM_ANC_PATH_FFB_ID;
            break;

        case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_ADAPT_ENABLE:
            path_id = STREAM_ANC_PATH_FB_ID;
            break;

        case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_SHIFT:
            path_id = STREAM_ANC_PATH_SM_LPF_ID;
            break;

        default:
            path_id = STREAM_ANC_PATH_NONE_ID;
            break;
    }

    return path_id;
}
