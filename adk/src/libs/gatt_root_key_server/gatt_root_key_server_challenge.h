/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_SERVER_CHALLENGE_H_
#define GATT_ROOT_KEY_SERVER_CHALLENGE_H_

void gattRootKeySendChallengeInd(GATT_ROOT_KEY_SERVER *instance, 
                                 gatt_root_key_challenge_status_t status_code);

void handleInternalChallengeWrite(const ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE_T *write);

void handleInternalKeysWrite(const ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE_T *write);

void handleInternalKeysCommit(const ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT_T *commit);


#endif /* GATT_ROOT_KEY_SERVER_CHALLENGE_H_ */
