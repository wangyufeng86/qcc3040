############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2016 - 2018 Qualcomm Technologies International, Ltd 
#
############################################################################
"""
Forms a wrapper around pydbug allowing pydbg to be started by
passing a heracles workspace where the paths to the  various
images are obtained.
This is primarily intended for use in the heracles tabs.

Usage:
> python pydbg_tab.py -p <path_to_pydbg> -d <debug_interface> \
      [-w <workspace_file>] [-t <type>]
where
<path_to_pydbg> is full path to pydbg.
<debug_interface> trb:scar or equivalent.
<workspace_file> Workspace file for current workspace.
<type> Leave blank for pydbg interactice session, 'prim_live_log'
for primitive logging session and 'trap_live_log' for trap logging
session.
"""

# Python 2 and 3
from __future__ import print_function

import os
import sys
from xml.etree import ElementTree
import argparse
import glob
from workspace_parse.workspace import Workspace

try:
    import Tkinter as Tki  # Python 2.x
except ImportError:
    import tkinter as Tki  # Python 3.x

class NoDeviceDetected(RuntimeError):
    """
    Heracles has passed an unexpanded argument to deploy.py because it hasn't
    detected a device to which it could point.  Arguably Heracles shouldn't be
    trying to invoke deploy.py in this case, but it's easy enough to detect 
    here
    """



def validate_args(args):
    """
    Check the passed args are sensible
    """
    if not args.pydbg_path:
        print("Please specify a path to pylib")
        return False

    if args.tab_type:
        tab_file = ''
        if args.tab_type.endswith('.py'):
            tab_file = args.tab_type

            if not os.path.isabs(tab_file):
                here = os.path.abspath(os.path.dirname(__file__))
                tab_file = os.path.join(here, args.tab_type)

            if os.path.isfile(tab_file):
                args.tab_type = tab_file
                return True
            else:
                print("Tab script file not found: {}".format(tab_file))
                return False

        if not (args.tab_type in ["trap_live_log", "prim_live_log", "fw_live_log"]):
            print("Incorrect tab type")
            return False

    if not args.workspace:
        print("Please enter a valid workspace")
        return False

    if not args.kit:
        print("Please enter a valid devkit")
        return False

    return True

def get_projects_from_workspace(workspace_file):
    """
    Given a workspace file (x2w) will return the 
    Path to the apps0, main project and kymera_audio
    projects contained within
    """
    project_dict = {}
    workspace_name = os.path.basename(workspace_file)
    main_proj_name, dont_care = os.path.splitext(workspace_name)
    main_proj_name = main_proj_name + ".x2p"
    ws_projects = Workspace(workspace_file).parse()
    for child in ws_projects.keys():
        project = ws_projects[child].filename
        if "apps0_firmware.x2p" in project:
            project_dict["apps0"] = project
        if "audio_firmware.x2p" in project:
            project_dict["audio_image"] = project
        if "kymera_audio.x2p" in project:
            project_dict["audio_package"] = project
        if main_proj_name in project:
            project_dict["apps1"] = project
    return project_dict


def get_elf_path_from_image_project(project_file, devkit):
    """
    Given a prebuilt image project file (x2v) will return the 
    Path to the image directory
    """
    project_tree = ElementTree.parse(project_file)
    project_root = project_tree.getroot()

    for file in project_root.iter('file'):
        file_path =  file.get('path')
        dont_care, file_extension = os.path.splitext(file_path)
        if '.elf' in file_extension:
            image_path = os.path.join(os.path.dirname(project_file), file_path)
            image_path = os.path.abspath(image_path)
            return image_path

    for file in project_root.iter('sdkfile'):
        file_path = os.path.join(devkit, file.get('path'))
        file_path = os.path.normpath(file_path)
        for f in glob.glob(file_path):
            file_extension = os.path.splitext(f)[1]
            if '.elf' in file_extension:
                return f

    return None

def get_elf_path_from_audio_project(project_file):
    """
    Given an audio project file (x2v) will return the 
    Path to the image directory
    """

    image_path = get_output_value_from_proj(project_file)
    if image_path:
        project_path = os.path.dirname(project_file)
        image_path = os.path.join(project_path, image_path)
        image_path = os.path.abspath(image_path)
    return image_path

def get_output_value_from_proj(project_file):
    """
    Given an audio project file (x2v) will return the 
    value of the property 'OUTPUT' if that field exists
    """
    project_tree = ElementTree.parse(project_file)
    project_root = project_tree.getroot()
    for configuration in project_root.iter('configuration'):
        if 'default' in configuration.get('options'):
            for property in configuration.iter('property'):
                if 'OUTPUT' in property.get('name'):
                    return property.text
    return None

def get_elf_path_for_std_project(project_file, devkit):
    """
    Given a Heracles project file (x2v) will return the 
    Path to the image directory
    """

    # Add required paths to sys.path if not there already
    import sys
    path = os.path.join(devkit, "tools","ubuild")
    if not path in sys.path:
        sys.path.append(path)
    import maker.util as util

    project_tree = ElementTree.parse(project_file)
    project_root = project_tree.getroot()

    for configuration in project_root.iter('configuration'):
        if 'default' in configuration.get('options'):
            config_name = configuration.get('name')

    project_path = os.path.dirname(os.path.abspath(project_file))
    fsprefix  = util.get_fsprefix(devkit)
    build_dir = "depend_" + config_name + "_" + fsprefix
    elf_dir = os.path.join(project_path, build_dir)
    # Now look for any files with a .elf extension
    elf_files = glob.glob(os.path.join(elf_dir, "*.elf"))
    # There should be exactly one if the project has been built
    if len(elf_files) == 1:
        return elf_files[0]

def is_device_connected(devkit_target):
    """
    Check to see if the device mentioned is a string in the expected format
    """
    devkit_target_split = devkit_target.split('://');
    # Check we have a device  expected format device://trb/usb2trb/155192/qcc5124
    if len(devkit_target_split) < 2:
        print('Could not attach to device = [{}]. Are you sure that it is connected?'.format(devkit_target))
        return False
    return True

class SimpleRadioBoxUI(object):
    """
    Creates a dialogue box containing a configurable list of radio buttons,
    with OK to return the selected button and Cancel (or closing the window) to
    exit with the effect of none of the windows having been selected.
    """
    def __init__(self, title, radio_buttons, label_text, icon=None):

        self.root = Tki.Tk()
        self.root.title(title)

        # The Pydbg window appears near the bottom by default, so put the
        # dialogue box somewhere around there too.
        scn_w = self.root.winfo_screenwidth()
        scn_h = self.root.winfo_screenheight()
        geom = "+%d+%d" % (scn_w//4,scn_h//2)
        self.root.geometry(geom)

        if icon:
            self.root.iconbitmap(bitmap=icon)

        # Put the instructional label at the top...
        lbl = Tki.Label(self.root, text=label_text)
        lbl.pack(side=Tki.TOP, padx=5,pady=5)

        # Followed by the radio buttons
        rbut_frame = Tki.Frame(self.root, borderwidth=1)
        rbut_frame.pack(fill=Tki.BOTH, expand=True)

        self._button_selected = Tki.IntVar()
        rbuts = []
        for button_desc in radio_buttons:
            rbut = Tki.Radiobutton(rbut_frame, text=button_desc, 
                                   var=self._button_selected, value=len(rbuts))
            rbut.pack(anchor=Tki.W)
            rbuts.append(rbut)
        
        # Exit buttons go at the bottom on the right.
        
        # Cancel button closes the window and ensures the selection is None
        cancel_button = Tki.Button(self.root, text="Cancel", 
                                   command=self._select_none_and_exit, width=6)
        cancel_button.pack(side=Tki.RIGHT)
        # Make closing the window act like pressing Cancel
        self.root.protocol("WM_DELETE_WINDOW", self._select_none_and_exit)
        # OK button closes the window preserving the radio button selection
        ok_button = Tki.Button(self.root, text="OK", command=self.root.destroy,
                               width=6)
        ok_button.pack(side=Tki.RIGHT, padx=5,pady=5)

        Tki.mainloop()
        
    def _select_none_and_exit(self):
        self._button_selected = None
        self.root.destroy()
        
    @property
    def selected(self):
        return self._button_selected.get() if self._button_selected is not None else None

def prompt_secondary_device_selection(devkit_root, primary_device_url):
    """
    Open a simple dialogue box listing devices that are detected as being
    connected to the host PC over either TRB or USBDBG, not including the
    primary, if there are any.  The dialogue box allows the user to select one
    of these as a secondary device to attach Pydbg to, or to cancel the process
    and continue with a single-device Pydbg session.
    """
    try:
        from csr.front_end.pydbg_front_end import get_attached_trb_and_usbdbg_devices
    except ImportError:
        return None
    
    # Get all the attached devices
    trb_devices, usbdbg_devices = get_attached_trb_and_usbdbg_devices()
    
    class TrbDesc(object):
        def __init__(self, trb_dev):
            self.id = trb_dev.id
            self.description = trb_dev.description
            self.driver = trb_dev.driver
        def __str__(self):
            return "usb2trb device with ID %s" % self.id
        
    class UsbdbgDesc(object):
        def __init__(self, usbdbg_dev):
            self.id = usbdbg_dev.id
        def __str__(self):
            return "usb2tc device with ID %s" % self.id
    
    # Eliminate the one that QMDE knows about (i.e. the primary device) and
    # create descriptive wrappers around the native dongle type classes 
    primary_device_type = primary_device_url.split(":",1)[0]
    primary_device_id = int(primary_device_url.split(":")[-1])
    
    if primary_device_type in ("trb",):
        other_trb_devices = [TrbDesc(trb_dev) for trb_dev in trb_devices 
                                                if trb_dev.id != primary_device_id]
        other_usbdbg_devices = [UsbdbgDesc(usbdbg_dev) for usbdbg_dev in usbdbg_devices]
    elif primary_device_type in ("tc","usb2tc"):
        # Unlike TRB devices USB degug devices return their ID as a string so we cast to int
        other_trb_devices = [TrbDesc(trb_dev) for trb_dev in trb_devices]
        other_usbdbg_devices = [UsbdbgDesc(usbdbg_dev) for usbdbg_dev in usbdbg_devices 
                                                if int(usbdbg_dev.id) != primary_device_id]
    else:
        print("WARNING: unrecognised primary device URL '%s' during secondary "
              "device selection: aborting secondary device selection" % primary_device_url)
        return None

    # Set up the list of radio button descriptions
    device_descs = [str(dev) for dev in other_trb_devices + other_usbdbg_devices]

    # If there are any other devices attached, bring up the dialogue box
    if device_descs:
        # Grab the QMDE icon to make the dialogue box look slightly more professional
        if os.path.exists(os.path.join(devkit_root, 'res', 'qmde_allsizes.ico')):
            qmde_icon = os.path.join(devkit_root, 'res', 'qmde_allsizes.ico')
        else:
            qmde_icon = None

        selector = SimpleRadioBoxUI("Pydbg secondary device", 
                                    device_descs, "Please select secondary device, or cancel "
                                            "for a single-device session", icon=qmde_icon)
    
        selected = selector.selected

        # If the user pressed cancel or closed the window, we infer that they 
        # didn't want a multi-device session
        if selected is None:
            return None
        
        # Otherwise, construct the URL of the secondary device, to be added to
        # the Pydbg command line
        if selected < len(other_trb_devices):
            selected_url = "trb:usb2trb:%d" % other_trb_devices[selected].id
        else:
            selected -= len(other_trb_devices)
            selected_url = "usb2tc:%s" % other_usbdbg_devices[selected].id
    
        return selected_url
    
    # No other devices connected - identical result to user cancelling.
    return None

def get_pylib_target(devkit_target):
    """
    Munge Heracles' syntax for dongles into pydbg's
    """
    # drop uri scheme, convert to list and drop device name from end
    # target_list = devkit_target.split('://')[1].split('/')[:-1]
    devkit_target_split = devkit_target.split('://');
    # Check we have a device  expected format xxxxx
    if len(devkit_target_split) < 2:
        raise ValueError('Could not attach to device = [{}]. Are you sure that it is connected?'.format(devkit_target))

    target_list = devkit_target_split[1].split('/')[:-1]

    # join list back together with ':' inbetween
    return ':'.join(target_list)
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("-p", "--pydbg", help="Path to pydbg.py",
                        dest="pydbg_path", default=None)

    parser.add_argument("-d", "--debug_interface", help="Debug interface eg. trb:scar",
                        dest="debug", default="trb:scar")

    parser.add_argument("-t", "--tab_type",
                        help=("prim_live_log, trap_live_log, a path to a python script"
                              "with an absolute path or a path relative to the location of this script,"
                              "or none for normal pydbg"),
                        dest="tab_type", default=None)

    parser.add_argument("-k", "--kit", help="The devkit this is being used in",
                        dest="kit", default=None)

    parser.add_argument("-w", "--workspace", help="The workspace file that is being debugged",
                        dest="workspace", default=None)

    parser.add_argument("--passOn", help="Options to be passed on to pydbg",
                        dest="passOn", default=None, nargs=argparse.REMAINDER)

    args = parser.parse_args()
    if not is_device_connected(args.debug):
        print("Can not connect to device. Is it attached?")
        raise NoDeviceDetected("Can not connect to device. Is it attached?")

    if validate_args(args):
        #Get the paths of the projects
        projects = get_projects_from_workspace(args.workspace)

        # Now get the paths to the various images
        firmware_args = []
        try:
            apps0_path = get_elf_path_from_image_project(projects['apps0'], args.kit)
            if apps0_path:
                firmware_args.append("apps0:" + apps0_path)
        except KeyError:
            pass

        try:
            apps1_path = get_elf_path_for_std_project(projects['apps1'], args.kit)
            if apps1_path:
                firmware_args.append("apps1:" + apps1_path)
        except KeyError:
            pass

        try:
            if 'audio_package' in projects:
                audio_path = get_elf_path_from_audio_project(projects['audio_package'])
            else:
                audio_path = get_elf_path_from_image_project(projects['audio_image'], args.kit)
            if audio_path:
                firmware_args.append("audio:" + audio_path)
        except KeyError:
            pass

        primary_device_url = get_pylib_target(args.debug)
        # Only interested in secondary device in Pydbg tab
        if args.tab_type is None:
            secondary_device_url = prompt_secondary_device_selection(args.kit,
                                                                 primary_device_url)
        else:
            secondary_device_url = None

        #Setup the calling string for pydbg
        command_line = [args.pydbg_path]
        command_line += ["-d", primary_device_url]
        if secondary_device_url:
            command_line[-1] += "," + secondary_device_url
        if firmware_args:
            command_line += ["-f", ",".join(firmware_args)] 
        if args.tab_type:
            command_line.append(args.tab_type)
        if args.passOn:
            command_line += args.passOn

        sys.argv = command_line
        fw_tools_path = os.path.realpath(os.path.dirname(args.pydbg_path))
        sys.path = [fw_tools_path, os.path.join(fw_tools_path,"pylib")] + sys.path
        os.environ["PYDBG_RUNNING_IN_SUBPROCESS"] = "1"
        sys.stderr = sys.stdout # reassing Python's notion of stderr so everything
        # goes to stdout.  This seems to work better wtih the QMDE process handling
        
        from csr.front_end.pydbg_front_end import PydbgFrontEnd
        PydbgFrontEnd.main_wrapper()
        