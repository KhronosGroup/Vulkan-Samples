#!/usr/bin/env python3

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

import os
import argparse
from common import print_vulkan_helper, StructMembers, gen_hpp_header
from helper_generator import HelperOutputGenerator

try:
    import reg
    from vkconventions import VulkanConventions
except ModuleNotFoundError:
    print_vulkan_helper()

from lxml import etree


class StructureTypeNameGenerator(HelperOutputGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def beginFile(self, genOpts):
        super().beginFile(genOpts)

        header = gen_hpp_header() + \
            '#include <volk.h>\n' + \
            '\n' + \
            'namespace components\n' + \
            '{\n' + \
            'namespace vulkan\n' + \
            '{\n' + \
            'template <typename Type>\n' + \
            'VkStructureType get_structure_type_name()\n' + \
            '{\n' + \
            '	throw "function not implemented";\n' + \
            '}\n'

        self.outFile.write(header)

    def endFile(self):

        self.outFile.write('}        // namespace vulkan\n' +
                           '}        // namespace components\n')

        super().endFile()

    def enterStruct(self, name: str, info: StructMembers, is_alias: bool) -> str:
        members = info.get_members()

        for member in members:
            if member.name == "sType":
                structureName = member.elem.get("values")

                if structureName:
                    section = 'template <>\n' + \
                        f'VkStructureType get_structure_type_name<{name}>()\n' + \
                        '{\n' + \
                        f'	return {structureName};\n' + \
                        '}\n'

                    return section

        return None


if __name__ == '__main__':
    import pathlib

    parser = argparse.ArgumentParser(
        description='Generate helper code from Khronos Vulkan registry (vk.xml)',
    )

    parser.add_argument('--output',
                        metavar='FILE',
                        default='vulkan_header.h',
                        type=pathlib.Path,
                        help='The path to a file to write the output to.')

    SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))

    parser.add_argument('--vk-xml',
                        metavar='XML_FILE',
                        # default to local vulkan registry
                        default=f'{SCRIPT_PATH}/../../third_party/vulkan/registry/vk.xml',
                        type=pathlib.Path,
                        help='The location of Khronos Vulkan Registry vk.xml file.')

    args = parser.parse_args()

    options = reg.GeneratorOptions(
        conventions=VulkanConventions(),
        filename=args.output.name,
        directory=str(args.output.parent),
        apiname='vulkan',
        versions='VK_VERSION_1_0',  # For now only generate Vulkan 1.0
        addExtensions='VK_KHR_swapchain',
        emitExtensions='VK_KHR_swapchain',
    )

    registry = reg.Registry(StructureTypeNameGenerator(), options)
    registry.loadElementTree(etree.parse(str(args.vk_xml)))
    registry.apiGen()
