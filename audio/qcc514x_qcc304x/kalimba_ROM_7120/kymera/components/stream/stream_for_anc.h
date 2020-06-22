/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \ingroup endpoints
 * \file stream_for_anc.h
 *
 * Temporary header file to make it easier to restructure the relationship
 * between audio and stream as far as ANC is concerned.
 */
#ifndef STREAM_FOR_ANC_H
#define STREAM_FOR_ANC_H

/* For pointer to ENDPOINT */
#include "stream_common.h"
/* For STREAM_ANC_INSTANCE and STREAM_ANC_PATH. */
#include "stream_type_alias.h"

#ifdef INSTALL_UNINTERRUPTABLE_ANC
/**
 * \brief stream_audio_anc_get_instance_id function
 *
 * \param endpoint pointer to the endpoint
 */
extern STREAM_ANC_INSTANCE stream_audio_anc_get_instance_id(ENDPOINT *ep);

/**
 * \brief stream_audio_anc_set_instance_id function
 *
 * \param endpoint Pointer to the endpoint
 * \param ANC      Instance ID to associate with the endpoint
 */
extern void stream_audio_anc_set_instance_id(ENDPOINT *ep,
                                             STREAM_ANC_INSTANCE instance_id);

/**
 * \brief stream_audio_anc_get_input_path_id function
 *
 * \param endpoint Pointer to the endpoint
 */
extern STREAM_ANC_PATH stream_audio_anc_get_input_path_id(ENDPOINT *ep);

/**
 * \brief stream_audio_anc_set_input_path_id function
 *
 * \param endpoint Pointer to the endpoint
 * \param ANC      Input ID to associate with the endpoint
 */
extern void stream_audio_anc_set_input_path_id(ENDPOINT *ep,
                                               STREAM_ANC_PATH path_id);

#ifdef INSTALL_ANC_STICKY_ENDPOINTS
/**
 * \brief stream_audio_anc_get_close_pending function
 *
 * \param endpoint Pointer to the endpoint
 */
extern bool stream_audio_anc_get_close_pending(ENDPOINT *ep);

/**
 * \brief stream_audio_anc_set_close_pending function
 *
 * \param endpoint Pointer to the endpoint
 */
extern void stream_audio_anc_set_close_pending(ENDPOINT *ep,
                                               bool close_pending);
#endif /* INSTALL_ANC_STICKY_ENDPOINTS */

#ifdef INSTALL_AUDIO_MODULE
/**
 * \brief Perform ANC configuration via stream source configure.
 *
 * \param endpoint Pointer to the endpoint.
 * \param key      Key identifying the entity to configure.
 * \param value    Value to configure.
 *
 * \return TRUE if successful, FALSE if error.
 */
extern bool stream_anc_source_configure(ENDPOINT *endpoint,
                                        STREAM_CONFIG_KEY key,
                                        uint32 value);

/**
 * \brief Perform ANC configuration via stream sink configure.
 *
 * \param endpoint Pointer to the endpoint.
 * \param key      Key identifying the entity to configure.
 * \param value    Value to configure.
 *
 * \return TRUE if successful, FALSE if error.
 */
extern bool stream_anc_sink_configure(ENDPOINT *endpoint,
                                      STREAM_CONFIG_KEY key,
                                      uint32 value);
#endif /* INSTALL_AUDIO_MODULE */

#endif /* INSTALL_UNINTERRUPTABLE_ANC */

#endif /* STREAM_FOR_ANC_H */
