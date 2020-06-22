'''
 Copyright (c) 2017-2019 Qualcomm Technologies International, Ltd.
'''

# pylint: disable=missing-docstring

from __future__ import print_function

import sys
import os
import xml.etree.ElementTree as ET
import errno

ME = os.path.abspath(__file__)
MEDIR = os.path.dirname(ME)
PARENT = os.path.abspath(os.path.join(MEDIR, os.pardir))
GPARENT = os.path.abspath(os.path.join(PARENT, os.pardir))
sys.path.insert(0, GPARENT)
sys.path.insert(0, PARENT)

# pylint: disable=import-error,wrong-import-position
from codegen.c_codegen import CommentBlockDoxygen, HeaderGuards, Enumeration, Array
from plant_uml_chain_diagram import PlantUmlChainDiagram

__all__ = ["process_file"]


class ChainTerminalException(Exception):
    '''Exception class indicating a problem with a named terminal'''
    pass


class ChainGenerator(object):  # pylint: disable=too-many-instance-attributes
    ''' Base chain generator class '''
    def __init__(self, element_tree_root, outfile=None):
        self.chain = element_tree_root
        self.chain_name = self.chain.attrib['name'].lower()
        self.chain_id = self.chain.attrib['id']
        self.operators = self.chain.findall("./operator")
        self.inputs = self.chain.findall("./input")
        self.outputs = self.chain.findall("./output")
        self.connections = self.chain.findall("./connection")
        self.configurations = self.chain.findall("./configuration")
        self.exclude_from_configure_sample_rate = [op.attrib['name'] for op in self.operators if op.get('set_sample_rate') == "false"]  # pylint: disable=invalid-name
        self.include_headers = self.chain.findall("./include_header")
        self.outfile = outfile
        try:
            self.generate_operator_roles_enum = (self.chain.attrib['generate_operator_roles_enum'] != 'False')
        except KeyError:
            self.generate_operator_roles_enum = True
        try:
            self.generate_endpoint_roles_enum = (self.chain.attrib['generate_endpoint_roles_enum'] != 'False')
        except KeyError:
            self.generate_endpoint_roles_enum = True
        try:
            self.chain_ucid = self.chain.attrib['ucid']
        except KeyError:
            self.chain_ucid = 0

        try:
            self.default_priority = self.chain.attrib['default_priority'].upper()
        except KeyError:
            self.default_priority = "DEFAULT"

    def terminal_num(self, operator, terminal, for_sink=True):
        ''' Given operator and terminal names:
            if for_sink is True, returns the sink terminal number, else the source terminal number. '''
        findstring = "./operator[@name='{}']/{}[@name='{}']"
        sink = self.chain.findall(findstring.format(operator, 'sink' if for_sink else 'source', terminal))
        if len(sink) != 1:
            mode = "sink" if for_sink else "source"
            msg = "'%s.%s' (%s). %d definitions found. Expected exactly 1" % (operator, terminal, mode, len(sink))
            raise ChainTerminalException(msg)

        return sink[0].attrib['terminal']

    def metadata(self, endpoint_item, for_sink=True):
        ''' Given an endpoint and whether it is a sink/source, extract the
            operator and terminal information and include the role information
            (possibly pre-defined in file).
            '''
        source_sink = endpoint_item.attrib["sink" if for_sink else "source"]
        op_term = source_sink.split('.')
        assert len(op_term) == 2
        metadata = {}
        metadata['operator'] = op_term[0]
        metadata['terminal'] = op_term[1]
        metadata['terminal_num'] = self.terminal_num(op_term[0], op_term[1], for_sink)
        try:
            metadata['role'] = endpoint_item.attrib['role']
        except KeyError:
            metadata['role'] = "{operator}_{terminal}".format(**metadata)
        return metadata

    def filename(self, file_extension):
        ''' The filename associated with this chain '''
        return "{}.{}".format(self.chain_name, file_extension)

    @staticmethod
    def opmsg_name(msgid, operator, config):
        ''' The opmsg name '''
        return "{}_{}_{}".format(msgid, operator, config)

    def opmsgs_config_name(self, config):
        ''' The name of the opmsgs config '''
        return "{}_opmsgs_config_{}".format(self.chain_name, config)

    def exclude_from_configure_sample_rate_array_name(self):  # pylint: disable=invalid-name
        ''' The name of the array '''
        return "{}_exclude_from_configure_sample_rate".format(self.chain_name)


class ChainConfigurationGenerator(ChainGenerator):
    ''' Given a xml definition of a chain,
        generates the C code to define the configuration of a chain '''

    def endpoint_enum_line(self, endpoint_item):
        ''' Generate the string for a single element of the endpoint enumeration '''
        metadata = self.metadata(endpoint_item, 'sink' in endpoint_item)
        return metadata['role']

    def operators_array_line(self, op_item):
        ''' Generate the string for a single element of the operators array

            Supports requests for priority and processor, forming the macro
            MAKE_OPERATOR_CONFIG[_<processor>][_PRIORITY_<priority>]
        '''
        requested_priority = op_item.get('priority', self.default_priority).upper()
        priority_string = "_PRIORITY_{}".format(requested_priority) if requested_priority != "DEFAULT" else ""

        requested_processor = "_" + op_item.get('processor', "P0").upper()
        processor_string = requested_processor if requested_processor != "_P0" else ""

        return "MAKE_OPERATOR_CONFIG{}{}({}, {})".format(processor_string,
                                                         priority_string,
                                                         op_item.attrib['id'],
                                                         op_item.attrib['name'])

    def inputs_array_line(self, input_item):
        ''' Generate the string for a single element of the inputs array '''
        metadata = self.metadata(input_item)
        return "{{{operator}, {role}, {terminal_num}}}".format(**metadata)

    def outputs_array_line(self, output_item):
        ''' Generate the string for a single element of the outputs array '''
        metadata = self.metadata(output_item, False)
        return "{{{operator}, {role}, {terminal_num}}}".format(**metadata)

    def connections_array_line(self, connection_item):
        ''' Generate the string for a single element of the connections array '''
        source_metadata = self.metadata(connection_item, False)
        sink_metadata = self.metadata(connection_item)
        str_a = '{{{operator}, {terminal_num}, '.format(**source_metadata)
        str_b = "{operator}, {terminal_num}, 1}}".format(**sink_metadata)
        return str_a + str_b

    @staticmethod
    def opmsgs_array_line(op_id_dict):
        ''' Generate the string for a single element of a opmsgs array '''
        return "{{{op_name}, {msg_name}, ARRAY_DIM({msg_name})}}".format(**op_id_dict)

    def generate(self, source=True):  # pylint: disable=too-many-branches,too-many-locals
        ''' Generate chain definitions '''
        if source:
            # Generate the C chain definition for the .c file
            headers = [self.filename('h'), "cap_id_prim.h", "opmsg_prim.h", "hydra_macros.h"]
            headers.extend([header.attrib['name'] for header in self.include_headers])

            for header in headers:
                print("#include <{}>".format(header), file=self.outfile)

            if self.operators:
                with Array("static const operator_config_t", "operators") as array:
                    array.extend([self.operators_array_line(operator) for operator in self.operators])  # pylint: disable=no-member

            if self.inputs:
                with Array("static const operator_endpoint_t", "inputs") as array:
                    array.extend([self.inputs_array_line(inp) for inp in self.inputs])

            if self.outputs:
                with Array("static const operator_endpoint_t", "outputs") as array:
                    array.extend([self.outputs_array_line(output) for output in self.outputs])

            if self.connections:
                with Array("static const operator_connection_t", "connections") as array:
                    array.extend([self.connections_array_line(connection) for connection in self.connections])

            if self.exclude_from_configure_sample_rate:
                with Array("const unsigned", self.exclude_from_configure_sample_rate_array_name()) as array:
                    array.extend(self.exclude_from_configure_sample_rate)

            if self.configurations:
                for configuration in self.configurations:
                    config_name = configuration.attrib['name'].lower()
                    opmsgs = []
                    for opmsg in list(configuration):
                        assert opmsg.tag == "opmsg"
                        op_name = opmsg.attrib['op']
                        msgid = opmsg.attrib['id']
                        message_name = self.opmsg_name(msgid.lower(), op_name.lower(), config_name)
                        opmsgs.append({'op_name': op_name, 'msg_name': message_name})
                        with Array("static const uint16", message_name) as array:
                            array.extend([opmsg.attrib['id']])
                            msg_data = opmsg.get('msg')
                            if msg_data is not None:
                                array.extend(msg_data.split(','))
                    with Array("const chain_operator_message_t", self.opmsgs_config_name(config_name)) as array:
                        array.extend([self.opmsgs_array_line(opmsg) for opmsg in opmsgs])

            config_str = "const chain_config_t {}_config = {{{}, {}, {}, {}, {}, {}, {}, {}, {}, {}}};\n"
            print(config_str.format(self.chain_name,
                                    self.chain_id,
                                    self.chain_ucid,
                                    'operators' if self.operators else 'NULL',
                                    len(self.operators),
                                    'inputs' if self.inputs else 'NULL',
                                    len(self.inputs),
                                    'outputs' if self.outputs else 'NULL',
                                    len(self.outputs),
                                    'connections' if self.connections else 'NULL',
                                    len(self.connections)), file=self.outfile)
        else:
            # Generate the C chain declarations for the .h file
            print("#include <chain.h>\n", file=self.outfile)

            if self.generate_operator_roles_enum:
                with Enumeration("{}_operators".format(self.chain_name)) as enum:
                    enum.extend([op.attrib['name'] for op in self.operators])  # pylint: disable=no-member

            if self.generate_endpoint_roles_enum:
                with Enumeration("{}_endpoints".format(self.chain_name)) as enum:
                    enum.extend([self.endpoint_enum_line(endpoint) for endpoint in self.inputs + self.outputs])

            for configuration in self.configurations:
                config_name = configuration.attrib['name'].lower()
                print("extern const chain_operator_message_t {}[{}];".format(self.opmsgs_config_name(config_name),
                                                                             len(list(configuration))), file=self.outfile)

            if self.exclude_from_configure_sample_rate:
                print("extern const unsigned {}[{}];".format(self.exclude_from_configure_sample_rate_array_name(),
                                                             len(self.exclude_from_configure_sample_rate)), file=self.outfile)

            print("extern const chain_config_t {}_config;\n".format(self.chain_name), file=self.outfile)


class ChainPlantUmlDiagramGenerator(ChainGenerator):
    ''' Generate plant UML diagram '''

    def generate(self, _outfile=None):
        ''' Generate the plant UML chain diagram '''
        with CommentBlockDoxygen(outfile=self.outfile) as cbd:
            cbd.doxy_page(self.chain_name)  # pylint: disable=no-member
            with PlantUmlChainDiagram(outfile=self.outfile) as puml:
                for operator in self.operators:
                    puml.object(operator.attrib['name'], operator.attrib['id'])
                for connection in self.connections:
                    source_metadata = self.metadata(connection, False)
                    sink_metadata = self.metadata(connection)
                    puml.wire(source_metadata, sink_metadata)
                for inpt in self.inputs:
                    sink_metadata = self.metadata(inpt)
                    puml.input(sink_metadata)
                for outpt in self.outputs:
                    source_metadata = self.metadata(outpt, False)
                    puml.output(source_metadata)


class DoxygenGenerator(ChainGenerator):
    ''' Generate doxygen file header comment '''
    def generate(self, file_extension):
        with CommentBlockDoxygen(outfile=self.outfile) as cbd:
            # pylint: disable=no-member
            cbd.doxy_copyright()
            cbd.doxy_filename(self.filename(file_extension))
            cbd.doxy_brief("The {} chain. This file is generated by {}.".format(self.chain_name, __file__))


def create_header(element_tree_root, file_handle):
    doxygen = DoxygenGenerator(element_tree_root, file_handle)
    doxygen.generate('h')
    with HeaderGuards(doxygen.chain_name, file_handle):
        try:
            ChainPlantUmlDiagramGenerator(element_tree_root).generate()
            ChainConfigurationGenerator(element_tree_root, file_handle).generate(False)
        except ChainTerminalException as err:
            print(err, file=sys.stderr)
            exit(2)


def generate_header(element_tree_root, write_to_file, filename, output_folder):
    output_file = _get_out_file(write_to_file, filename, 'h', output_folder)
    file_handle = None
    # outfile = None
    if output_file:
        with open(output_file, 'w') as file_handle:
            print('Generating: {}'.format(output_file))
            create_header(element_tree_root, file_handle)
    else:
        create_header(element_tree_root, file_handle)


def create_source(element_tree_root, file_handle):
    DoxygenGenerator(element_tree_root, file_handle).generate('c')
    try:
        ChainConfigurationGenerator(element_tree_root, file_handle).generate()
    except ChainTerminalException as err:
        print(err, file=sys.stderr)
        exit(2)


def generate_source(element_tree_root, write_to_file, filename, output_folder):
    output_file = _get_out_file(write_to_file, filename, 'c', output_folder)
    file_handle = None
    if output_file:
        with open(output_file, 'w') as file_handle:
            print('Generating: {}'.format(output_file))
            create_source(element_tree_root, file_handle)
    else:
        create_source(element_tree_root, file_handle)


def _create_folders(folder):
    try:
        os.makedirs(folder)
    except OSError as err:
        if err.errno != errno.EEXIST:
            raise


def create_uml(element_tree_root, file_handle):
    try:
        ChainPlantUmlDiagramGenerator(element_tree_root, file_handle).generate()
    except ChainTerminalException as err:
        print(err, file=sys.stderr)
        exit(2)


def generate_uml(element_tree_root, write_to_file, filename, output_folder):
    output_file = _get_out_file(write_to_file, filename, 'uml', output_folder)
    file_handle = None
    if output_file:
        with open(output_file, 'w') as file_handle:
            print('Generating: {}'.format(output_file))
            create_uml(element_tree_root, file_handle)
    else:
        create_uml(element_tree_root, None)


def _get_out_file(write_to_file, filename, ext, output_folder=None):
    '''
    If writing to file, return the output file name
    Otherwise return None
    '''
    if write_to_file:
        if output_folder is None:
            out_path = os.path.dirname(os.path.abspath(filename))
        else:
            if os.path.isabs(output_folder):
                out_path = output_folder
            else:
                out_path = os.path.join(os.path.dirname(os.path.abspath(filename)), output_folder)
            _create_folders(out_path)
        filename, _ = os.path.splitext(os.path.basename(filename))
        filename = '.'.join([filename, ext])
        return os.path.join(out_path, filename)
    else:
        return None


def _get_element_tree(filename):
    try:
        tree = ET.parse(filename)
    except IOError:
        print('ERROR: Failed to parse [{0}]'.format(filename), file=sys.stderr)
        return None
    return tree.getroot()


def process_file(header, source, uml, write_to_file, filename, output_folder):  # pylint: disable=too-many-arguments
    element_tree_root = _get_element_tree(filename)

    # Generate the header file
    if header:
        generate_header(element_tree_root, write_to_file, filename, output_folder)

    # Generate the source file
    if source:
        generate_source(element_tree_root, write_to_file, filename, output_folder)

    # Generate the source file
    if uml:
        generate_uml(element_tree_root, write_to_file, filename, output_folder)
