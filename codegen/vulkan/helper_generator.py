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

from common import StructMembers, CommandMembers, print_vulkan_helper

try:
    import reg
except ModuleNotFoundError:
    print_vulkan_helper()


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
        if text != None and text != "":
            self.sections[section].append(text)
            self.feature_not_empty = True

    def genType(self, typeinfo: reg.TypeInfo, name: str, alias: bool):
        super().genType(typeinfo, name, alias)
        typeElem = typeinfo.elem

        # Vulkan:
        # Determine the category of the type, and the type section to add
        # its definition to.
        # 'funcpointer' is added to the 'struct' section as a workaround for
        # internal issue #877, since structures and function pointer types
        # can have cross-dependencies.
        category = typeElem.get('category')

        if category in ('struct', 'union'):
            # If the type is a struct type, generate it using the
            # special-purpose generator.
            self.genStruct(typeinfo, name, alias)

    def genStruct(self, typeinfo: reg.TypeInfo, typeName: str, alias: bool):
        """
          Represents a Vulkan Struct
        """
        super().genStruct(typeinfo, typeName, alias)
        self.appendSection('struct', self.enterStruct(
            typeName, StructMembers(typeinfo), alias))

    def genGroup(self, groupinfo: reg.GroupInfo, groupName: str, alias: bool):
        """
          Represents groups of enum values
        """
        super().genGroup(groupinfo, groupName, alias)
        # ! we can ignore groupinfo
        # TODO: enter group

    def genEnum(self, enuminfo: reg.EnumInfo, typeName: str, alias: bool):
        """
          Represents individual enum values
        """
        super().genEnum(enuminfo, typeName, alias)
        # ! we can ignore enuminfo
        # TODO: enter enum

    def genCmd(self, cmdInfo: reg.CmdInfo, cmdName: str, alias: bool):
        """
          Represents a vulkan command
        """
        super().genCmd(cmdInfo, cmdName, alias)
        self.appendSection('command', self.enterCmd(
            cmdName, CommandMembers(cmdInfo), alias))

    # TODO: more object types to enter

    def enterStruct(self, name: str, info: StructMembers, is_alias: bool) -> str:
        pass

    def enterHandle(self, name: str, is_alias: bool) -> str:
        pass

    def enterCmd(self, name: str, info: CommandMembers, is_alias: bool) -> str:
        pass
