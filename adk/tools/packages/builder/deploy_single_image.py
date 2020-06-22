#!/usr/bin/env python
# Copyright (c) 2017 - 2018 Qualcomm Technologies International, Ltd.
"""
    Build and deploy a flash image file from the current workspace.
"""
# Python 2 and 3
from __future__ import absolute_import
from __future__ import print_function

import sys
import argparse
import os
import tempfile

from .prepare_single_image import PrepareSingleImage
from workspace_parse.workspace import Workspace
from workspace_builders.base_builder import BaseBuilder
from .nvscmd import NvsCmd
from .pydbg_flash_image import Pyflash
from .device_checker import check_device_with_timeout

try:
    import maker.exceptions as bdex

    def ubuild_print(line):
        bdex.log_buildproc_output('deployoutput', {'type': 'info'}, line)

    print = ubuild_print
except ImportError:
    def print_flush(line):
        print(line)
        sys.stdout.flush()

    print = print_flush


def parse_args(args):
    """ parse the command line arguments """
    parser = argparse.ArgumentParser(description='Build a flash image file')

    parser.add_argument('-k', '--devkit_root',
                        required=True,
                        help='specifies the path to the root folder of the devkit to use')
    parser.add_argument('-w', '--workspace',
                        required=True,
                        help='specifies the workspace file to use')
    parser.add_argument('-a', '--active_project',
                        action="store_true",
                        required=False,
                        help='specifies the output directory to use')
    parser.add_argument('-t', '--tools_dir',
                        required=True,
                        help='specifies the location of NvsCmd tool directory')
    parser.add_argument('-d', '--device_uri',
                        required=True,
                        help='specifies the transport URI')
    parser.add_argument('-o', '--build_output_folder',
                        default=None,
                        help='Specify location for object files and other build artefacts.')

    _parsed = parser.parse_args(args)

    if _parsed.build_output_folder is None:
        _parsed.build_output_folder = os.getenv("BUILD_OUTPUT_FOLDER")

    return _parsed


class PrepareForImage(BaseBuilder):
    def __init__(self, args='', single_image=None, inputpath=None):
        super(PrepareForImage, self).__init__(args)
        self.single_image = single_image
        self.inputpath = inputpath

    def _is_project_buildable(self, config):
        """ Returns True if the project can be deployed """
        return config.is_deployable()

    def _run_builder(self, project, config):
        super(PrepareForImage, self)._run_builder(project, config)
        print("Adding dependent project {}".format(project.filename))
        self.single_image.process_project(project.filename, self.inputpath)


def collect_all_workspace_projects(single_image, workspace_file, inputpath):
    """
    Collect all workspace projects and combine them into a single image.
    Build and collect the input xuv files from all the projects in the workspace
    """
    workspace = Workspace(workspace_file, PrepareForImage(single_image=single_image, inputpath=inputpath))
    workspace.build()


def collect_default_projects(single_image, workspace_file, inputpath):
    """
    Collect the default project dependencies and combine them into a single image.
    """
    # Build and collect the input xuv files from the default project in the workspace
    workspace = Workspace(workspace_file, PrepareForImage(single_image=single_image, inputpath=inputpath))
    workspace.build_default_project()


def _collect_projects(only_default_project, single_image, workspace_file):
    input_path = tempfile.mkdtemp()
    if only_default_project:
        print("Deploy active project")
        # Deploy active project and the dependent projects
        collect_default_projects(single_image, workspace_file, input_path)
    else:
        # Deploy all projects
        print("Deploy all projects")
        collect_all_workspace_projects(single_image, workspace_file, input_path)


def main(cmd_line_args):
    """ deploy_single_image main entry point.
    """
    args = parse_args(cmd_line_args)

    if args.build_output_folder is not None:
        output_path = args.build_output_folder
    else:
        output_path = os.path.dirname(args.workspace)

    single_image = PrepareSingleImage(args.devkit_root, args.workspace, args.workspace, args.build_output_folder, generate_audio_image_xuv=True)

    _collect_projects(args.active_project, single_image, args.workspace)

    if single_image.projects_exist():
        curator_image = single_image.get_prebuilt_image("curator")
        pyflash = Pyflash(args.devkit_root, args.device_uri)
        if curator_image:
            # Get known version of Curator in place first to guarantee nvscmd operation
            print("Program the curator SQIF with image: {}".format(curator_image))
            pyflash.burn("curator", curator_image, reset_device=True)

        nvscmd = NvsCmd(args.tools_dir, args.device_uri)
        apps_image, audio_image = single_image.create_flash_image(args.devkit_root, output_path)

        if audio_image:
            print("Program the audio SQIF with image: {}".format(audio_image))
            pyflash.enable_audio_sqif()
            nvscmd.burn(audio_image, "audio")

        btss_image = single_image.get_prebuilt_image("btss")
        if btss_image:
            print("Program the btss SQIF with image: {}".format(btss_image))
            pyflash.enable_bt_sqif()
            nvscmd.burn(btss_image, "btss")

        if apps_image:
            print("Program the apps SQIF with image: {}".format(apps_image))
            nvscmd.burn(apps_image, "apps")

        pyflash.device_reset()

    return check_device_with_timeout(args.device_uri, timeout=10, devkit_path=args.devkit_root)


if __name__ == '__main__':
    if not main(sys.argv[1:]):
        sys.exit(1)
    sys.exit(0)
