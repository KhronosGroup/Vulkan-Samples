//
// Created by swinston on 12/12/24.
//

#include "render_octomap.h"
#include "octomap/octomap.h"

render_octomap::render_octomap()
	: vertices()
	, indexCount()
	, pipelineCache(VK_NULL_HANDLE)
	, pipelineLayout(VK_NULL_HANDLE)
	, pipeline(VK_NULL_HANDLE)
	, descriptorPool(VK_NULL_HANDLE)
	, descriptorSetLayout(VK_NULL_HANDLE)
	, descriptorSet(VK_NULL_HANDLE)
	, gui(nullptr)
	, mMaxTreeDepth()
	, m_zMin()
	, m_zMax()
	, lastMapBuildSize()
{
	title = "Octomap Viewer";
	map = new octomap::OcTree(0.1f);
}
render_octomap::~render_octomap()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptorSetLayout, nullptr);
		vkDestroyPipelineCache(get_device().get_handle(), pipelineCache, nullptr);
		vkDestroyDescriptorPool(get_device().get_handle(), descriptorPool, nullptr);
		delete gui;
		gui = nullptr;
	}
	delete map;
	map = nullptr;
}
void render_octomap::BuildCubes() {
  const octomap::OcTree *tree = map;
  if (tree->size() == 0) {
    return;
  }
  float nextBuildSize = static_cast<float>(lastMapBuildSize) + (static_cast<float>(lastMapBuildSize) * 0.05f);
  if (static_cast<float>(tree->size()) < nextBuildSize) {
    return;
  }

  // maximum size to prevent crashes on large maps: (should be checked in a better way than a constant)
//  bool showAll = ((double)tree->size() < 5 * 1e6);
  double minX, minY, minZ, maxX, maxY, maxZ;
  tree->getMetricMin(minX, minY, minZ);
  tree->getMetricMax(maxX, maxY, maxZ);

  // set min/max Z for color height map
  m_zMin = static_cast<float>(minZ);
  m_zMax = static_cast<float>(maxZ);

  //this is to get just grey; doing full color heightmap for now.
  //h = std::min(std::max((h-m_zMin)/ (m_zMax - m_zMin), 0.0f), 1.0f) * 0.4f + 0.3f; // h \in [0.3, 0.7]

  instances.clear();
  for (auto it = tree->begin_tree(mMaxTreeDepth), end = tree->end_tree(); it != end; ++it) {
//  for(auto it = tree->begin_leafs_bbx(bb_min, bb_max, mMaxTreeDepth), end=tree->end_leafs_bbx(); it!= end; ++it) {
    if (it.isLeaf() && tree->isNodeOccupied(*it)) {
//      if (tree->isNodeOccupied(*it) ) {
      glm::vec3 coords = {it.getCoordinate().x(), it.getCoordinate().y(), it.getCoordinate().z()};
      coords.y *= -1;
      InstanceData instance;
      instance.pos[0] = coords[0];
      instance.pos[1] = coords[1];
      instance.pos[2] = coords[2];
      float h = coords[2];
      if (m_zMin >= m_zMax)
        h = 0.5f;
      else {
        h = (1.0f - std::min(std::max((h - m_zMin) / (m_zMax - m_zMin), 0.0f), 1.0f)) * 0.8f;
      }

      // blend over HSV-values (more colors)
      float r, g, b;
      float s = 1.0f;
      float v = 1.0f;

      h -= floor(h);
      h *= 6;
      int i;
      float m, n, f;

      i = floor(h);
      f = h - static_cast<float>(i);
      if (!(i & 1))
        f = 1 - f; // if "i" is even
      m = v * (1 - s);
      n = v * (1 - s * f);

      switch (i) {
        case 6:
        case 0:r = v;
          g = n;
          b = m;
          break;
        case 1:r = n;
          g = v;
          b = m;
          break;
        case 2:r = m;
          g = v;
          b = n;
          break;
        case 3:r = m;
          g = n;
          b = v;
          break;
        case 4:r = n;
          g = m;
          b = v;
          break;
        case 5:r = v;
          g = m;
          b = n;
          break;
        default:r = 1;
          g = 0.5f;
          b = 0.5f;
          break;
      }

      instance.col[0] = r;
      instance.col[1] = g;
      instance.col[2] = b;
      instance.col[3] = 1.0f;
      instance.scale = 1.0f; // static_cast<float>(it.getSize());
	  instances.push_back(instance);
	}
  } // end for all voxels
  // Create buffers
  if (!instances.empty()) {
  	instanceBuffer = std::make_unique<vkb::core::BufferC>(get_device(),
																   instances.size() * sizeof(InstanceData),
																   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
																   VMA_MEMORY_USAGE_CPU_TO_GPU);

	auto buf = instanceBuffer->map();
	memcpy(buf, instances.data(), instances.size() * sizeof(InstanceData));
	instanceBuffer->flush();
	instanceBuffer->unmap();
  }
  // instance buffer


  lastBuildTime = std::chrono::system_clock::now();
  lastMapBuildSize = tree->size();
}

void render_octomap::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.033f, 0.0f}};
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;
	gui->newFrame(frame_count == 0);
	gui->updateBuffers();

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		// Render imGui
		gui->drawFrame(draw_cmd_buffers[i]);
		VkViewport viewport                = vkb::initializers::viewport(gui->MapsView.mapSize.x, gui->MapsView.mapSize.y, 0.0f, 1.0f);
		viewport.x = gui->MapsView.mapPos.x;
		viewport.y = gui->MapsView.mapPos.y;
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
		VkRect2D scissorRect;
		scissorRect.offset.x      = static_cast<int>(gui->MapsView.mapPos.x);
		scissorRect.offset.y      = static_cast<int>(gui->MapsView.mapPos.y);
		scissorRect.extent.width  = static_cast<int>(gui->MapsView.mapSize.x);
		scissorRect.extent.height = static_cast<int>(gui->MapsView.mapSize.y);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissorRect);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
								&descriptorSet, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, &vertexBuffer->get_handle(), offsets);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, &instanceBuffer->get_handle(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], indexBuffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);


		vkCmdDrawIndexed(draw_cmd_buffers[i], indexCount, instances.size(), 0, 0, 0);
		// draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}
bool render_octomap::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}
	camera.type = vkb::CameraType::LookAt;
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 256.0f);
	camera.set_rotation({0.0f, 0.0f, 0.0f});
	camera.set_translation({0.0f, 0.0f, -1.0f});
	map->readBinary("assets/scenes/octMap.bin");
	BuildCubes();
	gui = new ImGUIUtil(this);
	gui->init(static_cast<float>(width), static_cast<float>(height));
	gui->initResources(render_pass, queue);
	createPipelines(render_pass);
	build_command_buffers();
	prepared = true;
	return true;
}

void render_octomap::createPipelines(VkRenderPass renderPass) {
  SetupVertexDescriptions();
  prepareUBO();
  auto inputAssemblyState = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
  auto raster_State = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
  auto blendAttachmentState = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
  auto colorBlendState = vkb::initializers::pipeline_color_blend_state_create_info(1, &blendAttachmentState);
  auto depthStencilState = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
  auto viewportState = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);
  auto multisampleState = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

  std::vector dynamicStateEnables = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
      VK_DYNAMIC_STATE_LINE_WIDTH
  };

  auto dynamicState = vkb::initializers::pipeline_dynamic_state_create_info(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);

  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK(vkCreatePipelineCache(get_device().get_handle(), &pipelineCacheCreateInfo, nullptr, &pipelineCache));

  // Rendering pipeline
  std::vector poolSizes = {
      // Graphics pipelines uniform buffers
      vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2)
  };
  VkDescriptorPoolCreateInfo descriptorPoolInfo = vkb::initializers::descriptor_pool_create_info(poolSizes, 3);
  VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptorPoolInfo, nullptr, &descriptorPool));

  std::vector setLayoutBindings = {
      // Binding 0: Vertex shader uniform buffer
      vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
  };

  auto descriptorLayout = vkb::initializers::descriptor_set_layout_create_info(setLayoutBindings);
  VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptorLayout, nullptr, &descriptorSetLayout));

  auto pPipelineLayoutCreateInfo = vkb::initializers::pipeline_layout_create_info(&descriptorSetLayout, 1);
  VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));

  // Load shaders
  std::vector shaderStages = {
      load_shader("render_octomap", "render.vert", VK_SHADER_STAGE_VERTEX_BIT),
      load_shader("render_octomap", "render.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
  };

  auto pipelineCreateInfo = vkb::initializers::pipeline_create_info(pipelineLayout, renderPass, 0);

  pipelineCreateInfo.pVertexInputState = &vertices.inputState;
  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &raster_State;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;
  pipelineCreateInfo.stageCount = shaderStages.size();
  pipelineCreateInfo.pStages = shaderStages.data();
  pipelineCreateInfo.renderPass = renderPass;

  VK_CHECK(
      vkCreateGraphicsPipelines(get_device().get_handle(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));

  auto allocInfo = vkb::initializers::descriptor_set_allocate_info(descriptorPool, &descriptorSetLayout, 1);
  VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &allocInfo, &descriptorSet));
	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniformBufferVS);
  std::vector<VkWriteDescriptorSet> baseImageWriteDescriptorSets = {
      vkb::initializers::write_descriptor_set(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                       &buffer_descriptor)
  };

  vkUpdateDescriptorSets(get_device().get_handle(), baseImageWriteDescriptorSets.size(), baseImageWriteDescriptorSets.data(), 0, nullptr);
}

void render_octomap::prepareUBO() {
  // Vertex shader uniform buffer block
	uniformBufferVS = std::make_unique<vkb::core::BufferC>(get_device(),
																   sizeof(uboVS),
																   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
																   VMA_MEMORY_USAGE_CPU_TO_GPU);
  updateUBO();
}

void render_octomap::updateUBO() {
  uboVS.projection = camera.matrices.perspective;
  uboVS.camera = camera.matrices.view;

  uniformBufferVS->convert_and_update(uboVS);
}

// This just gives the first vertexBuffer
void render_octomap::generateMasterCube() {
  // Setup vertices for a single quad made from two triangles
  std::vector<Vertex> verticesLoc =
      {
          {{0.5f, 0.5f, 0.5f}},
          {{0.5f, 0.5f, -0.5f}},
          {{0.5f, -0.5f, 0.5f}},
          {{0.5f, -0.5f, -0.5f}},
          {{-0.5f, 0.5f, 0.5f}},
          {{-0.5f, 0.5f, -0.5f}},
          {{-0.5f, -0.5f, 0.5f}},
          {{-0.5f, -0.5f, -0.5f}}
      };

  // Setup indices
  std::vector<uint32_t> indices = {
      //right face
      0, 1, 3, 3, 2, 0,
      //left face
      4, 5, 7, 7, 6, 4,

      //Top face
      0, 1, 5, 5, 4, 0,
      //Bottom face
      2, 3, 7, 7, 6, 2,

      //Back face
      0, 2, 6, 6, 4, 0,
      //Front face
      1, 3, 7, 7, 5, 1
  };
  indexCount = static_cast<uint32_t>(indices.size());

  // Create buffers
  // For the sake of simplicity we won't stage the vertex data to the gpu memory
  // Vertex buffer
	vertexBuffer = std::make_unique<vkb::core::BufferC>(get_device(),
																   verticesLoc.size() * sizeof(Vertex),
																   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
																   VMA_MEMORY_USAGE_CPU_TO_GPU);

	auto buf = vertexBuffer->map();
	memccpy(buf, verticesLoc.data(), 0, verticesLoc.size() * sizeof(Vertex));
	vertexBuffer->flush();
	vertexBuffer->unmap();
  // Index buffer
	indexBuffer = std::make_unique<vkb::core::BufferC>(get_device(),
																	 indices.size() * sizeof(uint32_t),
																	 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
																	 VMA_MEMORY_USAGE_CPU_TO_GPU);
 	buf = indexBuffer->map();
 	memccpy(buf, indices.data(), 0, indices.size() * sizeof(uint32_t));
 	indexBuffer->flush();
 	indexBuffer->unmap();
}

void render_octomap::SetupVertexDescriptions()
{
	generateMasterCube();
	// Binding description
	vertices.bindingDescriptions = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)};

	// Attribute descriptions
	// Describes memory layout and shader positions
	vertices.attributeDescriptions = {
	    // Location 0: Position
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),

	    // Per-Instance attributes
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),                           // Location 1: Position
	    vkb::initializers::vertex_input_attribute_description(1, 2, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 3),        // Location 2: Color
	    vkb::initializers::vertex_input_attribute_description(1, 3, VK_FORMAT_R32_SFLOAT, sizeof(float) * 7),                 // Location 3: Scale
	};

	// Assign to vertex buffer
	vertices.inputState                                 = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertices.inputState.vertexBindingDescriptionCount   = vertices.bindingDescriptions.size();
	vertices.inputState.pVertexBindingDescriptions      = vertices.bindingDescriptions.data();
	vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
	vertices.inputState.pVertexAttributeDescriptions    = vertices.attributeDescriptions.data();
}
bool render_octomap::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	rebuild_command_buffers();
	return true;
}

void render_octomap::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	prepare_frame();

	// Update imGui
	ImGuiIO& io = ImGui::GetIO();

	io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
	io.DeltaTime = delta_time;

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	submit_frame();
	if (!paused || camera.updated)
	{
		updateUBO();
	}
	BuildCubes();
}

std::unique_ptr<vkb::Application> create_render_octomap()
{
	return std::make_unique<render_octomap>();
}