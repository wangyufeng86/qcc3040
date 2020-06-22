/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <util.h>

#include <cryptovm.h>

#include "gatt_root_key_client_challenge.h"


void gattRootKeyGenerateHashB(GATT_ROOT_KEY_CLIENT *instance)
{
    gattRootKeyServiceGenerateHash(&instance->secret,
                                   &instance->remote_random, &instance->local_random,
                                   &instance->remote_address, &instance->local_address,
                                   &instance->hashB_out);
}


static void gattRootKeyGenerateHashA(GATT_ROOT_KEY_CLIENT *instance, GRKS_KEY_T *hash)
{
    gattRootKeyServiceGenerateHash(&instance->secret,
                                   &instance->local_random,  &instance->remote_random,
                                   &instance->local_address, &instance->remote_address,
                                   hash);
}


bool gattRootKeyCheckHash(GATT_ROOT_KEY_CLIENT *instance)
{
    GRKS_KEY_T hash;
    gattRootKeyGenerateHashA(instance, &hash);

    return 0 == memcmp(&instance->hashA_in, &hash, sizeof(hash));
}

