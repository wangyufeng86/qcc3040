#!/usr/bin/env python
############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
Given the path to a configuration file as a command line argument, the script
parse recursively the entire tree of configuration files. Finding the all the
configuration files to be included requires the root of the kymera source tree
to be specified as a command line argument.

The script outputs a file in the specified output directory. This file contains
all the definition from the configuration file written in such a way that the
file can be included from a makefile.
"""
import os
import re
import sys
import argparse

# Enable disable debug mode
debug = False

class Configuration:
    """
    A class to hold the build configuration.
    """
    def __init__(self):
        # Definitions used by the shared makefiles.
        self.config_list = ConfigList()
        # Definitions passed to the compiler via the "-D" option.
        self.config_cpp_defs = ConfigDefs("cpp")
        # Definitions used by the module specific makefiles.
        self.config_build_defs = ConfigDefs("build")
        # Definitions used by this script to generate cpp and build definitions
        self.config_options_defs = ConfigDefs("options")

    @staticmethod
    def __get_value(dict, key):
        """
        Convert string "key" into a boolean.
        When the string is not a straightforward True and False, then
        the value is looked up in the dictionary. If the key is present
        in the dictionary then True is returned otherwise False is returned.
        """
        if key == "True":
           return True
        if key == "False":
           return False
        if key not in dict:
            return False
        if dict[key] == "True":
            return True
        return False

    def resolve_options(self):
        """
        Go through all the options and generate more definitions.
        """
        items = self.config_options_defs.to_dict()
        for key, arg in items.items():
            match = arg.split()
            if len(match) == 1:
                if (match[0] == "True"):
                    value = True
                elif (match[0] == "False"):
                    value = False
            elif len(match) == 2:
                if (match[0] == "not"):
                    value = not self.__get_value(items, match[1])
                else:
                    raise ValueError("Invalid option string: " + arg)
            elif len(match) == 3:
                if (match[1] == "and"):
                    value = self.__get_value(items, match[0]) and \
                            self.__get_value(items, match[2])
                elif (match[1] == "or"):
                    value = self.__get_value(items, match[0]) or \
                            self.__get_value(items, match[2])
                else:
                    raise ValueError("Invalid option string: " + arg)
            elif len(match) == 4:
                if (match[1] == "and") and (match[2] == "not"):
                    value = self.__get_value(items, match[0]) and \
                            not self.__get_value(items, match[3])
                elif (match[0] == "not") and (match[2] == "and"):
                    value = not self.__get_value(items, match[1]) and \
                            self.__get_value(items, match[3])
                elif (match[1] == "or") and (match[2] == "not"):
                    value = self.__get_value(items, match[0]) or \
                            not self.__get_value(items, match[3])
                elif (match[0] == "not") and (match[2] == "or"):
                    value = not self.__get_value(items, match[1]) or \
                            self.__get_value(items, match[3])
                else:
                    raise ValueError("Invalid option string: " + arg)
            elif len(match) == 5:
                if (match[0] == "not") and (match[2] == "and") and (match[3] == "not"):
                    value = not self.__get_value(items, match[1]) and \
                            not self.__get_value(items, match[4])
                elif (match[0] == "not") and (match[2] == "or") and (match[3] == "not"):
                    value = not self.__get_value(items, match[1]) or \
                            not self.__get_value(items, match[4])
            else:
                raise ValueError("Invalid option string: " + arg)
            if (value):
                self.config_cpp_defs.add(key)
                self.config_build_defs.add(key + "=True")

    def read_config_file(self, config_path, config_file):
        """
        Parse the tree of configuration files starting from "config_path/config_file".
        The result is stored in the object.
        """
        # this stack holds lines from a config file
        stack = DataStack()
        supported_states = [
            "setCurrentFile",# Sets the value of the current file.  In other words signals when
                             # the script is returning to the previous config file.
            "include",       # State signals that the next action is including another config file
            "list",          # State used for reading the name of the config list.
            "listContinue",  # State for reading a config list previously defined.
            "cpp",           # State for reading the cpp defs
            "build",         # State for reading the build defs
            "options"        # State for reading the options defs
            ]

        # Set the initial state of the state machine as invalid to make sure the first valid
        # config line will change the state.
        state = "invalid"
        # Set the current file
        current_file = config_file
        # Set the first config line as one which includes the main config.
        # This is to kick the state machine.
        stack.push("%include " + config_file)

        # pattern Used to read the the config file line by line
        pattern = re.compile(r"^((%[a-zA-Z0-9]+)|([^\s]+))\s*([^#]*)")

        # Explaining the regular expression:
        # "^((%[a-zA-Z0-9]+)|([^\s]+))\s*([^#]*)"
        # |              group  0               |# The whole match is called group 0
        #   (         group 1        )           # Group 1 only exist to have a logical "or" between
        #                                          group 2 and 3
        #    (   group 2   )                     # Search for lines containing %<new_state>
        #                                          [optional parameter]as first part
        #                    ( g 3  )            # Serf for lines not starting with %
        #                                (g 4  ) # What remains from the line. This is ""
        #                                          and not None if nothing found.

        # loop while we have data
        config_line = stack.get()
        while config_line:
            match = re.search(pattern, config_line)
            if debug:
                print("current_file  " + current_file)
                print("config_line " + config_line)
                print("match.groups() starting with group 1 {0}\n".format(match.groups()))

            # Set new state
            if match.group(2) != None: # State changer instruction read!
                # something starting a new state
                # search is %new_state
                # read the new state.
                state = match.group(2)[1:]
                if state not in supported_states:
                    print("State %s not supported!" % state)
                    sys.exit(1)

            # Additional actions depending on the input and current state.
            if (match.group(3) != None) or (match.group(4) != ""):

                if state == "include":

                    # strip is used to remove leading and trailing white spaces
                    next_file = config_path + match.group(4).strip()
                    data = read_file(next_file, stop_if_failure=False)
                    if data is None:
                        print("Unable to open %s included in %s\n" % (next_file, current_file))
                        sys.exit(1)
                    # Push a set current file config line to the stack to signal when returning to
                    # the previous config file. This is very useful when debugging config files.
                    stack.push("%setCurrentFile " + current_file)
                    stack.push(data)
                    # Set the current file and pus the data to the stack.
                    current_file = next_file
                    # Set the new state as invalid to make sure the following config line
                    # will change the state.
                    state = "invalid"

                elif state == "listContinue":

                    self.config_list.add_to_current_list(match.group(3))

                elif state == "list":

                    self.config_list.set_current_list(match.group(4))
                    state = "listContinue"

                elif state == "cpp":

                    if match.group(4) != "":
                        self.config_cpp_defs.add("{} {}".format(match.group(3), match.group(4)))
                    else:
                        self.config_cpp_defs.add(match.group(3))

                elif state == "build":

                    if match.group(4) != "":
                        self.config_build_defs.add("{} {}".format(match.group(3), match.group(4)))
                    else:
                        self.config_build_defs.add(match.group(3))

                elif state == "options":

                    if match.group(4) != "":
                        self.config_options_defs.add("{} {}".format(match.group(3), match.group(4)))
                    else:
                        self.config_options_defs.add(match.group(3))

                elif state == "setCurrentFile":

                    if match.group(2) != None and state == match.group(2)[1:]:
                        # Returning from a config file. Set the current file.
                        current_file = match.group(4)
                    else:
                        print("Bad config line %s in %s." % (state, current_file))
                        sys.exit(1)

                else:
                    print("Bad state %s. %s is the current config file processed." %
                          (state, current_file))
                    sys.exit(1)

            # read the next config line and process it in the next cycle
            config_line = stack.get()
        self.resolve_options()

### Some utility functions

def update_file(file_to_write, content):
    """ This function updates a file. If the files doesn't exist
    it will be created. Only writes to the file if the content is different."""
    if read_file(file_to_write, stop_if_failure=False, split_to_lines=False) != content:
        if debug:
            print(file_to_write + " was updated!")
        with open(file_to_write, "w") as targetfile:
            targetfile.write(content)
        return True
    else:
        if debug:
            print(file_to_write + " remains the same.")
        return False

def read_file(file_name, stop_if_failure=True, split_to_lines=True):
    """ Reads the content of a file. For gaining some performance all the content is
    read in one go. Additionally we can split the file into lines."""
    content = None
    try:
        with open(file_name) as file:
            content = file.read()
        if split_to_lines:
            return content.split("\n")
    except IOError as err:
        if stop_if_failure:
            sys.stderr.write("%s: unable to open %s\n" % (sys.argv[0], file_name))
            raise err

    return content

def grep_words(words, file_name):
    """A function to see if any of the strings in the list of words occurs as in a file"""
    data = read_file(file_name, split_to_lines=False)
    for word in words:
        if word in data:
            return True
    return False

def matching_file(suffixes, file_name):
    """See if a filename has one of a list of suffixes"""
    (_, ext) = os.path.splitext(file_name)

    if len(ext) > 1:
        return ext in suffixes
    else:
        return False

def touch_file(file_name):
    """A function to touch a source file. This will modify the last modified date to now and
    so it will be rebuild. """
    os.utime(file_name, None)

# Find all the files on the directories with one of a list of suffices
# containing one of the changed definitions. Those files will be marked for rebuild.
def touch_dependencies_on_changes(kymera_path, dirs, suffixes, changes):
    """Find all the files dependent on the changes in the given directories and touch them.
    Touching is updating the time so the build will think it was changed and so it will be
    rebuild."""
    for directory in dirs:
        if directory[0] != '/':
            # This is a relative path to kymera root
            directory = kymera_path + directory
        if not os.path.exists(directory):
            print("Directory {0} ".format(directory) + "included in ALL_SRCDIRS, ALL_INCDIRS " +
                  "or CFG_LIBS doesn't exist, continuing...")
        else:
            for file_name in os.listdir(directory):
                full_file_path = os.path.join(directory, file_name)
                # Filter a list of filenames down to those with one of the given suffixes"
                if matching_file(suffixes, full_file_path):
                    # Find all the files from a set with one of a list of suffices
                    # containing one of the changed definitions
                    if grep_words(changes, full_file_path):
                        print("Mark file for rebuild: " + full_file_path)
                        touch_file(full_file_path)

def update_config_mkf_file(file_name, config_file_full_path, config_mkf,
                           config_cpp_defs, config_list):
    """ Update the content of file_name with the supplied objects. """
    content = "# Configuration file generated automatically by %s\n# from %s\n" \
              "# Any changes will be lost the next time the configuration is generated.\n" % \
              (sys.argv[0], config_file_full_path)

    content += "\nCONFIG = " + config_mkf
    content += str(config_list)
    content += config_cpp_defs.to_string_for_compile_extra_defs()
    content += "\n"
    return update_file(file_name, content)

def update_defs_in_file(file_name, definitions):
    """ Update the content of file_name with the supplied ConfigDefs object. """
    content = str(definitions)
    content += "\n"
    return update_file(file_name, content)

### Helper classes

class ConfigList:
    """ This class is responsible for storing and printing all the config list. """
    def __init__(self):
        self.current_list = None
        self.list_dict = {}

    @staticmethod
    def strip_module_name(list_with_module_name):
        """
        Remove the module name from a path/module string.
        i.e. "components/ps/ps" becomes "components/ps"
        """
        module_path = set([])
        pattern = re.compile(r"^\s*(\S+)\/")
        for module in list_with_module_name:
            match = re.search(pattern, module)
            module_path.add(match.group(1))

        return module_path

    def get_list(self, list_name):
        """ Return a reference to the configuration list from the object. """
        if list_name in self.list_dict:
            return self.list_dict[list_name]
        else:
            return None

    def set_current_list(self, list_name):
        """ Set the list that will be used when .add_to_current_list() is called. """
        self.current_list = list_name

    def add_to_current_list(self, value):
        """
        Add a configuration list to the object. The addition process is stateful.
        The method .set_current_list() must be called before calling it.
        """
        if self.current_list != None:
            if not self.current_list in self.list_dict:
                # this is the first valuse we add to this list so we should create the set
                self.list_dict[self.current_list] = set()
            if value[0] == "-":
                # this is an undefine
                print("Remove {0} from {1}".format(value[1:], self.current_list))
                self.list_dict[self.current_list].discard(value[1:])
                if len(self.list_dict[self.current_list]) == 0:
                    print("empty list " + self.current_list)
                    self.list_dict.pop(self.current_list)
                return
            self.list_dict[self.current_list].add(value)
        else:
            # Should never happen
            print("Config list not defined.")
            sys.exit(1)

    def remove_from_current_list(self, value):
        """ Used for debug purposes only. """
        if self.current_list != None:
            self.list_dict[self.current_list].remove(value)
            if len(self.list_dict[self.current_list]) == 0:
                ## NO defs in this set
                # remove key form dict
                # this is the first valuse we add to this list so we should create the set
                self.list_dict.pop(self.current_list, None)
        else:
            print("Remove when current_list for ConfigList is None ")
            sys.exit(1)

    def pop(self, value):
        """ Pop a configuration list from the object. """
        if value != None:
            if not value in self.list_dict:
                print("Error list not previously defined")
                sys.exit(1)
            self.list_dict.pop(value)
        else:
            # Should never happen
            print("Config list not defined.")
            sys.exit(1)

    def extend(self, cfg_list):
        """ Join the two subsets. """
        for key in cfg_list.list_dict.keys():
            if key in self.list_dict:
                self.list_dict[key] = self.list_dict[key].union(cfg_list.list_dict[key])
            else:
                self.list_dict[key] = cfg_list.list_dict[key]

    def __str__(self):
        string = "\n\n#\n# List variable definitions from section %list\n#"
        for key in sorted(self.list_dict):
            string += "\n\n" + key + " = "
            for value in sorted(self.list_dict[key], key=lambda x: re.sub('[^A-Za-z]+', '', x)):
                string += value + " \\\n\t"
            # remove the last tab and slash
            string = string[:-3]

        return string


class ConfigDefs(set):
    """Class for saving the defines which is mainly a set.
    Created because of the different to string function."""

    def __init__(self, name, def_set=None):
        self.__name__ = name
        if def_set is None:
            def_set = set([])
        set.__init__(self, def_set)

    @staticmethod
    def defs_from_file(file_name):
        """
        Factory function that takes a last_defines.txt file and returns defines a ConfigDefs.
        """
        data = read_file(file_name, stop_if_failure=False)
        if data is None:
            return None

        # Push the read data to a DataStack() which will remove all the comments and empty
        # lines when we extracting them
        stack = DataStack()
        stack.push(data)
        # Every line (the comments and empty lines were already removed) form the
        # last_defines.txt is define.
        return ConfigDefs("cpp", stack.get_all())

    @staticmethod
    def get_def_name(definition):
        """ Returns the definition name. """
        return definition.split("=")[0]

    def remove_defs_value(self):
        """ Remove the value (if exist) from all the kept defines. Used for searching
        for the usage of a definition."""
        just_defs_name = set()
        for defines in self:
            just_defs_name.add(self.get_def_name(defines))

        # Update the current defs
        self.clear()
        self.update(just_defs_name)

    def discard(self, element):
        """ Remove a definition from the set. """
        # If the element appears by its name just call standard set discard method
        if element in self:
            super(ConfigDefs, self).discard(element)
        else:
        # otherwise be a bit more clever and see if we are removing something like "MYDEF=something"
            for string in self:
                if len(string.split("=")) == 2:
                    if string.split("=")[0].strip() == element:
                        super(ConfigDefs, self).discard(string)
                        return

    def add(self, element):
        """ Add a definition to the set. """
        temp = element.split("=", 1)
        if len(temp) == 1:
            element = temp[0].strip()
        else:
            element = temp[0].strip() + "=" + temp[1].strip()
        if element[0] == "-":
            # this is an undefine
            print(" Remove {0}".format(element[1:]))
            self.discard(element[1:])
            return

        set.add(self, element)
        for key in self:
            if (self.get_def_name(element) == self.get_def_name(key)) and (element != key):
                print(" Redefining {0} to {1}".format(key, element))
                self.remove(key)
                return

    def to_string_for_compile_extra_defs(self):
        try:
            string = "\n\n#\n# Preprocessor definitions from section %" + \
                     str(self.__name__) + "\n#\n\n"
        except AttributeError:
            string = ""
        string += "COMPILE_EXTRA_DEFS = "
        for value in sorted(self):
            string += "-D" + value + " \\\n\t"
        # remove the last tab and slash
        return string[:-3]

    def to_dict(self):
        result = {}
        for value in sorted(self):
            temp = value.split("=", 1)
            if len(temp) == 1:
                result[temp[0]] = "True"
            else:
                result[temp[0]] = temp[1]
        # remove the last tab and slash
        return result

    def __str__(self):
        try:
            string = "\n\n#\n# Preprocessor definitions from section %" + \
                     str(self.__name__) + "\n#\n\n"
        except AttributeError:
            string = ""
        for value in sorted(self):
            string += value + "\n"
        # remove the last tab and slash
        return string[:-1]

class DataStack:
    """ This class is used to hold the data form the config file. It is similar to a
    stack because during processing we pop "config lines" out. When an include "config
    line" is read another cofig file date is pushed to the beginning of the stack. """

    def __init__(self):
        self.storage = []

    def is_empty(self):
        """ Returns True when the stack is empty. """
        return len(self.storage) == 0

    def push(self, line):
        """ Pushes a "config line" or "config file" at the beginning of the stack.
        Note: a "config file" (or config data) is made up from "config lines"."""
        if isinstance(line, list):
            self.storage = line + self.storage
        else:
            self.storage = [line] + self.storage

    def get_next_line(self):
        """ Returns the next line without comment. """
        return self.storage.pop(0).split("#")[0].strip()

    def get(self):
        """ Returns the first useful config line """
        if len(self.storage) != 0:
            line = self.get_next_line()
            while line == "":
                if len(self.storage) != 0:
                    line = self.get_next_line()
                else:
                    return None
            return line
        else:
            return None

    def get_all(self):
        """ Returns all the useful information from the stack. """
        result = []
        line = self.get()
        while line:
            result.append(line)
            line = self.get()
        return result

    def __str__(self):
        """ Used for debug purposes. """
        return str(self.storage)

########################### Main script ###############################################
if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="This script parse a config file and creates " +
                                     "or updates if necessary the builddefs.mkf and <CONFIG>.mkf" +
                                     "for the build. It checks if any of the %cpp defines " +
                                     "changed from the previous build and if so it marks the " +
                                     "relevant files for rebuild.")
    parser.add_argument("-o",
                        type=str, dest="output_path", required=True,
                        help="The target build directory")
    parser.add_argument("-k",
                        type=str, dest="kymera_path", required=True,
                        help="Kymera root directory")
    parser.add_argument("-f",
                        type=str, dest="config_file", required=True,
                        help="The config file")
    parser.add_argument("-e",
                        type=str, dest="extra_symbol", required=False,
                        help="Extra symbol to be added to the options section")
    parser.add_argument("--dnld-build-name",
                        type=str, dest="build_name",
                        help="Build name of the ELF (download only)")
    parser.add_argument("--dnld-config-files",
                        type=str, nargs='*', dest="download_cfg_files",
                        help="List of config files used by capabilities (download only)")
    options = parser.parse_args()

    # folders should end with "/"
    if options.kymera_path[-1] != "/":
        options.kymera_path = options.kymera_path + "/"

    if options.output_path[-1] != "/":
        options.output_path = options.output_path + "/"

    # Is the top-level config directory
    options.config_path = options.kymera_path + "build/config/"
    options.last_defines_file = options.output_path + "last_defines.txt"
    options.builddefs_mkf_file = options.output_path + "builddefs.mkf"

    try:
        options.config = options.config_file.split("config.")[1]
        options.config_mkf_file = options.output_path + options.config + ".mkf"
    except IndexError:
        print("config file %s not recognised " % options.config_file)
        sys.exit(1)

    # Read the config files
    config = Configuration()
    if options.extra_symbol is not None:
        config.config_options_defs.add("{}=True".format(options.extra_symbol))

    config.read_config_file(options.config_path, options.config_file)
    if options.download_cfg_files is not None and options.build_name is not None:
        if options.download_cfg_files == []:
            print("Error no download capability config file provided")
            sys.exit(1)
        else:
            # Remove CFG_LIBS from config.mkf file
            # (We don't want to try rebuilding bits of the base firmware.)
            config.config_list.pop("CFG_LIBS")
            # Remove other lists of libs from config.mkf file
            # (These are probably harmless, but not useful.)
            config.config_list.pop("MAXIM_LIBS")
            config.config_list.pop("PRIVATE_LIBS")
            # Now add CFG_LIBS and new defines added for each download capability in the list
            for cfg_file in options.download_cfg_files:
                config.read_config_file(options.config_path, cfg_file)
            # Get extra configuration information from config_deps folder
            config.read_config_file(os.path.join(options.kymera_path,
                                                 "tools/KCSMaker/bundle/config_deps/"),
                                    "config." + options.build_name)

    config_mkf_changed = update_config_mkf_file(options.config_mkf_file,
                                                options.config_path + options.config_file,
                                                options.config, config.config_cpp_defs,
                                                config.config_list)

    builddefs_mkf_changed = update_defs_in_file(options.builddefs_mkf_file,
                                                config.config_build_defs)

    if builddefs_mkf_changed:
        # Everything will be rebuild so just update the last_defines.txt (config_mkf  and
        # config_mkf was already updated) and exit.
        update_defs_in_file(options.last_defines_file, config.config_cpp_defs)
    elif config_mkf_changed:
        # We can have a change in the last_defines.txt so we need to check it.
        # This is because contains the defines

        defs = ConfigDefs.defs_from_file(options.last_defines_file)
        if defs is None:
            print("Unable to locate last_defines.txt from the previous build.")
            # Create the last defines for the next build and touch builddefs_mkf_file
            # so everything get rebuild.
            update_defs_in_file(options.last_defines_file, config.config_cpp_defs)
            touch_file(options.builddefs_mkf_file)
            sys.exit()

        # To extract the changed defs symmetric difference is used.
        # This is a new set with elements in either the first or second set but not both
        changed_defs = defs ^ config.config_cpp_defs

        if len(changed_defs):
            # remove the value form the defs.
            # This is used to find the usage of the definition in the files.
            changed_defs_name = ConfigDefs("changed", changed_defs)
            changed_defs_name.remove_defs_value()

            print("Changed definitions:")
            for define in sorted(changed_defs):
                if define in defs: # Was in the old build
                    if define in changed_defs_name: # def without value
                        # this was undefined
                        print("\tUndef " + define.replace("\n", ", "))
                    else:
                        # Search for the new value
                        for new_def in config.config_cpp_defs & changed_defs:
                            if ConfigDefs.get_def_name(new_def) == ConfigDefs.get_def_name(define):
                                print("\tUpdate " + define.replace("\n", ", ") +\
                                      " to " + new_def.replace("\n", ", "))
                else: # should be in new build only
                    if define in changed_defs_name: # def without value
                        # New define
                        print("\tNew def " + define.replace("\n", ", "))
                    else:
                        # An updated def which was/will be displayed
                        pass

            if debug:
                print("changed_defs_name\n" + changed_defs_name)

            # Search in ALL_SRCDIRS and ALL_INCDIRs
            searchdirs = config.config_list.get_list("ALL_SRCDIRS")
            searchdirs = searchdirs.union(config.config_list.get_list("ALL_INCDIRS"))
            searchdirs = searchdirs.union(\
                            ConfigList.strip_module_name(config.config_list.get_list("CFG_LIBS")))
            touch_dependencies_on_changes(options.kymera_path, searchdirs,
                                          [".c", ".h", ".asm", ".cfg", ".xml", ".dyn"],
                                          changed_defs_name)
            update_defs_in_file(options.last_defines_file, config.config_cpp_defs)
        else:
            # Last_defines.txt contains the same useful information
            # (maybe is just differently formatted)
            pass
    else:
        # if config_mkf_changed didn't changed (which contains all the defines as well)
        # then last_defines.txt should be up to date. There is nothing to do further
        pass

#############################################################################################



