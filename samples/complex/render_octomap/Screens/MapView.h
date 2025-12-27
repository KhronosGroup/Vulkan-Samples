/* Copyright (c) 2024-2025, Holochip Inc.
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

#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "api_vulkan_sample.h"

class MapView {
public:
	// View states for different rendering modes
	enum class ViewState {
		Octomap,        // Default octomap rendering
		GLTFRegular,    // Regular GLTF map
		GLTFSplats      // Gaussian splats GLTF
	};

	MapView();
	~MapView();
	
	glm::vec2 mapPos;
	glm::vec2 mapSize;
	
	// Current view state
	ViewState currentState = ViewState::Octomap;
	
	// Flag to indicate view state changed
	bool stateChanged = false;

	std::vector<VkWriteDescriptorSet> LoadAssets(ApiVulkanSample *base, const VkDescriptorSetAllocateInfo &allocInfo, VkQueue copyQueue);

	bool DrawUI();
	
private:
	void DrawSidebar();
};

#endif //MAPVIEW_H
