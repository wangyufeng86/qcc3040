"""
Copyright (c) 2016 - 2018 Qualcomm Technologies International, Ltd

Provides a helper function to allow flash memory to be programmed using pydbg/pylib
"""
# Python 2 and 3
from __future__ import absolute_import, print_function

from glob import fnmatch
import os
import sys

from . import subsystem_numbers as subsys_numbers


class Pyflash(object):
    def __init__(self, devkit_path, transport_uri):
        self.transport_uri = self.get_pylib_target(transport_uri)
        self.devkit_path = devkit_path
        self.register_pylib_path(devkit_path)
        self.device = self.attach()

    def register_pylib_path(self, devkit_path):
        """
        Get the parent path to the pylib in the devkit and add it to sys.path if
        it's not already there
        """
        path = os.path.join(devkit_path, os.path.join("apps", "fw", "tools"))
        if path not in sys.path:
            sys.path.extend([path, os.path.join(path, "pylib")])
        path = os.path.join(path, "make")
        if path not in sys.path:
            sys.path.append(path)

    def get_pylib_target(self, devkit_target):
        """
        Munge IDE syntax for dongles into pydbg's
        """
        # drop uri scheme, convert to list and drop device name from end
        target_list = devkit_target.split('://')[1].split('/')[:-1]

        # join list back together with ':' inbetween
        return ':'.join(target_list)

    @staticmethod
    def _find(where, what):
        for dirpath, dirnames, filenames in os.walk(where):
            for item in dirnames + filenames:
                if fnmatch.fnmatch(item, what):
                    return os.path.join(dirpath, item)
        return ''

    def get_firmware_builds(self, image_type=None, image_path=None):
        # This list will contain zero or one entries depending on whether an ELF
        # is present or not
        firmware_builds = []

        # Load Curator ELF if possible to support disabling deep sleep by poking the
        # kip table
        cur_root = os.path.join(self.devkit_path, "images", "curator_firmware")
        cur_elf = self._find(cur_root, "proc.elf")

        if cur_elf:
            firmware_builds.append("curator:{}".format(cur_elf))

        # Load the Apps P1 ELF in order to flash it
        if image_type == "apps1" and image_path:
            firmware_builds.append("apps1:{}".format(image_path))

        return ",".join(firmware_builds)

    def attach(self, image_type=None, image_path=None):
        from csr.front_end.pydbg_front_end import PydbgFrontEnd

        config = {
            "firmware_builds": self.get_firmware_builds(image_type, image_path),
            "device_url": self.transport_uri
        }

        device, _ = PydbgFrontEnd.attach(config)
        return device

    def device_reset(self):
        print("Reseting device...")
        self.device.reset()
        print("Reset done.")

    def enable_audio_sqif(self):
        print("Enabling Audio SQIF")
        self.__set_cur_mib("SqifAudioMaskROM", 1)

    def enable_bt_sqif(self):
        print("Enabling BT SQIF")
        self.__set_cur_mib("SqifBTMaskROM", 1)

    def __set_cur_mib(self, name, value):
        self.device.chip.curator_subsystem.core.fw.cucmd.set_mib(name, value)
        value_set = self.device.chip.curator_subsystem.core.fw.cucmd.get_mib(name)
        print(
            "{} = {}".format(name, value_set)
        )

    def burn(self, image_type, image_path, reset_device=True):
        success = False

        device = self.attach(image_type, image_path)

        subsys_id = subsys_numbers.subsystem_number_from_name[image_type]

        siflash = device.chip.curator_subsystem.siflash
        apps_fw = device.chip.apps_subsystem.p0.fw
        apps1_fw = device.chip.apps_subsystem.p1.fw

        if image_type != "curator":
            # Disable Curator USB interrupts to avoid flashing problems with USB attached
            disable_curator_usb_interrupts(device)
            # Try to disable deep sleep, unless it's the Curator itself that's being set up.
            disable_deep_sleep_if_possible(device)

        if image_type == "curator":
            print("Flash curator")
            image_dir = os.path.dirname(image_path)
            siflash.register_program_curator(dir_xuv=image_dir)
        elif image_type == "btss":
            # Special method which deals with the continuous read mode gubbins
            siflash.reprogram_bt(image_path, show_progress=True)
        elif image_type == "curator_fs":
            siflash.identify(subsys_id, 0)
            apps_fw.load_curator_fs(image_path)
        elif image_type == "p0_rw_config":
            siflash.identify(subsys_id, 0)
            apps_fw.load_rw_config(image_path)
        elif image_type == "p0_ro_cfg_fs":
            siflash.identify(subsys_id, 0)
            apps_fw.load_config_fs(image_path)
        elif image_type == "p0_ro_fs":
            siflash.identify(subsys_id, 0)
            apps_fw.load_fs(image_path)
        elif image_type == "p0_device_ro_fs":
            siflash.identify(subsys_id, 0)
            apps_fw.load_device_config_fs(image_path)
        elif image_type == "apps0":
            siflash.identify(subsys_id, 0)
            apps_fw.loader.load_custom("apps_p0", image_path)
        elif image_type == "apps1":
            siflash.identify(subsys_id, 0)
            apps1_fw.load()
        else:
            siflash.reprogram(subsys_id, 0, image_path)

        if reset_device is True:
            self.device_reset()

        success = True

        return success
