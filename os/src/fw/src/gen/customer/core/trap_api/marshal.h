#ifndef __MARSHAL_H__
#define __MARSHAL_H__
#include <app/marshal/marshal_if.h>

/*! file 
    @brief Marshal object hierarchies.

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

Example Usage
=============
~~~{.c}

// Type descriptor for basic uint8 type
const marshal_type_descriptor_t marshal_type_descriptor_uint8 =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(uint8);

// Enumerate ids for each type that will be marshalled.
enum MARSHAL_TYPES
{
    MARSHAL_TYPE(uint8),
    // ...
    NUMBER_OF_MARSHAL_OBJECT_TYPES
};

// Array of pointers to type descriptors - must be in the same order as the enum
const marshal_type_descriptor_t * const marshal_type_descriptors[NUMBER_OF_MARSHAL_OBJECT_TYPES] =
{
    &marshal_type_descriptor_uint8,
    // ...
};

// Function illustrating marshalling a uint8 to a buffer, returning number of bytes written.
size_t marshal_uint8(uint8 *u8p, uint8 *buf, size_t buf_len)
{
    size_t produced;
    marshaller_t m;

    m = MarshalInit(marshal_type_descriptors, ARRAY_DIM(marshal_type_descriptors));
    MarshalSetBuffer(m, buf, buf_len);
    Marshal(m, (void*)u8p, MARSHAL_TYPE(uint8));
    produced = MarshalProduced(m);
    MarshalDestroy(m, FALSE);
    return produced;
}

// Function illustrating unmarshalling
size_t do_unmarshal(const uint8 *buf, size_t buf_len)
{
    size_t consumed;
    void *object;
    marshal_type_t type;
    unmarshaller_t u;

    u = UnmarshaInit(marshal_type_descriptors, ARRAY_DIM(marshal_type_descriptors));
    UnmarshalSetBuffer(u, buf, buf_len);
    Unmarshal(u, &object, &type);
    // do something with the unmarshalled object (given its type)
    // ...
    free(object);
    consumed = UnmarshalConsumed(u);
    UnmarshalDestroy(u, FALSE);
    return consumed;
}
~~~
*/
      
#if TRAPSET_MARSHAL

/**
 *  \brief Create and initialise a marshaller.
 * The elements in the type_descriptor_list should not exceed MARSHAL_TYPES_MAX.
 * The type_descriptor_list used on marshaller and unmarshaller _must_ match for
 * marshalling / unmarshalling to complete successfully.
 *         
 *  \param type_desc_list A list of marshal type descriptors.
 *  \param type_desc_list_elements The number of types in the list.
 *  \return Handle to an initialised marshaller, or NULL if initialisation failed.
 * 
 * \ingroup trapset_marshal
 */
marshaller_t MarshalInit(const marshal_type_descriptor_t * const * type_desc_list, size_t type_desc_list_elements);

/**
 *  \brief Create and initialise a unmarshaller.
 * The elements in the type_descriptor_list should not exceed MARSHAL_TYPES_MAX.
 * The type_descriptor_list used on marshaller and unmarshaller _must_ match for
 * marshalling / unmarshalling to complete successfully.
 *         
 *  \param type_desc_list A list of unmarshal type descriptors.
 *  \param type_desc_list_elements The number of types in the list.
 *  \return Handle to an initialised unmarshaller, or NULL if initialisation failed.
 * 
 * \ingroup trapset_marshal
 */
unmarshaller_t UnmarshalInit(const marshal_type_descriptor_t * const * type_desc_list, size_t type_desc_list_elements);

/**
 *  \brief Set the marshaling byte stream buffer and space.
 * Setting the buffer does not clear the store of references to marshalled objects,
 * use \c MarshalClearStore. This function may be called with a NULL buf and/or
 * zero space. In either case, \c Marshal will return FALSE (but it will perform
 * the steps necessary to determine the size of the byte stream that will be
 * produced by marshalling the object, allowing \c MarshalRemaining to be called).
 * For \c Marshal to return TRUE, this function must be called after \c
 * MarshalInit before calling \c Marshal with a valid address and space.
 *         
 *  \param m Handle for the marshaller.
 *  \param buf Address to which the marshaller will write the marshalled byte stream.
 *  \param space The number of bytes space in the buffer.
 * 
 * \ingroup trapset_marshal
 */
void MarshalSetBuffer(marshaller_t m, void * buf, size_t space);

/**
 *  \brief Set the unmarshaling byte stream buffer and data size.
 * Setting the buffer does not clear the store of references to unmarshalled
 * objects, use \c UnmarshalClearStore. This function must be called after \c
 * UnmarshalInit before calling \c Unmarshal.
 *         
 *  \param u Handle for the marshaller.
 *  \param buf Address from which the unmarshaller will read the marshalled byte stream.
 *  \param data_bytes The number of data bytes in the buffer.
 * 
 * \ingroup trapset_marshal
 */
void UnmarshalSetBuffer(unmarshaller_t u, const void * buf, size_t data_bytes);

/**
 *  \brief Marshal a object hierarchy.
 * Hierarchies are marshalled piecemeal. The function \c MarshalProduced may be
 * used to determine how many bytes were written before the marshaller ran out of
 * space. When more space is available in the buffer, the function \c
 * MarshalSetBuffer should be called to inform the marshaller. \c Marshal and \c
 * MarshalSetBuffer should be called repeatedly until the full object hierarchy
 * is marshalled. \c Marshal should not be called with a new object until
 * marshalling is complete for the current object.
 *         
 *  \param m Handle for the marshaller.
 *  \param addr The address of the head object in the hierarchy.
 *  \param type The type of the head object.
 *  \return TRUE if the full object hierarchy was marshalled. FALSE if there was not enough 
 * space in the buffer to fully marshal the object hierarchy.
 *           
 * 
 * \ingroup trapset_marshal
 */
bool Marshal(marshaller_t m, void * addr, marshal_type_t type);

/**
 *  \brief Unmarshal a object hierarchy.
 * Hierarchies are unmarshalled piecemeal. The function \c UnmarshalConsumed may
 * be used to determine how many bytes were read before the unmarshaller ran out of
 * data. When more data is available in the buffer, the function \c
 * UnmarshalSetBuffer should be called to inform the unmarshaller. \c Unmarshal
 * and \c UnmarshalSetBuffer should be called repeatedly until the full object
 * hierarchy is unmarshalled.
 *         
 *  \param u Handle for the unmarshaller.
 *  \param addr The function will set addr to the address of the allocated object at the head of
 * the unmarshalled object hierarchy.
 *             
 *  \param type The function will set type to the type of the object at the head of the
 * unmarshalled object hierarchy. The type is an index into the
 * type_descriptor_list provided in \c UnmarshalInit.
 *             
 *  \return TRUE if the full object hierarchy was unmarshalled. FALSE if there was not
 * enough data in the buffer to fully unmarshal the object hierarchy.
 *           
 * 
 * \ingroup trapset_marshal
 */
bool Unmarshal(unmarshaller_t u, void ** addr, marshal_type_t* type);

/**
 *  \brief Query the number of bytes written since the last re-initialisation.
 *  \param m Handle for the marshaller.
 *  \return The number of marshal bytes written to the buffer.
 * 
 * \ingroup trapset_marshal
 */
size_t MarshalProduced(marshaller_t m);

/**
 *  \brief Query the number of bytes that remain to be written to complete marshalling of
 *  the current object.
 * This function will return a non-zero value when \c Marshal returns FALSE (which
 * indicates failure to marshal due to insufficient space in the buffer). This
 * function will return zero when \c Marshal returns TRUE (there are no remaining
 * bytes to be marshalled).
 *         
 *  \param m Handle for the marshaller.
 *  \return The number of bytes remaining to be written to the buffer.
 * 
 * \ingroup trapset_marshal
 */
size_t MarshalRemaining(marshaller_t m);

/**
 *  \brief Query the number of bytes read since the last re-initialisation.
 *  \param u Handle for the marshaller.
 *  \return The number of marshal bytes read from the buffer.
 * 
 * \ingroup trapset_marshal
 */
size_t UnmarshalConsumed(unmarshaller_t u);

/**
 *  \brief Clear the internal store of marshalled object references.
 * When marshalling objects, the marshaller stores the address and type of each
 * object it marshals. The index of objects in the store is used by the marshalling
 * protocol as a common reference for objects pointed to by other objects. Clearing
 * the store is useful for reusing marshallers without having to
 * destroy/initialise.
 *         
 *  \param m Handle for the marshaller.
 * 
 * \ingroup trapset_marshal
 */
void MarshalClearStore(marshaller_t m);

/**
 *  \brief Clear the internal store of marshalled object references.
 * When unmarshalling objects, the unmarshaller stores the address and type of each
 * object it unmarshals. The index of objects in the store is used by the
 * marshalling protocol as a common reference for objects pointed to by other
 * objects. Clearing the store is useful for reusing unmarshallers without having
 * to destroy/initialise.
 *         
 *  \param u Handle for the unmarshaller.
 * 
 * \ingroup trapset_marshal
 */
void UnmarshalClearStore(unmarshaller_t u);

/**
 *  \brief Destroy the marshaller.
 *  \param m Handle for the marshaller.
 *  \param free_all_objects If TRUE, all objects that have been marshaled will also be freed.
 * If FALSE, only the internal state will be freed and the caller is responsible
 * for freeing the marshalled objects.
 *             
 * 
 * \ingroup trapset_marshal
 */
void MarshalDestroy(marshaller_t m, bool free_all_objects);

/**
 *  \brief Destroy the unmarshaller.
 *  \param u Handle for the marshaller.
 *  \param free_all_objects If TRUE, all objects that have been unmarshaled will also be freed.
 * If FALSE, only the internal state will be freed and the caller is responsible
 * for freeing the unmarshalled objects.
 *             
 * 
 * \ingroup trapset_marshal
 */
void UnmarshalDestroy(unmarshaller_t u, bool free_all_objects);
#endif /* TRAPSET_MARSHAL */
#endif
