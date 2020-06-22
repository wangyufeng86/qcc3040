   /**
 * Copyright (c) 2017 Qualcomm Technologies International, Ltd.
 *
 * \file  ttp_pid.h
 *
 * \ingroup ttp
 *
 * Public header file for ttp pid controller module
 */

#ifndef TTP_PID_CONTROLLER_H
#define TTP_PID_CONTROLLER_H

/*****************************************************************************
Include Files
*/
#include "types.h"

/****************************************************************************
Private Constant Declarations
*/
#define TTP_PID_USE_DEFAULT_PARAM 0 /* use default value for a pid parameter */

/****************************************************************************
Public Function Declarations
*/

typedef struct ttp_pid_controller_struct ttp_pid_controller;

/**
 * \brief Creates module for ttp pid controlling
 * \param \return pointer to the ttp_pid instance, Null if insufficient resources.
 */
extern ttp_pid_controller *ttp_pid_controller_create(void);

/**
 * \brief Destroys the ttp_pid instance.
 * \param pid Pointer to the ttp_pid instance
 */
extern void ttp_pid_controller_destroy(ttp_pid_controller *pid);

/**
 * \brief Runs the PID controller for the given error.
 * \param pid Pointer to the ttp_pid instance.
 * \param error Time difference between the timestamp time and the expected playback time.
 */
extern void ttp_pid_controller_run(ttp_pid_controller *pid, TIME_INTERVAL error);

/**
 * \brief Resets the PID controller internal state.
 * \param pid Pointer to the ttp_pid instance
 */
extern void ttp_pid_controller_reset(ttp_pid_controller *pid);

/**
 * \brief set pid controller to use default settings
 * \param pid Pointer to the ttp_pid instance
 */
extern void ttp_pid_controller_use_default_parameters(ttp_pid_controller *pid);

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
 *       to default value
 */
extern void ttp_pid_controller_set_parameters(ttp_pid_controller *pid,
                                              int p_factor,
                                              int i_factor,
                                              int error_decay,
                                              int error_grow,
                                              int warp_scale);
/**
 * \brief Get pid controller current calculated warp value
 * \param pid Pointer to the ttp_pid instance
 * \return current warp value
 */
extern int ttp_pid_controller_get_warp(ttp_pid_controller *pid);

#endif /* TTP_PID_CONTROLLER_H */
