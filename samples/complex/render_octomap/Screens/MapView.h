//
// Created by swinston on 12/13/24.
//

#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "api_vulkan_sample.h"

class MapView {
public:
	MapView();
	~MapView();
	glm::vec2 mapPos;
	glm::vec2 mapSize;

	std::vector<VkWriteDescriptorSet> LoadAssets(ApiVulkanSample *base, const VkDescriptorSetAllocateInfo &allocInfo, VkQueue copyQueue);

	bool DrawUI();
};



#endif //MAPVIEW_H
