/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file

@brief  Header file for the GATT root key service.

        This file provides documentation for types that are used for the 
        service and so common to the client and the server.
*/

#ifndef GATT_ROOT_KEY_SERVICE_H_
#define GATT_ROOT_KEY_SERVICE_H_

#include <csrtypes.h>
#include <message.h>

#include <library.h>

#include "gatt_manager.h"


/*! Type used for features. 

    Type used in case features subsequently extended */
typedef uint16 GattRootKeyServiceFeatures;

/*! Names for features supported by the Root Key Server. */
typedef enum
{
    GrksFeatureMirrorChallenge = 0,
    GrksFeaturePrepareIr,
    GrksFeaturePrepareEr,
    GrksFeatureExecuteRootUpdate,
} GattRootKeyServiceFeaturesIds;

#define GRKS_FEATURE_BIT_MASK(bit) (1<<(bit))

#define GATT_ROOT_KEY_SERVICE_FEATURES_DEFAULT ( \
              GRKS_FEATURE_BIT_MASK(GrksFeatureMirrorChallenge) \
            + GRKS_FEATURE_BIT_MASK(GrksFeaturePrepareIr) \
            + GRKS_FEATURE_BIT_MASK(GrksFeaturePrepareEr) \
            + GRKS_FEATURE_BIT_MASK(GrksFeatureExecuteRootUpdate))

/*! Type used for Status. 

    Typedef used in case the status is subsequently extended */
typedef uint8 GattRootKeyServiceStatus;


/*!
    @brief Status code returned following a GattRootKeyChallengePeer().

    The status is return in a GATT_ROOT_KEY_CLIENT_CHALLENGE_IND_T message.
*/
typedef enum
{
        /*! Indicates that the challenge implemented in the GATT Root Key
            Transfer Service has completed successfully */
    gatt_root_key_challenge_status_success,
        /*! Indicates that the challenge implemented in the GATT Root Key
            Transfer Service has completed, but has failed.
            This can be used as an indication that the peer device can be 
            considered untrustworthy */
    gatt_root_key_challenge_status_fatal,
} gatt_root_key_challenge_status_t;



/*! Opcodes used relating to the Mirror Challenge Control Point. */
typedef enum
{
    GattRootKeyScOpcodeIncomingRequest = 1,
    GattRootKeyScOpcodeOutgoingRequest,
    GattRootKeyScOpcodeIncomingResponse,
    GattRootKeyScOpcodeOutgoingResponse,
} GattRootKeyServiceMirrorChallengeControlOpCode;


/*! Opcodes used relating to the Keys Control Point */
typedef enum
{
    GattRootKeyKeysOpcodePrepareIr = 1,
    GattRootKeyKeysOpcodePrepareEr,
    GattRootKeyKeysOpcodeExecuteRootUpdate,
} GattRootKeyServiceKeysControlOpCode;


#define GRKS_KEY_SIZE_128BIT_OCTETS     (128/8)
#define GRKS_KEY_SIZE_128BIT_WORDS      (128/16)

/*! Structure for holding a 128-bit key

    \implementation Not using a typedef of array. C is permissive, 
                so felt too dangerous. */
typedef struct
{
    uint8 key[GRKS_KEY_SIZE_128BIT_OCTETS];
} GRKS_KEY_T;

typedef GRKS_KEY_T GRKS_IR_KEY_T;
typedef GRKS_KEY_T GRKS_ER_KEY_T;


void gattRootKeyServiceGenerateRandom(GRKS_KEY_T *random_key);

void gattRootKeyServiceGenerateHash(const GRKS_KEY_T *secret, 
                                    const GRKS_KEY_T *randA, const GRKS_KEY_T *randB, 
                                    const bdaddr *addrA, const bdaddr *addrB, 
                                    GRKS_KEY_T *hash);

#endif /* GATT_ROOT_KEY_SERVICE_H_ */
