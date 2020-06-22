/**
 * Copyright (c) 2017 Qualcomm Technologies International, Ltd.
 *
 * \file  ttp_pid.c
 *
 * \ingroup ttp
 *
 * PID controller implementation for TTP play back
 *
 */

/****************************************************************************
Include Files
*/
#include "ttp_private.h"
#include "ttp_pid.h"

/****************************************************************************
Private Constant Declarations
*/
#define WARP_SHIFT_AMOUNT   8
#define WARP_P_TERM_MAX     (30000 << (WARP_SHIFT_AMOUNT))
#define WARP_I_ERROR_MAX    0x400000
#define DEFAULT_P_FACTOR    FRACTIONAL(1.0)
#define DEFAULT_I_FACTOR    FRACTIONAL(0.9)
#define DEFAULT_ERROR_DECAY FRACTIONAL(0.98)
#define DEFAULT_ERROR_GROW  FRACTIONAL(0.2)
#define DEFAULT_WARP_SCALE  32

/** Error shift */
#define ERROR_SHIFT_AMOUNT  8

/** Limits A between [L,U] */
#define CLAMP(A, L, U)  MIN(MAX((A), (L)),(U))

/****************************************************************************
Private Type Declarations
*/
/**
 * Structure for holding the internal settings of the PID controller.
 */
typedef struct pid_controller_settings_stuct
{
    /** P factor for the pid controller */
    int p_factor;

    /** I factor for the pid controller */
    int i_factor;

    /** Max speed the error can decline. */
    int error_decay;

    /** Max speed the error can grow */
    int error_grow;

    /** Scaling between controller output and fractional warp value */
    int warp_scale;
} pid_controller_settings;

/**
 * Structure for holding the PID controller state.
 */
typedef struct pid_controller_state_struct
{
    /**
     * Fractional value in the interval of (-0.005, 0.005) representing the rate
     * adjustment/warp value calculated by the PID controller to keep the endpoint
     * timed playback error in the minimum.
     */
    int warp;

    /**
     * The proportional part of the warp.
     */
    int warp_p_term;

    /**
     * The integral part of the warp.
     */
    int warp_i_term;

    /**
     * The integral part of the error used to calculate warp_i_term.
     */
    int error_i_sum;
    /* Min, max and average error is used to create a lowpass filter like calculation
     * to smooth out the sudden variation of the error.  */
    int min_error;
    int max_error;
    int avg_error;
}pid_controller_state ;

struct ttp_pid_controller_struct
{
    /* Settings for PID controller */
    pid_controller_settings pid_params;

    /* Internal state of PID controller */
    pid_controller_state pid_state;
};

/**
 * \brief Creates module for ttp pid controlling
 * \param \return pointer to the ttp_pid instance, Null if insufficient resources.
 */
ttp_pid_controller *ttp_pid_controller_create(void)
{
    return xzpnew(ttp_pid_controller);
}

/**
 * \brief Destroys the ttp_pid instance.
 * \param pid Pointer to the ttp_pid instance
 */
void ttp_pid_controller_destroy(ttp_pid_controller *pid)
{
    pdelete(pid);
}

/**
 * \brief Runs the PID controller for the given error.
 * \param pid Pointer to the ttp_pid instance.
 * \param error Time difference between the timestamp time and the expected playback time.
 */
void ttp_pid_controller_run(ttp_pid_controller *pid, TIME_INTERVAL error)
{
    int tmp, error_us, avg_error, min_error, max_error;
    pid_controller_state *pid_state = &pid->pid_state;
    pid_controller_settings *pid_params = &pid->pid_params;

    patch_fn_shared(timed_playback);
    /* calculate middle of min and max error */
    error_us = error << ERROR_SHIFT_AMOUNT;
    avg_error = (pid_state->min_error + pid_state->max_error) / 2;

    /* decay min error */
    min_error = pid_state->min_error - avg_error;
    min_error = frac_mult(min_error, pid_params->error_decay);
    pid_state->min_error = min_error + avg_error;

    /* decay max error */
    max_error = pid_state->max_error - avg_error;
    max_error = frac_mult(max_error, pid_params->error_decay);
    pid_state->max_error = max_error + avg_error;

    /* if error is less than min_error, move min_error towards current error.  error_grow
     * determines how fast min_error moves */
    if (error_us < pid_state->min_error)
    {
        int min_diff = pid_state->min_error - error_us;
        pid_state->min_error -= frac_mult(min_diff, pid_params->error_grow);
    }

    /* if error is greater than max_error, move max_error towards current error.  error_grow
     * determines how fast max_error moves */
    if (error_us > pid_state->max_error)
    {
        int max_diff = error_us - pid_state->max_error;
        pid_state->max_error += frac_mult(max_diff, pid_params->error_grow);
    }

    /* calculate mid-point between min and max */
    pid_state->avg_error = (pid_state->min_error + pid_state->max_error) / 2;

    /* calculate p_term and clamp within limits */
    tmp = frac_mult(pid_state->avg_error, pid_params->p_factor);
    pid_state->warp_p_term = CLAMP(tmp, -WARP_P_TERM_MAX, WARP_P_TERM_MAX);

    /* calculate i_term and clamp within limits */
    tmp = error;
    tmp = tmp + pid_state->error_i_sum;
    tmp = CLAMP(tmp, -WARP_I_ERROR_MAX, WARP_I_ERROR_MAX);
    pid_state->error_i_sum = tmp;
    pid_state->warp_i_term = frac_mult(tmp, pid_params->i_factor);

    /* sum p_term & i_term and scale to word-sized fractional */
    pid_state->warp = pid_params->warp_scale * (pid_state->warp_p_term + pid_state->warp_i_term);
}

/**
 * \brief Resets the PID controller internal state.
 * \param pid Pointer to the ttp_pid instance
 */
void ttp_pid_controller_reset(ttp_pid_controller *pid)
{
    /* Reset the PID controller state */
    pid_controller_state *pid_state = &pid->pid_state;
    pid_state->warp = 0;
    pid_state->warp_p_term = 0;
    pid_state->warp_i_term = 0;
    pid_state->error_i_sum = 0;

    /* set min_error to maximum value so that is gets updated immediately */
    pid_state->min_error = 32767 << ERROR_SHIFT_AMOUNT;
    /* set max_error to minimum value so that is gets updated immediately */
    pid_state->max_error = -32767 << ERROR_SHIFT_AMOUNT;
}

/**
 * \brief set pid controller to use default settings
 * \param pid Pointer to the ttp_pid instance
 */
void ttp_pid_controller_use_default_parameters(ttp_pid_controller *pid)
{
    /* use default params for all params */
    ttp_pid_controller_set_parameters(pid,
                                      TTP_PID_USE_DEFAULT_PARAM,
                                      TTP_PID_USE_DEFAULT_PARAM,
                                      TTP_PID_USE_DEFAULT_PARAM,
                                      TTP_PID_USE_DEFAULT_PARAM,
                                      TTP_PID_USE_DEFAULT_PARAM);
}

/**
 * \brief set pid controller parameters
 * \param pid Pointer to the ttp_pid instance
 * \param p_factor Proportional factor (default FRACTIONAL(1.0)).
 * \param i_factor Integral factor (default FRACTIONAL(0.9)).
 * \param error_decay Max speed the error can decline (default FRACTIONAL(0.98)).
 * \param error_grow Max speed the error can grow (default FRACTIONAL(0.2))
 * \warp_scale Scaling between controller output and fractional warp value (default 32)
 *
 * Note: Using TTP_PID_USE_DEFAULT_PARAM(0) for any parameter will set that parameter
 *       to default value.
 */
void ttp_pid_controller_set_parameters(ttp_pid_controller *pid,
                                       int p_factor,
                                       int i_factor,
                                       int error_decay,
                                       int error_grow,
                                       int warp_scale)
{
    pid_controller_settings *pid_params = &pid->pid_params;
    pid_params->p_factor    = (p_factor != TTP_PID_USE_DEFAULT_PARAM)?    p_factor:DEFAULT_P_FACTOR;
    pid_params->i_factor    = (i_factor != TTP_PID_USE_DEFAULT_PARAM)?    i_factor:DEFAULT_I_FACTOR;
    pid_params->error_decay = (error_decay != TTP_PID_USE_DEFAULT_PARAM)? error_decay:DEFAULT_ERROR_DECAY;
    pid_params->error_grow  = (error_grow != TTP_PID_USE_DEFAULT_PARAM)?  error_grow:DEFAULT_ERROR_GROW;
    pid_params->warp_scale  = (warp_scale != TTP_PID_USE_DEFAULT_PARAM)?  warp_scale:DEFAULT_WARP_SCALE;
}

/**
 * \brief Get pid controller current calculated warp value
 * \param pid Pointer to the ttp_pid instance
 * \return current warp value
 */
int ttp_pid_controller_get_warp(ttp_pid_controller *pid)
{
    return pid->pid_state.warp;
}
