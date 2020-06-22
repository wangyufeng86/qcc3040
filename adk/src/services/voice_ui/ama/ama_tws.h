/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Provides TWS support in the accessory domain
*/


#ifndef AMA_TWS_H
#define AMA_TWS_H

void AmaTws_HandleLocalDisconnectionCompleted(void);

#ifdef HOSTED_TEST_ENVIRONMENT
void Ama_tws_Reset(void);
#endif

#define AMA_TWS_DEBUG(x) {PRINT(x);}

#endif /* AMA_TWS_H */
