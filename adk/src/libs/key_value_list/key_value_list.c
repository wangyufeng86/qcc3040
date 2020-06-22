/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source file for a data structure with a list of { key, value } elements.

*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <panic.h>
#include <vmtypes.h>

#include "key_value_list.h"


#define KEY_VALUE_TYPE_MASK     (0x3)
#define KEY_VALUE_TYPE_SMALL    (0x1)
#define KEY_VALUE_TYPE_LARGE    (0x2)

#define KEY_VALUE_SMALL_SIZE    (sizeof(((key_value_pair_t *)0)->value.u32))

#define KEY_VALUE_MAX_SIZE      ((1 << 12) - 1)


typedef struct key_value_pair_tag
{
    uint16 key;
    uint16 size:12;
    uint16 flags:4;
    union
    {
        void *ptr;
        uint32 u32;
    } value;
    struct key_value_pair_tag *next;
} key_value_pair_t;

struct key_value_list_tag
{
    key_value_pair_t *head;
};

/*****************************************************************************/

static key_value_pair_t *keyValueList_getKeyValuePair(key_value_list_t list, key_value_key_t key);

static bool keyValueList_keyValueIsType(key_value_pair_t *key_value, uint16 type)
{
    return (type == (key_value->flags & KEY_VALUE_TYPE_MASK));
}

static void *getKeyValue(key_value_pair_t *key_value)
{
    void * value = NULL;

    if (keyValueList_keyValueIsType(key_value, KEY_VALUE_TYPE_LARGE))
    {
        value = key_value->value.ptr;
    }
    else
    {
        value = (void *)&key_value->value.u32;
    }

    return value;
}

static size_t keyValueList_getKeySize(key_value_pair_t *key_value)
{
    return key_value->size;
}

static key_value_pair_t *keyValueList_addKeyValuePairAtListHead(key_value_list_t list)
{
    key_value_pair_t *new_kvp = (key_value_pair_t *)PanicUnlessMalloc(sizeof(key_value_pair_t));
    new_kvp->next = list->head;
    list->head = new_kvp;
    return new_kvp;
}

static bool keyValueList_addKeyValuePair(key_value_list_t list, key_value_key_t key, const void * value, size_t size)
{
    key_value_pair_t *key_value = 0;
    bool success = FALSE;

    key_value = keyValueList_addKeyValuePairAtListHead(list);
    key_value->key = key;
    if (size <= KEY_VALUE_SMALL_SIZE)
    {
        key_value->flags = KEY_VALUE_TYPE_SMALL;
        memmove(&key_value->value.u32, value, size);
        key_value->size = size;
        success = TRUE;
    }
    else if (size <= KEY_VALUE_MAX_SIZE)
    {
        key_value->flags = KEY_VALUE_TYPE_LARGE;
        key_value->value.ptr = PanicUnlessMalloc(size);
        memmove(key_value->value.ptr, value, size);
        key_value->size = size;
        success = TRUE;
    }
    else
    {
        /* size is too large to store in the key_value_pair_t */
        Panic();
    }

    return success;
}

static void keyValueList_removeKeyValuePairFromList(key_value_list_t list, key_value_pair_t *key_value)
{
    key_value_pair_t *curr = list->head;

    if (list->head == key_value)
    {
        list->head = list->head->next;
    }
    else
    {
        while (curr->next != NULL)
        {
            if (curr->next == key_value)
            {
                curr->next = curr->next->next;
                break;
            }
            else curr = curr->next;
        }
    }
}

static void keyValueList_destroyKeyValuePair(key_value_list_t list, key_value_pair_t *key_value)
{
    keyValueList_removeKeyValuePairFromList(list, key_value);

    if (keyValueList_keyValueIsType(key_value, KEY_VALUE_TYPE_LARGE))
    {
        free(key_value->value.ptr);
    }

    free(key_value);
}

static key_value_pair_t *keyValueList_getKeyValuePair(key_value_list_t list, key_value_key_t key)
{
    key_value_pair_t *curr = list->head;
    while (curr != NULL)
    {
        if (curr->key == key)
            break;
        else
            curr = curr->next;
    }
    return curr;
}

/*****************************************************************************/
key_value_list_t KeyValueList_Create(void)
{
    size_t size = sizeof(struct key_value_list_tag);
    struct key_value_list_tag *list = PanicUnlessMalloc(size);
    memset(list, 0, size);
    return list;
}

void KeyValueList_Destroy(key_value_list_t* list)
{
    KeyValueList_RemoveAll(*list);
    free(*list);
    *list = NULL;
}

bool KeyValueList_Add(key_value_list_t list, key_value_key_t key, const void *value, size_t size)
{
    bool success = FALSE;

    PanicNull(list);

    if (KeyValueList_IsSet(list, key))
    {
        success = FALSE;
    }
    else
    {
        success = keyValueList_addKeyValuePair(list, key, value, size);
    }

    return success;
}

bool KeyValueList_Get(key_value_list_t list, key_value_key_t key, void **value, size_t *size)
{
    bool found = FALSE;
    key_value_pair_t * key_value = 0;

    PanicNull(value);
    PanicNull(size);

    key_value = keyValueList_getKeyValuePair(list, key);
    if (key_value)
    {
        *value = getKeyValue(key_value);
        *size = keyValueList_getKeySize(key_value);
        found = TRUE;
    }

    return found;
}

void KeyValueList_Remove(key_value_list_t list, key_value_key_t key)
{
    key_value_pair_t * key_value = 0;

    key_value = keyValueList_getKeyValuePair(list, key);
    if (key_value)
        keyValueList_destroyKeyValuePair(list, key_value);
}

void KeyValueList_RemoveAll(key_value_list_t list)
{
    while (list->head != NULL)
    {
        keyValueList_destroyKeyValuePair(list, list->head);
    }
}

bool KeyValueList_IsSet(key_value_list_t list, key_value_key_t key)
{
    bool is_set = FALSE;

    if (keyValueList_getKeyValuePair(list, key))
    {
        is_set = TRUE;
    }

    return is_set;
}
