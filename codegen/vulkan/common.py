# Copyright (c) 2022, Arm Limited and Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import math
import re
import os
import sys
import traceback

from dataclasses import dataclass
from lxml import etree

try:
    PATH = os.path.join(os.path.dirname(os.path.realpath(
        __file__)), '..', '..', 'third_party', 'vulkan', 'registry')
    sys.path.append(PATH)
    
    import reg
    from vkconventions import VulkanConventions
except ModuleNotFoundError:
    print("Failed to import vulkan registry, please make sure you have the vulkan registry submodule checked out")


def gen_hpp_header() -> str:
    return '/* Copyright (c) 2022, Arm Limited and Contributors\n' + \
        '*\n' + \
        '* SPDX-License-Identifier: Apache-2.0\n' + \
        '*\n' + \
        '* Licensed under the Apache License, Version 2.0 the "License";\n' + \
        '* you may not use this file except in compliance with the License.\n' + \
        '* You may obtain a copy of the License at\n' + \
        '*\n' + \
        '*     http://www.apache.org/licenses/LICENSE-2.0\n' + \
        '*\n' + \
        '* Unless required by applicable law or agreed to in writing, software\n' + \
        '* distributed under the License is distributed on an "AS IS" BASIS,\n' + \
        '* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n' + \
        '* See the License for the specific language governing permissions and\n' + \
        '* limitations under the License.\n' + \
        '*/\n' + \
        '\n' + \
        '#pragma once\n' + \
        '\n' + \
        '// DO NOT EDIT, THIS IS A GENERATED FILE!\n' + \
        '\n'


@dataclass
class MemberInfo:
    elem: etree.Element
    name: str
    type: str
    type_key: str
    length: str

    _force_fixed_array: bool

    def is_pointer(self):
        return '*' in self.type_key

    def is_string_array(self):
        return self.type_key in ("char* const*", "const char* const*")

    def is_string(self):
        return self.type_key in ('char*', 'char[]', 'const char*', 'const char[]')

    def is_fixed_length_array(self):
        # length is stored as an int if the array is of fixed length
        if self._force_fixed_array:
            return True

        try:
            int(self.length)
            return True
        except:
            return False

    def is_array(self):
        return (not self.is_string_array() and not self.is_string() and self.length is not None) or (not self.is_fixed_length_array() and '[]' in self.type_key)

    def is_const(self):
        return self.type_key.startswith("const")

    def length_needs_deference(self):
        # Match pPropertyCount but not propertyCount, pStruct->attributeCount cases
        return self.length and self.length.find("->") == -1 and len(self.length) >= 2 and self.length[0] == 'p' and self.length[1].isupper()

    def encode_length(self):
        if self.length_needs_deference():
            return "*" + self.length
        return self.length

    @property
    def encoder_function(self):
        if self.is_string():
            return 'encode_cstring'
        elif self.is_string_array():
            return 'encode_cstringArray'
        elif self.is_array() or self.is_fixed_length_array():
            return f'encode_{self.type}Array'
        else:
            return f'encode_{self.type}'

    @staticmethod
    def from_elem(elem):
        name_element = elem.find('name')
        type_element = elem.find('type')

        name = name_element.text
        type = type_element.text

        type_key = type + (type_element.tail or '').strip()
        if name_element.tail is not None and '[' in name_element.tail:
            type_key += '*'

        length = elem.get('len')

        # Some versions of the registry contain broken length fields
        if length and length.startswith("latexmath"):
            temp = length.replace("{", "REMOVE_THIS").replace(
                "}", "REMOVE_THIS").split("REMOVE_THIS")
            # Deepest nested bracket contains the actual name of the length variable
            length = temp[math.floor(len(temp)/2)]

        # If length is deduced using multiple methods, choose the first
        if length:
            length = length.split(",")[0]

        # Array defined without length name but a constant size
        if length == None and name_element.tail:
            count = re.findall(r'\[([0-9]+)\]', name_element.tail)
            if count:
                length = count[0]

        # Still no length, check if there is an enum element
        _force_fixed_array = False
        if not length:
            enum_element = elem.find('enum')
            if enum_element is not None:
                length = enum_element.text
                _force_fixed_array = True

        if elem.text:
            text = elem.text.strip()
            if text == 'const':
                type_key = 'const ' + type_key

        return MemberInfo(elem, name, type, type_key, length, _force_fixed_array)


class StructMembers:
    def __init__(self, type_info: reg.TypeInfo):
        self.type_info = type_info
        self.members = []
        for member in self.type_info.getMembers():
            self.members.append(MemberInfo.from_elem(member))

    def get_members(self) -> list[MemberInfo]:
        for member in self.members:
            yield member

    def is_chainable(self):
        """
        Return True if this is a chainable struct with
            VkStructureType sType;
            void * pNext;
        members.
        """
        first = self.members[0]
        second = self.members[1]
        return (first.name == 'sType' and first.type == 'VkStructureType'
                and second.name == 'pNext' and second.type == 'void')


class CommandMembers:
    def __init__(self, command_info: reg.CmdInfo):
        self.command_info = command_info
        self.members = []
        for member in self.command_info.getParams():
            self.members.append(MemberInfo.from_elem(member))

    def get_params(self) -> list[MemberInfo]:
        for member in self.members:
            yield member
