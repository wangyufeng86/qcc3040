/* Copyright (c) 2018 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 */

#include "marshal_base.h"

#ifdef INSTALL_MARSHAL

static void base_mobs_init(marshal_base_t *base)
{
    mob_t null_obj = MOB_ZERO();

    mobs_init(&base->object_set);
    mobs_init(&base->shared_member_set);

    /* Use type guaranteed to not be used to library client. */
    null_obj.type = MARSHAL_TYPES_MAX;

    /* Reserve a NULL object at the start of the set.
    Any attempts to marshal objects at address NULL will match this object at
    the special index MOB_INDEX_NULL. When a MOB_INDEX_NULL is unmarshalled
    this object with address=NULL will be returned. */
    assert(mobs_push(&base->object_set, &null_obj));
}

static void base_initialise_type_filters(marshal_base_t *base)
{
    uint32 type;
    for (type = 0; type < base->type_desc_list_elements; type++)
    {
        uint32 member;
        const marshal_type_descriptor_t *type_desc = base->type_desc_list[type];
        for (member = 0; member < type_desc->members_len; member++)
        {
            const marshal_member_descriptor_t *member_desc = &type_desc->u.members[member];
            if (member_desc->is_shared)
            {
                base->has_shared_objects = TRUE;
                return;
            }
        }
    }
}

void base_init(marshal_base_t *base,
               const marshal_type_descriptor_t * const *type_desc_list,
               size_t type_desc_list_elements)
{
    assert(type_desc_list_elements <= MARSHAL_TYPES_MAX);

    base_mobs_init(base);

    base->type_desc_list_elements = (uint8)type_desc_list_elements;
    base->type_desc_list = type_desc_list;
    base_initialise_type_filters(base);
}

void base_uninit(marshal_base_t *base, bool free_all_objects)
{
    if (free_all_objects)
    {
        mob_t object;
        while (mobs_pop(&base->object_set, &object))
        {
            if (object.address)
            {
                pfree(object.address);
            }
        }
    }
    mobs_destroy(&base->object_set);
    mobs_destroy(&base->shared_member_set);
}

void base_clear_store(marshal_base_t *base)
{
    base_uninit(base, FALSE);
    base_mobs_init(base);
}

static bool register_shared_member(marshal_base_t *base, mob_t *shared)
{
    assert(mobs_push(&base->shared_member_set, shared));
    return TRUE;
}

bool register_shared_members(mobs_t *set, mob_index_t index, mob_t *object)
{
    marshal_base_t *base = STRUCT_FROM_MEMBER(marshal_base_t, object_set, set);
    traverse_callbacks_t callbacks = {NULL, NULL, register_shared_member};

    UNUSED(index);

    return object_tree_traverse(base, object, &callbacks);
}

uint8 obj_disambiguator(marshal_base_t *base,
                        void *obj,
                        const marshal_member_descriptor_t *desc,
                        mob_t *parent)
{
    uint32 d = 0;
    const marshal_type_descriptor_t *obj_type = base->type_desc_list[desc->type];

    if (obj_type->dynamic_length)
    {
        const marshal_member_descriptor_t *dynamic_member;
        const marshal_type_descriptor_dynamic_t *obj_dyn_type;
        const marshal_type_descriptor_dynamic_t *parent_dyn_type;

        /* Upcast to derived dynamic descriptor type */
        obj_dyn_type = STRUCT_FROM_CONST_MEMBER(marshal_type_descriptor_dynamic_t,
                                                base, obj_type);

        if (obj_type->dynamic_type == MARSHAL_DYNAMIC_TAGGED_UNION)
        {
            if (obj_dyn_type->tagged_union_member)
            {
                /* The dynamic member is the final member */
                assert(obj_type->u.members && obj_type->members_len);
                dynamic_member = obj_type->u.members + (obj_type->members_len - 1);

                assert(base->type_desc_list[dynamic_member->type]->is_union &&
                       !dynamic_member->is_pointer &&
                       (dynamic_member->array_elements == 1));

                d = obj_dyn_type->tagged_union_member(obj, dynamic_member, 0);
            }
            else
            {
                assert(parent);
                parent_dyn_type = STRUCT_FROM_CONST_MEMBER(marshal_type_descriptor_dynamic_t, base,
                                                           base->type_desc_list[parent->type]);

                /* If member cannot determine its own active union member index,
                   the parent must provide the tagged union member callback */
                assert(parent_dyn_type->tagged_union_member);

                d = parent_dyn_type->tagged_union_member(parent->address, desc, 0);
            }
        }
        else /* MARSHAL_DYNAMIC_ARRAY */
        {
            if (obj_dyn_type->array_elements)
            {
                /* The dynamic member is the final member */
                assert(obj_type->u.members && obj_type->members_len);
                dynamic_member = obj_type->u.members + (obj_type->members_len - 1);

                d = obj_dyn_type->array_elements(obj, dynamic_member, 0);
            }
            else
            {
                assert(parent);
                parent_dyn_type = STRUCT_FROM_CONST_MEMBER(marshal_type_descriptor_dynamic_t, base,
                                                           base->type_desc_list[parent->type]);

                /* If member cannot determine its own number of array elements
                   the parent must provide the array elements callback */
                assert(parent_dyn_type->array_elements);

                d = parent_dyn_type->array_elements(parent->address, desc, 0);
            }
        }
        assert(d <= UCHAR_MAX);
    }
    return (uint8)d;
}

size_t obj_size(marshal_base_t *base, marshal_type_t type, uint8 disambiguator)
{
    const marshal_type_descriptor_t *type_desc = base->type_desc_list[type];
    size_t size = type_desc->size;

    if (type_desc->dynamic_length)
    {
        const marshal_member_descriptor_t *dynamic_member;
        const marshal_type_descriptor_t *dynamic_type_desc;

        assert(type_desc->u.members && type_desc->members_len);

        /* The dynamic member is the final member */
        dynamic_member = type_desc->u.members + (type_desc->members_len - 1);
        dynamic_type_desc = base->type_desc_list[dynamic_member->type];

        if (type_desc->dynamic_type == MARSHAL_DYNAMIC_TAGGED_UNION)
        {
            const marshal_member_descriptor_t *tagged_member;
            const marshal_type_descriptor_t *tagged_type;
            /* For tagged unions the disambiguator is the tagged union member
               index */
            uint32 tagged_index = disambiguator;

            assert(dynamic_type_desc->is_union &&
                   !dynamic_member->is_pointer &&
                   (dynamic_member->array_elements == 1));
            assert(dynamic_type_desc->u.members && (dynamic_type_desc->members_len > tagged_index));

            tagged_member = dynamic_type_desc->u.members + tagged_index;
            tagged_type = base->type_desc_list[tagged_member->type];

            size = dynamic_member->offset + tagged_type->size;
        }
        else
        {
            /* For MARSHAL_DYNAMIC_ARRAY the disambiguator is the number of
               array elements */
            uint32 elements = disambiguator;
            size = dynamic_member->offset + (elements * dynamic_type_desc->size);
        }
    }
    return size;
}

/* Based on the child type/member descriptors call appropriate callbacks */
static bool traverse_callback(marshal_base_t *base,
                              const marshal_member_descriptor_t *child_descriptor,
                              mob_t *child,
                              mob_t *parent,
                              const traverse_callbacks_t *callbacks)
{
    const marshal_type_descriptor_t *child_type = base->type_desc_list[child->type];
    bool success = TRUE;

    if (!child_descriptor->is_pointer && !child_type->members_len && callbacks->leaf_member)
    {
        success = callbacks->leaf_member(base, child);
    }
    if (success && child_descriptor->is_pointer && callbacks->pointer_member)
    {
        success = callbacks->pointer_member(base, child->address, child_descriptor, parent);
    }
    if (success && child_descriptor->is_shared && callbacks->shared_member)
    {
        success = callbacks->shared_member(base, child);
    }
    return success;
}

/* Pre-order traverse the object's tree */
bool object_tree_traverse(marshal_base_t *base, mob_t *object,
                          const traverse_callbacks_t *callbacks)
{
#ifdef ITERATIVE_TRAVERSE
    mobs_t object_set;
    mobs_init(&object_set);
    assert(mobs_push(&object_set, object));

    while (mobs_size(&object_set))
    {
        mob_t parent;
        assert(mobs_pop(&object_set, &parent));
#else
    {
        mob_t parent = *object;
#endif
        uint32 member;
        const marshal_type_descriptor_t *parent_type;

        parent_type = base->type_desc_list[parent.type];

        /* Special case where the parent has no members -  it is a leaf */
        if (!parent_type->members_len && callbacks->leaf_member &&
            !callbacks->leaf_member(base, &parent))
        {
#ifdef ITERATIVE_TRAVERSE
            mobs_destroy(&object_set);
#endif
            return FALSE;
        }

        for (member = 0; member < parent_type->members_len; member++)
        {
            uint32 element;
            uint32 array_elements;
            const marshal_member_descriptor_t *member_descriptor;

            assert(parent_type->u.members);
            member_descriptor = &parent_type->u.members[member];
            array_elements = member_descriptor->array_elements;

            /* Only change array elements for final member */
            if (member == ((uint32)parent_type->members_len - 1))
            {
                if (parent_type->dynamic_length &&
                    parent_type->dynamic_type == MARSHAL_DYNAMIC_ARRAY)
                {
                    array_elements = parent.disambiguator;
                }
            }

            for (element = 0; element < array_elements; element++)
            {
                mob_t child = MOB_ZERO();
                const marshal_type_descriptor_t *member_type = base->type_desc_list[member_descriptor->type];
                const marshal_member_descriptor_t *arr_descriptor = member_descriptor;

                /* In the case of an array of unions, we must iterate over the
                union type length, not the tagged union member type length. */
                child.address = (char*)parent.address + member_descriptor->offset +
                    (element * (arr_descriptor->is_pointer ? sizeof(void*) :
                                                             member_type->size));

                /* For union members callback the parent for the tagged union member */
                if (member_type->is_union && !arr_descriptor->is_pointer)
                {
                    uint32 index;

                    /* Upcast to derived dynamic descriptor type */
                    const marshal_type_descriptor_dynamic_t *dyn_type_desc;
                    dyn_type_desc = STRUCT_FROM_CONST_MEMBER(marshal_type_descriptor_dynamic_t, base, parent_type);

                    /* Parents of union types must provide callback to read tagged member. */
                    assert(dyn_type_desc->tagged_union_member);
                    index = dyn_type_desc->tagged_union_member(parent.address, arr_descriptor, element);
                    assert(index < member_type->members_len);
                    /* Update member from list of union members */
                    arr_descriptor = &member_type->u.members[index];
                    member_type = base->type_desc_list[arr_descriptor->type];
                }

                child.type = arr_descriptor->type;
                if (!traverse_callback(base, arr_descriptor, &child, &parent, callbacks))
                {
#ifdef ITERATIVE_TRAVERSE
                    mobs_destroy(&object_set);
#endif
                    return FALSE;
                }

                if (!arr_descriptor->is_pointer && member_type->members_len)
                {
#ifdef ITERATIVE_TRAVERSE
                    /* Push item on the set - this function will subsequently
                       iterate through the members of the child */
                    assert(mobs_push(&object_set, &child));
#else
                    if (!object_tree_traverse(base, &child, callbacks))
                    {
                        return FALSE;
                    }
#endif
                }
            }
        }
    }
#ifdef ITERATIVE_TRAVERSE
    mobs_destroy(&object_set);
#endif
    return TRUE;
}

#endif /* INSTALL_MARSHAL */
