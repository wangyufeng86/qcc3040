/* Copyright (c) 2018 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 * Marshal public header file.
 * This file contains public interfaces.
 */
/**
\defgroup marshal marshal
\ingroup core
The marshal module supports marshalling and unmarshalling of C object hierarchies.

Introduction
============
Marshalling is the process converting a C object hierarchy (comprising structs,
unions, arrays, primitive types, pointers) to a formatted stream of bytes.

Unmarshalling is the process of converting the formatted stream of bytes back
to a C object hierarchy.

This process may also known by the terms pickle, serialise, flatten etc.

Terminology
===========
In the marshalling module the following terminology is used:

Object
------
- An object is a C variable instance.
- An object is not a member of another object (see shared member).
- An object is not a pointer.
- An object has an address and a type.

Member
------
- A member is a member of a C struct or union type.
- A member has a type.
- A member has an offset in the parent struct/union (obtained using the
  offsetof macro).
- Only one member of a union type instance may be active.

For example, a, b, c, d are members of the struct alpha with type uint32.
~~~{.c}
    struct alpha
    {
        uint32 a;
        uint32 b;
        uint32 c;
        uint32 d;
    };
~~~

Shared Member
-------------
- A shared member is a member.
- A shared member's address is exposed/shared by the shared member's
  parent such that another object's member may point to the shared member.

For example if the address of struct alpha.a was exposed,
struct pointers.p's value could be the address of struct alpha.a (or any
other uint32).
~~~{.c}
    struct pointers
    {
        uint32 *p;
    };
~~~
Shared members are marshaled once by the parent object, not the object
with a pointer member that refers to the shared member.

Hierarchy
---------
The struct hierarchy has members that themselves have members, which
forms a hierarchy. When marshalling hierarchy, all members of the
hierarchy must be marshaled.
~~~{.c}
    struct hierarchy
    {
        struct alpha alp;
        struct pointers ptr;
    };
~~~

Supported
=========
- Primitive types
- Struct types
- Union types (including dynamic length structs containing a tagged union)
- Pointer members (including share objects and recursive objects)
- Arrays (including structs containing dynamic length arrays)

Not Supported
=============
- Bitfields
    The module uses the offsetof macro to determine the byte offset of
    object members. Since it is not possible to determine the offset of a
    bitfield in a structure (since bitfields may start at non-byte
    boundaries), it is not possible to marshal structs containing bitfields.
    A work-around is to wrap the bitfield in a sub-struct - the offsetof the
    sub-struct is determinable.
- Pointers to pointers
    Not supported in this version.
- Constants
- Globals
    A object with global scope may be marshalled, but the marshalling module
    does not support unmarshalling that object to the equivalent global object
    on the unmarshaller. Instead, the unmarshaller will dynamically allocate
    memory for the object.

Type Descriptors
================
The marshal module has no way of knowing the format/properties of types
the user of the module wishes to marshal. Therefore the format/properties
of all types must be described in a generic way the marshal module can use at
runtime.

The marshalling module is initialised with an array of
\c marshal_type_descriptor_t. The type descriptors describe the type's size,
its members and provides a number of callbacks where dynamic information
about an instance of a type is obtained. If a type has members (i.e. a struct)
these are described in an array of \c marshal_member_descriptor_t. Each member
has an byte offset in the structure, a type, and various other properties
(pointer, array etc). Since a type's members may also have members, a
hierarchy may be formed. The marshal module uses the type descriptors to
traverse the hierarchy of objects and marshal all objects.
To correctly reconstruct the object hierarchy from the marshalled byte stream,
the unmarshaller must have an identical description of the types.

Marshalling/Unmarshalling Process Overview
==========================================
1.  The marshaller traverses the object hierarchy breadth first - following
    pointers.
    It stores the address and type of each object visited in the object set.
    Once complete, all objects in the hierarchy are known.

2.  The marshaller iterates through the object set and traverses each object's
    hierarchy - not follow pointers. It writes the type and value of each
    object to the byte stream.
    This information tells the unmarshaller the type and value of each object
    in the hierarchy.

3.  The unmarshaller reads the byte stream, creating all objects and assigning
    values to all objects. It knows the size and structure of each object from
    the marshal_type_descriptor_t list. At this point all objects in the
    hierarchy are created with values, but the structure of the hierarchy is
    flat - the objects' pointers are not set.
    The unmarshaller stores the address and type of each object it unmarshals
    in its own object set. Since the marshaller/unmarshaller push objects to
    their sets in the same order, the index of each object in the marshalled/
    unmarshalled object sets is common - this is key to the next step.
    The structure of the objects in the hierarchy can be defined using these
    indexes.

4.  The marshaller iterates through the object set and traverses each object's
    hierarchy - not following pointers.
    It writes the object set index of each pointer member to the byte stream.
    This information tells the unmarshaller the structure of the objects (e.g.
    member X is a pointer to the object at index 1 in the object set).

5.  The unmarshaller iterates through its object set and traverses each
    object's hierarchy - not following pointers (they're not valid yet). For
    each pointer member it reads the index of the referred object from the
    byte stream and looks up the address/type of the index in its object set.
    It sanity checks the type of the indexed object against the type of the
    pointer member. It assigns the indexed address to the pointer.
    Now the unmarshaller has created, set values and set pointers for all
    objects in the marshaller's object hierarchy. The unmarshaller returns the
    address and type of the object to the \c unmarshal caller.

Memory Allocation
=================
The unmarshaller uses the information in the marshal_type_descriptor_t to
dynamically allocate memory for each typed object it unmarshals.

Marshal Byte Stream
===================
There is currently one byte stream format. It is Qualcomm proprietary and
raw binary (as opposed to a human readable form e.g. ASCII).

Diagrammatically, the marshal byte stream is:
    ||||||
    | :--------: | :--------: | :----------: | :-----------: | :-----------: |
    | TYPE-VALUE | TYPE-VALUE | END_OF_TYPES | POINTER_INDEX | POINTER_INDEX |

Full/Empty Buffer Handling
==========================

Handling a Full Buffer when Marshalling
---------------------------------------
In step 2, as the marshaller iterates through the objects in its set, it
stores the index of the next object whose value it will marshal. If the
marshal fails due to a full buffer, the marshaller reverts any incomplete
writes to the buffer and returns from \c marshal with value FALSE.
When there is more space in the buffer and \c marshal is called again, the
marshaller uses the stored index to restart the iteration at the correct
object index.
The same approach is taken in step 4 when indexes are marshalled.

Handling an Empty Buffer when Unmarshalling
-------------------------------------------
In step 3 above, the unmarshaller receives a series of type-value
structures. If a type-value structure cannot be completely read due to an
empty buffer, the unmarshaller reverts the incomplete read and returns from
\c unmarshal() with value FALSE. When more data arrives, the unmarshaller
resumes reading from the start of a type-value structure.
The same approach is taken in step 5 when indexes are unmarshalled.

Shared Members
==============
Shared members are signalled to the marshal module using the
\c marshal_member_descriptor_t.is_shared flag. Referring to the
"Marshalling/Unmarshalling Process Overview" (above), in step 1, some
pointers encountered may point to shared members. Shared members are
handled as follows:

A.  After step 1, the marshaler iterates the object set and traverses each
    object's hierarchy - not following pointers. It stores any members flagged
    as shared in a seperate shared member set. It then removes any
    shared members from the main object set. The main object set stores the
    address/type of all objects in the hierarchy. The shared member set
    stores the address/type of all shared members.

B.  After step 3, the unmarshsher does as described in step A (but doesn't
    need to remove any objects from the main object set, since only the
    objects in the main object set are marshaled).
    Now the marshaler and unmarshaler have identical main object sets and
    shared member sets.

C.  During step 4/5, the marshaler/unmarshaler have to distinguish between
    indexes in the main object set and the shared member set. An index from the
    shared member set is marshaled by adding to it the number of objects in
    the main object set.

Memory Usage
============

Dynamic
-------
The main memory usage by the marshaller/unmarshaller is the storage of
object address/type in the object sets. The \c mobs_t stores objects
in a linked-list where each node in the list stores a block of
objects (\c mob_block_t) (to minimise overhead of list next pointer). The
number of objects in each block is configurable through the #BLOCK_SIZE
definition.

Const
-----
The \c marshal_type_descriptor_t and \c marshal_member_descriptor_t for
all types should be stored in constant memory.
 */

#ifndef MARSHAL_H_
#define MARSHAL_H_

#include "hydra/hydra_types.h"
#include "hydra/hydra_macros.h"
#include "app/marshal/marshal_if.h"

/** Create and initialise a marshaller.
 *  \param type_desc_list A list of marshal type descriptors.
 *  \param type_desc_list_elements The number of types in the list.
 *  \return Handle to an initialised marshaller, or NULL if initialisation failed.
 *
 *  \note The elements in the type_descriptor_list should not exceed
 *        MARSHAL_TYPES_MAX. The type_descriptor_list used on marshaller and
 *        unmarshaller _must_ match for marshalling / unmarshalling to complete
 *        successfully.
 */
marshaller_t marshal_init(const marshal_type_descriptor_t * const *type_desc_list,
                          size_t type_desc_list_elements);

/** Set the marshaling byte stream buffer and space.
 *  \param m Handle for the marshaller.
 *  \param buf Address to which the marshaller will write the marshalled byte stream.
 *  \param space The number of bytes space in the buffer.
 *  \note Setting the buffer does not clear the store of references to marshalled
 *        objects, use \c marshal_clear_store.
 *  \note This function may be called with a NULL buf and/or zero space. In
 *        either case, \c marshal will return FALSE (but it will perform the
 *        steps necessary to determine the size of the byte stream that will be
 *        produced by marshalling the object, allowing \c marshal_remaining to
 *        be called). For \c marshal to return TRUE, this function must be called
 *        after \c marshal_init before calling \c marshal with a valid address
 *        and space.
 */
void marshal_set_buffer(marshaller_t m, void *buf, size_t space);

/** Marshal a object hierarchy.
 *  \param m Handle for the marshaller.
 *  \param addr The address of the head object in the hierarchy.
 *  \param type The type of the head object. The type is used as an index into
 *         the type_descriptor_list provided in \c marshal_init.
 *  \return TRUE if the full object hierarchy was marshalled.
 *          FALSE if there was not enough space in the buffer to fully marshal
 *          the object hierarchy.
 *  \note   Hierarchies are marshalled piecemeal. The function
 *          \c marshal_produced may be used to determine how many bytes were
 *          written before the marshaller ran out of space. When more space
 *          is available in the buffer, the function \c marshal_set_buffer
 *          should be called to inform the marshaller. \c marshal and \c
 *          marshal_set_buffer should be called repeatedly until the full object
 *          hierarchy is marshalled.
 *  \note   \c marshal should not be called with a new object until marshalling
 *          is complete for the current object.
 */
bool marshal(marshaller_t m, void *addr, marshal_type_t type);

/** Query the number of bytes written since the last re-initialisation.
 *  \param m Handle for the marshaller.
 *  \return The number of marshal bytes written to the buffer.
 *  \note Zero will be returned after \c marshal_set_buffer is called.
*/
size_t marshal_produced(marshaller_t m);

/** Query the number of bytes that remain to be written to complete marshalling
 *  of the current object.
 *  \param m Handle for the marshaller.
 *  \return The number of bytes remaining to be written to the buffer.
 *  \note This function will return a non-zero value when \c marshal returns
 *        FALSE (which indicates failure to marshal due to insufficient space
 *        in the buffer). This function will return zero when \c marshal returns
 *        TRUE (there are no remaining bytes to be marshalled).
*/
size_t marshal_remaining(marshaller_t m);

/** Clear the internal store of marshalled object references.
 *  \param m Handle for the marshaller.
 *  \note When marshalling objects, the marshaller stores the address and type
 *        of each object it marshals. The index of objects in the store is
 *        used by the marshalling protocol as a common reference for objects
 *        pointed to by other objects.
 *        Clearing the store is useful for reusing marshallers without having to
 *        destroy/initialise.
 */
void marshal_clear_store(marshaller_t m);

/** Destroy the marshaller.
 *  \param m Handle for the marshaller.
 *  \param free_all_objects If TRUE, all marshalled objects referenced in the
 *         store will also be freed.
 *         If FALSE, only the internal state will be freed.
*/
void marshal_destroy(marshaller_t m, bool free_all_objects);

/** Create and initialise a unmarshaller.
 *  \param type_descriptor_list A list of marshal type descriptors.
 *  \param type_descriptor_list_elements The number of types in the list.
 *  \return Handle to an initialised unmarshaller, or NULL if initialisation failed.
 *
 *  \note The elements in the type_descriptor_list should not exceed
 *        MARSHAL_TYPES_MAX. The type_descriptor_list used on marshaller and
 *        unmarshaller _must_ match for marshalling / unmarshalling to complete
 *        successfully.
*/
unmarshaller_t unmarshal_init(const marshal_type_descriptor_t * const *type_descriptor_list,
                              size_t type_descriptor_list_elements);

/** Set the unmarshaling byte stream buffer and data size.
 *  \param u Handle for the unmarshaller.
 *  \param buf Address from where the unmarshaller will read the marshalled byte
 *             stream.
 *  \param data_size The number of data bytes in the buffer.
 *  \note Setting the buffer does not clear the store of references to
 *        unmarshalled objects, use \c unmarshal_clear_store.
 *  \note This function must be called after \c unmarshal_init before calling
 *        \c unmarshal.
 */
void unmarshal_set_buffer(unmarshaller_t u, const void *buf, size_t data_size);

/** Unmarshal a marshalled byte stream.
 *  \param u Handle for the unmarshaller.
 *  \param addr The function will set addr to the address of the allocated object
 *         at the head of the unmarshalled object hierarchy.
 *  \param type The function will set type to the type of the object at the
 *         head of the unmarshalled object hierarchy. The type is an index into the
 *         type_descriptor_list provided in \c unmarshal_init.
 *  \return TRUE if the full object hierarchy was unmarshalled.
 *          FALSE if there was not enough data in the buffer to fully unmarshal
 *          the object hierarchy.
 *  \note   Hierarchies are unmarshalled piecemeal. The function
 *          \c unmarshal_consumed may be used to determine how many bytes were
 *          read before the unmarshaller ran out of data. When more data is
 *          available in the buffer, the function \c unmarshal_set_buffer
 *          should be called to inform the unmarshaller. \c unmarshal and
 *          \c unmarshal_set_buffer should be called repeatedly until the full
 *          object hierarchy is unmarshalled.
 */
bool unmarshal(unmarshaller_t u, void **addr, marshal_type_t *type);

/** Query the number of bytes read since the last re-initialisation.
 *  \param u Handle for the unmarshaller.
 *  \return The number of marshal bytes read from the buffer.
 *  \note Zero will be returned after \c unmarshal_set_buffer is called.
*/
size_t unmarshal_consumed(unmarshaller_t u);

/** Clear the internal store of unmarshalled object references.
 *  \param u Handle for the unmarshaller.
 *  \note When unmarshalling objects, the unmarshaller stores the address and type
 *        of each object it unmarshals. The index of objects in the store is
 *        used by the marshalling protocol as a common reference for objects
 *        pointed to by other objects.
 *        Clearing the store is useful for reusing unmarshallers without having to
 *        destroy/initialise.
 */
void unmarshal_clear_store(unmarshaller_t u);

/** Destroy the unmarshaller.
    \param u Handle for the unmarshaller.
    \param free_all_objects If TRUE, all unmarshalled objects will be freed.
           If FALSE, only the internal state will be freed.
*/
void unmarshal_destroy(unmarshaller_t u, bool free_all_objects);

#endif
