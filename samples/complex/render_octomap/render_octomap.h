//
// Created by swinston on 12/12/24.
//

#ifndef RENDER_OCTOMAP_H
#define RENDER_OCTOMAP_H

#include "api_vulkan_sample.h"
#include <ImGUIUtil.h>
#include <chrono>
#include <memory>
#include <vector>

namespace octomap {
	class OcTree;
}

class render_octomap : public ApiVulkanSample
{
  public:
	render_octomap();
	~render_octomap() override;
	void BuildCubes();
	void build_command_buffers() override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void render(float delta_time) override;
	void createPipelines(VkRenderPass renderPass);
	void prepareUBO();
	void updateUBO();
	void generateMasterCube();
	void SetupVertexDescriptions();
	bool resize(const uint32_t width, const uint32_t height) override;

  private:
	struct
	{
		VkPipelineVertexInputStateCreateInfo           inputState;
		std::vector<VkVertexInputBindingDescription>   bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;
	struct InstanceData
	{
		float pos[3];
		float col[4];
		float scale{0.0f};
	};
	struct
	{
		glm::mat4 projection;
		glm::mat4 camera;
	} uboVS;
	std::unique_ptr<vkb::core::BufferC> vertexBuffer;
	std::unique_ptr<vkb::core::BufferC> indexBuffer;
	std::unique_ptr<vkb::core::BufferC> instanceBuffer;
	std::unique_ptr<vkb::core::BufferC> uniformBufferVS;
	uint32_t                            indexCount;
	VkPipelineCache                     pipelineCache;
	VkPipelineLayout                    pipelineLayout;
	VkPipeline                          pipeline;
	VkDescriptorPool                    descriptorPool;
	VkDescriptorSetLayout               descriptorSetLayout;
	VkDescriptorSet                     descriptorSet;
	octomap::OcTree                    *map;
	ImGUIUtil                          *gui;
	unsigned int                        mMaxTreeDepth;

	float                                              m_zMin;
	float                                              m_zMax;
	uint64_t                                           lastMapBuildSize;
	std::chrono::time_point<std::chrono::system_clock> lastBuildTime;
	std::vector<InstanceData>                          instances;
};

std::unique_ptr<vkb::Application> create_render_octomap();

#endif //RENDER_OCTOMAP_H
