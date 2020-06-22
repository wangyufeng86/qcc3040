/* Copyright (c) 2018 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 *
 * Header for the marshal base functionality common to marshaller and unmarshaller.
 */

#ifndef MARSHAL_BASE_H_
#define MARSHAL_BASE_H_

#include "assert.h"

#include "hydra/hydra_macros.h"
#include "pmalloc/pmalloc.h"

#include "marshal.h"
#include "marshal_object_set.h"

/** Special index indicating NULL pointer.
 *  The base adds a NULL object to the object set during initialisation.
 *  Any attempts to marshal objects at address NULL will match this object at
 *  the special index MOB_INDEX_NULL. When a MOB_INDEX_NULL is unmarshalled
 *  an object with address=NULL will be returned.
 */
#define MOB_INDEX_NULL 0x00

/** Type definition of the marshal base object.
 *  The base is common to both marshaller/unmarshaller.
 */
typedef struct marshal_base
{
    /** Set of objects. */
    mobs_t object_set;

    /** Set of shared members. */
    mobs_t shared_member_set;

    /** Client's list of type descriptors */
    const marshal_type_descriptor_t * const *type_desc_list;

    /** The number of types in the list of type descriptors */
    uint8 type_desc_list_elements;

    /** Set to one during initialisation if the type descriptions contain
        shared objects */
    unsigned has_shared_objects : 1;

} marshal_base_t;

/** Callback function from object_tree_traverse for indicating members.
 *  \param base The marshal base object.
 *  \param member The current member visited in the tree traversal.
 *  \return If the callback returns FALSE, the tree traverse will stop.
 */
typedef bool (*traverse_callback_member_t)(marshal_base_t *base, mob_t *member);

/** Callback function from object_tree_traverse for indicating pointer members.
 *  \param base The marshal base object.
 *  \param pointer_addr The address of the pointer visited in the tree traversal.
 *  \param desc The pointer's member descriptor.
 *  \param parent The pointer member's parent.
 *  \return If the callback returns FALSE, the tree traverse will stop.
 */
typedef bool (*traverse_callback_pointer_member_t)(
                    marshal_base_t *base,
                    void *pointer_addr,
                    const marshal_member_descriptor_t *desc,
                    mob_t *parent);

/** Type definition for the callback functions from the object_tree_traverse
 *  function.
 */
typedef struct object_members_hierarchy_callbacks
{
    /** Called when the tree traverse visits a leaf member.
     *  A leaf member is one which is not a pointer and has no members.
     */
    traverse_callback_member_t leaf_member;

    /** Called when the tree traverse visits a pointer member.
     *  A pointer member is one which has the is_pointer flag set in the
     *  marshal_member_descriptor structure.
     */
    traverse_callback_pointer_member_t pointer_member;

    /** Called when the tree traverse visits a shared member.
     *  A shared member is one which has the is_shared flag set in the
     *  marshal_member_descriptor structure.
     */
    traverse_callback_member_t shared_member;

} traverse_callbacks_t;

/** Initialise the marshal base.
 *  \param base The base.
 *  \param type_desc_list Pointer to array of pointers to type descriptors.
 *  \param type_desc_list_elements Number of elements in the
 *         type_desc_list array.
 */
void base_init(marshal_base_t *base,
               const marshal_type_descriptor_t * const *type_desc_list,
               size_t type_desc_list_elements);

/** Uninitialise the marshal base.
 *  \param base The base.
 *  \param free_all_objects If TRUE, all objects will be freed.
           If FALSE, only the internal state will be freed.
 */
void base_uninit(marshal_base_t *base, bool free_all_objects);

/** Clear the internal store of marshalled object references.
 *  \param base The base.
 */
void base_clear_store(marshal_base_t *base);

/** Traverse objects in the tree/hierarchy of the object.
 *  \param base The base.
 *  \param object The object at which to start the traverse.
 *  \param callbacks Set of callback functions - all callbacks are optional,
 *         unwanted callbacks may be set to NULL.
 *  \return TRUE if the traverse completed, FALSE if a callback returned FALSE
 *          halting the traverse.
 *
 *  The traverse iteratively visits each member of the object's tree (sub-tree etc)
 *  in "pre-order", but does not follow pointers, instead calling the
 *  pointer_member callback (if set).
 *
 *  The traverse supports:
 *      + Basic types (type without members)
 *      + Structured types (types with members)
 *      + Pointers to other types (but pointers are not followed)
 *      + Types with fixed length arrays (including arrays of pointers)
 *      + Types with dynamic length arrays using the parent type's array_elements
 *        callback (including arrays of pointers).
 *      + Types with union types using the tagged_union_member callback.
 */
bool object_tree_traverse(marshal_base_t *base, mob_t *object,
                          const traverse_callbacks_t *callbacks);

/** Traverse the tree/hierarchy of object adding any shared members to the
    base shared object set.
 *  \param set The set being iterated.
 *  \param index The index of the parent object.
 *  \param object The object to traverse.
 *  \return Always returns TRUE.
 */
bool register_shared_members(mobs_t *set, mob_index_t index, mob_t *object);

/** Get an object's disambiguator.
 *  \param base The base.
 *  \param obj The address of the object to disambiguate.
 *  \param desc The member descriptor of the object.
 *  \param parent The object's parent object.
 *  \return The object's disambiguator.
 *
 *  Dynamic length objects (tagged unions and dynamic length arrays) have
 *  disambiguators. The disambiguator is the tagged union member index or
 *  the number of array elements.
 */
uint8 obj_disambiguator(marshal_base_t *base,
                        void *obj,
                        const marshal_member_descriptor_t *desc,
                        mob_t *parent);

/** Determine size of a type given its disambiguator.
 *  \param base The base.
 *  \param type The object type whose size will be determined.
 *  \param disambiguator The number of elements in a dynamic length array or
 *         the tagged union member index.
 *  \return The size of the object.
 */
size_t obj_size(marshal_base_t *base, marshal_type_t type, uint8 disambiguator);

#endif
