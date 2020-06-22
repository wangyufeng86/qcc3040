/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>

#include <util.h>
#include <panic.h>
#include <cryptovm.h>

#include "gatt_root_key_service.h"
#include "gatt_root_key_service_debug.h"


/*! Size of bluetooth address, packed, in octets */
#define GRKS_BDADDR_SIZE_OCTETS (48/8)

void gattRootKeyServiceGenerateRandom(GRKS_KEY_T *random_key)
{
    int r;
    uint16 *key = (uint16*)random_key;
    
    for (r = 0; r < GRKS_KEY_SIZE_128BIT_OCTETS / sizeof(uint16); r++)
    {
        key[r] = UtilRandom();
    }
}


static void gattRootKeyExtendMessageForHashing(uint8 *msg, uint32 *len, uint32 max_len,
                                               const uint8 *content, uint32 content_len)
{
    if (*len + content_len  > max_len)
    {
        GATT_ROOT_KEY_SERVICE_DEBUG("gattRootKeyExtendMessageForHashing. Too long. %d + %d > %d",
                                   *len,content_len, max_len);
        Panic();
    }
    memcpy(&msg[*len], content, content_len);
    *len += content_len;
}

static void gattRootKeyExtendMessageForHashingWithBdaddr(uint8 *msg, uint32 *len, uint32 max_len,
                                                         const bdaddr*address)
{
    uint8 *start;
    if (*len + 6 > max_len)
    {
        GATT_ROOT_KEY_SERVICE_DEBUG("gattRootKeyExtendMessageForHashingWithBdaddr. Too long. %d + 6 > %d",
                                   *len, max_len);
        Panic();
    }
    /* bdaddr is MSB - LSB, NAP[2]-UAP[1]-LAP[3] */
    start = &msg[*len];
    *start++ = (address->nap >>  8) & 0xFF;
    *start++ = (address->nap      ) & 0xFF;

    *start++ = (address->uap      ) & 0xFF;

    *start++ = (address->lap >> 16) & 0xFF;
    *start++ = (address->lap >>  8) & 0xFF;
    *start++ = (address->lap      ) & 0xFF;
    *len += 6;
}

void gattRootKeyServiceGenerateHash(const GRKS_KEY_T *secret, 
                                    const GRKS_KEY_T *randA, const GRKS_KEY_T *randB, 
                                    const bdaddr *addrA, const bdaddr *addrB, 
                                    GRKS_KEY_T *hash)
{
    const uint16 srclen =   GRKS_KEY_SIZE_128BIT_OCTETS + GRKS_KEY_SIZE_128BIT_OCTETS 
                          + GRKS_BDADDR_SIZE_OCTETS + GRKS_BDADDR_SIZE_OCTETS;
    uint8 source [srclen];
    uint32 len=0;

    gattRootKeyExtendMessageForHashing(source, &len, ARRAY_DIM(source),
                                       randA->key,sizeof(*randA));
    gattRootKeyExtendMessageForHashing(source, &len, srclen,
                                       randB->key, sizeof(*randB));
    gattRootKeyExtendMessageForHashingWithBdaddr(source, &len, srclen,
                                                 addrA);
    gattRootKeyExtendMessageForHashingWithBdaddr(source, &len, srclen,
                                                 addrB);

    CryptoVmAesCmac(secret->key, source, srclen, hash->key);
}

