############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2014 - 2018 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
dm_append.py

Appends initialised data values in DM and PM RAM to ROM file contents
"""
import sys

# Some constants
MAX_ROM_SIZE = 0x200000     # 1M of 32-bit words, but we represent the ROM In 16-bit words

KALARCH_DEFAULT = 4

MAX_KALARCH4_DM_SIZE = 0xC0000 # Bigger DM for KAL ARCH 4
MAX_KALARCH4_PM_SIZE = 0x30000 # Bigger PM for KAL ARCH 4
KALARCH4_PM_BASE_ADDR = 0x04000000 # Base address for PM RAM in KAL ARCH 4

class XuvRom:
    """
    Class to manipulate XUV data.
    """
    def __init__(self):
        """
        Keeps the content of the ROM in the object.
        """
        self.rom = [None] * MAX_ROM_SIZE

    def read_rom_content(self, file):
        """
        Load the content of the ROM as 16 bits wide words into a global variable
        and return the address of the last location.
        """
        max_addr = 0

        for line in file:
            saddr, sdata = line.split()
            addr = int(saddr.strip('@'), 16)
            data = int(sdata, 16)

            if addr > max_addr:
                max_addr = addr

            # Blindly update previous content (shouldn't be anything there)
            if self.rom[addr//2] is None:
                self.rom[addr//2] = 0
            if addr & 1:
                self.rom[addr//2] = (self.rom[addr//2] & 0x00FF) | (data<<8)
            else:
                self.rom[addr//2] = (self.rom[addr//2] & 0xFF00) | (data)

        return max_addr//2

    def merge_xuv_content(self, file):
        """
        Merge a hardcoded .xuv fragment into the image we've built
        Complain if there are any overlaps
        """
        highest_addr = None

        for rawline in file:
            # Real .xuv files don't always allow comments, but we'll allow them
            # in our input for readability (and strip them)
            line = rawline.split('#')[0].strip()
            if line == '':
                # ignore blank or comment-only lines
                continue

            saddr, sdata = line.split()
            addr = int(saddr.strip('@'), 16)
            data = int(sdata, 16)

            if self.rom[addr] is None:
                self.rom[addr] = data
            else:
                print("-merge file attempts to redefine content at xuv address " + hex(addr))
                sys.exit(3)

            if highest_addr is None:
                highest_addr = addr
            else:
                highest_addr = max(highest_addr, addr)

        return highest_addr

    def xuv_force_append(self, xuv_address, file_name):
        """
        Copy contents of DM or PM file to the end of the xuv
        """
        line_count = 0
        xuv_index = 0
        file = open(file_name)

        for line in file:
            saddr, sdata = line.split()
            # We actually dont care much for the address.
            addr = int(saddr.strip('@'), 16)
            data = int(sdata, 16)

            xuv_index = xuv_address+(line_count//2)

            # Blindly update previous content (shouldn't be anything there)
            if self.rom[xuv_index] is None:
                self.rom[xuv_index] = 0
            if addr & 1:
                self.rom[xuv_index] = (self.rom[xuv_index] & 0x00FF) | (data<<8)
            else:
                self.rom[xuv_index] = (self.rom[xuv_index] & 0xFF00) | (data)
            # finally increase the line count in the loop
            line_count += 1

        file.close()
        return line_count//2

    def write_new_xuv(self, file, size):
        """
        Write the XUV ROM to disk.
        """
        for addr in range(size):
            content = self.rom[addr]
            if content is None:
                # it's OK to emit xuv files with holes
                continue
            file.write("@{0:0>8X} {1:0>4X}\n".format(addr, content))

def parse_options():
    """
    Parse and return the command line arguments.
    """
    offset = 0
    size = 0
    arch = KALARCH_DEFAULT
    file = None
    for x in range(len(sys.argv))[2:]:
        if sys.argv[x] == "-dmsize":
            size = int(sys.argv[x+1].lstrip('0x'), 16)
        if sys.argv[x] == "-offset":
            offset = int(sys.argv[x+1].lstrip('0x'), 16)
        if sys.argv[x] == "-kalarch":
            arch = int(sys.argv[x+1].lstrip('0x'), 16)
        if sys.argv[x] == "-merge":
            # Merge in a raw .xuv fragment and check for overlaps.
            # This was added to include some temporary test content in the
            # Crescendo r00 ROM. It's not expected to have a long-term
            # future. If it's in the way, get rid of it.
            file = sys.argv[x+1]

    return size, offset, arch, file

def read_dm_content(file):
    """
    Find the first and last address used by the DM block.
    """
    last_used = 0
    # Initialise DM array and first used based on architecture
    dm_ram = [0] * MAX_KALARCH4_DM_SIZE
    first_used = MAX_KALARCH4_DM_SIZE-1

    for line in file:
        saddr, sdata = line.split()
        addr = int(saddr.strip('@'), 16)
        if addr >= MAX_KALARCH4_DM_SIZE:
            raise IndexError("DM index {0} is out of range DM [0-{1}]." \
                             "This is indicative of an error in the linker script." \
                             .format(hex(addr), hex(MAX_KALARCH4_DM_SIZE)))
        data = int(sdata, 16)
        dm_ram[addr] = data
        if data != 0:
            if addr > last_used:
                last_used = addr
            if addr < first_used:
                first_used = addr

    return first_used, last_used


def read_pm_content(file):
    """
    Find the first and last address used by the PM block.
    """
    pm_ram = [0] * MAX_KALARCH4_PM_SIZE
    first_used = MAX_KALARCH4_PM_SIZE-1
    last_used = 0

    for line in file:
        saddr, sdata = line.split()
        addr = int(saddr.strip('@'), 16)
        data = int(sdata, 16)
        # KAL ARCH4 has PM RAM starting at a different base than 0.
        addr = addr-KALARCH4_PM_BASE_ADDR
        if addr >= MAX_KALARCH4_PM_SIZE:
            raise IndexError("PM index {0} is out of range DM [0-{1}]" \
                             "This is indicative of an error in the linker script." \
                             .format(hex(addr), hex(MAX_KALARCH4_PM_SIZE)))
        pm_ram[addr] = data
        if data != 0:
            if addr > last_used:
                last_used = addr
            if addr < first_used:
                first_used = addr

    # PM range should be 16-bit aligned
    # Round down first to an even address
    first_used &= ~1
    # Round up last to an odd address
    last_used |= 1

    return first_used, last_used

if __name__ == '__main__':
    xuv_rom = XuvRom()

    if len(sys.argv) < 2:
        print("dm_append filename" +
              "[-dmsize <size>] [-offset <offset>] [-kalarch 4] [-merge extra.xuv]\n")
        sys.exit(2)

    base_file_name = sys.argv[1]
    dm_size, dm_offset, kalarch, mergefile = parse_options()

    if kalarch != 4:
        print("Unsupported architecture: " + str(kalarch))
        sys.exit(3)

    rom_file_name = base_file_name + ".rom"
    dm_file_name = base_file_name + ".dm"
    pm_file_name = base_file_name + ".pm"
    xuv_file_name = base_file_name + ".xuv"

    rom_file = open(rom_file_name)
    dm_file = open(dm_file_name)
    pm_file = open(pm_file_name)

    # First read the base ROM content from the .rom file
    last_xuv_address = xuv_rom.read_rom_content(rom_file)

    # Make sure everything we add is 32-bit aligned
    # Tag data starts at (last_xuv_address+1) in 16-bit addresses
    # so last_xuv_address should be odd.
    if (last_xuv_address & 1) == 0:
        last_xuv_address += 1

    # Now read the DM and PM files
    first_dm_address, last_dm_address = read_dm_content(dm_file)
    first_pm_address, last_pm_address = read_pm_content(pm_file)

    have_pm_ram = (last_pm_address >= first_pm_address)

    rom_file.close()
    dm_file.close()
    pm_file.close()

    # On 32-bit Kalimbas (such as Crescendo) we can let the linker script do a lot more of the
    # work, so we don't have to write out a little script here ('tags') for crt0 to follow.
    # Instead we just do the heavy lifting of copying DM and PM initialisers to where the
    # already-linked code expects to find them (Zero initalisation is handled purely in link
    # script / crt0.).
    # (FIXME: we're only doing this here because kalelf2mem didn't do what we wanted if we did
    # it in the linker script. Now that we're abandoning kalelf2mem per
    # <http://ukbugdb/B-169702#h1226267>, can we move this to the linker script, and turn this
    # script into a pure ELF-to-xuv converter?)
    if have_pm_ram:
        last_xuv_address += xuv_rom.xuv_force_append(last_xuv_address+1, pm_file_name)
    last_xuv_address += xuv_rom.xuv_force_append(last_xuv_address+1, dm_file_name)

    # Add any hardcoded extra xuv content
    if mergefile:
        merge_xuv_file = open(mergefile)
        last_xuv_address = max(last_xuv_address, xuv_rom.merge_xuv_content(merge_xuv_file))

    # Finally write out the new .xuv file
    xuvfile = open(xuv_file_name, "w")
    xuv_rom.write_new_xuv(xuvfile, last_xuv_address+1)
    xuvfile.close()
