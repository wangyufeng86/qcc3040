/* Copyright (c) 2018 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 */

#include "marshal_object_set.h"
#include "assert.h"
#include "pmalloc/pmalloc.h"

#ifdef INSTALL_MARSHAL

/** Objects are stored in blocks. Block size may be changed to suit the size
 *  of available pools. */
#define BLOCK_SIZE 4

struct marshal_object_block
{
    struct marshal_object_block *next;
    void *address[BLOCK_SIZE];
    marshal_type_t type[BLOCK_SIZE];
    uint8 disambiguator[BLOCK_SIZE];
};

void mobs_init(mobs_t *set)
{
    set->head = NULL;
    set->tail = NULL;
    set->elements = 0;
}

void mobs_destroy(mobs_t *set)
{
    mob_block_t *block;
    mob_block_t *next;
    for (block = set->head; block; block = next)
    {
        next = block->next;
        pfree(block);
    }
    set->head = NULL;
    set->tail = NULL;
    set->elements = 0;
}

mob_index_t mobs_size(mobs_t *set)
{
    return set->elements;
}

bool mobs_push(mobs_t *set, mob_t *object)
{
    /* Limit size of set */
    assert(set->elements < MOBS_MAX_OBJECTS);

    /* No duplicates */
    if (!mobs_has_object(set, object, NULL))
    {
        mob_block_t *tail = set->tail;
        mob_index_t index = set->elements % BLOCK_SIZE;
        if (index == 0)
        {
            tail = zpmalloc(sizeof(*tail));
            if (!set->head)
            {
                set->head = set->tail = tail;
            }
            else
            {
                set->tail = set->tail->next = tail;
            }
        }
        tail->address[index] = object->address;
        tail->type[index] = object->type;
        tail->disambiguator[index] = object->disambiguator;

        set->elements++;
        return TRUE;
    }
    return FALSE;
}

bool mobs_pop(mobs_t *set, mob_t *object)
{
    mob_block_t *tail = set->tail;
    mob_index_t index = (set->elements - 1) % BLOCK_SIZE;

    if (set->elements)
    {
        if (object)
        {
            object->address = tail->address[index];
            object->type = tail->type[index];
            object->disambiguator = tail->disambiguator[index];
        }

        if (index == 0)
        {
            if (set->head == tail)
            {
                pfree(tail);
                set->head = set->tail = NULL;
            }
            else
            {
                mob_block_t **b;
                for (b = &set->head; (*b)->next != tail; b = &(*b)->next)
                    ;
                pfree((*b)->next);
                (*b)->next = NULL;
                set->tail = *b;
            }
        }
        --set->elements;
        return TRUE;
    }
    return FALSE;
}

/* A consequence of using blocks (versus a single element list) is inefficient
   object removal, since elements after the removed have to be 'shunted' into
   the place of the removed element */
bool mobs_remove(mobs_t *set, const mob_t *object)
{
    mob_index_t index;
    mob_block_t *b = set->head;

    /* Two element pipeline of values for shunting across blocks boundaries */
    void **addr0 = b->address;
    void **addr1;
    marshal_type_t *type0 = b->type;
    marshal_type_t *type1;
    uint8 *disam0 = b->disambiguator;
    uint8 *disam1;

    bool shunt = FALSE;

    for (index = 0; index < set->elements; index++)
    {
        mob_index_t block_index = index % BLOCK_SIZE;

        if (!block_index)
        {
            if (index)
            {
                b = b->next;
                assert(b);
            }
        }
        /* Pass addresses down the pipeline */
        addr1 = addr0;
        type1 = type0;
        disam1 = disam0;
        addr0 = b->address + block_index;
        type0 = b->type + block_index;
        disam0 = b->disambiguator + block_index;

        if (shunt)
        {
            /* Overwrite previous index's values with this index's values */
            *addr1 = *addr0;
            *type1 = *type0;
            *disam1 = *disam0;
        }
        else if ((*addr0 == object->address) && (*type0 == object->type))
        {
            shunt = TRUE;
        }
    }
    if (shunt)
    {
        assert(mobs_pop(set, NULL));
        return TRUE;
    }
    return FALSE;
}

void mobs_difference_update(mobs_t *set, const mobs_t *remove)
{
    mob_index_t index;
    mob_block_t *b = remove->head;

    for (index = 0; index < remove->elements; index++)
    {
        mob_index_t block_index = index % BLOCK_SIZE;
        mob_t object;

        object.address = b->address[block_index];
        object.type = b->type[block_index];
        /* Don't care if object was actually removed */
        (void)mobs_remove(set, &object);

        if (block_index == (BLOCK_SIZE - 1))
        {
            b = b->next;
        }
    }
}

bool mobs_has_object(mobs_t *set, const mob_t *object, mob_index_t *index_p)
{
    mob_index_t index;
    mob_block_t *b = set->head;

    for (index = 0; index < set->elements; index++)
    {
        mob_index_t block_index = index % BLOCK_SIZE;

        if (object->address == b->address[block_index])
        {
            /* NULL address matches all types */
            if ((b->type[block_index] == object->type) || (object->address == NULL))
            {
                if (index_p)
                {
                    *index_p = index;
                }
                return TRUE;
            }
        }

        if (block_index == (BLOCK_SIZE - 1))
        {
            b = b->next;
        }
    }
    return FALSE;
}

static mob_block_t *get_block(mobs_t *set, mob_index_t index)
{
    mob_block_t *b = NULL;

    if (index < set->elements)
    {
        uint32 steps = index / BLOCK_SIZE;
        for (b = set->head; b && steps; b = b->next, --steps)
            ;
    }
    return b;
}

bool mobs_get_object(mobs_t *set, mob_index_t index, mob_t *object)
{
    mob_block_t *b = get_block(set, index);
    if (b)
    {
        if (object)
        {
            mob_index_t block_index = index % BLOCK_SIZE;
            object->address = b->address[block_index];
            object->type = b->type[block_index];
            object->disambiguator = b->disambiguator[block_index];
        }
        return TRUE;
    }
    return FALSE;
}

bool mobs_iterate(mobs_t *set, mob_index_t start, mobs_callback_t cb)
{
    mob_block_t *b = get_block(set, start);
    if (b)
    {
        for ( ; start < set->elements; start++)
        {
            mob_t object;
            mob_index_t block_index = start % BLOCK_SIZE;

            object.address = b->address[block_index];
            object.type = b->type[block_index];
            object.disambiguator = b->disambiguator[block_index];

            if (!cb(set, start, &object))
            {
                return FALSE;
            }

            if (block_index == (BLOCK_SIZE - 1))
            {
                b = b->next;
            }
        }
    }
    return TRUE;
}

#endif /* INSTALL_MARSHAL */
