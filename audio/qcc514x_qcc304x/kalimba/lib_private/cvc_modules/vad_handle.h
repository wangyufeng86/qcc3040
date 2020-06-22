/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup vad
 * \ingroup lib_private
 * \file  vad_handle.h
 * \ingroup vad
 *
 * vad handle private header file. <br>
 *
 */
 
/******************** Private functions ***************************************/


#ifndef _VAD_HANDLE_H_
#define _VAD_HANDLE_H_

/**
@brief Initialize the feature handle for VAD
*/
bool load_vad_handle(void** f_handle, OPERATOR_DATA *op_data);

/**
@brief Remove and unload the feature handle for VAD
*/
void unload_vad_handle(void* f_handle);

#endif /* _VAD_HANDLE_H_ */

