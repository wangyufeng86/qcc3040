#!/usr/bin/env python
# Copyright (c) 2017-2019 Qualcomm Technologies International, Ltd.
# The format of this file is documented in CS-00410685-UG / 80-CG297-1
{
    "flash_device": {
        "block_size": 4 * 1024,
        "boot_block_size": 4 * 1024,
        "alt_image_offset": 32 * 64 * 1024
    },
    "encrypt": False,
    "layout": [
        ("curator_fs",      { "capacity" :   3 * 4 * 1024,  "authenticate": False, "src_file_signed": False}),
        ("apps_p0",         { "capacity" : 150 * 4 * 1024,  "authenticate": True, "src_file_signed": True}),
        ("apps_p1",         { "capacity" : 150 * 4 * 1024,  "authenticate": False}),
        # Device config filesystem size limited by size of production test buffer,  ( 1024*2)-10.
        ("device_ro_fs",    { "capacity" :   1 * 4 * 1024,  "authenticate": False, "inline_auth_hash": True }),
        ("rw_config",       { "capacity" :  42 * 4 * 1024}), 
        # read write file system must have 1 sector even it not being used to support DFU
        ("rw_fs",           { "capacity" :  16 * 4 * 1024}), 
        ("ro_cfg_fs",       { "capacity" :  18 * 4 * 1024,  "authenticate": False}),
        ("ro_fs",           { "capacity" :  94 * 4 * 1024,  "authenticate": False})
    ]
}
