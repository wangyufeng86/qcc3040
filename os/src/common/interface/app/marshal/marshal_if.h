/* Copyright (c) 2018 Qualcomm Technologies International, Ltd. */
/*   %%version */

#ifndef MARSHAL_IF_H_
#define MARSHAL_IF_H_

/** Marshaller handle */
typedef struct marshal_context *marshaller_t;

/** Unmarshaller handle */
typedef struct unmarshal_context *unmarshaller_t;

/** Marshal object type. The object types are defined by the user of the library.
 *  Up to MARSHAL_TYPES_MAX may be defined.
 */
typedef uint8 marshal_type_t;

/** Helper marco to create marshal type names from c type names.
 *  For example, MARSHAL_TYPE(uint32) is expanded to MARSHAL_TYPE_uint32.
 *  This macro is used in other macros in this file.
 *  An enumeration of all the marshal types should be declared using this macro.
 *  The type is used as an index into the type_descriptor_list provided at
 *  initialisation. Ensure the enumeration of types match the indexes
 *  of the type_descriptor_list. Ensure the types defined on marshaller and
 *  unmarshaller are identical.
 */
#define MARSHAL_TYPE(type_name) MARSHAL_TYPE_ ## type_name

/** The maximum number of marshal types supported. */
#define MARSHAL_TYPES_MAX UCHAR_MAX

/** This type describes the properties of a member of an C struct or union.
 *  It is recommended that the MAKE_MARSHAL_MEMBER.. macros are used to
 *  initialise instances of this type.
*/
typedef struct marshal_member_descriptor
{
    /** The offset of the member within the parent structure obtained using
     *  offsetof(x). */
    uint8 offset;
    /** The type of the member. */
    marshal_type_t type;
    /** The number of array elements, or 1 for a single element. A value of zero
        may be set for flexible arrays. Note that arrays with greater than 63
        elements cannot be described and therefore cannot be marshalled. 
        An option to work-around this limitation is to split larger arrays into
        multiple members, manually calculating the offset. */
    unsigned array_elements : 6;
    /* The member is a pointer. Pointer to pointer is not supported. */
    unsigned is_pointer : 1;
    /* The member's address is shared. */
    unsigned is_shared : 1;

} marshal_member_descriptor_t;

/** Initialiser for a standard member.
 *  \param struct_type The C type name of the parent structure.
 *  \param type The C type name of the member.
 *  \param member The name of the member.
 */
#define MAKE_MARSHAL_MEMBER(struct_type, type, member) { \
    offsetof(struct_type, member), MARSHAL_TYPE(type), 1, 0, 0, \
}

/** Initialiser for an array member.
 *  \param struct_type The C type name of the parent structure.
 *  \param type The C type name of the array member.
 *  \param member The name of the array member.
 *  \param elements The number of elements in the array.
 */
#define MAKE_MARSHAL_MEMBER_ARRAY(struct_type, type, member, elements) { \
    offsetof(struct_type, member), MARSHAL_TYPE(type), elements, 0, 0, \
}

/** Initialiser for a shared member.
 *  \param struct_type The C type name of the parent structure.
 *  \param type The C type name of the shared member.
 *  \param member The name of the shared member.
 */
#define MAKE_MARSHAL_MEMBER_SHARED(struct_type, type, member) { \
    offsetof(struct_type, member), MARSHAL_TYPE(type), 1, 0, 1, \
}

/** Initialiser for a pointer member.
 *  \param struct_type The C type name of the parent structure.
 *  \param type The C type name of the pointer member.
 *  \param member The name of the pointer member.
 */
#define MAKE_MARSHAL_MEMBER_POINTER(struct_type, type, member) { \
    offsetof(struct_type, member), MARSHAL_TYPE(type), 1, 1, 0, \
}

/** Initialiser for a array of pointers member.
 *  \param struct_type The C type name of the parent structure.
 *  \param type The C type name of the array of pointers member.
 *  \param member The name of the array of pointers member.
 */
#define MAKE_MARSHAL_MEMBER_ARRAY_OF_POINTERS(struct_type, type, member, elements) { \
    offsetof(struct_type, member), MARSHAL_TYPE(type), elements, 1, 0, \
}

/** Type for custom copying callbacks.

    \param dest The address to write content.
    \param src The address from which to read content.
    \param n The maximum number of bytes to copy from src to dest.
    \return Return dest.
    */
typedef void *(*marshal_custom_copy_cb)(void *dest, const void *src, size_t n);

/** Marshal/unmarshal custom copy callbacks */
typedef struct
{
    /**  This callback is applicable when marshalling:
      *  The implementation of this callback shall read the object instance from
      *  src address and write a byte stream representation of the object to the
      *  dest address. At most n bytes shall be written.
      */
    marshal_custom_copy_cb marshal_copy;

    /**  This callback is applicable when unmarshalling:
      *  The implementation of this callback shall read at most n bytes from the
      *  marshal byte stream from src address and write a new object instance to
      *  dest address.
      */
    marshal_custom_copy_cb unmarshal_copy;

} marshal_custom_copy_cbs;


/** The dynamic size type has a tagged union. */
#define MARSHAL_DYNAMIC_TAGGED_UNION 0
/** The dynamic size type has a flexible length array. */
#define MARSHAL_DYNAMIC_ARRAY 1

/** This type describes the properties of a C type.
 *  It is recommended that the MAKE_MARSHAL_TYPE.. macros are used to initialise
 *  instances of this type.
 */
typedef struct marshal_type_descriptor
{
    /** These members can be union-ed since only a type without members may
     *  directly read/write the marshal byte stream. For types with members, the
     *  member's read/write functions are used to write to the byte stream.
     */
    union
    {
        /** Points to a list of members of C struct or union types, NULL for types
            without members. */
        const marshal_member_descriptor_t *members;

        /** Pointer to custom copy callbacks.
         *
         *  The marshal_copy callback is called when marshalling the type to the
         *  marshal byte stream.
         *
         *  The unmarshal_copy callback is called when unmarshalling the type from
         *  the marshal byte stream.
         *
         *  If a callback is NULL, the marshal library will use memcpy to
         *  copy bytes from/to the marshal byte stream.
         *
         *  The callbacks may be used (for example) if a type's bytes needs to
         *  be marshaled with a particular endianness. Most implementations will
         *  not use the custom copy callbacks.
         */
        const marshal_custom_copy_cbs *custom_copy_cbs;

    } u;

    /** The total size of the type in bytes, must always be set to the sizeof
     *  the type. */
    uint8 size;

    /** If the type has members (struct or union), its constituent parts are
        listed in the members list and the members_len is set to a non-zero value.
        If the type is a fundamental type or can be marshalled as a single object,
        the members_len is set to zero and the members list is set to NULL. */
    uint8 members_len;

    /** The type is a union. tagged_union_member will be called on the parent
     *  object to get the index of the active union member (index into the members
     *  field). The indexed member will be marshalled / unmarshalled.
     */
    bool is_union : 1;

    /** The type's length is dynamic. Dynamic length types shall use the
     *  marshal_type_descriptor_dynamic_t which 'inherits' from this base type.
     *
     *  A dynamic length type has a final member that is either:
     *  1) A dynamic length array.
     *      When marshalling a dynamic length array, the marshaller will callback
     *      array_elements() and marshal the current number of elements in the
     *      array. The marshaller writes the active number of array elements
     *      prior to any of the object's values allowing the unmarshaller to
     *      allocate memory for the object of the correct size to accommodate
     *      the current size of the array.
     *
     *      It is assumed a member of the object defines the current length of
     *      the array so that the array_elements callback can determine the
     *      number of elements in the array.
     *
     *  2) A tagged union.
     *      When marshalling a tagged union, the marshaller will callback
     *      tagged_union_member() and marshal the tagged member. The marshaller
     *      writes the tagged union member index prior to any of the object's
     *      values allowing the unmarshaller to allocate memory for the object
     *      of the correct size to accommodate the tagged union member.
     *
     *      It is assumed a member of the object defines the active union member
     *      so that the tagged_union_member callback can determine the tagged
     *      member.
     */
    bool dynamic_length : 1;

    /** For dynamic length objects, set to MARSHAL_DYNAMIC_ARRAY or
     *  MARSHAL_DYNAMIC_TAGGED_UNION */
    unsigned dynamic_type : 1;

} marshal_type_descriptor_t;

/** This type describes the properties of a C type of dynamic length or one that
 *  has a union member.
 *  It is recommended that the MAKE_MARSHAL_TYPE.. macros are used to initialise
 *  instances of this type.
 */
typedef struct marshal_type_descriptor_dynamic
{
    /** The base type descriptor, must be the first member. */
    marshal_type_descriptor_t base;

    /** The implementation of this callback shall return the index of the tagged
     *  (active) union member in its members array.
     *  \param object The address of the parent object containing the union.
     *  \param member The descriptor of the union member.
     *  \param array_element Normally this will be set to 0. If a structure
     *         contains a array of unions, this will be set to the index of the
     *         union being marshalled / unmarshalled.
     *  \return The index of the tagged union member.
    *   \note Implemention of this callback is optional.
     */
    uint32 (*tagged_union_member)(const void *object,
                                  const marshal_member_descriptor_t *member,
                                  uint32 array_element);

    /** The implementation of this callback shall return the number of elements
     *  in a dynamic length array.
     *  \param object The address of the parent object containing the dynamic
     *         array, or pointer to the dynamic length array.
     *  \param member The descriptor of the array member.
     *  \param array_element Normally this will be set to 0. If a structure
     *         contains a array of pointers to dynamic length arrays, this 
     *         will be set to the current index of the array of pointers.
     *  \return The number of array elements.
    *   \note Implemention of this callback is optional.
     */
    uint32 (*array_elements)(const void *object,
                             const marshal_member_descriptor_t *member,
                             uint32 array_element);

} marshal_type_descriptor_dynamic_t;

/** Initialiser for C basic type (#marshal_type_descriptor_t).
 *  \param type The C type name of the type.
 *  \note A basic type has no members.
 */
#define MAKE_MARSHAL_TYPE_DEFINITION_BASIC(type) { \
        {NULL}, sizeof(type), 0, FALSE, FALSE, 0, \
}

/** Initialiser for C union type (#marshal_type_descriptor_t).
 *  \param type The C type name of the union.
 *  \param members Array of the union's members of type
 *         \c marshal_member_descriptor_t.
 */
#define MAKE_MARSHAL_TYPE_DEFINITION_UNION(type, members) { \
        {members}, sizeof(type), ARRAY_DIM(members), TRUE, FALSE, 0, \
}

/** Initialiser for C struct type (#marshal_type_descriptor_t).
 *  \param type The C type name of the structure.
 *  \param members Array of the structure's members of type
 *         \c marshal_member_descriptor_t.
 */
#define MAKE_MARSHAL_TYPE_DEFINITION(type, members) { \
        {members}, sizeof(type), ARRAY_DIM(members), FALSE, FALSE, 0, \
}

/** Initialiser for C struct type containing union member
 *  (#marshal_type_descriptor_dynamic_t).
 *  \param type The C type name of the structure.
 *  \param members Array of the structure's members of type
 *         \c marshal_member_descriptor_t.
 *  \param tagged_union_member_cb Callback returning index of the active tagged
 *         union member.
 *  \note At least one member in the structure must be a union. The union(s)
 *        may be positioned anywhere in the structure. This type is _not_
 *        dynamically resized based on the tagged union member.
 */
#define MAKE_MARSHAL_TYPE_DEFINITION_HAS_UNION(type, members, tagged_union_member_cb) { \
        MAKE_MARSHAL_TYPE_DEFINITION(type, members), \
        tagged_union_member_cb, NULL, \
}

/** Initialiser for C struct type containing dynamic sized union member. The
 *  length of the structure is dynamically sized to contain the tagged union member.
 *  (#marshal_type_descriptor_dynamic_t).
 *  \param type The C type name of the structure.
 *  \param members Array of the structure's members of type
 *         \c marshal_member_descriptor_t.
 *  \param tagged_union_member_cb Callback returning index of the active tagged
 *         union member.
 *  \note At least one member in the structure must be a union. One of the
 *        union(s) must be positioned at the end of the structure. This type is
 *        dynamically resized based on the size of the tagged final union member.
 */
#define MAKE_MARSHAL_TYPE_DEFINITION_HAS_DYNAMIC_UNION(type, members, tagged_union_member_cb) { \
        { {members}, sizeof(type), ARRAY_DIM(members), FALSE, TRUE, MARSHAL_DYNAMIC_TAGGED_UNION }, \
        tagged_union_member_cb, NULL, \
}

/** Initialiser for C struct type containing dynamic length array member. The
 *  length of the structure is dynamically sized to contain the array length.
 *  \param type The C type name of the structure.
 *  \param members Array of the structure's members of type
 *         \c marshal_member_descriptor_t.
 *  \param array_elements_cb Callback returning the length of the dynamic array.
 *  \note The dynamic array must be the final member in the structure. The
 *        structure is dynamically resized based on the array length.
 */
#define MAKE_MARSHAL_TYPE_DEFINITION_HAS_DYNAMIC_ARRAY(type, members, array_elements_cb) { \
        { {members}, sizeof(type), ARRAY_DIM(members), FALSE, TRUE, MARSHAL_DYNAMIC_ARRAY }, \
        NULL, array_elements_cb, \
}

/** Initialiser for C struct type containing pointer(s) to dynamic length array(s).
 *  \param type The C type name of the structure.
 *  \param members Array of the structure's members of type
 *         \c marshal_member_descriptor_t.
 *  \param array_elements_cb Callback returning the length of the dynamic array.
 */
#define MAKE_MARSHAL_TYPE_DEFINITION_HAS_PTR_TO_DYNAMIC_ARRAY(type, members, array_elements_cb) { \
        { {members}, sizeof(type), ARRAY_DIM(members), FALSE, FALSE, 0 }, \
        NULL, array_elements_cb, \
}

#endif
