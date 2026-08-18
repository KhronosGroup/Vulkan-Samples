add_subdirectory(NvVideoParser)

find_package(Vulkan COMPONENTS shaderc_combined REQUIRED)
find_package(FFmpeg COMPONENTS avcodec avformat REQUIRED)


add_library(VkCodecUtils STATIC
        Helpers.cpp
        HelpersDispatchTable.cpp
        nvVkFormats.cpp
        pattern.cpp
        VkBufferResource.cpp
        VkEncoderRenderFrame.cpp
        crcgenerator.cpp
        VkImageResource.cpp
        VulkanBistreamBufferImpl.cpp
        VulkanCommandBufferPool.cpp
        VulkanCommandBuffersSet.cpp
        VulkanComputePipeline.cpp
        VulkanDescriptorSetLayout.cpp
        VulkanDeviceContext.cpp
        VulkanDeviceMemoryImpl.cpp
        VulkanFilterYuvCompute.cpp
        VulkanFrame.cpp
        VulkanSamplerYcbcrConversion.cpp
        VulkanShaderCompiler.cpp
        VulkanVideoDisplayQueue.cpp
        VulkanVideoImagePool.cpp
        VulkanVideoProcessor.cpp
        VulkanVideoSession.cpp
        VulkanVideoSessionParameters.cpp
        VulkanVideoUtils.cpp
        VkDecoderUtils/ElementaryStream.cpp
        VkDecoderUtils/VideoStreamDemuxer.cpp
        VkDecoderUtils/FFmpegDemuxer.cpp
        VkVideoDecoder/VkVideoDecoder.cpp
        VkVideoDecoder/VkParserVideoPictureParameters.cpp
        VkVideoParser/VulkanVideoParser.cpp
        VulkanVideoFrameBuffer/VulkanVideoFrameBuffer.cpp
        BackBuffer.cpp
)

target_link_libraries(VkCodecUtils PUBLIC Vulkan::Vulkan Vulkan::shaderc_combined FFMPEG::avcodec FFMPEG::avformat nvidia-vkvideo-parser)
target_include_directories(VkCodecUtils PUBLIC include ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/..)
target_compile_definitions(VkCodecUtils PUBLIC VK_USE_VIDEO_QUEUE VK_USE_VIDEO_DECODE_QUEUE VK_USE_VIDEO_ENCODE_QUEUE)