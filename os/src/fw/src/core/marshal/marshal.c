/* Copyright (c) 2018 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 */

#include "marshal_base.h"

#ifdef INSTALL_MARSHAL

/** Marshal states */
enum marshal_state
{
    /** Object hierarchy pointers being added to the object set */
    REGISTER_POINTERS = 0,
    /** Object values are marshalled */
    MARSHAL_VALUES,
    /** Final object value has been marshalled */
    MARSHAL_VALUES_END,
    /** Object pointer indexes are marshalled */
    MARSHAL_POINTER_INDEXES
};

/** Marshaller context */
typedef struct marshal_context
{
    /** Base marshal object - containing type descriptors and main object set */
    struct marshal_base base;

    /** Address of the start of the buffer */
    uint8 *buf;

    /** The size (amount of space) in the marshal buffer */
    size_t size;

    /** Current write index to the marshal buffer */
    size_t index;

    /** Stores the write index before starting a write to the marshalling buffer.
     *  If the write fails (full), the backup is used to restore the index to
     *  the value prior to the failed write.
     */
    size_t backup_index;

    /** Remaining bytes to be marshalled for current object */
    size_t remaining;

    /** The current marshalling state */
    enum marshal_state state;

    /** Index of next object to have its values marshalled. */
    mob_index_t next_values_index;

    /** Index of next object to have its pointers marshalled. */
    mob_index_t next_pointers_index;

} marshal_context_t;

marshal_context_t *marshal_init(const marshal_type_descriptor_t * const *type_desc_list,
                                size_t type_desc_list_elements)
{
    struct marshal_context *m = zpmalloc(sizeof(*m));

    base_init(&m->base, type_desc_list, type_desc_list_elements);

    /* Don't marshal the NULL object at the start of the object set */
    m->next_values_index = MOB_INDEX_NULL+1;
    m->next_pointers_index = MOB_INDEX_NULL+1;

    return m;
}

void marshal_set_buffer(marshal_context_t *m, void *buf, size_t space)
{
    assert(m);

    m->buf = buf;
    m->size = space;
    m->index = 0;
}

void marshal_destroy(marshal_context_t *m, bool free_all_objects)
{
    assert(m);
    base_uninit(&m->base, free_all_objects);
    pfree(m);
}

void marshal_clear_store(marshaller_t m)
{
    assert(m);
    base_clear_store(&m->base);
    m->next_values_index = MOB_INDEX_NULL+1;
    m->next_pointers_index = MOB_INDEX_NULL+1;
}

static inline void marshal_backup(marshal_context_t *m)
{
    m->backup_index = m->index;
}

static inline void marshal_restore(marshal_context_t *m)
{
    m->index = m->backup_index;
}

static bool write(marshal_base_t *base, const void *object, size_t sizeof_object)
{
    marshal_context_t *m = STRUCT_FROM_MEMBER(marshal_context_t, base, base);

    if (m->buf)
    {
        size_t next_index;
        next_index = m->index + sizeof_object;
        if (next_index <= m->size)
        {
            memcpy(m->buf + m->index, object, sizeof_object);
            m->index = next_index;
            return TRUE;
        }
    }
    return FALSE;
}

/* For pointer object members the address of the member is a pointer to a pointer.
   This function dereferences that pointer and returns the address of the object
   being pointed to. */
static inline void *dereference_pp(void *pointer_address)
{
    return *(void**)(pointer_address);
}

static bool write_pointer_index(marshal_base_t *base,
                                void *pointer_addr,
                                const marshal_member_descriptor_t *desc,
                                mob_t *parent)
{
    mob_t referred;
    mob_index_t referred_index;

    UNUSED(parent);

    referred.type = desc->type;
    referred.address = dereference_pp(pointer_addr);

    /* Either the base object set or the shared member set should contain an
       object corresponding to this pointer member. Objects from the shared
       member set are given indexes above the number of objects in the base
       object set */
    if (!mobs_has_object(&base->object_set, &referred, &referred_index))
    {
        assert(mobs_has_object(&base->shared_member_set, &referred, &referred_index));
        referred_index += mobs_size(&base->object_set);
    }
    return write(base, &referred_index, sizeof(referred_index));
}

static bool write_value(marshal_base_t *base, mob_t *leaf)
{
    marshal_context_t *m = STRUCT_FROM_MEMBER(marshal_context_t, base, base);
    const marshal_type_descriptor_t *type_desc = base->type_desc_list[leaf->type];

    if (m->buf)
    {
        size_t next_index;
        next_index = m->index + type_desc->size;
        if (next_index <= m->size)
        {
            marshal_custom_copy_cb copy_cb = memcpy;
            const marshal_custom_copy_cbs *cbs = type_desc->u.custom_copy_cbs;

            if (cbs && cbs->marshal_copy)
            {
                copy_cb = cbs->marshal_copy;
            }

            (void)copy_cb(m->buf + m->index, leaf->address, type_desc->size);
            m->index = next_index;
            return TRUE;
        }
    }
    return FALSE;
}

static bool register_pointer(marshal_base_t *base,
                             void *pointer_addr,
                             const marshal_member_descriptor_t *desc,
                             mob_t *parent)
{
    mob_t referred;

    referred.type = desc->type;
    referred.address = dereference_pp(pointer_addr);
    referred.disambiguator = obj_disambiguator(base, referred.address, desc, parent);

    /* This push is allowed to fail - failure indicates the pointer has
       already been registered by another object */
    (void)mobs_push(&base->object_set, &referred);

    return TRUE;
}

/*  Traverse the tree/hierarchy of the object adding any pointer members to the
    base set. */
static bool register_pointer_members(mobs_t *set, mob_index_t index, mob_t *object)
{
    marshal_base_t *base = STRUCT_FROM_MEMBER(marshal_base_t, object_set, set);
    const traverse_callbacks_t callbacks = {NULL, register_pointer, NULL};

    UNUSED(index);

    return object_tree_traverse(base, object, &callbacks);
}

/* Traverse the tree/hierarcy of the object writing the index of pointer members */
static bool marshal_pointer_indexes(mobs_t *set, mob_index_t index, mob_t *object)
{
    marshal_base_t *base = STRUCT_FROM_MEMBER(marshal_base_t, object_set, set);
    marshal_context_t *m = STRUCT_FROM_MEMBER(marshal_context_t, base, base);
    const traverse_callbacks_t callbacks = {NULL, write_pointer_index, NULL};

    marshal_backup(m);

    if (object_tree_traverse(&m->base, object, &callbacks))
    {
        assert(index < MOBS_MAX_OBJECTS);
        m->next_pointers_index = (mob_index_t)(index + 1);
        return TRUE;
    }
    marshal_restore(m);
    return FALSE;
}

/* Traverse the tree/hierarcy of the object writing the type and values of
   leaf members */
static bool marshal_values(mobs_t *set, mob_index_t index, mob_t *object)
{
    marshal_base_t *base = STRUCT_FROM_MEMBER(marshal_base_t, object_set, set);
    marshal_context_t *m = STRUCT_FROM_MEMBER(marshal_context_t, base, base);
    const traverse_callbacks_t callbacks = {write_value, NULL, NULL};

    marshal_backup(m);

    /* The object type is the header. Since object values are marshalled in
       index order, the objects will be pushed onto the un-marshaller's set
       in the same order and have the same indexes */
    if (write(base, &object->type, sizeof(object->type)))
    {
        uint8 disambiguator = object->disambiguator;
        if (!base->type_desc_list[object->type]->dynamic_length ||
            write(base, &disambiguator, sizeof(disambiguator)))
        {
            if (object_tree_traverse(base, object, &callbacks))
            {
                assert(index < MOBS_MAX_OBJECTS);
                m->next_values_index = (mob_index_t)(index + 1);
                return TRUE;
            }
        }
    }

    marshal_restore(m);
    return FALSE;
}

static bool count_remaining_index(marshal_base_t *base,
                                  void *pointer_addr,
                                  const marshal_member_descriptor_t *desc,
                                  mob_t *parent)
{
    marshal_context_t *m = STRUCT_FROM_MEMBER(marshal_context_t, base, base);
    m->remaining += sizeof(mob_index_t);
    
    UNUSED(pointer_addr);
    UNUSED(desc);
    UNUSED(parent);
    return TRUE;
}

static bool count_remaining_indexes(mobs_t *set, mob_index_t index, mob_t *object)
{
    marshal_base_t *base = STRUCT_FROM_MEMBER(marshal_base_t, object_set, set);
    const traverse_callbacks_t callbacks = {NULL, count_remaining_index, NULL};

    assert(object_tree_traverse(base, object, &callbacks));

    UNUSED(index);
    return TRUE;
}

static bool count_remaining_value(marshal_base_t *base, mob_t *leaf)
{
    marshal_context_t *m = STRUCT_FROM_MEMBER(marshal_context_t, base, base);
    m->remaining += base->type_desc_list[leaf->type]->size;
    return TRUE;
}

static bool count_remaining_values(mobs_t *set, mob_index_t index, mob_t *object)
{
    marshal_base_t *base = STRUCT_FROM_MEMBER(marshal_base_t, object_set, set);
    marshal_context_t *m = STRUCT_FROM_MEMBER(marshal_context_t, base, base);
    const traverse_callbacks_t callbacks = {count_remaining_value, NULL, NULL};

    m->remaining += sizeof(object->type);
    if (base->type_desc_list[object->type]->dynamic_length)
    {
        m->remaining += sizeof(uint8);
    }
    assert(object_tree_traverse(base, object, &callbacks));

    UNUSED(index);
    return TRUE;
}

bool marshal(marshal_context_t *m, void *addr, marshal_type_t type)
{
    mobs_t *set = &m->base.object_set;

    assert(m);
    assert(addr);
    assert(type < m->base.type_desc_list_elements);

    switch (m->state)
    {
        case REGISTER_POINTERS:
        {
            mob_t object;
            marshal_member_descriptor_t desc = {0, 0, 0, 0, 0};

            desc.type = type;
            desc.array_elements = 1;

            object.address = addr;
            object.type = type;
            object.disambiguator = obj_disambiguator(&m->base, addr, &desc, NULL);

            if (mobs_push(set, &object))
            {
                /* Iterative breadth-first traversal of the object tree. Each
                   pointer member registered adds an object to the base set, the
                   iterator handles new objects being added during the iteration. */
                assert(mobs_iterate(set, m->next_values_index, register_pointer_members));

                if (m->base.has_shared_objects)
                {
                    /* Iterate the base set, adding shared members to the shared
                    member set. */
                    assert(mobs_iterate(set, m->next_values_index, register_shared_members));

                    /* During the iterations, the pointer registration may register
                    the address of a shared member of another object. Such objects
                    should not be treated as independent objects - the parent of
                    the object is responsible for marshalling all its members.
                    Remove any shared members from the main set */
                    mobs_difference_update(set, &m->base.shared_member_set);

                    assert((mobs_size(set) + mobs_size(&m->base.shared_member_set)) <= MOBS_MAX_OBJECTS);
                }
            }
            else
            {
                /* If the push failed it means a duplicate object is already in
                the set, which means its already been marshalled */
                return TRUE;
            }
            m->state = MARSHAL_VALUES;
        }
        /*lint -fallthrough */

        case MARSHAL_VALUES:
        {
            if (!mobs_iterate(set, m->next_values_index, marshal_values))
            {
                break;
            }
            m->state = MARSHAL_VALUES_END;
        }
        /*lint -fallthrough */

        case MARSHAL_VALUES_END:
        {
            /* Write a type value header indicating the end of object values. */
            marshal_type_t header = MARSHAL_TYPES_MAX;
            if (!write(&m->base, &header, sizeof(header)))
            {
                break;
            }
            m->state = MARSHAL_POINTER_INDEXES;
        }
        /*lint -fallthrough */

        case MARSHAL_POINTER_INDEXES:
        {
            if (mobs_iterate(set, m->next_pointers_index, marshal_pointer_indexes))
            {
                m->state = REGISTER_POINTERS;
                return TRUE;
            }
        }
        break;

        default:
            assert(FALSE);
        break;
    }

    return FALSE;
}

size_t marshal_produced(marshal_context_t *m)
{
    assert(m);
    return m->index;
}

size_t marshal_remaining(marshaller_t m)
{
    mobs_t *set = &m->base.object_set;
    assert(m);

    m->remaining = 0;
    switch (m->state)
    {
        default:
        case REGISTER_POINTERS:
        break;

        case MARSHAL_VALUES:
            assert(mobs_iterate(set, m->next_values_index, count_remaining_values));

        /*lint -fallthrough */
        case MARSHAL_VALUES_END:
            m->remaining += sizeof(marshal_type_t);

        /*lint -fallthrough */
        case MARSHAL_POINTER_INDEXES:
            assert(mobs_iterate(set, m->next_pointers_index, count_remaining_indexes));
        break;
    }
    return m->remaining;
}

#endif /* INSTALL_MARSHAL */
