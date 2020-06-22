/****************************************************************************
Copyright (c) 2020 Qualcomm Technologies International, Ltd.
*/

#ifndef CHAIN_PRIVATE_H_
#define CHAIN_PRIVATE_H_

#include <chain.h>

/*! \brief Reset any static variables during testing.

This is only intended for unit test and should not be used in application code.
*/
void ChainTestReset(void);

/*! \brief Returns audio use case ID for a given chain configuration

Returns the audio_ucid memeber from the chain_config_t structure.

This is only intended for unit test and should not be used in application code.
*/
audio_ucid_t ChainGetUseCase(const chain_config_t *config);

#endif /* CHAIN_CONFIG_H_ */
