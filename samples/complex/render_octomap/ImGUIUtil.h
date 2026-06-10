/* Copyright (c) 2024-2026, Holochip Inc.
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

#ifndef IMGUI_UTIL_H
#define IMGUI_UTIL_H

#include "api_vulkan_sample.h"
#include "core/buffer.h"
#include "platform/input_events.h"

#include "Screens/MapView.h"
#include <imgui.h>

// ----------------------------------------------------------------------------
// ImGUI class
// ----------------------------------------------------------------------------

class ImGUIUtil
{
  private:
	// Vulkan resources for rendering the UI
	VkSampler                             sampler;
	std::unique_ptr<vkb::core::BufferC>   vertexBuffer;
	std::unique_ptr<vkb::core::BufferC>   indexBuffer;
	uint32_t                              vertexCount = 0;
	uint32_t                              indexCount  = 0;
	VkDeviceMemory                        fontMemory  = VK_NULL_HANDLE;
	std::unique_ptr<vkb::core::Image>     font_image;
	std::unique_ptr<vkb::core::ImageView> font_image_view;
	VkPipelineCache                       pipelineCache;
	VkPipelineLayout                      pipelineLayout;
	VkPipeline                            pipeline;
	VkDescriptorPool                      descriptorPool;
	VkDescriptorSetLayout                 descriptorSetLayout;
	VkDescriptorSet                       descriptorSet;
	VkPhysicalDeviceDriverProperties      driverProperties = {};
	ApiVulkanSample                      *base;
	ImGuiStyle                            vulkanStyle;
	ImFont                               *montserratExtraBoldNormal;
	ImFont                               *montserratExtraBoldSmall;
	ImFont                               *montserratBoldNormal;
	ImFont                               *montserratRegularNormal;
	int                                   selectedStyle = 0;
	float                                 windowWidth, windowHeight;
	bool                                  needsUpdateBuffers = false;

  public:
	enum ViewState
	{
		LIVEMAPS_ACTIVE,
	} state;

	MapView MapsView;

	// UI params are set via push constants
	struct PushConstBlock
	{
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

	explicit ImGUIUtil(ApiVulkanSample *_base);
	~ImGUIUtil();

	// Initialize styles, keys, etc.
	void        init(float width, float height);
	void        setStyle(uint32_t index);
	static void TextColorAlign(int align, const ImVec4 &col, const char *text, ...);

	// Initialize all Vulkan resources used by the ui
	void initResources(VkRenderPass renderPass, VkQueue copyQueue);

	// Starts a new imGui frame and sets up windows and ui elements
	bool newFrame(bool updateFrameGraph);

	// Update vertex and index buffer containing the imGui elements when required
	// Returns true if buffers were recreated (requiring command buffer rebuild)
	bool updateBuffers();

	// Draw current imGui frame into a command buffer
	void drawFrame(VkCommandBuffer commandBuffer);

	// Framework input path (platform-agnostic, incl. Direct-to-Display)
	static void handle_key_event(vkb::KeyCode code, vkb::KeyAction action);

	static bool GetWantKeyCapture();

	static void charPressed(uint32_t key);
};

#endif        // IMGUI_UTIL_H
