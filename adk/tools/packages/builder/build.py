"""
%%fullcopyright(2019)
%%version
Provides the command line interface for the build utility
"""
# Python 2 and 3
from __future__ import print_function

import collections
import os
import pprint
import sys
import time
import glob
import shutil
import tempfile
import platform
import re
import random
import exceptions

import multiprocessing as MP
import subprocess

try:
    import maker.exceptions as bdex
except ImportError:
    class FakeBdex(object):
        def __init__(self):
            self.LOGGER_INSTANCE = 'logger'

        @staticmethod
        def Logger(dummy):
            pass

        @staticmethod
        def raise_bd_err(msg, *args, **kwargs):
            raise RuntimeError(msg, *args, **kwargs)

        @staticmethod
        def log_buildproc_output(type, attribs, line=''):
            print(line)

        @staticmethod
        def trace(line):
            print(line)

    bdex = FakeBdex()

from workspace_parse.workspace import Workspace
from workspace_parse.project import Project
from workspace_parse import ubuild_compat
from builder.adk_toolkit import AdkToolkit
import quickChargeConfig.quickChargeHexGen


class ADKToolsNotFoundError(Exception):
    def __init__(self, working_path):
        self.woring_path = working_path
        msg = 'Unable to locate an adk/tools directory within {}.'.format(working_path)
        super(ADKToolsNotFoundError, self).__init__(msg)


def get_make_jobs():
    """ Get number of make jobs to use for the build

        Some older laptops with just 2 real cores can literally freeze when make uses too many jobs.
        Empirical testing shows that using half of the "cpu_count" (which includes virtual cores)
        keeps the older laptops usable while building without adding significant time to the build
        On newer faster laptops "cpu_count - 1" keeps the builds faster and the laptop responsive
    """
    cpu_count = MP.cpu_count()
    if cpu_count == 1:
        return 1
    elif cpu_count <= 4:
        return MP.cpu_count() // 2
    else:
        return MP.cpu_count() - 1


class ProcLauncher(object):
    """ Launches the subordinate process with environment and command line arguments. """
    def __init__(self, cmd=None, env_vars=None, cwd=None, output_pipe=None):
        '''
        NB: On Windows, in order to run a side-by-side assembly the specified env
        must include a valid SystemRoot.
        '''
        self.output_pipe = output_pipe

        self.proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=env_vars,
            cwd=cwd,
            universal_newlines=True,
            startupinfo=None,
            creationflags=0)

    def run(self):
        """ capture the child's STDOUT and wait for it to finish. Return the
        exit status of the child """
        try:
            line = self.proc.stdout.readline()
            while line != '':
                self.output_pipe.send(line)
                line = self.proc.stdout.readline()

            self.output_pipe.close()
            self.proc.wait()
            return self.proc.returncode
        except BaseException as excep:
            print("Hit exception {type} val = {val}".format(type(excep), excep))


def quote_list_items(list_of_strings):
    """ Takes a list of strings and add double quote to any item that
    contains spaces.
    """
    return ["\""+s+"\"" if ' ' in s else s for s in list_of_strings]


def launch_cmd(cmd_line, env, tgt_config, working_dir, elf_path, is_audio=False):
    """ Spawns a separate process to launch the build and wait for it to end.
    The stdout of the build is read here until the pipe is closed
    This process then waits for the build process to terminate
    """
    attribs = collections.OrderedDict([
        ('type', 'info'),
        ('config', 'tgt_config'),
        ('core', 'audio/p0' if is_audio else 'app/p1'),
        ('module', 'build')
    ])

    bdex.log_buildproc_output('buildstart', attribs)

    sys.stdout.flush()
    sys.stdout.write('>>> [%s]\n' % ' '.join(quote_list_items(cmd_line)))

    process = subprocess.Popen(cmd_line, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=working_dir)
    while True:
        output = process.stdout.readline()
        if not output and process.poll() is not None:
            break
        if output:
            bdex.log_buildproc_output('buildoutput', attribs, output.strip())
    exitcode = process.poll()

    bdex.log_buildproc_output('elfpath', attribs, elf_path)

    attribs['exit_status'] = exitcode
    bdex.log_buildproc_output('buildend', attribs)
    return exitcode == 0

def launch_simple_cmd(cmd_lines, env=None, cwd=None):
    if env is None:
        env = os.environ.copy()
    if cwd is None:
        cwd = os.getcwd()

    return launch_cmd(cmd_lines, env, None, cwd, None)

def print_section(section, indent=0, title=None):
    """ Print the contents of a section of a devkit file using the specified
        indentation and title.
    """
    if title != None:
        print("%s%s" % (' ' * indent, ''.join(['-', title, '-'])))

    indent += 2
    if isinstance(section, collections.defaultdict):
        mwid = len(max(section, key=len))
        for key, val in section.items():
            print("%s%s:%s" % (' ' * indent, key.ljust(mwid), val))
    elif isinstance(section, dict):
        # standard pretty print knows how to make a decent job
        # of vanilla dictionaries.
        pprint.pprint(section)
    elif isinstance(section, list):
        for i in section:
            print("%s%s" % (' ' * indent, i))
    elif isinstance(section, str):
        print("%s%s" % (' ' * indent, section))
    else:
        print("UNKNOWN TYPE", type(section))
        print("VAL", section)
        sys.exit(-1)

def format_props_for_print(props):
    print("PROPS")
    for k in sorted(props.keys()):
        val = props[k]
        if "DEFS" in val:
            # print all the DEFS in alphabetical order
            indent = "DEFS="
            x = val.replace(indent,"")
            words = x.split()
            for w in sorted(words):
                print("{}{}".format(indent,w))
                indent="\t"
            continue
        if len(val) > 80:
            # Fold any other long lines (like INCPATHS) at spaces
            val = val.replace(" ","\n\t")
        print("{}={}".format(k,val))

def print_info(p_inst, dk_inst, script_args):
    """ print summary of the project and devkit file contents """
    print("================================")
    print("PROJECT INFO ")
    print("================================")
    print("  project_file = %s" % script_args.project_file)
    print("   devkit_root = %s" % script_args.devkit_root)
    print(" configuration = %s" % script_args.configuration)

    print_section(p_inst.get_properties(), title="PROJECT PROPERTIES")

    print("\nSOURCE FILES")
    for src_file in p_inst.get_source_files():
        print(src_file)

    config_list = p_inst.get_configurations()
    for config in config_list:
        print("\nCONFIG NAME =", config)
        props = p_inst.get_properties_from_config(config)
        format_props_for_print(props)

    print("------------------------------------")
    print("DEVKIT INFO ")
    print("------------------------------------")

    print("  Param -d = ", script_args.devkit_root)
    print("  Script = ", sys.argv[0])

    print_section(dk_inst.get_identity(), title="KITIDENT", indent=0)

    level = 0
    print_section(dk_inst.get_top_elements(), title="TOP LEVEL ELEMENTS", indent=0)

    level += 4
    print_section(dk_inst.get_commands(), title="COMMANDS", indent=level)

    print_section(dk_inst.get_plugins(), title="TOOLS", indent=0)

def construct_makefile_frag(proj, per_config_depend, config_name, build_output_folder=None, is_audio=False, capability=None):
    """ Creates a makefile stub suitable for the subordinate build system """
    source_files = proj.files
    make_vars = proj.get_properties(config_name)
    proj_name = proj.name
    if is_audio:
        makefile_name = make_vars["MAKEFILE_TO_CALL"]
        cap_path = os.path.abspath(os.path.join(proj.dirname, "..", "..", "capabilities", capability))
        makefile_path = os.path.join(cap_path, makefile_name)
        cap_prop_list = proj.get_cap_props(config_name, capability)
    else:
        makefile_name = '%s.%s.mak' % (proj.name, config_name.lower())
        makefile_path = os.path.join(proj.dirname, makefile_name)

    # Prepare Makefile boilerplate text
    time_string = time.strftime("%a %d. %b %H:%M:%S %Y")
    output = ''
    output += '###########################################################\n'
    output += '# Makefile generated by QMDE for CSRA68100                 \n'
    output += '#                                                          \n'
    output += '# Project: %s\n'                           % (proj_name)
    output += '# Configuration: %s\n'                     % (config_name)
    output += '# Generated: %s\n'                         % (time_string)
    output += '#                                                          \n'
    output += '# WARNING: Do not edit this file. Any changes will be lost \n'
    output += '#          when the project is rebuilt.                    \n'
    output += '#                                                          \n'
    output += '###########################################################\n'
    output += '\n'
    if is_audio:
        output += """#########################################################################
# Define root directory (relative so we can be installed anywhere)
#########################################################################

BUILD_ROOT = ../../build
include $(BUILD_ROOT)/roots.mkf
"""
        if 'MAKEFILE_INCLUDE' in cap_prop_list:
            items = cap_prop_list["MAKEFILE_INCLUDE"].split(",")
            for include in items:
                output += "\ninclude %s\n" % include

        output += """
#########################################################################
# Enter source files and directories and header directories here.
#
# Makerules will add the standard interface paths
#########################################################################
"""
        # List source files
        for src_file in [x for x in source_files if (os.path.splitext(x)[1]==".c") and (x.startswith(cap_path + os.sep))]:
            output += '\nC_SRC+=%s' % os.path.basename(src_file)

        for src_file in [x for x in source_files if (os.path.splitext(x)[1]==".asm") and (x.startswith(cap_path + os.sep))]:
            output += '\nS_SRC+=%s' % os.path.basename(src_file)
        output += '\n'

        if 'GEN_ASM_HDRS' in cap_prop_list:
            items = cap_prop_list["GEN_ASM_HDRS"].split(",")
            for file in items:
                output += "\nGEN_ASM_HDRS += %s" % file

        if 'GEN_ASM_DEFS' in cap_prop_list:
            items = cap_prop_list["GEN_ASM_DEFS"].split(",")
            for struct in items:
                output += "\nGEN_ASM_DEFS += %s" % struct

        # Add an H_PATH, C_PATH and S_PATH for each source_file located in a sub-directory so we can find them
        source_paths = set([os.path.dirname(x) for x in source_files])
        source_paths.discard(cap_path)
        source_paths = [os.path.join(os.path.curdir,os.path.relpath(x,cap_path)) for x in source_paths]

        for path in source_paths:
            output += "\nH_PATH += %s" % path
            output += "\nC_PATH += %s" % path
            output += "\nS_PATH += %s" % path

        if 'H_PATH' in cap_prop_list:
            items = cap_prop_list["H_PATH"].split(",")
            for file in items:
                output += "\nH_PATH += %s" % file

        if 'C_PATH' in cap_prop_list:
            items = cap_prop_list["C_PATH"].split(",")
            for file in items:
                output += "\nC_PATH += %s" % file

        if 'S_PATH' in cap_prop_list:
            items = cap_prop_list["S_PATH"].split(",")
            for file in items:
                output += "\nS_PATH += %s" % file

        # Include other makefiles
        output += """\n#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = %s

""" % capability
        output += """#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf
"""
    else:
        # Prepare Makefile variables
        if 'OUTDIR' not in make_vars or make_vars['OUTDIR'] == '':
            make_vars['OUTDIR'] = proj.dirname

        if 'OUTPUT' not in make_vars or make_vars['OUTPUT'] == '':
            make_vars['OUTPUT'] = proj_name

        if 'BUILDOUTPUT_PATH' not in make_vars or make_vars['BUILDOUTPUT_PATH'] == '':
            make_vars['BUILDOUTPUT_PATH'] = per_config_depend
        # adjust libpaths if output location specified
        if build_output_folder is not None:
            if make_vars.has_key('LIBPATHS'):
                libpaths = make_vars['LIBPATHS']
                paths = ""
                BUILDOUTPUT_PATH = ""
                prefix = libpaths.split('installed_libs')[0]
                suffix = proj.name
                for path in libpaths.split():
                    if path.startswith(prefix):
                        path = path[len(prefix):]
                    if per_config_depend.endswith(suffix):
                        BUILDOUTPUT_PATH = per_config_depend[:len(per_config_depend)-len(suffix)]
                    path = BUILDOUTPUT_PATH + path
                    paths += " " + path
                make_vars['LIBPATHS'] = paths + " " + libpaths
            if make_vars.has_key('INCPATHS'):
                incpaths = make_vars['INCPATHS']
                paths = ""
                BUILDOUTPUT_PATH = ""
                prefix = incpaths.split('installed_libs')[0]
                suffix = proj.name
                for path in incpaths.split():
                    if path.startswith(prefix):
                        path = path[len(prefix):]
                    if per_config_depend.endswith(suffix):
                        BUILDOUTPUT_PATH = per_config_depend[:len(per_config_depend)-len(suffix)]
                    path = BUILDOUTPUT_PATH + path
                    paths += " " + path
                make_vars['INCPATHS'] = incpaths + paths 

        for var, val in sorted(make_vars.items()):
            if len(var) > 0:
                output += "%s=%s\n" % (var, val.replace(" "," \\\n    "))

        # List source files
        output += '\n\nINPUTS=\\\n'
        for src_file in source_files:
            output += '    %s \\\n' % os.path.relpath(src_file, start=proj.dirname)
        output += '    %s \\\n' % os.path.relpath(os.path.join(proj.dirname, 'build_id_str.c'), start=proj.dirname)
        output += '$(DBS)\n'
        output += '\n'

        # Include other makefiles
        output += '-include %s\n' % (proj_name + '.mak')

        output += """# Check required variables have been defined
ifdef MAKEFILE_RULES_DIR
  $(info Using $(MAKEFILE_RULES_DIR)/Makefile.rules)
  include $(MAKEFILE_RULES_DIR)/Makefile.rules
else
  ifdef SDK
    include $(SDK)/Makefile.rules
  else
    $(error Variable SDK has not been defined. It should be set to the location of the Devkit tools folder.)
  endif
endif
"""

    # Is there an existing makefile? If so, write a temporary file and check whether contents are the same
    # If they are the same, do not overwrite existing makefile to avoid make detecting file has been touched
    # and rebuild everything
    if os.path.isfile(makefile_path):
        same = True
        # Write temporary makefile
        with open(makefile_path + "_tmp", 'w') as make_file:
            make_file.write(output)
        #Get differences
        with open(makefile_path, 'r') as file1:
            with open(makefile_path + "_tmp", 'r') as file2:
                same = set(file1).symmetric_difference(file2)
        for line in same:
            if not line.startswith("#"):
                same = False
        # If not the same, write the new makefile
        if not same:
            with open(makefile_path, 'w') as make_file:
                make_file.write(output)
        # Remove temporary makefile
        os.remove(makefile_path + "_tmp")
    else:
        with open(makefile_path, 'w') as make_file:
            make_file.write(output)
    return makefile_path

def get_tool_root(devkit):
    """ Get the path to the tools directory """
    return devkit.root

def is_audio_build(proj, tgt_config):
    config = proj.get_properties(tgt_config)
    return (config.get('SUBSYSTEM_NAME', '') == "audio")

def build(proj, devkit, tgt_config, build_style, build_output_folder=None):
    """ determine build style and act accordingly """
    if build_style not in ('build', 'clean'):
        bdex.raise_bd_err('INVALID_BUILD_STYLE', build_style)

    """ Audio and Apps build systems will need different makefiles"""
    is_audio = is_audio_build(proj, tgt_config)
    return do_make(proj, devkit, tgt_config, build_style, is_audio, build_output_folder)

def find_folder_in_path(start_path, path_to_find):
    ''' Starting at "start_path", walk up tree towards root, looking for "path_to_find" '''
    while True:
        looking_for = os.path.join(start_path, path_to_find)
        if os.path.isdir(looking_for):
            ''' Found path_to_find '''
            return looking_for
        start_path, tail = os.path.split(start_path)
        if not tail:
            ''' reached top of tree: STOP'''
            return None

def get_makefile_rules_path(proj, default_makefile_rules_path):
    ''' Find path to makefile.rules '''
    start_path = os.path.normpath(proj.dirname)
    path_to_find = os.path.join("adk", "tools")
    adk_tools_path = find_folder_in_path(start_path, path_to_find)
    if not adk_tools_path:
        raise ADKToolsNotFoundError(start_path)

    return adk_tools_path

def do_make(proj, devkit, tgt_config, build_style, is_audio, build_output_folder=None):
    """ build a project using a given configuration and devkit
        Expects that proj, devkit and tgt_config have already been validated
    """
    subproc_env=os.environ.copy()
    if is_audio:
        try:
            del subproc_env["PYTHONPATH"]
        except KeyError:
            pass

    make_vars = proj.get_properties(tgt_config)

    if build_output_folder is not None:
        per_config_depend = build_output_folder
    else:
        per_config_depend = 'depend_{}_{}'.format(tgt_config, make_vars.get('CHIP_TYPE', 'none'))


    makefile_path = ""
    if is_audio:
        cap_list = proj.configurations[tgt_config].capabilities
        if len(cap_list) == 0:
            raise Exception("Error. Audio project without CAPABILITIES found")
        for cap in cap_list:
            construct_makefile_frag(proj, per_config_depend, tgt_config, build_output_folder, is_audio, cap)
    else:
        makefile_path = construct_makefile_frag(proj, per_config_depend, tgt_config, build_output_folder)

    # Note: the variable SDK (sdk_var) is just used by the Makefile fragment
    # to find Makefile.rules.  Within Makefile.rules, DEVKIT_ROOT and
    # VM_LIBS_INSTALL_DIR are used to determine
    # where to find tools and where to find the installed libs
    tools_path = devkit['tools']
    make_exe_path = devkit['make.exe']
    sdk_var = "SDK=%s" % tools_path

    devkit_var = "DEVKIT_ROOT=%s" % devkit.root

    working_dir = proj.dirname

    path_to_makefile_rules = get_makefile_rules_path(proj, tools_path)

    makefile_rules_path = "MAKEFILE_RULES_DIR=%s" % path_to_makefile_rules

    # work out what the elf file is called.
    config = proj.get_properties(tgt_config)
    output_filename = config['OUTPUT']
    if is_audio:
        # Audio projects provide the .elf extension in the OUTPUT tag
        elf_path = os.path.abspath(os.path.join(working_dir, output_filename))
    else:
        if output_filename != '':
            elf_path = os.path.join(working_dir, per_config_depend, output_filename + '.elf')
        else:
            elf_path = os.path.join(working_dir, per_config_depend, proj.name + '.elf')

    # Ensure paths use a '/' separator
    make_exe_path = make_exe_path.replace('\\', '/')
    sdk_var = sdk_var.replace('\\', '/')
    makefile_path = makefile_path.replace('\\', '/')
    working_dir = working_dir.replace('\\', '/')
    devkit_var = devkit_var.replace('\\', '/')

    make_jobs = make_vars.get('MAKE_JOBS', "-j{:d}".format(get_make_jobs()))

    cmd_line = [
        make_exe_path,
        sdk_var,
        devkit_var,
        "-s",
        make_jobs,
        build_style]

    if is_audio:
        if make_vars['KALSIM_MODE'] == "true":
            config_make = make_vars['KALSIM_CONFIG_MAKE']
        else:
            config_make = make_vars['CONFIG_MAKE']

        cmd_line += [
            '-f', make_vars['MAKEFILE_TO_CALL'],
            config_make,
            make_vars['BUNDLE_NAME'],
            make_vars['BUILD_NAME'],
            make_vars["KYMERA_SRC_PATH"],
            make_vars["BUILD_PATH"],
            make_vars["BUILD_ROOT"],
            make_vars['OSTYPE']
        ]
    else:
        cmd_line += [
            '-f', makefile_path,
            makefile_rules_path
        ]

    bdex.trace("////////////// BUILD //////////////")
    bdex.trace("+++ PROJECT  '%s'" % proj.name)
    bdex.trace("+++ CONFIG   '%s'" % tgt_config)
    bdex.trace("+++ MAKEFILE '%s'" % makefile_path)
    bdex.trace("!!! MAKE_EXE_PATH=%s " % make_exe_path)
    bdex.trace("!!! SDK VAR=%s" % sdk_var)
    bdex.trace("!!! MAKEFILE_RULES_DIR=%s" % makefile_rules_path)
    bdex.trace("+++ MAKE COMMAND '%s'" % make_exe_path)
    bdex.trace("+++ MAKE COMMAND CMD_ARGS '%s'" % cmd_line)

    # exec the build command and capture output for streaming to caller
    return launch_cmd(cmd_line, subproc_env, tgt_config, working_dir, elf_path, is_audio)

def register_pylib_path(devkit_path):
    """
    Get the parent path to the pylib in the devkit and add it to sys.path if
    it's not already there

    TODO This is duplicated in deploy.py- needs to be available in
    """
    path = os.path.normpath(os.path.join(devkit_path, "apps", "fw", "tools"))
    if path not in sys.path:
        sys.path += [path, os.path.join(path, "pylib")]

def _run_prepare_fs(tool_root, root_dir, xuv_path, offset=0, appsFs=False):
    # Import prepare_fs.py and run it
    packfile_command = os.path.join(tool_root, "tools", "bin", "Packfile.exe")

    register_pylib_path(tool_root)

    from prepare_fs import pack_dir
    with open(xuv_path, "w") as out:
        pack_dir(root_dir, out, packfile=packfile_command, offset=offset, appsfs=appsFs)

def copydir(src, dst):
    # create the directory in the destination folder if sorce directory doesn't exists
    if not os.path.exists(dst):
        os.makedirs(dst)
        shutil.copystat(src, dst)
    dirlist = os.listdir(src)
    for item in dirlist:
        s = os.path.join(src, item)
        d = os.path.join(dst, item)

        # if s is directory then copy the dir
        if os.path.isdir(s):
            copydir(s, d)
        else:
            shutil.copy2(s, d)


def walk_up(start):
    """ like os.walk, but in reverse, goes up the directory tree.
        Unlike os.walk, doesn't differentiate files and folders """
    current = os.path.realpath(start)
    previous = ''

    while current != previous:
        try:
            yield current, os.listdir(current)
        except (IOError, OSError):
            return
        previous = current
        current = os.path.realpath(os.path.join(current, '..'))

def build_filesystem(proj, devkit, script_args, crypto_key, build_output_folder):
    """
    Build any of the possible flavours of the filesystem project
    """

    devkit_root = devkit.root
    register_pylib_path(devkit_root)
    working_dir = proj.dirname
    workspace_file = script_args.workspace_file
    # The option to use NvsCmd for deploying is passed in the Ubuild --special option.
    # If using NvsCmd to deploy, the filesystems must be built using the correct endian format.
    appsFs = False
    try:
        if use_nvscmd(script_args):
            appsFs = True
    except AttributeError:
        appsFs = False
    project_files = proj.files
    tool_root = devkit.root
    config = proj.get_properties('filesystem')
    output_filename = config.get('OUTPUT', None)
    try:
        filesystem_type = config['TYPE']
    except KeyError as excep:
        print("ERROR! Build Setting {} missing in project. {}".format(excep, proj.name))
        return False
    if filesystem_type == "curator_config":
        # Curator config filesystem should not use the appsFs argument
        appsFs = False
    if not output_filename:
        output_filename = filesystem_type + "_filesystem"
        if appsFs:
            # This is a build for an apps filesystem that is needed in the form
            # required by DFU, rather than in the form required for deploy
            output_filename = output_filename + "_dfu"

    if build_output_folder is not None:               
        xuv_path = os.path.join(build_output_folder, output_filename + '.xuv')
    else:
        xuv_path = os.path.join(working_dir, output_filename + '.xuv')


    def __get_hydracode_sdbfile_path(proj):
        """
        Lets see if the project has specified a specific hydracore sdb file to use
        If so then then there will be a property HYDRACORE_CONFIG_SDB_FILE defined in the project.x2p file and set to the path to use
        examples
            <property name="HYDRACORE_CONFIG_SDB_FILE">sdk://tools/config/hydracore_config_ALTERNATIVE1.sdb</property>
            <property name="HYDRACORE_CONFIG_SDB_FILE">../../MY_hydracore_config.sdb</property>
            <property name="HYDRACORE_CONFIG_SDB_FILE">C:\TEST_CONFIGS\hydracore_config.sdb</property>
        If this field is defined then that is what we are going to use and checks are made to ensure present.
        If the field is NOT defined or empty then walk up from the project file and search for the 'adk' folder, then:
            adk/bin/<chip_name>/hydracore_config.sdb
        """
        sdb_file = None
        config = proj.get_properties('filesystem')
        attribs = {
            'type': 'warning',
            'config': 'filesystem',
            'core': 'apps/p1',
            'module': 'build'
        }

        sdb_file_override = config.get('HYDRACORE_CONFIG_SDB_FILE')
        # Check to see if project override exists
        if sdb_file_override is not None and len(sdb_file_override) > 0:
            if os.path.isabs(sdb_file_override):
                sdb_override_full_path = sdb_file_override
            else:
                sdb_override_full_path = os.path.realpath(os.path.join(proj.dirname, sdb_file_override))

            if not os.path.isfile(sdb_override_full_path):
                msg = ["WARNING - Can not find HYDRACORE_CONFIG_SDB_FILE defined file = {}".format(sdb_override_full_path)]
                if sdb_file_override != sdb_override_full_path:
                    msg += [
                        "Property HYDRACORE_CONFIG_SDB_FILE is defined as = {}".format(sdb_file_override),
                        "Default to looking for the SDB file in the current device's bin folder"
                    ]

                bdex.log_buildproc_output('buildoutput', attribs, "\n".join(msg))
            else:
                sdb_file = sdb_override_full_path

        if not sdb_file:
            for current_dir, contents in walk_up(proj.dirname):
                if os.path.basename(current_dir) == 'adk':
                    if 'bin' in contents:
                        try:
                            sdb_file = glob.glob(os.path.join(current_dir, 'bin', config['CHIP_TYPE'], '*.sdb'))[0]
                        except IndexError:
                            sdb_file = None
                    else:
                        bdex.raise_bd_err('INVALID_CONFIG', "Can not find bin folder for this branch: {}".format(current_dir))

        if not sdb_file or not os.path.isfile(sdb_file):
            bdex.raise_bd_err('INVALID_CONFIG', "Can not find a suitable HYDRACORE_CONFIG_SDB_FILE")

        return sdb_file

    def get_ps_store_size():
        ''' The ps store size is calculated as 1/2 of the rw_config size '''
        from prepare_single_image import PrepareSingleImage
        prepare_image = PrepareSingleImage(devkit_root, script_args.workspace_file, None)
        flash_config = prepare_image.flash_config
        rw_config_size = 0

        for section, attrs in flash_config.get("layout", None):
            if section == "rw_config":
                rw_config_size = attrs.get("capacity", 0)

        if rw_config_size == 0:
            raise Exception('Flash config layout must contain a valid rw_config section')
        flash_device = flash_config.get('flash_device', None)
        if not flash_device:
            raise Exception('flash config must contain a flash_device section')
        block_size = flash_device.get('block_size', 0)
        if ((rw_config_size / block_size) % 2) != 0:
            raise Exception('rw_config size must be an even number of blocks')
        return rw_config_size // 2

    def gather_files(proj, sdb_system_name, image_directory):
        """
        Helper function that grabs all htfs and compiles them into image_directory,
        and copies all hcfs into the same directory
        """
        supported_fs_ext = [".hcf", ".dkcs"]

        sdb_file = __get_hydracode_sdbfile_path(proj)
        if sdb_file is None:
            return False
        print('SDB File: {}'.format(sdb_file))
        print('tool_root = {}'.format(tool_root))
        config_command = os.path.join(tool_root, "tools", "bin", "ConfigCmd.exe")

        for cfg_file in project_files:
            if os.path.splitext(cfg_file)[1] == ".htf":
                cmd_line = [config_command, "-noprefix", "binary", cfg_file,
                            image_directory, "-system", sdb_system_name, "-quiet",
                            "-database", sdb_file]
                if not launch_simple_cmd(cmd_line):
                    print("ConfigCmd failed: invoked as '%s'" % " ".join(cmd_line))
                    return False
            elif os.path.splitext(cfg_file)[1] in supported_fs_ext:
                # Precompiled - just copy into place
                shutil.copy(cfg_file, image_directory)

        return True

    if filesystem_type in ("firmware_config", "curator_config", "device_config"):
        # Grab the firmware htfs and the sdbs from the image projects and run
        # configcmd to produce the contents of a local images directory.  Then
        # run packfile on it.
        sdb_system_name = config["system_label"]
        image_directory = tempfile.mkdtemp()
        if not gather_files(proj, sdb_system_name, image_directory):
            return False

        _run_prepare_fs(tool_root, image_directory, xuv_path, appsFs=appsFs)

        def make_writeable(func, path, exc_info):
            if func is os.remove:
                os.chmod(path, 0o640)
                func(path)
        shutil.rmtree(image_directory, onerror=make_writeable)

    elif filesystem_type == "user_ps":
        # Grab the user ps htf and convert it to XUV using the psflash_converter
        # module

        htf_files = [f for f in project_files
                        if os.path.splitext(f.lower())[1] == '.htf']
        if len(htf_files) > 0:
            print("Building user key persistent store image")

            ps_store_size =  get_ps_store_size()

            from csr.dev.fw.psflash_converter import PsflashConverter
            try:
                psfs = PsflashConverter(crypto_key, stores_offset=0,
                                        store_size=ps_store_size)
            except TypeError:
                # TODO: Older API needed a default crypto_key to be passed to
                # PsflashConverter. This can be removed once all builds use
                # the PsflashConverter implementation which comes with its own default.
                crypto_key = (0, 0, 0, 0)
                psfs = PsflashConverter(crypto_key, stores_offset=0,
                                        store_size=ps_store_size)
            # We also need to push the PS keys into the SQIF
            # 1. Load the htf
            psfs.convert(htf_files, xuv_path)
        else:
            print("No PS keys to flash")
            # Better delete any xuv file that might have been hanging around
            # so we don't accidentally flash it later
            if os.path.isfile(xuv_path):
                os.remove(xuv_path)

    elif filesystem_type == "customer_ro":
        # Point packfile at the customer-supplied filesystem root to produce an
        # XUV
        # Temporary: this filesystem needs to contain the Apps config
        # as well as any customer RO filesystem

        try:
            fs_root = config["FS_ROOT"]
            no_setting = False if fs_root else True
        except KeyError:
            no_setting = True

        quick_charge_config_exists = bool('QUICKCHARGE_CONFIG' in config)
        sdb_system_name = config["system_label"]

        # Create a temporary directory to gather everything into
        image_directory = tempfile.mkdtemp()

        ws_projects = Workspace(workspace_file).parse()
        for name, project in ws_projects.items():
            proj_subsystem = project.default_configuration.properties.get('SUBSYSTEM_NAME')
            if "audio" in name or "audio" == proj_subsystem:
                project_files += get_capabilities_files_from_props(config, project)
                break

        bundle_files = [x for x in project_files if x.endswith("dkcs")]

        for bundle in bundle_files:
            # Get the associated ELF
            bundle_elf = os.path.splitext(bundle)[0] + ".elf"
            if os.path.isfile(bundle_elf):
                # Now report the ELF to the IDE for loading when debugging
                attribs = collections.OrderedDict()
                attribs['type'] = 'info'
                attribs['config'] = script_args.configuration
                # bundles are only for audio SS
                attribs["core"] = "audio/p0"
                attribs['module'] = 'deploy'
                bdex.log_buildproc_output('elfpath', attribs, bundle_elf)

        print("\nCopying files to %s filesystem...\n" % filesystem_type)

        # Firstly, copy any files that are added to the customer RO filesystem project
        if project_files:
            print("\nCopying files added to %s filesystem project...\n" % filesystem_type)
            for ro_file in project_files:
                print("Copying file %s" % ro_file)
                shutil.copy(ro_file, image_directory)

        sys.stdout.flush()

        # Then, if there is a FS_ROOT directory specified in the customer RO filesystem project properties,
        # copy all the files under this root directory
        if not no_setting:
            if not os.path.isdir(fs_root):
                # Assume it's relative to the project root
                fs_root = os.path.normpath(os.path.join(working_dir, fs_root))
                if not os.path.isdir(fs_root):
                    print("FS_ROOT directory does not exist.\nCreating: {}".format(fs_root))
                    os.makedirs(fs_root)
                    if not os.path.isdir(fs_root):
                        return False

            # Generate the quick charge configuration file
            if quick_charge_config_exists:
                quick_charge_config_file = os.path.join(
                    working_dir,
                    config["QUICKCHARGE_CONFIG"]
                )
                quickChargeConfig.quickChargeHexGen.xml_to_hex(
                    quick_charge_config_file,
                    os.path.join(fs_root, "quick_charge_config")
                )

            print("\nCopying files under FS_ROOT...")
            print("FS_ROOT (%s) with working dir (%s):" % (fs_root, working_dir))
            for root, dirs, files in os.walk(fs_root):
                for file in files:
                    print("Copying file %s\%s" % (root,file))
            sys.stdout.flush()
            copydir(fs_root, image_directory)

        _run_prepare_fs(tool_root, image_directory, xuv_path, appsFs=appsFs)

    return True


def get_capabilities_files_from_props(config, audio_project):
    audio_props = audio_project["prebuilt_image"].properties
    try:
        caps_path = audio_props['DKCS_PATH']
    except KeyError:
        return []

    if not os.path.isabs(caps_path):
        caps_path = os.path.normpath(os.path.join(audio_project.dirname, caps_path))

    if os.path.exists(caps_path):
        if not os.path.isdir(caps_path):
            raise ValueError("DKCS_PATH should point to a folder not a file")
    else:
        raise ValueError("DKCS_PATH points to non-existent location")

    return [os.path.join(caps_path, cap) for cap in config.get('DOWNLOADABLE_CAPABILITIES', '').split()]


def finalize_prebuilt_image(proj, devkit, tgt_config, build_output_folder):
    working_dir = proj.dirname

    config = proj.configurations[tgt_config].properties
    output_filename = config['OUTPUT']
    is_audio = is_audio_build(proj, tgt_config)
    if is_audio:
        elf_path = os.path.abspath(os.path.join(working_dir, output_filename))
    else:
        if build_output_folder is not None:
            per_config_depend = build_output_folder
        else:
            per_config_depend = 'depend_{}_{}'.format(tgt_config, config.get('CHIP_TYPE', 'none'))

        if output_filename != '':
            elf_path = os.path.join(working_dir, per_config_depend, output_filename + '.elf')
        else:
            elf_path = os.path.join(working_dir, per_config_depend, proj.name + '.elf')
    attribs = collections.OrderedDict([
        ('type', 'info'),
        ('config', 'tgt_config'),
        ('core', 'audio/p0' if is_audio else 'app/p1'),
        ('module', 'build')
    ])
    bdex.log_buildproc_output('elfpath', attribs, elf_path)

def build_with_makefile(proj, devkit, build_style, devkit_root, build_output_folder=None):
    config = proj.configurations['makefile_project'].properties
    makefile = config['MAKEFILE_TO_CALL']

    if build_style == "build":
        arguments_to_pass = config.get('BUILD_ARGUMENTS', '')
    else:
        arguments_to_pass = config.get('CLEAN_ARGUMENTS', '')

    arguments_to_pass = arguments_to_pass.split()

    if 'CHIP_TYPE' in config:
        arguments_to_pass += ["CHIP_TYPE=%s" % config['CHIP_TYPE']]

    if 'BUILD_TYPE' in config:
        arguments_to_pass += ["BUILD_TYPE=%s" % config['BUILD_TYPE']]

    if 'BUILD_LIBS_AS_PRIVATE' in config:
        arguments_to_pass += [
            "PRIVATE_LIBS=%s" % config['BUILD_LIBS_AS_PRIVATE'],
            "PRIVATE_LIBS_SEED=%d" % random.getrandbits(16)
        ]

    if 'INCPATHS' in config:
        arguments_to_pass += ["INCPATHS=%s" % config['INCPATHS']]

    basename = os.path.dirname(proj.name)
    output_suffix = " "
    if build_output_folder is not None:
        output_suffix = os.path.relpath(build_output_folder, basename)
        output_suffix = output_suffix.replace('\\', '/')
        arguments_to_pass += [
            "BUILDOUTPUT_PATH=%s" % output_suffix
        ]

    makefile = makefile.replace('\\', '/')

    # Pick up make.exe from inside the devkit
    make_exe_path = devkit['make.exe'].replace('\\', '/')
    devkit_root = (devkit.root).replace('/', '\\')

    is_audio = is_audio_build(proj, "makefile_project")

    cmd_line = [
        make_exe_path,
        "-s",
        "-f", makefile,
        "-R",
        "DEVKIT_ROOT=%s" % devkit_root,
        "-j%d" % get_make_jobs()
    ]
    if not is_audio:
        # Using -Otarget in the Audio build system makes it unnervingly silent,
        # possibly because it is recursive.  But it's handy the rest of the time.
        cmd_line.append("-Otarget")
    cmd_line += arguments_to_pass

    path_to_makefile_rules = get_makefile_rules_path(proj, devkit_root)

    cmd_line.append("MAKEFILE_RULES_DIR=%s" % path_to_makefile_rules)

    # The audio firmware makefile uses DEVKIT_ROOT but get_kcc_version.py expects
    # it in an environment variable, so supply it there as well as in the command line
    # The Audio build also needs PATH not to contain MSYS, so we reset it to the
    # minimum we can get away with
    subproc_env = os.environ.copy()
    subproc_env["DEVKIT_ROOT"] = devkit_root
    print("Added DEVKIT_ROOT=%s to the environment" % devkit_root)
    if is_audio and platform.system() == "Windows":
        print("Adjusting PATH for Audio build")
        subproc_env["PATH"] = "%s;%s" % (os.getenv("SYSTEMROOT"),
                                         os.path.join(os.getenv("SYSTEMROOT"),
                                                      "system32"))
        try:
            del subproc_env["PYTHONPATH"]
        except KeyError:
            pass
    print("Running %s in %s" % (" ".join(cmd_line), proj.dirname))
    subproc_env["PYTHON"] = os.path.join(devkit_root, "python27", "python.exe")
    if not launch_simple_cmd(cmd_line, cwd=proj.dirname, env=subproc_env):
        print("%s in %s failed!" % (" ".join(cmd_line), proj.dirname))
        return False
    return True

def get_crypto_key(script_args):
    """
    Search all projects in the workspace to find where the crypto key is and extract it.
    :param script_args:
    :return: the crypto key needed to encrypt the PS storage
    """
    crypto_key = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    # Get the project paths from the workspace because we need to be able
    # to find information from other projects
    # Get the list of the workspace proj_path
    ws_projects = Workspace(script_args.workspace_file).parse()

    # Go through each project, find the filesystem and then look for *.htf files under different filesystems
    # and scan them to find the encryption key "PsAesKey"
    for proj in ws_projects.values():
        config = proj.get_properties('filesystem')
        filesystem_type = config.get('TYPE')
        if filesystem_type in ("firmware_config", "curator_config", "device_config", "user_ps"):
            project_files = proj.files
            htf_files = [f for f in project_files if os.path.splitext(f.lower())[1] == '.htf']
            for file in htf_files:
                with open(file, 'r') as htf_file:
                    file_content = htf_file.read().splitlines()
                    for i in range(len(file_content)):
                        # Do not consider the key or anything else which is commented out
                        file_content[i] = re.sub("#.*$", "", file_content[i])
                        if "PsAesKey" in file_content[i]:
                            # PsAesKey = [ 00 2f 00 80 00 00 00 00 00 00 00 00 00 00 00 10]
                            # after splitting ['PsAesKey ', ' [ 00 2f 00 80 00 00 00 00 00 00 00 00 00 00 00 10]']
                            crypto_key = file_content[i].split("=")[-1:]

                            # removing  "[ ]" and extra spaces
                            crypto_key = crypto_key[0].replace("[", "").replace("]", "").replace(" ", "")

                            # creating 16 elements, each octet long this is what P0 expects
                            # e.g. [0, 47, 0, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16]
                            crypto_key = [int(crypto_key[i:i + 2], 16) for i in range(0, len(crypto_key), 2)]

    return crypto_key


def build_configs(proj, devkit, script_args):
    """ If the configuration named in tgt_config is 'all' then build
        every configuration listed in the project file.
        Otherwise build the one specified
    """

    if not bdex.LOGGER_INSTANCE:
        bdex.Logger(script_args)

    if not isinstance(proj, Project):
        if hasattr(proj, 'proj_fname'):
            # Looks like a ubuild project object
            proj = ubuild_compat.create_project(proj)
        else:
            print("Wrong project type {type!s} for project {project!s}".format(type(proj), proj))
            return False

    if not isinstance(devkit, AdkToolkit):
        if hasattr(devkit, 'devkit_dirname'):
            # Looks like a ubuild devkit object
            devkit = AdkToolkit(adk_toolkit_path=devkit.devkit_dirname)
        else:
            print("Wrong devkit type {type!s} for devkit {devkit!s}".format(type(devkit), devkit))
            return False

    tgt_config = script_args.configuration
    build_style = script_args.build_style
    build_output_folder = script_args.build_output_folder
    props = proj.configurations[tgt_config].properties

    if build_output_folder is None:
        build_output_folder = props.get('BUILD_OUTPUT_FOLDER', os.getenv("BUILD_OUTPUT_FOLDER"))

    if build_output_folder is not None:
        is_audio = is_audio_build(proj, tgt_config)
        if not is_audio:
            build_output_folder = os.path.join(build_output_folder, proj.name)

        if not os.path.exists(build_output_folder):
            os.makedirs(build_output_folder)
        build_output_folder = os.path.abspath(build_output_folder)

    config_list = proj.configurations

    if tgt_config.lower() == 'all':
        for config in config_list:
            if not build(proj, devkit, config, build_style, build_output_folder):
                return False

    elif tgt_config.lower() == 'prebuilt_image':
        finalize_prebuilt_image(proj, devkit, tgt_config, build_output_folder)
        print("Nothing to build for 'prebuilt_image' project")

    elif tgt_config.lower() == "filesystem":
        crypto_key = get_crypto_key(script_args)
        return build_filesystem(proj, devkit, script_args, crypto_key, build_output_folder)

    elif tgt_config.lower() == 'makefile_project':
        print("Build using a makefile")
        return build_with_makefile(proj, devkit, build_style, script_args.devkit_root, build_output_folder)

    else:
        if tgt_config in config_list:
            return build(proj, devkit, tgt_config, build_style, build_output_folder)
        else:
            bdex.raise_bd_err('INVALID_CONFIG', proj.filename, tgt_config)

    return True


def use_nvscmd(parsed_args):
    ubuild_special = parsed_args.special
    if ubuild_special is not None:
        if "nvscmd" in ubuild_special:
            return True
    return False
