/*
* Copyright 2020 NVIDIA Corporation.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "assert.h"
#include <iostream>
#include <fstream>

#include "VulkanShaderCompiler.h"
#include <shaderc/shaderc.h>
#include "Helpers.h"
#include "VkCodecUtils/VulkanDeviceContext.h"

// Translate Vulkan Shader Type to shaderc shader type
static shaderc_shader_kind getShadercShaderType(VkShaderStageFlagBits type)
{
    switch (type) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return shaderc_glsl_vertex_shader;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return shaderc_glsl_fragment_shader;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return shaderc_glsl_tess_control_shader;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return shaderc_glsl_tess_evaluation_shader;
    case VK_SHADER_STAGE_GEOMETRY_BIT:
        return shaderc_glsl_geometry_shader;
    case VK_SHADER_STAGE_COMPUTE_BIT:
        return shaderc_glsl_compute_shader;
    default:
        std::cerr << "VulkanShaderCompiler: " << "invalid VKShaderStageFlagBits" << "type = " <<  type;
    }
    return static_cast<shaderc_shader_kind>(-1);
}

VulkanShaderCompiler::VulkanShaderCompiler()
    : compilerHandle(0)
{
    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    compilerHandle = compiler;
}

VulkanShaderCompiler::~VulkanShaderCompiler() {

    if (compilerHandle) {
        shaderc_compiler_t compiler = (shaderc_compiler_t)compilerHandle;
        shaderc_compiler_release(compiler);
        compilerHandle = nullptr;
    }
}

VkShaderModule VulkanShaderCompiler::BuildGlslShader(const char *shaderCode, size_t shaderSize,
                                                     VkShaderStageFlagBits type)
{
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (compilerHandle) {
        shaderc_compiler_t compiler = (shaderc_compiler_t)compilerHandle;

        shaderc_compilation_result_t spvShader = shaderc_compile_into_spv(
                    compiler, shaderCode, shaderSize, getShadercShaderType(type),
                    "shaderc_error", "main", nullptr);
        if (shaderc_result_get_compilation_status(spvShader) !=
                shaderc_compilation_status_success) {

            std::cerr << "Compilation error: \n" << shaderc_result_get_error_message(spvShader) << std::endl;

            return VK_NULL_HANDLE;
        }

        // build vulkan shader module
        VkShaderModuleCreateInfo shaderModuleCreateInfo = VkShaderModuleCreateInfo();
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.pNext = nullptr;
        shaderModuleCreateInfo.codeSize = shaderc_result_get_length(spvShader);
        shaderModuleCreateInfo.pCode = (const uint32_t *)shaderc_result_get_bytes(spvShader);
        shaderModuleCreateInfo.flags = 0;
        VkResult result = VulkanDeviceContext::GetThe()->CreateShaderModule(VulkanDeviceContext::GetThe()->getDevice(), &shaderModuleCreateInfo, nullptr, &shaderModule);
        assert(result == VK_SUCCESS);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create shader module" << std::endl;
            return VK_NULL_HANDLE;
        }
        shaderc_result_release(spvShader);
    }
    return shaderModule;
}

// Create VK shader module from given glsl shader file
VkShaderModule VulkanShaderCompiler::BuildShaderFromFile(const char *fileName,
                                                         VkShaderStageFlagBits type)
{
    // read file from the path
    std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

    if (is.is_open()) {

        size_t size = is.tellg();
        is.seekg(0, std::ios::beg);
        char* shaderCode = new char[size];
        is.read(shaderCode, size);
        is.close();

        assert(size > 0);

        VkShaderModule shaderModule = BuildGlslShader(shaderCode, size, type);

        delete [] shaderCode;

        return shaderModule;
    }

    return VK_NULL_HANDLE;
}
