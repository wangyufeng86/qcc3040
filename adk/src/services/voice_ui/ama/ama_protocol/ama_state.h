#ifndef __AMA_STATE_H_
#define __AMA_STATE_H_

#include "ama_protocol.h"
#include "ama_private.h"

typedef enum _AMA_SETUP_STATE {
    AMA_SETUP_STATE_READY ,
    AMA_SETUP_STATE_WAITING ,  // just got at least one protbuf message
    AMA_SETUP_STATE_START,
    AMA_SETUP_STATE_COMPLETED,
    AMA_SETUP_STATE_SYNCHRONIZE

}AMA_SETUP_STATE;

/* Internal APIs */
ama_error_code_t AmaState_GetState(uint32 feature, uint32* Pstate, ama_state_value_case_t* pValueCase);
ama_error_code_t AmaState_SetState(uint32 feature, uint32 state, ama_state_value_case_t valueCase);
void AmaState_Init(void);
AMA_SETUP_STATE AmaState_GetSetupState(void);
void AmaState_CompleteSetup(bool completed) ;
bool AmaState_SendBooleanStateEvent(uint32 feature,bool True, bool get);
bool AmaState_SendIntegerStateEvent(uint32 feature ,uint16 integer, bool get);
bool AmaState_IsCompleteSetup(void);

#endif /* __AMA_STATE_H_ */
