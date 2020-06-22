#!/usr/bin/env python
############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2011 - 2018 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
(1) Makes the xIDE project files suitable for either Kalsim or HAPS debugging.
(2) Adds a list of all source files under the main directory to the project file.
"""
import os
import glob
import argparse

class Arguments:
    """ Class to parse and hold the arguments of the script. """
    def __init__(self):
        """
        Parse the command line arguments and store them into the object.
        """
        parser = argparse.ArgumentParser(description="Creates the xIDE project files suitable " + \
                                         "for either Kalsim or HAPS debugging.")
        parser.add_argument("-c", "--config",
                            action="store", type=str, dest="config", required=True,
                            help="The config of the build. i.e crescendo_rom or crescendo.")
        parser.add_argument("-d", "--debug_bin_dir",
                            action="store", type=str, dest="debug_bin_dir", required=True,
                            help="The folder containing the build output, i.e. the *elf, etc")
        parser.add_argument("-t", "--target_root",
                            action="store", type=str, dest="target_root", required=True,
                            help="The target root, i.e. \"kymera_crescendo_audio\"")
        parser.add_argument("--subsysid",
                            action="store", type=int, dest="subsysid", default=3,
                            help="The subsystem id that will be baked into the project file.")
        parser.add_argument("--debugtrans",
                            action="store", type=str, dest="debugtrans", default="usb",
                            help="The debug transport that will be used " +
                            "when connecting a debugger.")
        options = parser.parse_args()

        self.config = options.config
        self.debug_bin_dir = options.debug_bin_dir
        self.target_root = options.target_root
        self.subsysid = options.subsysid
        self.debugtrans = options.debugtrans

    def is_kalsim(self):
        """ Returns True if the target is a simulator. """
        return 'kalsim' in self.config or self.debugtrans == "kalsim"

    def get_transport(self):
        """ Returns the transport string to connect to the debugger. """
        if 'kalsim' in self.config or self.debugtrans == "kalsim":
            transport = 'SPITRANS=KALSIM SPIPORT=1'
        elif self.debugtrans == "lpt":
            transport = '[SPITRANS=LPT SPIPORT=1 SPIMUL=4]'
        elif self.debugtrans == "trb":
            transport = '[trb/scar/0]'
        else:
            transport = '[SPITRANS=USB SPIPORT=0 SPIMUL=4]'
        return transport

class FileScanner:
    """
    Scan through subdirectories in search of project files.
    """
    def __init__(self, isKalsim, debug_bin_dir):
        self.is_kalsim = isKalsim
        self.debug_bin_dir = debug_bin_dir
        self.interesting_files = []

    def __file_is_interesting(self, file_name):
        """
        Returns true if file_name ends in .c/.cpp/.asm and isn't in another config's folder
        """
        if ('kalsim' in file_name) and not self.is_kalsim:
            return False
        if ('/output/' in file_name) and ('kalsim' not in file_name) and self.is_kalsim:
            return False

        ext = os.path.splitext(file_name)[1]
        return (ext == '.c') or (ext == '.cpp') or (ext == '.asm')

    def __add_file(self, path, start):
        """
        Given a base path and a file path, figure out the relative path and
        store it into the object.
        """
        start_list = os.path.abspath(start).split('/')
        path_list = os.path.abspath(path).split('/')

        # Work out how much of the filepath is shared by start and path.
        i = len(os.path.commonprefix([start_list, path_list]))

        rel_list = ['..'] * (len(start_list)-i) + path_list[i:]
        rel_path = os.path.join(*rel_list)

        self.interesting_files.append(rel_path)

    def __parse_file_list(self, file_list_name):
        """
        Parse a gdb debug info file and add the list of files it
        contains to the object.
        """
        try:
            with open(file_list_name) as file:
                line_cnt = 0
                for line in file:
                    striped_line = line.rstrip('\n')
                    self.__add_file(striped_line, self.debug_bin_dir)
                    line_cnt += 1
            return line_cnt
        except IOError:
            return 0
        return 0

    def process_gdb_info(self, file_list_name):
        """
        Try to extract a list of project files from a GDB info file.
        Returns true if at least one file has been found.
        """
        number_found_files = self.__parse_file_list(file_list_name)
        if number_found_files > 0:
            print("Flist loaded from file [" + file_list_name + "]" + \
                  "(loaded " + str(number_found_files) + " files)")
            return True
        return False

    def search_files(self, src_dir):
        """
        Perform a recursive search for project files.
        Returns true if at least one file has been found.
        """
        found = False
        files = glob.glob(src_dir + '/*') # gives all files/folders in this directory

        for file in files:
            if self.search_files(file):
                found = True
            elif self.__file_is_interesting(file):
                self.__add_file(file, self.debug_bin_dir)
                found = True

        return found

class ProjectFileGenerator:
    """
    Generate a couple of XML files needed by xIDE.
    """
    def __init__(self, file_scanner, debug_bin_dir):
        self.file_scanner = file_scanner
        self.debug_bin_dir = debug_bin_dir

    @staticmethod
    def make_file_path_element(path_list):
        """ Generate a string of valid XML elements from a list of file paths. """
        result = ""
        for path in path_list:
            result += ' <file path="' + path + '"/>' + os.linesep
        return result

    def generate_xiw_file(self, target_root):
        """ Write a xIDE workspace file to disk. """
        with open(self.debug_bin_dir + os.sep + target_root + r'.xiw', 'w') as file:
            file.write(r'<workspace>' + os.linesep +
                       r' <project path="' + target_root + r'.xip"/>' + os.linesep +
                       r'</workspace>' + os.linesep)

    def generate_xip_file(self, target_root, subsysid, transport):
        """ Write a xIDE project file to disk. """
        xml = r'<project buildenvironment="{6907f3f0-0ed4-11d8-be02-0050dadfe87d}" ' + \
              r'buildenvironmentname="kalimba" executionenvironmentoption="binutils" ' + \
              r'buildenvironmentoption="" executionenvironmentname="kalimba" ' + \
              r'executionenvironment="{2ca81310-0ecd-11d8-be02-0050dadfe87d}">' + os.linesep + \
              self.make_file_path_element(self.file_scanner.interesting_files) + \
              r' <properties currentconfiguration="Release">' + os.linesep + \
              r'  <configuration name="Release">' + os.linesep + \
              r'   <property key="buildMatch" >(?:ERROR|FATAL|WARNING|MESSAGE)' + \
              r':\s+(file)\s+(line):</property>' + os.linesep + \
              r'   <property key="debugtransport">' + transport + r'</property>' + os.linesep + \
              r'   <property key="hardware" >0</property>' + os.linesep + \
              r'   <property key="subsysid" >'+ str(subsysid)+ r'</property>' + os.linesep + \
              r'  </configuration>' + os.linesep + \
              r' </properties>' + os.linesep + \
              r'</project>' + os.linesep
        with open(self.debug_bin_dir + os.sep + target_root + r'.xip', 'w') as file:
            file.write(xml)

if __name__ == "__main__":
    ARGUMENTS = Arguments()

    SCANNER = FileScanner(ARGUMENTS.is_kalsim(), ARGUMENTS.debug_bin_dir)
    GENERATOR = ProjectFileGenerator(SCANNER, ARGUMENTS.debug_bin_dir)

    print('Generating xIDE project files...')
    GENERATOR.generate_xiw_file(ARGUMENTS.target_root)

    print('...Searching for source...')
    # Attempt parse a GDB info file first, failing that scan the disk.
    if not SCANNER.process_gdb_info(ARGUMENTS.debug_bin_dir + "/.gdb_info_sources.txt"):
        SCANNER.search_files(os.path.dirname(os.getcwd()))

    print('...Adding source list...')
    GENERATOR.generate_xip_file(ARGUMENTS.target_root,
                                ARGUMENTS.subsysid,
                                ARGUMENTS.get_transport())

    print('...done.')
