#!/usr/bin/env python3

# Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation, either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.
#

from enum import IntEnum
try:
    from enum import IntFlag
except ImportError:
    IntFlag = IntEnum
from collections import OrderedDict
import struct
import argparse
import zlib
import hashlib
import json
import sys
from functools import reduce, partial

class ModuleFunction(IntEnum):
    NONE = 0
    RESOURCE = 1
    BOOTLOADER = 2
    MONO_FIRMWARE = 3
    SYSTEM_PART = 4
    USER_PART = 5
    SETTINGS = 6
    NCP_FIRMWARE = 7
    RADIO_STACK = 8

class Platform(IntEnum):
    ARGON = 12
    BORON = 13
    ESOMX = 15
    ASOM = 22
    BSOM = 23
    B5SOM = 25
    TRACKER = 26
    TRACKERM = 28
    P2 = 32
    MSOM = 35

class StructSerializable(struct.Struct):
    def __init__(self, fmt):
        super().__init__(fmt)
        self._store = OrderedDict()

    def __getitem__(self, key):
        if not key.startswith('_'):
            if key in self._store:
                return self._store[key]
            else:
                return self.__dict__[key]
        else:
            return self.__dict__[key]

    def __setitem__(self, key, value):
        if not key.startswith('_'):
            self._store[key] = value
        else:
            self.__dict__[key] = value

    def __getattr__(self, key):
        return self.__getitem__(key)

    def __setattr__(self, key, value):
        self.__setitem__(key, value)

    def __contains__(self, key):
        if not key.startswith('_'):
            return key in self._store
        else:
            return key in self.__dict__

    def items(self):
        return self._store

    def dump(self, buf=None, offset=0):
        values = [x for x in self._store.values() if not isinstance(x, StructSerializable)]
        serializable = [x for x in self._store.values() if isinstance(x, StructSerializable)]
        if buf:
            self.pack_into(buf, offset, *values)
        else:
            buf = self.pack(*values)
        for s in serializable:
            buf += s.dump()
        return buf

    def __str__(self):
        c = self._store.copy()
        for k, v in c.items():
            if isinstance(v, bytes):
                c[k] = v.hex()
            elif isinstance(v, StructSerializable):
                c[k] = c[k]._store
        o = {self.__class__.__name__: c}
        return json.dumps(o, indent=4)

    def __repr__(self):
        return str(self)

class ModuleDependency(StructSerializable):
    def __init__(self, function=ModuleFunction.NONE, version=0, index=0):
        super().__init__('<BBH')
        if not isinstance(function, ModuleFunction):
            raise ValueError('Invalid module function')
        self.function = function
        self.index = index
        self.version = version

class ModuleFooter(StructSerializable):
    def __init__(self, sha=bytes(), reserved=0, product=False):
        super().__init__('<H32sH')
        self.reserved = reserved
        self.sha = sha
        self.fsize = self.size
        if product:
            self.fsize += 4

class ModuleFlags(IntFlag):
    NONE = 0x00
    DROP_MODULE_INFO = 0x01
    #COMPRESSED = 0x02
    #COMBINED = 0x04
    ENCRYPTED = 0x08

class ModuleHeader(StructSerializable):
    def __init__(self, start, length, version, platform, function, index=0, flags=None,
                 dependencies=None, **kwargs):
        super().__init__('<LLBBHHBB')

        if not isinstance(function, ModuleFunction):
            raise ValueError('Invalid module function')
        if not isinstance(platform, Platform):
            raise ValueError('Invalid platform id')
        if dependencies:
            if len(dependencies) > 2:
                raise ValueError('Module may have at most two dependencies')
            if len(dependencies) > 0:
                for d in dependencies:
                    if not isinstance(d, ModuleDependency):
                        raise ValueError('Invalid dependency')
        if flags is not None:
            if not isinstance(flags, ModuleFlags) and not isinstance(flags, int):
                raise ValueError('Invalid module flags')
        else:
            flags = ModuleFlags.NONE

        self.module_start_address = start
        self.module_end_address = start + length + self.size + ModuleFooter().size + ModuleDependency().size * 2
        self.mcu_target = 0
        self.flags = flags
        self.module_version = version
        self.platform_id = platform
        self.module_function = function
        self.module_index = index
        self.dependency = ModuleDependency()
        self.dependency2 = ModuleDependency()

        if dependencies:
            if len(dependencies) > 0:
                self.dependency = dependencies[0]
            if len(dependencies) > 1:
                self.dependency2 = dependencies[1]

        for k, v in kwargs.items():
            if k in self and v is not None:
                self[k] = v

class Module(object):
    def __init__(self, binary, address, platform, function, version, index=0, flags=None, dependencies=None, mcu=None, product=False):
        self.binary = binary
        self.address = address
        self.platform = platform
        self.function = function
        self.version = version
        self.index = index
        self.dependencies = dependencies
        self.mcu = mcu
        self.flags = flags
        self.product = product

    def _generate(self):
        header = ModuleHeader(self.address, len(self.binary), self.version, self.platform, self.function, self.index,
                              flags=self.flags, dependencies=self.dependencies, mcu_target=self.mcu)
        output = header.dump() + self.binary
        sha256 = hashlib.sha256(self.binary).digest()
        footer = ModuleFooter(sha256, product=self.product)
        output += footer.dump()
        crc32 = zlib.crc32(output)
        output += struct.pack('>L', crc32)
        return (output, header, footer, crc32)

    def dump(self):
        output, _, _, _ = self._generate()
        return output

    def __str__(self):
        _, header, footer, crc = self._generate()
        s = '%s\n%s\n%s' % (header, footer, hex(crc))
        return s

    def __repr__(self):
        return str(self)

def parse_dependency(dep):
    if dep:
        (func, version, index) = dep
        try:
            func = ModuleFunction[func.upper()]
        except:
            raise ValueError('Invalid module function: %s' % (func)) from None

        try:
            version = int(version)
        except:
            raise ValueError('Invalid version: %s' % (version)) from None

        try:
            index = int(index)
        except:
            raise ValueError('Invalid index: %s' % (index)) from None

        return ModuleDependency(func, version, index)
    return ModuleDependency()

GEN3_PLATFORMS = [Platform.ARGON, Platform.BORON, Platform.ASOM, Platform.BSOM, Platform.B5SOM, Platform.TRACKER, Platform.ESOMX]
GEN3_RADIO_STACK_VERSION_OFFSET = 0x300c
GEN3_RADIO_STACK_MBR_OFFSET = 0x1000
GEN3_RADIO_STACK_FLAGS = ModuleFlags.DROP_MODULE_INFO
# A bootloader and system-part1 supporting SoftDevice updates are required for radio stack modules generate for Gen 3 platforms
GEN3_RADIO_STACK_DEPENDENCY = ModuleDependency(ModuleFunction.BOOTLOADER, 501)
GEN3_RADIO_STACK_DEPENDENCY2 = ModuleDependency(ModuleFunction.SYSTEM_PART, 1321, 1)

RTL_PLATFORMS = [Platform.P2, Platform.TRACKERM, Platform.MSOM]
RTL_MBR_OFFSET = 0x08000000
RTL_KM0_PART1_OFFSET = 0x08014000

def main():
    platforms = [x.name.lower() for x in Platform]
    functions = [x.name.lower() for x in ModuleFunction]
    flags = [x.name.lower() for x in ModuleFlags]
    parser = argparse.ArgumentParser(description='Convert a raw binary into a Particle module binary')
    parser.add_argument('input', metavar='INPUT', type=argparse.FileType('rb'), help='Input raw bin file')
    parser.add_argument('output', metavar='OUTPUT', type=argparse.FileType('wb'), help='Output Particle module bin file')
    parser.add_argument('--address', default=0, type=partial(int, base=0), help='Start address of the module')
    parser.add_argument('--version', default=0, type=int, help='Module version (automatically derived for Gen 3 SoftDevice)')
    parser.add_argument('--platform', required=True, help='Module platform name', choices=platforms)
    parser.add_argument('--function', required=True, help='Module function', choices=functions)
    parser.add_argument('--index', default=0, type=int, help='Module index number')
    parser.add_argument('--dependency', default=[], nargs=3, action='append', metavar=('FUNCTION', 'VERSION', 'INDEX'), help='Module dependency')
    parser.add_argument('--mcu', type=int, default=0, help='MCU target')
    parser.add_argument('--flag', default=[], help='Module flag', action='append', choices=flags)
    parser.add_argument('--product', dest='product', action='store_true', help='Product firmware')

    args = parser.parse_args()

    dependencies = [parse_dependency(x) for x in args.dependency]
    platform = Platform[args.platform.upper()]
    function = ModuleFunction[args.function.upper()]
    index = args.index
    flags = reduce(lambda x, y: x|y, [ModuleFlags[x.upper()] for x in args.flag], ModuleFlags.NONE)
    version = args.version

    if not args.input.name.endswith('.hex'):
        bin = args.input.read()
    else:
        try:
            from intelhex import IntelHex
            # Reopen in non-binary mode :|
            name = args.input.name
            args.input.close()
            with open(args.input.name, 'r') as f:
                ihex = IntelHex(f)
                bin = ihex.tobinstr()
        except ImportError:
            print('`intelhex` library is required for hex file support')
            sys.exit(1)
        except:
            raise

    # Special handling for Gen 3 SoftDevice
    if platform in GEN3_PLATFORMS and function == ModuleFunction.RADIO_STACK and args.version == 0:
        (version,) = struct.unpack_from('<H', bin, GEN3_RADIO_STACK_VERSION_OFFSET)
        if len(dependencies) == 0:
            dependencies = [GEN3_RADIO_STACK_DEPENDENCY, GEN3_RADIO_STACK_DEPENDENCY2]
        if args.address == 0:
            # Skip MBR
            args.address = GEN3_RADIO_STACK_MBR_OFFSET
            bin = bin[GEN3_RADIO_STACK_MBR_OFFSET:]
        if len(args.flag) == 0:
            flags = GEN3_RADIO_STACK_FLAGS


    if platform in RTL_PLATFORMS and function == ModuleFunction.BOOTLOADER:
        if args.address == RTL_MBR_OFFSET:
            flags |= ModuleFlags.DROP_MODULE_INFO
            index = 1
        if args.address == RTL_KM0_PART1_OFFSET:
            flags |= ModuleFlags.DROP_MODULE_INFO
            index = 2

    product = (args.product and (function == ModuleFunction.USER_PART))
    m = Module(bin, args.address, platform, function, version, index, flags, dependencies, mcu=args.mcu, product=product)
    args.output.write(m.dump())
    print(m)

if __name__ == '__main__':
    main()
