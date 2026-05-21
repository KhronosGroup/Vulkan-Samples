/* Copyright (c) 2023, Holochip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "video.h"

#include <FrameProcessor.h>
#include <VulkanDecoderFrameProcessor.h>
#include <VulkanVideoProcessor.h>

video::video():
    pipeline(VK_NULL_HANDLE), pipeline_layout(VK_NULL_HANDLE), descriptor_set(VK_NULL_HANDLE), descriptor_set_layout(VK_NULL_HANDLE)
{
	title       = "Vulkan Video Decoding";
	camera.type = vkb::CameraType::LookAt;

	// Note: Using reversed depth-buffer for increased precision, so Z-Near and Z-Far are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 512.0f, 0.1f);
	camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 0.0f, -14.0f));
	camera.translation_speed = 2.5f;

	// VK_KHR_video_queue requires Vulkan 1.1
	set_api_version(VK_API_VERSION_1_1);

	// Vulkan Video required extensions
	add_device_extension(VK_EXT_YCBCR_2PLANE_444_FORMATS_EXTENSION_NAME);
	add_device_extension(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME);
	add_device_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
	add_device_extension(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME);
	add_device_extension(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME, true);
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, true);
	add_device_extension(VK_KHR_MAINTENANCE3_EXTENSION_NAME, true);
	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, true);
	add_device_extension(VK_KHR_DEVICE_GROUP_EXTENSION_NAME, true);
	add_device_extension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, true);
}

video::~video()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
		VulkanDeviceContext::GetThe()->Release();
	}
}
void video::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	ApiVulkanSample::request_gpu_features(gpu);
}

void video::draw()
{
	prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	const BackBuffer* backBuffer = (VulkanDeviceContext::GetThe()->currentBackBuffer >= 0) ? &m_backBuffers[VulkanDeviceContext::GetThe()->currentBackBuffer] : nullptr;

	bool contintueLoop = false;
	if (backBuffer != nullptr) {
		contintueLoop = m_frameProcessor->OnFrame(frame_count==0 ?
													  -(int32_t)backBuffer->GetImageIndex() :
													  backBuffer->GetImageIndex(),
												  1, // waitSemaphoreCount
												  backBuffer->GetAcquireSemaphore(),
												  1, // signalSemaphoreCount
												  &backBuffer->GetRenderSemaphore());
	} else {
		contintueLoop = m_frameProcessor->OnFrame(-1, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE);
	}

	if (!contintueLoop) {
	    LOGI("Video is complete");
	}

	if (backBuffer == nullptr) {
		return;
	}

	uint32_t imageIndex = backBuffer->GetImageIndex();
	const auto &queue = get_device().get_queue_by_present(0);

	VkSwapchainKHR sc = get_render_context().get_swapchain().get_handle();

	VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	present_info.pNext            = nullptr;
	present_info.swapchainCount   = 1;
	present_info.pSwapchains      = &sc;
	present_info.pImageIndices    = &imageIndex;

	VkDisplayPresentInfoKHR disp_present_info{};
	if (get_device().is_extension_supported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) &&
		window->get_display_present_info(&disp_present_info, width, height))
	{
		// Add display present info if supported and wanted
		present_info.pNext = &disp_present_info;
	}
    present_info.pWaitSemaphores    = &backBuffer->GetRenderSemaphore();
    present_info.waitSemaphoreCount = 1;

	VkResult present_result = queue.present(present_info);

	if (!((present_result == VK_SUCCESS) || (present_result == VK_SUBOPTIMAL_KHR)))
	{
		if (present_result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// Swap chain is no longer compatible with the surface and needs to be recreated
			resize(width, height);
			return;
		}
		VK_CHECK(present_result);
	}

	backBuffer->m_lastPresentTime = std::chrono::high_resolution_clock::now();
	static const std::chrono::nanoseconds targetDuration(12 * 1000 * 1000); // 16 mSec targeting ~60 FPS
	backBuffer->m_targetTimeDelta = targetDuration;
	backBuffer->m_framePresentAtTime = backBuffer->m_lastPresentTime + targetDuration;
}

void video::build_command_buffers()
{

}

bool video::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	ProgramConfig programConfig("vulkan_samples");
	programConfig.videoFileName = "assets/Videos/Holochip640x480.mp4";
	VulkanDeviceContext::GetThe()->InitVulkanDevice(programConfig.appName.c_str(),
													 programConfig.verbose);
	VulkanDeviceContext::GetThe()->setPhysicalDevice(get_device().get_gpu().get_handle());

    VkSharedBaseObj<VulkanVideoProcessor> vulkanVideoProcessor;
    auto result = VulkanVideoProcessor::Create(programConfig, vulkanVideoProcessor);
    if (result != VK_SUCCESS) {
        return false;
    }

    VkSharedBaseObj<VkVideoQueue<VulkanDecodedFrame>> videoQueue(vulkanVideoProcessor);

    result = CreateDecoderFrameProcessor(videoQueue, m_frameProcessor);
    if (result != VK_SUCCESS) {
        return false;
    }

	// Get decode queue
	decode.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_VIDEO_DECODE_BIT_KHR);
	vkGetDeviceQueue(get_device().get_handle(), decode.queue_family_index, 0, &decode.queue);

	// Get encode queue
	encode.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_VIDEO_ENCODE_BIT_KHR);
	vkGetDeviceQueue(get_device().get_handle(), encode.queue_family_index, 0, &encode.queue);

	// Get transfer queue
    transfer.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_TRANSFER_BIT);
    vkGetDeviceQueue(get_device().get_handle(), transfer.queue_family_index, 0, &transfer.queue);

    // Get compute queue
    compute.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_COMPUTE_BIT);
    vkGetDeviceQueue(get_device().get_handle(), compute.queue_family_index, 0, &compute.queue);

    VulkanDeviceContext::GetThe()->setDevice(get_device().get_handle(), -1, -1, transfer.queue, queue, queue, compute.queue, decode.queue, encode.queue);

	// BackBuffer is used to track which swapchain image and its associated
	// sync primitives are busy.  Having more BackBuffer's than swapchain
	// images may allow us to replace CPU wait on present_fence by GPU wait
	// on acquire_semaphore.
	constexpr int count = 5;
	m_backBuffers.resize(count);
	for (auto &backBuffer : m_backBuffers) {
		backBuffer.Create();
		AcquireBuffer * acquireBuffer;
		acquireBuffer->Create();
		m_acquireBuffers.push(acquireBuffer);
	}

	VulkanDeviceContext::GetThe()->currentBackBuffer = 0;
	VulkanDeviceContext::GetThe()->acquiredFrameId = 0;
	VulkanDeviceContext::GetThe()->setSwapchain(get_render_context().get_swapchain().get_handle());
	VulkanDeviceContext::GetThe()->setExtent(get_render_context().get_surface_extent());
	VulkanDeviceContext::GetThe()->setFormat(get_render_context().get_swapchain().get_surface_format());
	VulkanDeviceContext::GetThe()->setSurface(reinterpret_cast<VkSurfaceKHR *>(get_surface()));
	m_frameProcessor->AttachShell(count);

    vulkanVideoProcessor->Initialize(programConfig, decode.queue, encode.queue, VK_NULL_HANDLE, queue);

	return true;
}

void video::render(float delta_time)
{
	if (!prepared)
		return;


	    if(!m_acquireBuffers.empty()) {
        AcquireBuffer* acquireBuf = m_acquireBuffers.front();

        assert(acquireBuf != nullptr);

        uint32_t imageIndex = 0;

            VulkanDeviceContext::GetThe()->AcquireNextImageKHR(get_device().get_handle(),
             get_render_context().get_swapchain().get_handle(),
                      UINT64_MAX,
                      acquireBuf->m_semaphore, acquireBuf->m_fence,
                      &imageIndex);

        assert(imageIndex < m_backBuffers.size());
        BackBuffer& backBuffer = m_backBuffers[imageIndex];

        // wait until acquire and render semaphores are waited/unsignaled
        vkWaitForFences(get_device().get_handle(), 1, &acquireBuf->m_fence, true, UINT64_MAX);
        // reset the fence
        vkResetFences(get_device().get_handle(), 1, &acquireBuf->m_fence);

        VulkanDeviceContext::GetThe()->currentBackBuffer = imageIndex;
        m_acquireBuffers.pop();
        // Now return to the queue the old frame.
        AcquireBuffer* oldAcquireBuffer = backBuffer.SetAcquireBuffer(imageIndex, acquireBuf);
        if (oldAcquireBuffer) {
            m_acquireBuffers.push(oldAcquireBuffer);
        }
        VulkanDeviceContext::GetThe()->acquiredFrameId++;

    } else {
        // If the queue is empty - the is nothing that can be done here.
        VulkanDeviceContext::GetThe()->currentBackBuffer = -1;
        assert(!"Swapchain queue is empty!");
    }
	draw();
}

std::unique_ptr<vkb::VulkanSampleC> create_video()
{
	return std::make_unique<video>();
}
