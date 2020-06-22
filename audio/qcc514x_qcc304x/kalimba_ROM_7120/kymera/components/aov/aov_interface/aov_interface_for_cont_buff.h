/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  aov_interface_for_cont_buff.h
 * \ingroup  aov
 *
 * AOV interface to streams. <br>
 * This file contains the AOV APIs that can be used by streams. <br>
 */

#ifndef AOV_INTERFACE_FOR_CONT_BUFF_H
#define AOV_INTERFACE_FOR_CONT_BUFF_H

/****************************************************************************
Public Function Definitions
*/
#ifdef HAVE_CONTINUOUS_BUFFERING
/**
 * \brief Sets a flag in the AOV interface that indicates if continuous buffering
 *  has been enabled. This is subsequently read by the AOV state machine.
 *
 * \param continuous_buffering_mode Continuous buffering state.
 *
 * \return TRUE
 */
void aov_set_continuous_buff_mode(bool continuous_buff_mode);
#endif /* HAVE_CONTINUOUS_BUFFERING */

#endif /* AOV_INTERFACE_FOR_CONT_BUFF_H */
