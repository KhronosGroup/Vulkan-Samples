#!/usr/bin/env python3

import argparse
import math
import re
import sys
import traceback

from dataclasses import dataclass
from lxml import etree

try:
    import reg
    from vkconventions import VulkanConventions
except ModuleNotFoundError:
    print(traceback.format_exc())
    print(
        'Unable to import the Vulkan registry script. Please make sure you have'
        ' a copy of the Vulkan-Docs repository and have set PYTHONPATH'
        ' appropriately.'
    )
    print('e.g (for bash):')
    print(' $ git clone https://github.com/KhronosGroup/Vulkan-Docs')
    print(' $ export PYTHONPATH=Vulkan-Docs/scripts')
    sys.exit(1)

@dataclass
class VarInfo:
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
            temp = length.replace("{", "REMOVE_THIS").replace("}", "REMOVE_THIS").split("REMOVE_THIS")
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

        return VarInfo(elem, name, type, type_key, length, _force_fixed_array)


class StructInfo:
    def __init__(self, type_info):
        self.type_info = type_info
        self.members = []
        for member in self.type_info.getMembers():
            self.members.append(VarInfo.from_elem(member))

    def get_members(self):
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


class HelperOutputGenerator(reg.OutputGenerator):
    """Generates C-language API interfaces."""

    # This is an ordered list of sections in the header file.
    SECTIONS = ['basetype', 'handle', 'enum', 'group', 'bitmask',
                'funcpointer', 'struct', 'command']

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Internal state - accumulators for different inner block text
        self.sections = {section: [] for section in self.SECTIONS}
        self.feature_not_empty = False

    def beginFile(self, genOpts):
        super().beginFile(genOpts)

    def endFile(self):
        super().endFile()

    def beginFeature(self, interface, emit):
        super().beginFeature(interface, emit)

        self.sections = {section: [] for section in self.SECTIONS}
        self.feature_not_empty = False

    def endFeature(self):
        "Actually write the interface to the output file."
        if self.emit:
            if self.feature_not_empty:
                for section in self.SECTIONS:
                    contents = self.sections[section]
                    self.outFile.writelines(contents)
                    self.newline()
                self.newline()

        super().endFeature()

    def appendSection(self, section, text):
        "Append a definition to the specified section"
        self.sections[section].append(text)
        self.feature_not_empty = True


    def genCmd(self, cmd_info, cmd_name, alias):
        """Generate command (vulkan function) encoders."""
        super().genCmd(cmd_info, cmd_name, alias)
        # self.appendSection('command', body)

if __name__ == '__main__':
    import pathlib
    import sys

    parser = argparse.ArgumentParser(
        description='Generate helper code from Khronos Vulkan registry (vk.xml)',
    )

    parser.add_argument('--output',
                        metavar='FILE',
                        default='vulkan_header.h',
                        type=pathlib.Path,
                        help='The path to a file to write the output to.')
    parser.add_argument('--vk-xml',
                        metavar='XML_FILE',
                        default='vk.xml',
                        type=pathlib.Path,
                        help='The location of Khronos Vulkan Registry vk.xml file.')

    args = parser.parse_args()

    options = reg.GeneratorOptions(
        conventions=VulkanConventions(),
        filename=args.output.name,
        directory=str(args.output.parent),
        apiname='vulkan',
        versions='VK_VERSION_1_0', # For now only generate Vulkan 1.0
        addExtensions='VK_KHR_swapchain',
        emitExtensions='VK_KHR_swapchain',
    )

    registry = reg.Registry(HelperOutputGenerator(), options)
    registry.loadElementTree(etree.parse(str(args.vk_xml)))
    registry.apiGen()
