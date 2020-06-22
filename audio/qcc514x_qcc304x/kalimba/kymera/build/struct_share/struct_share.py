############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2013 - 2018 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
Generates a header file usable by the Kalimba assembler for the structures
and enumerations present in an object file. Undesired structures and enumumerations
from the object file can be ignored by providing them on the command line.
"""
import sys
import os

TYPES = 12
ENUMS = 13
DM_OCTET_ADDRESSING = 17

class ObjectReader(object):
    """Reads a .o for struct and enum definitions, writes to a header file"""

    structs = [] #list containing structs
    enums = {}#each entry key is the enum name, value is a dict of enum values (keyed on name)
    obj_info = []

    def __init__(self, header_file, o_file, struct_filter_list):

        self.header_name = header_file
        self.object_file_path = o_file
        self.struct_filter_list = struct_filter_list

        self.library_name = os.path.split(self.object_file_path)[1].split('.')[0]

        # Import kalelfreader
        if sys.platform.startswith('linux'):
            sys.path.append(os.path.join(os.sep, 'home', 'devtools', 'kal-python-tools', 'linux',
                                         '1.1.6', 'kal_python_tools_linux64_1.1.6'))
        elif sys.platform.startswith('win'):
            sys.path.append(os.path.join(os.environ["DEVKIT_ROOT"], 'tools', 'pythontools'))
        else:
            # Unsupported OS so assert
            raise OSError
        import kalelfreader_lib_wrappers as kerlib


        # Extract from object file
        ker = kerlib.Ker()
        obj_path1 = os.path.normcase(self.object_file_path)
        self.obj_info = ker.loadelf(obj_path1)

        self.read_enum()
        self.read_struct()

        if  not self.enums and not self.structs:
            print("Warning: no enum or struct definitions in ", o_file)
        else:
            self.header_print(self.enums, self.structs)

    def read_struct(self):
        """ Parses out structs from the type list returned by Ker """
        types = self.obj_info[TYPES]
        for t in types: # pylint: disable=C0103
            if t.form == 1:
                mbrs = []
                for j in range(t.member_count):
                    mbrs.append((t.members[j].type_id, t.members[j].name, t.members[j].offset,
                                 t.members[j].bit_offset_from_offset, t.members[j].bit_size))
                num_words = t.size_in_addressable_units
                if self.obj_info[DM_OCTET_ADDRESSING]:
                    num_words = num_words // 4
                elem = (t.form, t.type_id, t.name, num_words, t.ref_type_id, t.member_count,
                        mbrs, t.array_count, t.array_type_id)
                if elem not in self.structs:
                    self.structs.append(elem)

    def read_enum(self):
        """ Parses out enum types from the enum dictionary returned by Ker """
        for enum_type in self.obj_info[ENUMS]:
            enum_group = self.obj_info[ENUMS][enum_type]
            for enum, value in enum_group.items():
                self.enums[enum] = value #add to enum dictionary

    @staticmethod
    def match_enum(enum, filter_list):
        """ Checks if a value from filter_list is in enum  """
        for filter_list_value in filter_list:
            if enum[0:len(filter_list_value)] == filter_list_value:
                return True
        return False

    def header_print(self, enum_dict, struct_list):
        """ Using processed lists header files are written attempting to follow
            the ASM naming convention."""
        #Open headers and write enums.
        with open(self.header_name, 'w') as file_to_write:
            for enum, value in enum_dict.items():
                if self.struct_filter_list == [] or self.match_enum(enum, self.struct_filter_list):
                    file_to_write.write('.CONST\t$' + self.library_name + '.' + enum + '\t' +
                                        str(value) + ';\n')

            # Append structs.
            for struct in struct_list:
                if struct[2]: # if named
                    if (self.struct_filter_list == []) or (struct[2] in self.struct_filter_list):
                        # When only a pointer to a structure is available, the structure appears
                        # to have a size of 0. Do not print it out.
                        if struct[3] != 0:
                            file_to_write.write('// ' + str(struct[2]) + '.struct \n')
                            for member in struct[6]:
                                file_to_write.write('.CONST \t$' + self.library_name +
                                                    '.'+ struct[2] +
                                                    '_struct.' + member[1].upper() + '_FIELD\t' +
                                                    str(member[2]) + ';\n')
                            file_to_write.write('.CONST\t$'+ self.library_name +'.' + struct[2]+
                                                '_struct.STRUC_SIZE \t'+ str(struct[3]) + ';\n')


if __name__ == '__main__':
    # Param 1 : target header file
    # Param 2 : input object file
    _ = ObjectReader(sys.argv[1], sys.argv[2], sys.argv[3:])
