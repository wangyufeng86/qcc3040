'''
 Copyright (c) 2019 Qualcomm Technologies International, Ltd.
    %%version

A script to generate type definitions and descriptors from an xml description of a c structure, union or enumeration.

Description of XML format
------------------------
1. Target file name is provided using "name" attribute in <types> tag.
    3 files are generated using this,
         - <name>_typedef.h: C header file with generated structure, unions and enumerations.
         - <name>_typedef_marshal.h: C header file with marshal type declarations.
         - <name>_typedef_marshal.c: C source file with marshal type definitions.

2. Include files
  Include files are added to xml using <include_header> tag. Name of the header file is specified using "name" attribute. 
 
3. Source code
  Source code can be added as performatted text within <source></source> tags.
  
  Source code can be added to the generated typedef_header file using 
  <typedef_header_source></<typedef_header_source> tags.

  There are no attributes associated with <source> or <typedef_header_source> tag.
  
4. Structure/Union format
  Structure and Union are defined using <typedef_struct> and <typedef_union> tags respectively.
  Members are defined within <member> </member> tags.

  4.1 Attributes for <typedef_struct>/<typedef_union> tags
    - name: name of structure
    - doc: description of structure
    - basic: "true" if this is a BASIC marshaling type, "false" otherwise
    - has: Used if it is not a BASIC type. This can have one of the following values - 
        union/dynamic_union/dynamic_array/ptr_to_dynamic_array
    - tagged_union_member_cb/array_elements_cb: This field should point to a function which will provide information on 
        members in a dynamic type (dynamic_union/dynamic_array/ptr_to_dynamic_array). 
        - For struct types, the callback should return the size of the members.
        - For union types, the callback should return the active union member.
        Callback function's definition can be added using <source></source> tags.

  4.2 Attributes for <members> tag
    - doc: description of member
    - marshal: "true" if member needs to be marshalled, "false" otherwise

5. Enumeration format
  Enumerations are defined using <typedef_enum> tag. 
  
  5.1 Attributes for <typedef_enum> tag
    - doc: description of enumeration

  Members of enumeration are provided as performatted text within <typedef_enum></typedef_enum> tags.

6. Standalone
  If the (optional) standalone attribute is set to "true" in the <types> tag,
  typegen will produce different content in the <name>_marshal_typedef.[ch] files.
  Standalone signifies that the types defined in the typedef file are the complete
  set of types being used with a marshaller instance. Non-standalone signifies
  that the types defined in the typedef file are part of a larger set of types
  being used with a marshaller instance, i.e. the output from multiple typedef files
  are aggregated into a single larger type set.
  When standalone is true, an array of pointers to marshal type descriptors
  is also defined in the <name>_marshal_typedef.c file, and an enum of marshal
  types are declared in the <name>_marshal_typedef.h file with a extern definition
  of the array of pointers to marshal type descriptors.
  Standalone has no impact on the <name>_typedef.h file.

7. Inherit
  A typedef can inherit marshal types from another file. This is of particular use
  for reusing the marshal definitions for common types in another type definition.
  Types can be inherited using the <inherit> tag. The inherit tag is only allowed
  if the standalone attribute is "true". Only one inherit is allowed.

  The following attributes within the inherit tag define the properties of the types
  to inherit:

  header
        Defines the header file that contains the marshal type
        definitions to inherit.

  count
        Defines the number of inherited types (can be a numeral or definition from
        the header file).

  x_macro_table
        Defines the name of the x_macro table of marshalling types in the inherited
        header file. The x_macro table is expected to be in the form:
            #define MARSHAL_TABLE_NAME(ENTRY) ENTRY(type1_t) ENTRY(type2_t) /* etc */
        Inherit expects marshal_type_descriptor_t named (for example)
        marshal_type_descriptor_type1_t, marshal_type_descriptor_type2_t for each
        inherited type.

    Example of how to inherit common marshal types:
    <inherit header="marshal_common.h" count="NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES" x_macro_table="MARSHAL_COMMON_TYPES_TABLE"/>


Note - Source code which is added as text (i.e. within <source>,<typedef_header_source> or <typedef_enum> tags) need to escape special characters
       '>', '<' as these cannot be parsed by XML parser. For example, '<' need to be replaced with '&lt;' in source.
  
Example .typedef file:
----------------------
<?xml version="1.0" encoding="UTF-8"?>
<!--Copyright (c) 2019 Qualcomm Technologies International, Ltd.-->

<types name="test">

    <include_header name="marshal_common.h"/>

    <source>
/* Source code can be added in source tags */

/* This is a static callback function */
static uint32 tagged_union_member_cb(const void *object,
                                     const marshal_member_descriptor_t *member,
                                     uint32 array_element)
{
    /* Implementation specific handling */
    UNUSED(object);
    UNUSED(member);
    UNUSED(array_element);
    return 0;
}
    </source>

    <typedef_union name="test_union_t" doc="A test union">
        <member marshal="true" doc="Member">uint8 x</member>
        <member marshal="true" doc="Pointer member">uint8 *y</member>
    </typedef_union>

    <typedef_struct name="test_struct_t" has="union" tagged_union_member_cb="tagged_union_member_cb" doc="A test struct">
        <member marshal="true" doc="Member">uint8 a</member>
        <member marshal="false" doc="Pointer member that is not marshalled">uint8 *b</member>
        <member marshal="true" doc="Array member">uint16 c[2]</member>
        <member marshal="true" doc="Array of pointers">uint16 *d[3]</member>
        <member marshal="true" doc="Union member">test_union_t e</member>
    </typedef_struct>

    <typedef_enum name="test_enum" doc="A test enumeration">
    /*! Description for TEST_FLAG_A*/
    TEST_FLAG_A = (1 &lt;&lt; 0),
    /*! Description for TEST_FLAG_B */
    TEST_FLAG_B = (1 &lt;&lt; 1),
    /*! Description for TEST_FLAG_C */
    TEST_FLAG_C = (1 &lt;&lt; 2)
    </typedef_enum>
</types>
'''

import sys
import os
import xml.etree.ElementTree as ET
import re

file_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(file_dir)
sys.path.insert(0, os.path.join(parent_dir, "codegen"))
from c_codegen import CommentBlock, CommentBlockDoxygen, HeaderGuards, Enumeration, Array, StructTypedef, UnionTypedef, Include, Indented, EnumerationTypedef

class TypesGenerator(object):
    ''' Type generator class '''
    def __init__(self, element_tree_root):
        self._root = element_tree_root
        self._types_name = self._root.attrib['name'].lower()
        standalone = self._root.get('standalone')
        if standalone:
            self._standalone = standalone.lower() == "true"
        else:
            self._standalone = False
        self._regex = re.compile(r"([a-zA-Z0-9_]+)\s*(\*)*\s*([a-zA-Z0-9_]+)\s*(\[[a-zA-Z0-9_]+\])?")

        self._validate_inherit()

    def _validate_inherit(self):
        ''' Validate inherit tag usage '''
        inherit_count = len(self._root.findall('inherit'))

        if inherit_count > 1:
            raise Exception("Only one inherit tag is allowed")

        if inherit_count and not self._standalone:
            raise Exception("Inheritance is only allowed in standalone mode")

    def _generate_dox_header(self, file_type=""):
        ''' Generate doxygen file header comment '''
        with CommentBlockDoxygen() as cbd:
            cbd.doxy_copyright()
            cbd.doxy_version()
            cbd.doxy_filename("")
            cbd.doxy_brief("The {} {}. This file is generated by {}.".format(self._types_name, file_type, __file__))

    def _member_descriptor(self, parent_type, member):
        ''' Return the member's descriptor macro, as defined in marshal_if.h '''
        groups = self._regex.match(member.text).groups()
        member_type = groups[0]
        member_name = groups[2]
        pointer = groups[1] is not None
        array = groups[3] is not None
        elements = groups[3].split('[')[1].split(']')[0] if array else 0

        if not array and not pointer:
            decl = "MAKE_MARSHAL_MEMBER({}, {}, {})".format(parent_type, member_type, member_name)
        elif array and not pointer:
            decl = "MAKE_MARSHAL_MEMBER_ARRAY({}, {}, {}, {})".format(parent_type, member_type, member_name, elements)
        elif not array and pointer:
            decl = "MAKE_MARSHAL_MEMBER_POINTER({}, {}, {})".format(parent_type, member_type, member_name)
        else:
            decl = "MAKE_MARSHAL_MEMBER_ARRAY_OF_POINTERS({}, {}, {}, {})".format(parent_type, member_type, member_name, elements)

        return decl

    def _type_descriptor(self, t):
        ''' Return the type's descriptor macro, as defined in marshal_if.h '''
        name = t.get('name')
        t.attrib['members'] = "{}_member_descriptors".format(name)
        descriptor = "MAKE_MARSHAL_TYPE_DEFINITION"
        has = t.get('has')
        basic = t.get('basic')

        if basic == "true":
            descriptor += "_BASIC({name})"
            is_dynamic = False
        elif t.tag == "typedef_union":
            descriptor += "_UNION({name}, {members})"
            is_dynamic = False
        elif has == "union":
            descriptor += "_HAS_UNION({name}, {members}, {tagged_union_member_cb})"
            is_dynamic = True
        elif has == "dynamic_union":
            descriptor += "_HAS_DYNAMIC_UNION({name}, {members}, {tagged_union_member_cb})"
            is_dynamic = True
        elif has == "dynamic_array":
            descriptor += "_HAS_DYNAMIC_ARRAY({name}, {members}, {array_elements_cb})"
            is_dynamic = True
        elif has == "ptr_to_dynamic_array":
            descriptor += "_HAS_PTR_TO_DYNAMIC_ARRAY({name}, {members}, {array_elements_cb})"
            is_dynamic = True
        else:
            descriptor += "({name}, {members})"
            is_dynamic = False

        return descriptor.format(**t.attrib), is_dynamic

    def _generate_type_definitions(self):
        ''' Generate C type defintions for each type defined in the root '''
        for t in self._root:
            if t.tag == "typedef_struct":
                classtype = StructTypedef
            elif t.tag == "typedef_union":
                classtype = UnionTypedef
            elif t.tag == "typedef_enum":
                classtype = EnumerationTypedef
            elif t.tag == "source" or t.tag == "include_header" or t.tag == "inherit" or t.tag == 'typedef_header_source':
                #  Tag doesn't contain type information
                continue
            else:
                assert(False)

            name = t.get('name')

            with classtype(name, doc=t.get('doc')) as ct:
                if t.tag == "typedef_enum":
                    print(Indented.indent() + (t.text).strip())
                else:
                    members = self._root.findall("./{}[@name='{}']/member".format(t.tag, name))
                    for m in members:
                        ct.extend_with_doc([m.text], [m.get('doc')])

    def _generate_source(self):
        ''' Print source tags from the root '''
        for t in self._root:
            if t.tag in ["source"]:
                print(t.text.strip()+'\n')

    def _generate_typedef_header_source(self):
        ''' Print text in typedef_header_source tags from the root '''
        for t in self._root:
            if t.tag in ["typedef_header_source"]:
                print(t.text.strip()+'\n')

    def _generate_marshal_member_descriptor_definitions(self):
        ''' Generate marshal member descriptor definitions '''
        for t in self._root:
            if t.tag in ["typedef_struct", "typedef_union"]:
                name = t.get('name')
                # Basic types are marshalled as a binary 'blob', their members are not described.
                if t.get('basic') != "true":
                    desc_name = "{}_member_descriptors".format(name)
                    with Array("static const marshal_member_descriptor_t", desc_name, t.get('doc')) as arr:
                        members = self._root.findall("./{}[@name='{}']/member".format(t.tag, name))
                        for m in members:
                            is_marshalled = m.get('marshal')
                            # The XML definition of the type _must_ define if the member if marshalled or not.
                            assert(is_marshalled in ['true', 'false']), "{}.{} did not set marshal attribute to 'true' or 'false'".format(name, m.text)
                            if is_marshalled != "false":
                                arr.extend_with_doc([self._member_descriptor(name, m)], [m.get('doc')])

    def _generate_marshal_type_descriptor_definitions(self):
        ''' Generate marshal type descriptor definitions '''
        for t in self._root:
            if t.tag in ["typedef_struct", "typedef_union", "typedef_enum"]:
                name = t.get('name')
                descriptor, is_dynamic = self._type_descriptor(t)
                dynamic = "_dynamic" if is_dynamic else ""
                desc_name = "const marshal_type_descriptor{}_t marshal_type_descriptor_{}".format(dynamic, name)
                print(desc_name + " = " + descriptor + ";")
        print("\n")

    def _generate_marshal_type_descriptor_declarations(self):
        ''' Generate marshal member descriptor declarations '''
        for t in self._root:
            if t.tag in ["typedef_struct", "typedef_union", "typedef_enum"]:
                name = t.get('name')
                _, is_dynamic  = self._type_descriptor(t)
                dynamic = "_dynamic" if is_dynamic else ""
                print("extern const marshal_type_descriptor{}_t marshal_type_descriptor_{};".format(dynamic, name))

    def _types_table_name(self):
        ''' Return the name of the marshal types table '''
        return self._types_name.upper() + "_MARSHAL_TYPES_TABLE"

    def _generate_marshal_type_table(self):
        ''' Generate table of marshal types declarations '''
        print("\n#define " + self._types_table_name() + "(ENTRY)"),
        for t in self._root:
            if t.tag in ["typedef_struct", "typedef_union", "typedef_enum"]:
                name = t.get('name')
                print ("\\")
                with Indented():
                    print(Indented.indent() + "ENTRY(" + name + ")"),
        print("\n")

    def _generate_marshal_type_enum(self):
        ''' Generate enum of marshal types (standalone) '''
        print("#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),")
        with Enumeration(self._types_name.upper() + "_MARSHAL_TYPES", eol="") as enum:
            for t in self._root:
                if t.tag == "inherit":
                    enum.extend(["DUMMY = ({}-1),".format(t.get('count'))])
            enum.extend([self._types_table_name() + "(EXPAND_AS_ENUMERATION)"])
            enum.extend(['NUMBER_OF_{}_MARSHAL_TYPES'.format(self._types_name.upper())])
        print("#undef EXPAND_AS_ENUMERATION\n")

    def _generate_marshal_type_descriptors_array(self):
        declaration = "(const marshal_type_descriptor_t *)&marshal_type_descriptor_{}"
        print("#define EXPAND_AS_TYPE_DEFINITION(type) " + declaration.format("##type,"))
        with Array("const marshal_type_descriptor_t * const", self._types_name + "_marshal_type_descriptors", eol="") as arr:
            for t in self._root:
                if t.tag == "inherit":
                    arr.extend(["{}(EXPAND_AS_TYPE_DEFINITION)".format(t.get('x_macro_table'))])
            arr.extend([self._types_table_name() + "(EXPAND_AS_TYPE_DEFINITION)"])
        print("#undef EXPAND_AS_TYPE_DEFINITION\n")

    def _print_headers(self, extra=None):
        ''' Add headers required in all files then add xml defined and extras '''
        headers = ["csrtypes.h"]
        headers.extend([header.attrib['name'] for header in self._root.findall("./include_header")])
        if extra:
            headers.extend(extra)
        for t in self._root:
            if t.tag == "inherit":
                headers.extend([t.get('header')])
        Include(headers)

    def generate_typedef_header(self):
        ''' Generate a header file defining the C types in the root '''
        self._generate_dox_header("c type definitions")
        with HeaderGuards(self._types_name + "_TYPEDEF"):
            self._print_headers()
            self._generate_typedef_header_source()
            self._generate_type_definitions()

    def generate_marshal_header(self):
        ''' Generate a header file declaring the marshal types in the root '''
        self._generate_dox_header("marshal type declarations")
        with HeaderGuards(self._types_name + "_MARSHAL_TYPEDEF"):
            self._print_headers(["app/marshal/marshal_if.h"])
            if self._standalone:
                self._generate_marshal_type_table()
                self._generate_marshal_type_enum()
                print("extern const marshal_type_descriptor_t * const {}_marshal_type_descriptors[];".format(self._types_name))
            else:
                self._generate_marshal_type_descriptor_declarations()
                self._generate_marshal_type_table()

    def generate_marshal_source(self):
        ''' Generate a source file defining the marshal types in the root '''
        self._generate_dox_header("marshal type definitions")
        self._print_headers(["app/marshal/marshal_if.h",
                             self._types_name + "_typedef.h",
                             self._types_name + "_marshal_typedef.h"])
        self._generate_source()
        self._generate_marshal_member_descriptor_definitions()
        self._generate_marshal_type_descriptor_definitions()
        if self._standalone:
            self._generate_marshal_type_descriptors_array()

def main():

    import argparse
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('filename', type=str, help='The name of the xml file containing the description of the type(s)')
    parser.add_argument('--typedef_header', action='store_true', help='Generate a C type definition of the type(s)')
    parser.add_argument('--marshal_header', action='store_true', help='Generate a C marshal type declaration of the type(s)')
    parser.add_argument('--marshal_source', action='store_true', help='Generate a C marshal type definition of the type(s)')
    args = parser.parse_args()

    try:
        tree = ET.parse(args.filename)
    except IOError:
        print >> stderr, '*** ERROR -- Failed to parse [{0}]'.format(args.filename)
        return False
    element_tree_root = tree.getroot()

    tg = TypesGenerator(element_tree_root)

    # Generate the typedef header
    if args.typedef_header:
        tg.generate_typedef_header()

    # Generate the marshal typedef header
    if args.marshal_header:
        tg.generate_marshal_header()

    # Generate the marshal source
    if args.marshal_source:
        tg.generate_marshal_source()

    return True

if __name__ == "__main__":
    if main() is True:
        exit(0)
    else:
        exit(1)
