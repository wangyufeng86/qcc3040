/****************************************************************************
Copyright (c) 2017 Qualcomm Technologies International, Ltd.

FILE NAME
    chain_list.c

DESCRIPTION
    Linked list of all created chains
*/

#include <panic.h>
#include "chain_list.h"
#include "chain_private.h"

static kymera_chain_t *chain_list = NULL;

/******************************************************************************/
void chainListAdd(kymera_chain_t *chain)
{
    kymera_chain_t **head;
    chain->next = NULL;
    for (head = &chain_list; *head != NULL; head = &(*head)->next);
    *head = chain;
}

/******************************************************************************/
void chainListRemove(kymera_chain_t *chain)
{
    kymera_chain_t **head;

    for (head = &chain_list; *head != NULL; head = &(*head)->next)
    {
        if (chain == *head)
        {
            *head = chain->next;
            break;
        }
    }
}

/******************************************************************************/
#ifdef HOSTED_TEST_ENVIRONMENT
void ChainTestReset(void)
{
    kymera_chain_t *item;
    for (item = chain_list; item != NULL; item = chain_list)
    {
        ChainDestroy(item);
        ChainSetDownloadableCapabilityBundleConfig(NULL);
    }
}
#endif
