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


#include "VulkanFilterYuvCompute.h"
#include "nvidia_utils/vulkan/ycbcrvkinfo.h"

VkResult VulkanFilterYuvCompute::Create(const VulkanDeviceContext* vkDevCtx,
                                        uint32_t queueFamilyIndex,
                                        uint32_t queueIndex,
                                        FilterType flterType,
                                        uint32_t maxNumFrames,
                                        VkFormat inputFormat,
                                        VkFormat outputFormat,
                                        const VkSamplerYcbcrConversionCreateInfo* pYcbcrConversionCreateInfo,
                                        const YcbcrPrimariesConstants* pYcbcrPrimariesConstants,
                                        const VkSamplerCreateInfo* pSamplerCreateInfo,
                                        VkSharedBaseObj<VulkanFilter>& vulkanFilter)
{

    VkSharedBaseObj<VulkanFilterYuvCompute> yCbCrVulkanFilter(new VulkanFilterYuvCompute(vkDevCtx,
                                                                                         queueFamilyIndex,
                                                                                         queueIndex,
                                                                                         flterType,
                                                                                         maxNumFrames,
                                                                                         inputFormat,
                                                                                         outputFormat,
                                                                                         pYcbcrPrimariesConstants));

    if (!yCbCrVulkanFilter) {
       return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    VkResult result = yCbCrVulkanFilter->Init(pYcbcrConversionCreateInfo, pSamplerCreateInfo);
    if (result != VK_SUCCESS) {
        return result;
    }

    vulkanFilter = yCbCrVulkanFilter;
    return VK_SUCCESS;
}

VkResult VulkanFilterYuvCompute::Init(const VkSamplerYcbcrConversionCreateInfo* pYcbcrConversionCreateInfo,
                                      const VkSamplerCreateInfo* pSamplerCreateInfo)
{

    VkResult result = VK_SUCCESS;
    if (pYcbcrConversionCreateInfo) {
         result = m_samplerYcbcrConversion.CreateVulkanSampler(pSamplerCreateInfo,
                                                               pYcbcrConversionCreateInfo);
         if (result != VK_SUCCESS) {
             assert(!"ERROR: samplerYcbcrConversion!");
             return result;
         }
    }

    assert(m_queue != VK_NULL_HANDLE);

    result = InitDescriptorSetLayout(m_maxNumFrames);
    if (result != VK_SUCCESS) {
        assert(!"ERROR: InitDescriptorSetLayout!");
        return result;
    }

    result = m_commandBuffersSet.CreateCommandBufferPool(m_queueFamilyIndex, m_maxNumFrames);
    if (result != VK_SUCCESS) {
        assert(!"ERROR: CreateCommandBufferPool!");
        return result;
    }

    result = m_filterWaitSemaphoreSet.CreateSet(m_maxNumFrames);
    if (result != VK_SUCCESS) {
        assert(!"ERROR: m_filterWaitSemaphoreSet.CreateSet!");
        return result;
    }

    // Before start recording the command buffer, the filter waits on the corresponding fence
    // For the first frame, however, there is not command buffer submission, so we need to create
    // the fence signaled. See RecordCommandBuffer()
    result = m_filterCompleteFenceSet.CreateSet(m_maxNumFrames, VK_FENCE_CREATE_SIGNALED_BIT);
    if (result != VK_SUCCESS) {
        assert(!"ERROR: CreateCommandBufferPool!");
        return result;
    }

    std::string computeShader;
    size_t computeShaderSize = 0;
    switch (m_filterType) {
     case YCBCRCOPY:
         computeShaderSize = InitYCBCRCOPY(computeShader);
         break;
     case YCBCRCLEAR:
         computeShaderSize = InitYCBCRCLEAR(computeShader);
         break;
     case YCBCR2RGBA:
         computeShaderSize = InitYCBCR2RGBA(computeShader);
         break;
     case RGBA2YCBCR:
         assert(!"TODO RGBA2YCBCR");
         break;
     default:
         assert(!"Invalid filter type");
         break;
    }

    return m_computePipeline.CreatePipeline(m_vulkanShaderCompiler,
                                            computeShader.c_str(), computeShaderSize,
                                            "main",
                                            m_workgroupSizeX, m_workgroupSizeY,
                                            &m_descriptorSetLayout);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VkResult VulkanFilterYuvCompute::InitDescriptorSetLayout(uint32_t maxNumFrames)
{

    VkSampler ccSampler = m_samplerYcbcrConversion.GetSampler();
    assert(ccSampler != VK_NULL_HANDLE);
    VkDescriptorType type = (ccSampler != VK_NULL_HANDLE) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    const VkSampler* pImmutableSamplers = (ccSampler != VK_NULL_HANDLE) ? &ccSampler : nullptr;

    const std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{
        //                        binding,  descriptorType,          descriptorCount, stageFlags, pImmutableSamplers;
        // Binding 0: Input image (read-only) RGBA or RGBA YCbCr sampler sampled
        VkDescriptorSetLayoutBinding{ 0, type,                             1, VK_SHADER_STAGE_COMPUTE_BIT, pImmutableSamplers},
        // Binding 1: Input image (read-only) Y plane of YCbCr Image
        VkDescriptorSetLayoutBinding{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        // Binding 2: Input image (read-only) Cb or CbCr plane
        VkDescriptorSetLayoutBinding{ 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        // Binding 3: Input image (read-only) Cr plane
        VkDescriptorSetLayoutBinding{ 3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},

        // Binding 4: Output image (write) RGBA or YCbCr single-plane image
        VkDescriptorSetLayoutBinding{ 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        // Binding 5: Output image (write) Y plane of YCbCr Image
        VkDescriptorSetLayoutBinding{ 5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        // Binding 6: Output image (write) CbCr plane of 2-plane or Cb of 3-plane YCbCr Image
        VkDescriptorSetLayoutBinding{ 6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        // Binding 7: Output image (write) Cr plane of 3-pane YCbCr Image
        VkDescriptorSetLayoutBinding{ 7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},

        // Binding 8: uniform buffer for input parameters.
        VkDescriptorSetLayoutBinding{ 8, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
    };

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT; // Stage the push constant is for
    pushConstantRange.offset = 0;
    pushConstantRange.size = 2 * sizeof(uint32_t); // Size of the push constant - source and destination image layers

    return m_descriptorSetLayout.CreateDescriptorSet(
                                                     setLayoutBindings,
                                                     VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
                                                     1, &pushConstantRange,
                                                     &m_samplerYcbcrConversion,
                                                     maxNumFrames,
                                                     false);
}

static YcbcrBtStandard GetYcbcrPrimariesConstantsId(VkSamplerYcbcrModelConversion modelConversion)
{
    switch (modelConversion) {
    case VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709:
        return YcbcrBtStandardBt709;
    case VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601:
        return YcbcrBtStandardBt601Ebu;
    case VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020:
        return YcbcrBtStandardBt709;
    default:
        ;// assert(0);
    }

    return YcbcrBtStandardUnknown;
}

size_t VulkanFilterYuvCompute::InitYCBCR2RGBA(std::string& computeShader)
{
    // The compute filter uses two input images as separate planes
    // Y (R) binding = 1
    // CbCr (RG) binding = 2
    // TODO: Add more YCbCr formats
    m_inputImageAspects = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT;

    // The compute filter uses RGBA output image with binding = 4
    m_outputImageAspects = VK_IMAGE_ASPECT_COLOR_BIT;

    // Create compute pipeline
    std::stringstream shaderStr;
    shaderStr << "#version 450\n"
                        "layout(push_constant) uniform PushConstants {\n"
                        "    uint srcImageLayer;\n"
                        "    uint dstImageLayer;\n"
                        "} pushConstants;\n"
                        "\n"
                        "layout (local_size_x = 16, local_size_y = 16) in;\n"
                        " // TODO: use set and binding from the layout\n"
                        " // TODO: use r16 for 16-bit formats\n"
                        "layout (set = 0, binding = 1, r8) uniform readonly image2DArray inputImageY;\n"
                        " // TODO: use rg16 for 16-bit formats\n"
                        "layout (set = 0, binding = 2, rg8) uniform readonly image2DArray inputImageCbCr;\n"
                        " // TODO: use rgba16 for 16-bit formats\n"
                        "layout (set = 0, binding = 4, rgba8) uniform writeonly image2DArray outImage;\n"
                        "\n"
                        " // TODO: normalize only narrow\n"
                        "float normalizeY(float Y) {\n"
                            "//    return (Y - (16.0 / 255.0)) * (255.0 / (235.0 - 16.0));\n"
                            "return (Y - 0.0627451) * 1.164383562;\n"
                        "}\n"
                        "\n"
                        "vec2 shiftCbCr(vec2 CbCr) {\n"
                        "    return CbCr - 0.5;\n"
                        "}\n"
                        "\n"
                        "vec3 shiftCbCr(vec3 ycbcr) {\n"
                        "    const vec3 shiftCbCr  = vec3(0.0, -0.5, -0.5);\n"
                        "    return ycbcr + shiftCbCr;\n"
                        "}\n"
                        "\n"
                        " // TODO: normalize only narrow\n"
                        "vec2 normalizeCbCr(vec2 CbCr) {\n"
                        "    // return (CbCr - (16.0 / 255.0)) / ((240.0 - 16.0) / 255.0);\n"
                        "    return (CbCr - 0.0627451) * 1.138392857;\n"
                        "}\n"
                        "\n";


    const VkSamplerYcbcrConversionCreateInfo& samplerYcbcrConversionCreateInfo = m_samplerYcbcrConversion.GetSamplerYcbcrConversionCreateInfo();
    const VkMpFormatInfo * mpInfo = YcbcrVkFormatInfo(samplerYcbcrConversionCreateInfo.format);
    const unsigned int bpp = (8 + mpInfo->planesLayout.bpp * 2);

    const YcbcrBtStandard btStandard = GetYcbcrPrimariesConstantsId(samplerYcbcrConversionCreateInfo.ycbcrModel);
    const YcbcrPrimariesConstants primariesConstants = GetYcbcrPrimariesConstants(btStandard);
    const YcbcrRangeConstants rangeConstants = GetYcbcrRangeConstants(YcbcrLevelsDigital);
    const YcbcrBtMatrix yCbCrMatrix(primariesConstants.kb,
                                    primariesConstants.kr,
                                    rangeConstants.cbMax,
                                    rangeConstants.crMax);


    shaderStr <<
        "vec3 convertYCbCrToRgb(vec3 yuv) {\n"
        "    vec3 rgb;\n";
    yCbCrMatrix.ConvertYCbCrToRgbString(shaderStr);
    shaderStr <<
        "    return rgb;\n"
        "}\n"
        "\n";


    YcbcrNormalizeColorRange yCbCrNormalizeColorRange(bpp,
            (samplerYcbcrConversionCreateInfo.ycbcrModel == VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY) ?
                    YCBCR_COLOR_RANGE_NATURAL : (YCBCR_COLOR_RANGE)samplerYcbcrConversionCreateInfo.ycbcrRange);
    shaderStr <<
        "vec3 normalizeYCbCr(vec3 yuv) {\n"
        "    vec3 yuvNorm;\n";
    yCbCrNormalizeColorRange.NormalizeYCbCrString(shaderStr);
    shaderStr <<
        "    return yuvNorm;\n"
        "}\n"
        "\n";

    shaderStr <<
        "void main()\n"
        "{\n"
        "    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
        "\n"
        "    // Fetch from the texture.\n"
        "    float Y = imageLoad(inputImageY, ivec3(pos, pushConstants.srcImageLayer)).r;\n"
        "    // TODO: it is /2 only for sub-sampled formats\n"
        "    vec2 CbCr = imageLoad(inputImageCbCr, ivec3(pos/2, pushConstants.srcImageLayer)).rg;\n"
        "\n"
        "    vec3 ycbcr = shiftCbCr(normalizeYCbCr(vec3(Y, CbCr)));\n"
        "    vec4 rgba = vec4(convertYCbCrToRgb(ycbcr),1.0);\n"
        "    // Store it back.\n"
        "    imageStore(outImage, ivec3(pos, pushConstants.dstImageLayer), rgba);\n"
        "}\n";

    computeShader = shaderStr.str();
    std::cout << "\nCompute Shader:\n" << computeShader;
    return computeShader.size();
}

size_t VulkanFilterYuvCompute::InitYCBCRCOPY(std::string& computeShader)
{
    // The compute filter uses two input images as separate planes
    // Y (R) binding = 1
    // CbCr (RG) binding = 2
    // TODO: Add more YCbCr formats
    m_inputImageAspects = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT;

    // The compute filter uses two output images as separate planes
    // Y (R) binding = 5
    // CbCr (RG) binding = 6
    // TODO: Add more YCbCr formats
    m_outputImageAspects = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT;

    std::stringstream shaderStr;
    // Create compute pipeline
    shaderStr << "#version 450\n"
                        "layout(push_constant) uniform PushConstants {\n"
                        "    uint srcImageLayer;\n"
                        "    uint dstImageLayer;\n"
                        "} pushConstants;\n"
                        "\n"
                        "layout (local_size_x = 16, local_size_y = 16) in;\n"
                        " // TODO: use set and binding from the layout\n"
                        " // TODO: use r16 for 16-bit formats\n"
                        "layout (set = 0, binding = 1, r8) uniform  readonly  image2DArray inputImageY;\n"
                        " // TODO: use rg16 for 16-bit formats\n"
                        "layout (set = 0, binding = 2, rg8) uniform readonly  image2DArray inputImageCbCr;\n"
                        " // TODO: use rgba16 for 16-bit formats\n"
                        "layout (set = 0, binding = 5, r8) uniform  writeonly image2DArray outImageY;\n"
                        " // TODO: use rg16 for 16-bit formats\n"
                        "layout (set = 0, binding = 6, rg8) uniform writeonly image2DArray outImageCbCr;\n"
                        "\n"
                        "\n";

    shaderStr <<
        "void main()\n"
        "{\n"
        "    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
        "\n"
        "    // Read Y value from source Y plane and write it to destination Y plane\n"
        "    float Y = imageLoad(inputImageY, ivec3(pos, pushConstants.srcImageLayer)).r;\n"
        "    imageStore(outImageY, ivec3(pos, pushConstants.dstImageLayer), vec4(Y, 0, 0, 1));\n"
        "\n"
        "    // Do the same for the CbCr plane, but remember about the 4:2:0 subsampling\n"
        "    if (pos % 2 == ivec2(0, 0)) {\n"
        "        pos /= 2;\n"
        "        vec2 CbCr = imageLoad(inputImageCbCr, ivec3(pos, pushConstants.srcImageLayer)).rg;\n"
        "        imageStore(outImageCbCr, ivec3(pos, pushConstants.dstImageLayer), vec4(CbCr, 0, 1));\n"
        "    }\n"
        "}\n";

    computeShader = shaderStr.str();
    std::cout << "\nCompute Shader:\n" << computeShader;
    return computeShader.size();
}

size_t VulkanFilterYuvCompute::InitYCBCRCLEAR(std::string& computeShader)
{
    // The compute filter uses NO input images
    m_inputImageAspects = VK_IMAGE_ASPECT_NONE;

    // The compute filter uses two output images as separate planes
    // Y (R) binding = 5
    // CbCr (RG) binding = 6
    // TODO: Add more YCbCr formats
    m_outputImageAspects = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT;

    // Create compute pipeline
    std::stringstream shaderStr;
    shaderStr << "#version 450\n"
                        "layout(push_constant) uniform PushConstants {\n"
                        "    uint srcImageLayer;\n"
                        "    uint dstImageLayer;\n"
                        "} pushConstants;\n"
                        "\n"
                        "layout (local_size_x = 16, local_size_y = 16) in;\n"
                        " // TODO: use rgba16 for 16-bit formats\n"
                        "layout (set = 0, binding = 5, r8) uniform writeonly image2DArray outImageY;\n"
                        " // TODO: use rg16 for 16-bit formats\n"
                        "layout (set = 0, binding = 6, rg8) uniform writeonly image2DArray outImageCbCr;\n"
                        "\n"
                        "\n";

    shaderStr <<
        "void main()\n"
        "{\n"
        "    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
        "\n"
        "    imageStore(outImageY, ivec3(pos, pushConstants.dstImageLayer), vec4(0.5, 0, 0, 1));\n"
        "\n"
        "    // Do the same for the CbCr plane, but remember about the 4:2:0 subsampling\n"
        "    if (pos % 2 == ivec2(0, 0)) {\n"
        "        pos /= 2;\n"
        "        imageStore(outImageCbCr, ivec3(pos, pushConstants.dstImageLayer), vec4(0.5, 0.5, 0.0, 1.0));\n"
        "    }\n"
        "}\n";

    computeShader = shaderStr.str();
    std::cout << "\nCompute Shader:\n" << computeShader;
    return computeShader.size();
}
