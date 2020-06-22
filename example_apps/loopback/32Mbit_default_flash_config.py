# Copyright (c) 2017-2019 Qualcomm Technologies International, Ltd.
# The format of this file is documented in CS-00410685-UG / 80-CG297-1

# The block assignment in this layout should satisfy:
#   boot_block_size + image_header + sum(layout['capacity']) <= alt_image_offset
# image_header size is usually just 1 * block_size

{
    "flash_device": {
        "block_size": 4 * 1024,
        "boot_block_size": 1 * 4 * 1024,
        "alt_image_offset": 512 * 4 * 1024
    },
    "encrypt": False,
    "layout": [
        # image_header partition
        ("curator_fs",      {"capacity":   2 * 4 * 1024,  "authenticate": False, "src_file_signed": False}),
        ("apps_p0",         {"capacity": 154 * 4 * 1024,  "authenticate": True, "src_file_signed": True}),
        ("apps_p1",         {"capacity": 161 * 4 * 1024,  "authenticate": False}),
        # Device config filesystem size limited by size of production test buffer,  ( 1024*2)-10.
        ("device_ro_fs",    {"capacity":   1 * 4 * 1024,  "authenticate": False, "inline_auth_hash": True }),
        ("rw_config",       {"capacity":  32 * 4 * 1024}),
        ("rw_fs",           {"capacity":   1 * 4 * 1024}),
        ("ro_cfg_fs",       {"capacity":  64 * 4 * 1024,  "authenticate": False}),
        ("ro_fs",           {"capacity":  95 * 4 * 1024,  "authenticate": False})
    ]
}
