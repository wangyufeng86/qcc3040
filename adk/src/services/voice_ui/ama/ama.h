/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       ama.h
\brief  Interfaces defination of Amazon AVS interfaces
*/

#ifndef AMA_H
#define AMA_H

#include "ama_protocol.h"
#include "logging.h"
#include <csrtypes.h>
#include <stdio.h>
#include "voice_ui_container.h"

#define ASSISTANT_OVERRIDEN             FALSE
#define ASSISTANT_OVERRIDE_REQUIRED     TRUE

#if (defined(__QCC302X_APPS__) || defined(__QCC512X_APPS__)) && defined(INCLUDE_AMA) && !defined(INCLUDE_KYMERA_AEC)
    #error AMA needs the INCLUDE_KYMERA_AEC compilation switch for this platform
#endif


#if defined(INCLUDE_GAA) && defined(INCLUDE_AMA) && !defined(INCLUDE_VA_COEXIST)
    #error GAA and AMA are mutually exclusive unless VA coexistence feature is enabled
#endif

/*! Callback function pointer to transmit data to the handset */
typedef bool (*ama_tx_callback_t)(uint8 *data, uint16 size_data);

/*! \brief Initialise the AMA component.

    \param task The init task
    \return TRUE if component initialisation was successful, otherwise FALSE.
*/
bool Ama_Init(Task init_task);

/*! \brief Return the Voice UI protected interface of the AMA component.

    \return Pointer to structure holding protected functions.
*/
voice_ui_protected_if_t* Ama_GetVoiceUiProtectedInterface(void);

/*! \brief Get the AMA transport over which data will be sent.

    \return ama_transport_t indicating the active transport.
*/
ama_transport_t Ama_GetActiveTransport(void);

/*! \brief Get the Voice Assistant handle of the AMA component.

    \return Pointer to the Voice Assistant descriptor.
*/
voice_ui_handle_t* Ama_GetHandle(void);

/*! \brief Parse AMA protocol data received from the handset.

    \param data Pointer to uint8 data to parse
    \param size_data Number of octets to parse
    \return TRUE if the data was completely parsed, otherwise FALSE.
*/
bool Ama_ParseData(uint8 *data, uint16 size_data);

/*! \brief Send AMA protocol data to the handset.

    \param data Pointer to uint8 data to send
    \param size_data Number of octets to send
    \return TRUE if the data was successfully sent, otherwise FALSE.
*/
bool Ama_SendData(uint8 *data, uint16 size_data);

/*! \brief Set the callback function to use to send data to the handset
           for a given transport.

    \param callback Pointer to callback function
    \param transport Identifies the transport which will use this callback
*/
void Ama_SetTxCallback(ama_tx_callback_t callback, ama_transport_t transport);

/*! \brief Inform the AMA component that the transport has changed.

    \param transport Identifies the transport in use
*/
void Ama_TransportSwitched(ama_transport_t transport);

/*! \brief Inform the AMA component that the transport has disconnected.
*/
void Ama_TransportDisconnected(void);

#endif /* AMA_H */
