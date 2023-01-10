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

    lastUsedFeature = None

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
            'VkStructureType get_structure_type()\n' + \
            '{\n' + \
            '	throw "function not implemented";\n' + \
            '}\n'

        self.outFile.write(header)

    def endFile(self):

        self.outFile.write(
            '#endif\n' +  # end last feature
            '}        // namespace vulkan\n' +
            '}        // namespace components\n')

        super().endFile()

    def enterStruct(self, name: str, info: StructMembers, is_alias: bool) -> str:
        members = info.get_members()

        if is_alias:
            return None

        # A new feature is either the first feature of a set...
        is_first_feature = self.lastUsedFeature == None
        is_new_feature = is_first_feature
        nextFeatureName = self.featureName

        # ... Or the first change between feature sets
        if self.featureName is not self.lastUsedFeature:
            is_new_feature = True

        for member in members:
            if member.name == "sType":
                sTypeValue = member.elem.get("values")

                section = ""

                # end previous feature
                if is_new_feature:
                    if not is_first_feature:
                        # first endif isn't required
                        section += f'#endif\n'

                    section += f'#ifdef {nextFeatureName}\n'

                    # Crucial: do not remove
                    self.lastUsedFeature = self.featureName

                if sTypeValue:
                    section += 'template <>\n' + \
                        f'VkStructureType get_structure_type<{name}>()\n' + \
                        '{\n' + \
                        f'	return {sTypeValue};\n' + \
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
                        default='structure_type_helpers.hpp',
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
        # generate for the entire API
        versions='.*',
        addExtensions='.*',
        emitExtensions='.*',
    )

    registry = reg.Registry(StructureTypeNameGenerator(), options)
    registry.loadElementTree(etree.parse(str(args.vk_xml)))
    registry.apiGen()
