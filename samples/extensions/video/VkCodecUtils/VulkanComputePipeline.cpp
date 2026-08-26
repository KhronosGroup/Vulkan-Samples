/*
 * Copyright 2023 NVIDIA Corporation.
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

#include "VulkanComputePipeline.h"

// Create Graphics Pipeline
VkResult VulkanComputePipeline::CreatePipeline(VulkanShaderCompiler& shaderCompiler,
                                               const char *shaderCode, size_t shaderSize,
                                               const char* pEntryName, // "main"
                                               uint32_t workgroupSizeX, // usually 16
                                               uint32_t workgroupSizeY, // usually 16
                                               const VulkanDescriptorSetLayout* pDescriptorSetLayout)
{
    if (m_pipelineCache == VK_NULL_HANDLE) {
        // Create the pipeline cache
        VkPipelineCacheCreateInfo pipelineCacheInfo = VkPipelineCacheCreateInfo();
        pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheInfo.pNext = nullptr;
        pipelineCacheInfo.initialDataSize = 0;
        pipelineCacheInfo.pInitialData = nullptr;
        pipelineCacheInfo.flags = 0;  // reserved, must be 0
        VkResult result = VulkanDeviceContext::GetThe()->CreatePipelineCache(VulkanDeviceContext::GetThe()->getDevice(), &pipelineCacheInfo, nullptr, &m_pipelineCache);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    DestroyShaderModule();
    m_shaderModule = shaderCompiler.BuildGlslShader(shaderCode,
                                                    shaderSize,
                                                    VK_SHADER_STAGE_COMPUTE_BIT);

    // Create the pipeline
    VkComputePipelineCreateInfo computePipelineCreateInfo {};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;


    const uint32_t specValues[] = {workgroupSizeX, workgroupSizeY};

                                               // constantID,     offset,                  size
    const VkSpecializationMapEntry specMapEntries[] {{ 0,            0,               sizeof(specValues[0])},
                                                     { 1,      sizeof(specValues[0]), sizeof(specValues[1])}};

    const VkSpecializationInfo specializationInfo { (uint32_t)(sizeof(specMapEntries)/sizeof(specMapEntries[0])), specMapEntries,
                                                     sizeof(specValues), specValues};

    // Specify the compute shader module and entry point
    computePipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computePipelineCreateInfo.stage.flags = 0;
    computePipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computePipelineCreateInfo.stage.module = m_shaderModule;
    computePipelineCreateInfo.stage.pName = pEntryName;
    computePipelineCreateInfo.stage.pSpecializationInfo = &specializationInfo;

    computePipelineCreateInfo.layout = pDescriptorSetLayout->GetPipelineLayout();
    computePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    computePipelineCreateInfo.basePipelineIndex = 0;

    // Make sure we destroy the existing pipeline, if it were to exist.
    DestroyPipeline();
    VkResult pipelineResult = VulkanDeviceContext::GetThe()->CreateComputePipelines(VulkanDeviceContext::GetThe()->getDevice(), m_pipelineCache, 1,
                                                                  &computePipelineCreateInfo,
                                                                  nullptr, &m_pipeline);

    return pipelineResult;
}
