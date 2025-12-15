/* Copyright (c) 2025 Holochip Corporation
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
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_hpp_macros.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "camera_component.h"
#include "entity.h"
#include "memory_pool.h"
#include "mesh_component.h"
#include "model_loader.h"
#include "platform.h"
#include "thread_pool.h"

// Fallback defines for optional extension names (allow compiling against older headers)
#ifndef VK_EXT_ROBUSTNESS_2_EXTENSION_NAME
#	define VK_EXT_ROBUSTNESS_2_EXTENSION_NAME "VK_EXT_robustness2"
#endif
#ifndef VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME
#	define VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME "VK_KHR_dynamic_rendering_local_read"
#endif
#ifndef VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME
#	define VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME "VK_EXT_shader_tile_image"
#endif

// Forward declarations
class ImGuiSystem;

/**
 * @brief Structure for Vulkan queue family indices.
 */
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> computeFamily;
	std::optional<uint32_t> transferFamily;        // optional dedicated transfer queue family

	[[nodiscard]] bool isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
	}
};

/**
 * @brief Structure for swap chain support details.
 */
struct SwapChainSupportDetails
{
	vk::SurfaceCapabilitiesKHR        capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR>   presentModes;
};

/**
 * @brief Structure for individual light data in the storage buffer.
 */
struct LightData
{
	alignas(16) glm::vec4 position;                // Light position (w component used for direction vs position)
	alignas(16) glm::vec4 color;                   // Light color and intensity
	alignas(16) glm::mat4 lightSpaceMatrix;        // Light space matrix for shadow mapping
	alignas(4) int lightType;                      // 0=Point, 1=Directional, 2=Spot, 3=Emissive
	alignas(4) float range;                        // Light range
	alignas(4) float innerConeAngle;               // For spotlights
	alignas(4) float outerConeAngle;               // For spotlights
};

/**
 * @brief Structure for the uniform buffer object (now without fixed light arrays).
 */
struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec4 camPos;
	alignas(4) float exposure;
	alignas(4) float gamma;
	alignas(4) float prefilteredCubeMipLevels;
	alignas(4) float scaleIBLAmbient;
	alignas(4) int lightCount;
	alignas(4) int padding0;          // match shader UBO layout
	alignas(4) float padding1;        // match shader UBO layout
	alignas(4) float padding2;        // match shader UBO layout
	alignas(8) glm::vec2 screenDimensions;
	alignas(4) float nearZ;
	alignas(4) float farZ;
	alignas(4) float slicesZ;
	alignas(4) float _uboPad3;
	// Planar reflections
	alignas(16) glm::mat4 reflectionVP;        // projection * mirroredView
	alignas(4) int reflectionEnabled;          // 1 when sampling reflection in main pass
	alignas(4) int reflectionPass;             // 1 during reflection render pass
	alignas(8) glm::vec2 _reflectPad0;
	alignas(16) glm::vec4 clipPlaneWS;        // world-space plane ax+by+cz+d=0
	// Controls
	alignas(4) float reflectionIntensity;                 // scales reflection mix in glass
	alignas(4) int enableRayQueryReflections  = 1;        // 1 to enable reflections in ray query mode
	alignas(4) int enableRayQueryTransparency = 1;        // 1 to enable transparency/refraction in ray query mode
	alignas(4) float _padReflect[1]{};
	// Ray-query specific: number of per-instance geometry infos in buffer
	alignas(4) int geometryInfoCount{0};
	alignas(4) int _padGeo0{0};
	alignas(4) int _padGeo1{0};
	alignas(4) int _padGeo2{0};
	alignas(16) glm::vec4 _rqReservedWorldPos{0.0f, 0.0f, 0.0f, 0.0f};
	// Ray-query specific: number of materials in materialBuffer
	alignas(4) int materialCount{0};
	alignas(4) int _padMat0{0};
	alignas(4) int _padMat1{0};
	alignas(4) int _padMat2{0};
};

// Ray Query uses a dedicated uniform buffer with its own tightly-defined layout.
// This avoids relying on the (much larger) shared raster UBO layout and prevents
// CPU↔shader layout drift from breaking Ray Query-only fields.
//
// IMPORTANT: This layout must match `RayQueryUniforms` in `shaders/ray_query.slang`.
struct RayQueryUniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec4 camPos;

	alignas(4) float exposure;
	alignas(4) float gamma;
	// Match raster UBO conventions so Ray Query can run the same lighting math.
	alignas(4) float scaleIBLAmbient;
	alignas(4) int lightCount;
	alignas(4) int enableRayQueryReflections;
	alignas(4) int enableRayQueryTransparency;

	alignas(8) glm::vec2 screenDimensions;
	alignas(4) int geometryInfoCount;
	alignas(4) int materialCount;
	alignas(4) int _pad0;
};

static_assert(sizeof(RayQueryUniformBufferObject) == 256, "RayQueryUniformBufferObject size must match shader layout");
static_assert(offsetof(RayQueryUniformBufferObject, model) == 0);
static_assert(offsetof(RayQueryUniformBufferObject, view) == 64);
static_assert(offsetof(RayQueryUniformBufferObject, proj) == 128);
static_assert(offsetof(RayQueryUniformBufferObject, camPos) == 192);
static_assert(offsetof(RayQueryUniformBufferObject, exposure) == 208);
static_assert(offsetof(RayQueryUniformBufferObject, gamma) == 212);
static_assert(offsetof(RayQueryUniformBufferObject, scaleIBLAmbient) == 216);
static_assert(offsetof(RayQueryUniformBufferObject, lightCount) == 220);
static_assert(offsetof(RayQueryUniformBufferObject, enableRayQueryReflections) == 224);
static_assert(offsetof(RayQueryUniformBufferObject, enableRayQueryTransparency) == 228);
static_assert(offsetof(RayQueryUniformBufferObject, screenDimensions) == 232);
static_assert(offsetof(RayQueryUniformBufferObject, geometryInfoCount) == 240);
static_assert(offsetof(RayQueryUniformBufferObject, materialCount) == 244);
static_assert(offsetof(RayQueryUniformBufferObject, _pad0) == 248);

/**
 * @brief Structure for PBR material properties.
 * This structure must match the PushConstants structure in the PBR shader.
 */
struct MaterialProperties
{
	alignas(16) glm::vec4 baseColorFactor;
	alignas(4) float metallicFactor;
	alignas(4) float roughnessFactor;
	alignas(4) int baseColorTextureSet;
	alignas(4) int physicalDescriptorTextureSet;
	alignas(4) int normalTextureSet;
	alignas(4) int occlusionTextureSet;
	alignas(4) int emissiveTextureSet;
	alignas(4) float alphaMask;
	alignas(4) float alphaMaskCutoff;
	alignas(16) glm::vec3 emissiveFactor;        // Emissive factor for HDR emissive sources
	alignas(4) float emissiveStrength;           // KHR_materials_emissive_strength extension
	alignas(4) float transmissionFactor;         // KHR_materials_transmission
	alignas(4) int useSpecGlossWorkflow;         // 1 if using KHR_materials_pbrSpecularGlossiness
	alignas(4) float glossinessFactor;           // SpecGloss glossiness scalar
	alignas(16) glm::vec3 specularFactor;        // SpecGloss specular color factor
	alignas(4) float ior = 1.5f;                 // index of refraction
	alignas(4) bool hasEmissiveStrengthExtension;
};

/**
 * @brief Rendering mode selection
 */
enum class RenderMode
{
	Rasterization,        // Traditional rasterization pipeline
	RayQuery              // Ray query compute shader
};

/**
 * @brief Class for managing Vulkan rendering.
 *
 * This class implements the rendering pipeline as described in the Engine_Architecture chapter:
 * @see en/Building_a_Simple_Engine/Engine_Architecture/05_rendering_pipeline.adoc
 */
class Renderer
{
  public:
	/**
	 * @brief Constructor with a platform.
	 * @param platform The platform to use for rendering.
	 */
	explicit Renderer(Platform *platform);

	/**
	 * @brief Destructor for proper cleanup.
	 */
	~Renderer();

	/**
	 * @brief Initialize the renderer.
	 * @param appName The name of the application.
	 * @param enableValidationLayers Whether to enable validation layers.
	 * @return True if initialization was successful, false otherwise.
	 */
	bool Initialize(const std::string &appName, bool enableValidationLayers = true);

	/**
	 * @brief Clean up renderer resources.
	 */
	void Cleanup();

	/**
	 * @brief Render the scene.
	 * @param entities The entities to render.
	 * @param camera The camera to use for rendering.
	 * @param imguiSystem The ImGui system for UI rendering (optional).
	 */
	void Render(const std::vector<std::unique_ptr<Entity>> &entities, CameraComponent *camera, ImGuiSystem *imguiSystem = nullptr);

	/**
	 * @brief Wait for the device to be idle.
	 */
	void WaitIdle();

	/**
	 * @brief Dispatch a compute shader.
	 * @param groupCountX The number of local workgroups to dispatch in the X dimension.
	 * @param groupCountY The number of local workgroups to dispatch in the Y dimension.
	 * @param groupCountZ The number of local workgroups to dispatch in the Z dimension.
	 * @param inputBuffer The input buffer.
	 * @param outputBuffer The output buffer.
	 * @param hrtfBuffer The HRTF data buffer.
	 * @param paramsBuffer The parameters buffer.
	 * @return A fence that can be used to synchronize with the compute operation.
	 */
	vk::raii::Fence DispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
	                                vk::Buffer inputBuffer, vk::Buffer outputBuffer,
	                                vk::Buffer hrtfBuffer, vk::Buffer paramsBuffer);

	/**
	 * @brief Check if the renderer is initialized.
	 * @return True if the renderer is initialized, false otherwise.
	 */
	bool IsInitialized() const
	{
		return initialized;
	}

	/**
	 * @brief Get the Vulkan device.
	 * @return The Vulkan device.
	 */
	vk::Device GetDevice() const
	{
		return *device;
	}

	// Expose max frames in flight for per-frame resource duplication
	uint32_t GetMaxFramesInFlight() const
	{
		return MAX_FRAMES_IN_FLIGHT;
	}

	/**
	 * @brief Get the Vulkan RAII device.
	 * @return The Vulkan RAII device.
	 */
	const vk::raii::Device &GetRaiiDevice() const
	{
		return device;
	}

	// Expose uploads timeline semaphore and last value for external waits
	vk::Semaphore GetUploadsTimelineSemaphore() const
	{
		return *uploadsTimeline;
	}
	uint64_t GetUploadsTimelineValue() const
	{
		return uploadTimelineLastSubmitted.load(std::memory_order_relaxed);
	}

	/**
	 * @brief Get the compute queue.
	 * @return The compute queue.
	 */
	vk::Queue GetComputeQueue() const
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		return *computeQueue;
	}

	/**
	 * @brief Find a suitable memory type.
	 * @param typeFilter The type filter.
	 * @param properties The memory properties.
	 * @return The memory type index.
	 */
	uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
	{
		return findMemoryType(typeFilter, properties);
	}

	/**
	 * @brief Get the compute queue family index.
	 * @return The compute queue family index.
	 */
	uint32_t GetComputeQueueFamilyIndex() const
	{
		if (queueFamilyIndices.computeFamily.has_value())
		{
			return queueFamilyIndices.computeFamily.value();
		}
		// Fallback to graphics family to avoid crashes on devices without a separate compute queue
		return queueFamilyIndices.graphicsFamily.value();
	}

	/**
	 * @brief Submit a command buffer to the compute queue with proper dispatch loader preservation.
	 * @param commandBuffer The command buffer to submit.
	 * @param fence The fence to signal when the operation completes.
	 */
	void SubmitToComputeQueue(vk::CommandBuffer commandBuffer, vk::Fence fence) const
	{
		// Use mutex to ensure thread-safe access to queues
		vk::SubmitInfo submitInfo{
		    .commandBufferCount = 1,
		    .pCommandBuffers    = &commandBuffer};
		std::lock_guard<std::mutex> lock(queueMutex);
		// Prefer compute queue when available; otherwise, fall back to graphics queue to avoid crashes
		if (*computeQueue)
		{
			computeQueue.submit(submitInfo, fence);
		}
		else
		{
			graphicsQueue.submit(submitInfo, fence);
		}
	}

	/**
	 * @brief Create a shader module from SPIR-V code.
	 * @param code The SPIR-V code.
	 * @return The shader module.
	 */
	vk::raii::ShaderModule CreateShaderModule(const std::vector<char> &code)
	{
		return createShaderModule(code);
	}

	/**
	 * @brief Create a shader module from a file.
	 * @param filename The filename.
	 * @return The shader module.
	 */
	vk::raii::ShaderModule CreateShaderModule(const std::string &filename)
	{
		auto code = readFile(filename);
		return createShaderModule(code);
	}

	/**
	 * @brief Load a texture from a file.
	 * @param texturePath The path to the texture file.
	 * @return True if the texture was loaded successfully, false otherwise.
	 */
	bool LoadTexture(const std::string &texturePath);

	// Asynchronous texture loading APIs (thread-pool backed).
	// The 'critical' flag is used to front-load important textures (e.g.,
	// baseColor/albedo) so the scene looks mostly correct before the loading
	// screen disappears. Non-critical textures (normals, MR, AO, emissive)
	// can stream in after geometry is visible.
	std::future<bool> LoadTextureAsync(const std::string &texturePath, bool critical = false);

	/**
	 * @brief Load a texture from raw image data in memory.
	 * @param textureId The identifier for the texture.
	 * @param imageData The raw image data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param channels The number of channels in the image.
	 * @return True if the texture was loaded successfully, false otherwise.
	 */
	bool LoadTextureFromMemory(const std::string &textureId, const unsigned char *imageData,
	                           int width, int height, int channels);

	// Asynchronous upload from memory (RGBA/RGB/other). Safe for concurrent calls.
	std::future<bool> LoadTextureFromMemoryAsync(const std::string &textureId, const unsigned char *imageData,
	                                             int width, int height, int channels, bool critical = false);

	// Progress query for UI
	uint32_t GetTextureTasksScheduled() const
	{
		return textureTasksScheduled.load();
	}
	uint32_t GetTextureTasksCompleted() const
	{
		return textureTasksCompleted.load();
	}

	// GPU upload progress (per-texture jobs processed on the main thread).
	uint32_t GetUploadJobsTotal() const
	{
		return uploadJobsTotal.load();
	}
	uint32_t GetUploadJobsCompleted() const
	{
		return uploadJobsCompleted.load();
	}

	// Block until all currently-scheduled texture tasks have completed.
	// Intended for use during initial scene loading so that descriptor
	// creation sees the final textureResources instead of fallbacks.
	void WaitForAllTextureTasks();

	// Process pending texture GPU uploads on the calling thread.
	// This should be invoked from the main/render thread so that all
	// Vulkan work happens from a single thread while worker threads
	// perform only CPU-side decoding.
	//
	// Parameters allow us to:
	//  - limit the number of jobs processed per call (for streaming), and
	//  - choose whether to include critical and/or non-critical jobs.
	void ProcessPendingTextureJobs(uint32_t maxJobs            = UINT32_MAX,
	                               bool     includeCritical    = true,
	                               bool     includeNonCritical = true);

	// Track which entities use a given texture ID so that descriptor sets
	// can be refreshed when textures finish streaming in.
	void RegisterTextureUser(const std::string &textureId, Entity *entity);
	void OnTextureUploaded(const std::string &textureId);

	// Global loading state (model/scene). Consider the scene "loading" while
	// either the model is being parsed/instantiated OR there are still
	// outstanding critical texture uploads (e.g., baseColor/albedo).
	// Loading state: show blocking loading overlay only until the initial scene is ready.
	// Background streaming may continue after that without blocking the scene.
	bool IsLoading() const
	{
		return (loadingFlag.load() || criticalJobsOutstanding.load() > 0) && !initialLoadComplete.load();
	}
	void SetLoading(bool v)
	{
		loadingFlag.store(v);
		if (!v)
		{
			// Mark initial load complete; non-critical streaming can continue in background.
			initialLoadComplete.store(true);
		}
		else
		{
			// New load cycle starting
			initialLoadComplete.store(false);
		}
	}

	// Descriptor set deferred update machinery
	void MarkEntityDescriptorsDirty(Entity *entity);
	void ProcessDirtyDescriptorsForFrame(uint32_t frameIndex);
	bool updateDescriptorSetsForFrame(Entity            *entity,
	                                  const std::string &texturePath,
	                                  bool               usePBR,
	                                  uint32_t           frameIndex,
	                                  bool               imagesOnly = false,
	                                  bool               uboOnly    = false);

	// Texture aliasing: map canonical IDs to actual loaded keys (e.g., file paths) to avoid duplicates
	inline void RegisterTextureAlias(const std::string &aliasId, const std::string &targetId)
	{
		std::unique_lock<std::shared_mutex> lock(textureResourcesMutex);
		if (aliasId.empty() || targetId.empty())
			return;
		// Resolve targetId without re-locking by walking the alias map directly
		std::string resolved = targetId;
		for (int i = 0; i < 8; ++i)
		{
			auto it = textureAliases.find(resolved);
			if (it == textureAliases.end())
				break;
			if (it->second == resolved)
				break;
			resolved = it->second;
		}
		if (aliasId == resolved)
		{
			textureAliases.erase(aliasId);
		}
		else
		{
			textureAliases[aliasId] = resolved;
		}
	}
	inline std::string ResolveTextureId(const std::string &id) const
	{
		std::shared_lock<std::shared_mutex> lock(textureResourcesMutex);
		std::string                         cur = id;
		for (int i = 0; i < 8; ++i)
		{        // prevent pathological cycles
			auto it = textureAliases.find(cur);
			if (it == textureAliases.end())
				break;
			if (it->second == cur)
				break;        // self-alias guard
			cur = it->second;
		}
		return cur;
	}

	/**
	 * @brief Transition an image layout.
	 * @param image The image.
	 * @param format The image format.
	 * @param oldLayout The old layout.
	 * @param newLayout The new layout.
	 */
	void TransitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
	{
		transitionImageLayout(image, format, oldLayout, newLayout);
	}

	/**
	 * @brief Copy a buffer to an image.
	 * @param buffer The buffer.
	 * @param image The image.
	 * @param width The image width.
	 * @param height The image height.
	 */
	void CopyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
	{
		// Create a default single region for backward compatibility
		std::vector<vk::BufferImageCopy> regions = {{.bufferOffset      = 0,
		                                             .bufferRowLength   = 0,
		                                             .bufferImageHeight = 0,
		                                             .imageSubresource  = {
		                                                  .aspectMask     = vk::ImageAspectFlagBits::eColor,
		                                                  .mipLevel       = 0,
		                                                  .baseArrayLayer = 0,
		                                                  .layerCount     = 1},
		                                             .imageOffset = {0, 0, 0},
		                                             .imageExtent = {width, height, 1}}};
		copyBufferToImage(buffer, image, width, height, regions);
	}

	/**
	 * @brief Get the current command buffer.
	 * @return The current command buffer.
	 */
	vk::raii::CommandBuffer &GetCurrentCommandBuffer()
	{
		return commandBuffers[currentFrame];
	}

	/**
	 * @brief Get the swap chain image format.
	 * @return The swap chain image format.
	 */
	vk::Format GetSwapChainImageFormat() const
	{
		return swapChainImageFormat;
	}

	/**
	 * @brief Set the framebuffer resized flag.
	 * This should be called when the window is resized to trigger swap chain recreation.
	 */
	void SetFramebufferResized()
	{
		framebufferResized.store(true, std::memory_order_relaxed);
	}

	/**
	 * @brief Set the model loader reference for accessing extracted lights.
	 * @param _modelLoader Pointer to the model loader.
	 */
	void SetModelLoader(ModelLoader *_modelLoader)
	{
		modelLoader = _modelLoader;
	}

	/**
	 * @brief Set static lights loaded during model initialization.
	 * @param lights The lights to store statically.
	 */
	void SetStaticLights(const std::vector<ExtractedLight> &lights)
	{
		staticLights = lights;
		std::cout << "[Lights] staticLights set: " << staticLights.size() << " entries" << std::endl;
	}

	/**
	 * @brief Set the gamma correction value for PBR rendering.
	 * @param _gamma The gamma correction value (typically 2.2).
	 */
	void SetGamma(float _gamma)
	{
		gamma = _gamma;
	}

	/**
	 * @brief Set the exposure value for HDR tone mapping.
	 * @param _exposure The exposure value (1.0 = no adjustment).
	 */
	void SetExposure(float _exposure)
	{
		exposure = _exposure;
	}

	// Reflection intensity (UI + shader control)
	void SetReflectionIntensity(float v)
	{
		reflectionIntensity = v;
	}
	float GetReflectionIntensity() const
	{
		return reflectionIntensity;
	}

	void SetPlanarReflectionsEnabled(bool enabled);
	void TogglePlanarReflections();
	bool IsPlanarReflectionsEnabled() const
	{
		return enablePlanarReflections;
	}

	// Ray query rendering mode control
	void SetRenderMode(RenderMode mode)
	{
		currentRenderMode = mode;
	}
	RenderMode GetRenderMode() const
	{
		return currentRenderMode;
	}
	void ToggleRenderMode()
	{
		currentRenderMode = (currentRenderMode == RenderMode::Rasterization) ? RenderMode::RayQuery : RenderMode::Rasterization;
	}

	// Ray query capability getters
	bool GetRayQueryEnabled() const
	{
		return rayQueryEnabled;
	}
	bool GetAccelerationStructureEnabled() const
	{
		return accelerationStructureEnabled;
	}

	// Ray Query static-only mode (disable animation/physics updates and TLAS refits to render a static opaque scene)
	void SetRayQueryStaticOnly(bool v)
	{
		rayQueryStaticOnly = v;
	}
	bool IsRayQueryStaticOnly() const
	{
		return rayQueryStaticOnly;
	}

	/**
	 * @brief Request acceleration structure build at next safe frame point.
	 * Safe to call from any thread (e.g., background loading thread).
	 */
	void RequestAccelerationStructureBuild()
	{
		asBuildRequested.store(true, std::memory_order_release);
	}
	// Overload with reason tracking for diagnostics
	void RequestAccelerationStructureBuild(const char *reason)
	{
		if (reason)
			lastASBuildRequestReason = reason;
		else
			lastASBuildRequestReason = "(no reason)";
		asBuildRequested.store(true, std::memory_order_release);
	}

	/**
	 * @brief Build acceleration structures for ray query rendering.
	 * @param entities The entities to include in the acceleration structures.
	 * @return True if successful, false otherwise.
	 */
	bool buildAccelerationStructures(const std::vector<std::unique_ptr<Entity>> &entities);

	// Refit/UPDATE the TLAS with latest entity transforms (no rebuild)
	bool refitTopLevelAS(const std::vector<std::unique_ptr<Entity>> &entities);

	/**
	 * @brief Update ray query descriptor sets with current resources.
	 * @param frameIndex The frame index to update (or all frames if not specified).
	 * @return True if successful, false otherwise.
	 */
	bool updateRayQueryDescriptorSets(uint32_t frameIndex, const std::vector<std::unique_ptr<Entity>> &entities);

	/**
	 * @brief Create or resize light storage buffers to accommodate the given number of lights.
	 * @param lightCount The number of lights to accommodate.
	 * @return True if successful, false otherwise.
	 */
	bool createOrResizeLightStorageBuffers(size_t lightCount);

	/**
	 * @brief Update the light storage buffer with current light data.
	 * @param frameIndex The current frame index.
	 * @param lights The light data to upload.
	 * @return True if successful, false otherwise.
	 */
	bool updateLightStorageBuffer(uint32_t frameIndex, const std::vector<ExtractedLight> &lights);

	/**
	 * @brief Update all existing descriptor sets with new light storage buffer references.
	 * Called when light storage buffers are recreated to ensure descriptor sets reference valid buffers.
	 */
	// Update PBR descriptor sets to point to the latest light SSBOs.
	// When allFrames=true, refresh all frames (use only when the device is idle — e.g., after waitIdle()).
	// Otherwise, refresh only the current frame at the frame safe point to avoid touching in‑flight frames.
	void updateAllDescriptorSetsWithNewLightBuffers(bool allFrames = false);

	// Upload helper: record both layout transitions and the copy in a single submit with a fence
	void uploadImageFromStaging(vk::Buffer                              staging,
	                            vk::Image                               image,
	                            vk::Format                              format,
	                            const std::vector<vk::BufferImageCopy> &regions,
	                            uint32_t                                mipLevels = 1);

	// Generate full mip chain for a 2D color image using GPU blits
	void generateMipmaps(vk::Image  image,
	                     vk::Format format,
	                     int32_t    texWidth,
	                     int32_t    texHeight,
	                     uint32_t   mipLevels);

	vk::Format findDepthFormat();

	/**
	 * @brief Pre-allocate all Vulkan resources for an entity during scene loading.
	 * @param entity The entity to pre-allocate resources for.
	 * @return True if pre-allocation was successful, false otherwise.
	 */
	bool preAllocateEntityResources(Entity *entity);

	/**
	 * @brief Pre-allocate Vulkan resources for a batch of entities, batching mesh uploads.
	 *
	 * This variant is optimized for large scene loads (e.g., GLTF Bistro). It will:
	 *  - Create per-mesh GPU buffers as usual, but record all buffer copy commands
	 *    into a single command buffer and submit them in one batch.
	 *  - Then create uniform buffers and descriptor sets per entity.
	 *
	 * Callers that load many geometry entities at once (like GLTF scene loading)
	 * should prefer this over repeated preAllocateEntityResources() calls.
	 */
	bool preAllocateEntityResourcesBatch(const std::vector<Entity *> &entities);

	/**
	 * @brief Recreate the instance buffer for an entity that had its instances cleared.
	 *
	 * When an entity that was originally set up for instanced rendering needs to be
	 * converted to a single non-instanced entity (e.g., for animation), this method
	 * recreates the GPU instance buffer with a single identity instance.
	 *
	 * @param entity The entity whose instance buffer should be recreated.
	 * @return True if successful, false otherwise.
	 */
	bool recreateInstanceBuffer(Entity *entity);

	// Shared default PBR texture identifiers (to avoid creating hundreds of identical textures)
	static const std::string SHARED_DEFAULT_ALBEDO_ID;
	static const std::string SHARED_DEFAULT_NORMAL_ID;
	static const std::string SHARED_DEFAULT_METALLIC_ROUGHNESS_ID;
	static const std::string SHARED_DEFAULT_OCCLUSION_ID;
	static const std::string SHARED_DEFAULT_EMISSIVE_ID;
	static const std::string SHARED_BRIGHT_RED_ID;

	/**
	 * @brief Determine the appropriate texture format based on the texture type.
	 * @param textureId The texture identifier to analyze.
	 * @return The appropriate Vulkan format (sRGB for baseColor, linear for others).
	 */
	static vk::Format determineTextureFormat(const std::string &textureId);

  private:
	// Platform
	Platform *platform = nullptr;

	// Model loader reference for accessing extracted lights
	class ModelLoader *modelLoader = nullptr;

	// PBR rendering parameters
	float gamma               = 2.2f;        // Gamma correction value
	float exposure            = 1.2f;        // HDR exposure value (default tuned to avoid washout)
	float reflectionIntensity = 1.0f;        // User control for glass reflection strength

	// Ray Query tuning
	int rayQueryMaxBounces = 1;        // 0 = no secondary rays, 1 = one-bounce reflection/refraction

	// Vulkan RAII context
	vk::raii::Context context;

	// Vulkan instance and debug messenger
	vk::raii::Instance               instance       = nullptr;
	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;

	// Vulkan device
	vk::raii::PhysicalDevice physicalDevice = nullptr;
	vk::raii::Device         device         = nullptr;

	// Memory pool for efficient memory management
	std::unique_ptr<MemoryPool> memoryPool;

	// Vulkan queues
	vk::raii::Queue graphicsQueue = nullptr;
	vk::raii::Queue presentQueue  = nullptr;
	vk::raii::Queue computeQueue  = nullptr;

	// Vulkan surface
	vk::raii::SurfaceKHR surface = nullptr;

	// Swap chain
	vk::raii::SwapchainKHR           swapChain = nullptr;
	std::vector<vk::Image>           swapChainImages;
	vk::Format                       swapChainImageFormat = vk::Format::eUndefined;
	vk::Extent2D                     swapChainExtent      = {0, 0};
	std::vector<vk::raii::ImageView> swapChainImageViews;
	// Tracked layouts for swapchain images (VVL requires correct oldLayout in barriers).
	// Initialized at swapchain creation and updated as we transition.
	std::vector<vk::ImageLayout> swapChainImageLayouts;

	// Dynamic rendering info
	vk::RenderingInfo                        renderingInfo;
	std::vector<vk::RenderingAttachmentInfo> colorAttachments;
	vk::RenderingAttachmentInfo              depthAttachment;

	// Pipelines
	vk::raii::PipelineLayout pipelineLayout           = nullptr;
	vk::raii::Pipeline       graphicsPipeline         = nullptr;
	vk::raii::PipelineLayout pbrPipelineLayout        = nullptr;
	vk::raii::Pipeline       pbrGraphicsPipeline      = nullptr;
	vk::raii::Pipeline       pbrBlendGraphicsPipeline = nullptr;
	// Transparent PBR pipeline variant for premultiplied alpha content
	vk::raii::Pipeline pbrPremulBlendGraphicsPipeline = nullptr;
	// Opaque PBR pipeline variant used after a depth pre-pass (depth read-only, compare with pre-pass depth)
	vk::raii::Pipeline pbrPrepassGraphicsPipeline = nullptr;
	// Reflection PBR pipeline used for mirrored off-screen pass (cull none to avoid winding issues)
	vk::raii::Pipeline pbrReflectionGraphicsPipeline = nullptr;
	// Specialized pipeline for architectural glass (windows, lamp glass, etc.).
	// Shares descriptor layouts and vertex input with the PBR pipelines but uses
	// a dedicated fragment shader entry point for more stable glass shading.
	vk::raii::Pipeline       glassGraphicsPipeline  = nullptr;
	vk::raii::PipelineLayout lightingPipelineLayout = nullptr;
	vk::raii::Pipeline       lightingPipeline       = nullptr;

	// Fullscreen composite pipeline to draw the opaque off-screen color to the swapchain
	// (used to avoid gamma-incorrect vkCmdCopyImage and to apply tone mapping when desired).
	vk::raii::PipelineLayout             compositePipelineLayout      = nullptr;
	vk::raii::Pipeline                   compositePipeline            = nullptr;
	vk::raii::DescriptorSetLayout        compositeDescriptorSetLayout = nullptr;        // not used; reuse transparentDescriptorSetLayout
	std::vector<vk::raii::DescriptorSet> compositeDescriptorSets;                       // unused; reuse transparentDescriptorSets

	// Pipeline rendering create info structures (for proper lifetime management)
	vk::PipelineRenderingCreateInfo mainPipelineRenderingCreateInfo;
	vk::PipelineRenderingCreateInfo pbrPipelineRenderingCreateInfo;
	vk::PipelineRenderingCreateInfo lightingPipelineRenderingCreateInfo;
	vk::PipelineRenderingCreateInfo compositePipelineRenderingCreateInfo;

	// Create composite pipeline
	bool createCompositePipeline();

	// Compute pipeline
	vk::raii::PipelineLayout             computePipelineLayout      = nullptr;
	vk::raii::Pipeline                   computePipeline            = nullptr;
	vk::raii::DescriptorSetLayout        computeDescriptorSetLayout = nullptr;
	vk::raii::DescriptorPool             computeDescriptorPool      = nullptr;
	std::vector<vk::raii::DescriptorSet> computeDescriptorSets;
	vk::raii::CommandPool                computeCommandPool = nullptr;

	// Thread safety for queue access - unified mutex since queues may share the same underlying VkQueue
	mutable std::mutex queueMutex;
	// Thread safety for descriptor pool/set operations across all engine threads
	mutable std::mutex descriptorMutex;
	// Monotonic generation counter for descriptor pool rebuilds (future use for hardening)
	std::atomic<uint64_t> descriptorPoolGeneration{0};

	// Command pool and buffers
	vk::raii::CommandPool                commandPool = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffers;
	// Protect usage of shared commandPool for transient command buffers
	mutable std::mutex commandMutex;

	// Dedicated transfer queue (falls back to graphics if unavailable)
	vk::raii::Queue transferQueue = nullptr;

	// Synchronization objects
	std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	std::vector<vk::raii::Fence>     inFlightFences;

	// Upload timeline semaphore for transfer -> graphics handoff (signaled per upload)
	vk::raii::Semaphore uploadsTimeline = nullptr;
	// Tracks last timeline value that has been submitted for signaling on uploadsTimeline
	std::atomic<uint64_t> uploadTimelineLastSubmitted{0};

	// Depth buffer
	vk::raii::Image                         depthImage           = nullptr;
	std::unique_ptr<MemoryPool::Allocation> depthImageAllocation = nullptr;
	vk::raii::ImageView                     depthImageView       = nullptr;

	// Forward+ configuration
	bool                      useForwardPlus       = true;        // default enabled
	uint32_t                  forwardPlusTileSizeX = 16;
	uint32_t                  forwardPlusTileSizeY = 16;
	uint32_t                  forwardPlusSlicesZ   = 16;         // clustered depth slices
	static constexpr uint32_t MAX_LIGHTS_PER_TILE  = 256;        // conservative cap

	struct TileHeader
	{
		uint32_t offset;        // into tileLightIndices
		uint32_t count;         // number of indices for this tile
		uint32_t pad0;
		uint32_t pad1;
	};

	struct ForwardPlusPerFrame
	{
		// SSBOs for per-tile light lists
		vk::raii::Buffer                        tileHeaders           = nullptr;
		std::unique_ptr<MemoryPool::Allocation> tileHeadersAlloc      = nullptr;
		vk::raii::Buffer                        tileLightIndices      = nullptr;
		std::unique_ptr<MemoryPool::Allocation> tileLightIndicesAlloc = nullptr;
		size_t                                  tilesCapacity         = 0;        // number of tiles allocated
		size_t                                  indicesCapacity       = 0;        // number of indices allocated

		// Uniform buffer with view/proj, screen size, tile size, etc.
		vk::raii::Buffer                        params       = nullptr;
		std::unique_ptr<MemoryPool::Allocation> paramsAlloc  = nullptr;
		void                                   *paramsMapped = nullptr;

		// Optional compute debug output buffer (uints), host-visible
		vk::raii::Buffer                        debugOut                 = nullptr;
		std::unique_ptr<MemoryPool::Allocation> debugOutAlloc            = nullptr;
		bool                                    debugOutAwaitingReadback = false;

		// One-frame color probes (host-visible, small buffers)
		vk::raii::Buffer                        probeOffscreen        = nullptr;
		std::unique_ptr<MemoryPool::Allocation> probeOffscreenAlloc   = nullptr;
		vk::raii::Buffer                        probeSwapchain        = nullptr;
		std::unique_ptr<MemoryPool::Allocation> probeSwapchainAlloc   = nullptr;
		bool                                    probeAwaitingReadback = false;

		// Compute descriptor set for culling
		vk::raii::DescriptorSet computeSet = nullptr;
	};
	std::vector<ForwardPlusPerFrame> forwardPlusPerFrame;        // size MAX_FRAMES_IN_FLIGHT
	// Per-frame light count used by shaders (set once before main pass)
	uint32_t lastFrameLightCount = 0;

	// Forward+ compute resources
	vk::raii::PipelineLayout      forwardPlusPipelineLayout      = nullptr;
	vk::raii::Pipeline            forwardPlusPipeline            = nullptr;
	vk::raii::DescriptorSetLayout forwardPlusDescriptorSetLayout = nullptr;

	// Depth pre-pass pipeline
	vk::raii::Pipeline depthPrepassPipeline = nullptr;

	// Ray query rendering mode
	RenderMode currentRenderMode = RenderMode::RayQuery;

	// Ray query pipeline and resources
	vk::raii::PipelineLayout             rayQueryPipelineLayout      = nullptr;
	vk::raii::Pipeline                   rayQueryPipeline            = nullptr;
	vk::raii::DescriptorSetLayout        rayQueryDescriptorSetLayout = nullptr;
	std::vector<vk::raii::DescriptorSet> rayQueryDescriptorSets;

	// Dedicated ray query UBO (one per frame in flight) - separate from entity UBOs
	std::vector<vk::raii::Buffer>                        rayQueryUniformBuffers;
	std::vector<std::unique_ptr<MemoryPool::Allocation>> rayQueryUniformAllocations;
	std::vector<void *>                                  rayQueryUniformBuffersMapped;

	// Ray query output image (storage image for compute shader output)
	vk::raii::Image                         rayQueryOutputImage           = nullptr;
	std::unique_ptr<MemoryPool::Allocation> rayQueryOutputImageAllocation = nullptr;
	vk::raii::ImageView                     rayQueryOutputImageView       = nullptr;

	// Acceleration structures for ray query
	struct AccelerationStructure
	{
		vk::raii::Buffer                        buffer        = nullptr;
		std::unique_ptr<MemoryPool::Allocation> allocation    = nullptr;
		vk::raii::AccelerationStructureKHR      handle        = nullptr;        // Use RAII for proper lifetime management
		vk::DeviceAddress                       deviceAddress = 0;
	};
	std::vector<AccelerationStructure> blasStructures;        // Bottom-level AS (one per mesh)
	AccelerationStructure              tlasStructure;         // Top-level AS (scene)

	// Deferred deletion queue for old AS structures
	// Keeps old AS buffers alive until all frames in flight have finished using them
	struct PendingASDelete
	{
		std::vector<AccelerationStructure> blasStructures;
		AccelerationStructure              tlasStructure;
		uint32_t                           framesSinceDestroy = 0;        // Increment each frame, delete when >= MAX_FRAMES_IN_FLIGHT
	};
	std::vector<PendingASDelete> pendingASDeletions;

	// GPU data structures for ray query proper normal and material access
	struct GeometryInfo
	{
		uint64_t vertexBufferAddress;        // Device address of vertex buffer
		uint64_t indexBufferAddress;         // Device address of index buffer
		uint32_t vertexCount;                // Number of vertices
		uint32_t materialIndex;              // Index into material buffer
		uint32_t indexCount;                 // Number of indices (to bound primitiveIndex in shader)
		uint32_t _pad0;
		// Instance-space -> world-space normal transform (3 columns). Matches raster convention.
		// Stored as float4 columns (xyz used, w unused) for stable std430 layout.
		alignas(16) glm::vec4 normalMatrix0;
		alignas(16) glm::vec4 normalMatrix1;
		alignas(16) glm::vec4 normalMatrix2;
	};

	struct MaterialData
	{
		alignas(16) glm::vec3 albedo;
		alignas(4) float metallic;
		alignas(16) glm::vec3 emissive;
		alignas(4) float roughness;
		alignas(4) float ao;
		alignas(4) float ior;
		alignas(4) float emissiveStrength;
		alignas(4) float alpha;
		alignas(4) float transmissionFactor;
		alignas(4) float alphaCutoff;
		// glTF alpha mode encoding (matches shader): 0=OPAQUE, 1=MASK, 2=BLEND
		alignas(4) int32_t alphaMode;
		alignas(4) uint32_t isGlass;         // bool as uint32
		alignas(4) uint32_t isLiquid;        // bool as uint32

		// Raster parity: texture-set flags (-1 = no texture; 0 = sample from binding 6 table).
		// Ray Query uses a single texture table (binding 6); indices are always valid even when
		// the set flag is -1, so the shader can choose the correct no-texture behavior.
		alignas(4) int32_t baseColorTextureSet;
		alignas(4) int32_t physicalDescriptorTextureSet;
		alignas(4) int32_t normalTextureSet;
		alignas(4) int32_t occlusionTextureSet;
		alignas(4) int32_t emissiveTextureSet;

		// Ray Query texture table indices (binding 6). These always reference a valid descriptor
		// (real streamed texture or a shared default slot).
		alignas(4) int32_t baseColorTexIndex;
		alignas(4) int32_t normalTexIndex;
		alignas(4) int32_t physicalTexIndex;        // metallic-roughness (default) or spec-gloss when useSpecGlossWorkflow=1
		alignas(4) int32_t occlusionTexIndex;
		alignas(4) int32_t emissiveTexIndex;

		// Specular-glossiness workflow support (KHR_materials_pbrSpecularGlossiness)
		alignas(4) int32_t useSpecGlossWorkflow;        // 1 if SpecGloss
		alignas(4) float glossinessFactor;
		alignas(16) glm::vec3 specularFactor;
		alignas(4) int32_t hasEmissiveStrengthExt;
		alignas(4) uint32_t _padMat[3];
	};

	// Ray query geometry and material buffers
	vk::raii::Buffer                        geometryInfoBuffer     = nullptr;
	std::unique_ptr<MemoryPool::Allocation> geometryInfoAllocation = nullptr;
	vk::raii::Buffer                        materialBuffer         = nullptr;
	std::unique_ptr<MemoryPool::Allocation> materialAllocation     = nullptr;

	// Ray query baseColor texture array (binding 6)
	static constexpr uint32_t RQ_MAX_TEX = 2048;
	// Reserved slots in the Ray Query texture table (binding 6)
	static constexpr uint32_t RQ_SLOT_DEFAULT_BASECOLOR  = 0;
	static constexpr uint32_t RQ_SLOT_DEFAULT_NORMAL     = 1;
	static constexpr uint32_t RQ_SLOT_DEFAULT_METALROUGH = 2;
	static constexpr uint32_t RQ_SLOT_DEFAULT_OCCLUSION  = 3;
	static constexpr uint32_t RQ_SLOT_DEFAULT_EMISSIVE   = 4;
	// NOTE: Textures can stream in asynchronously and their underlying VkImageView/VkSampler
	// can be destroyed/recreated. Therefore, the Ray Query texture table must NOT cache
	// VkDescriptorImageInfo (which contains raw handles). Instead, cache only the canonical
	// texture key per slot and rebuild VkDescriptorImageInfo each descriptor update.
	//
	// Slots 0..4 are reserved for shared default PBR textures.
	std::vector<std::string>                  rayQueryTexKeys;                 // slot -> canonical texture key
	std::vector<uint32_t>                     rayQueryTexFallbackSlots;        // slot -> fallback slot (type-appropriate default)
	uint32_t                                  rayQueryTexCount = 0;            // number of valid slots in rayQueryTexKeys
	std::unordered_map<std::string, uint32_t> rayQueryTexIndex;                // canonicalKey -> slot

	// Per-material texture path mapping captured at AS build time; used for streaming requests
	// and debugging, but Ray Query primarily uses per-material texture indices.
	struct RQMaterialTexPaths
	{
		std::string baseColor;
		std::string normal;
		std::string physical;
		std::string occlusion;
		std::string emissive;
	};
	std::vector<RQMaterialTexPaths> rqMaterialTexPaths;

	// Count of GeometryInfo instances currently uploaded (CPU-side tracking)
	size_t geometryInfoCountCPU = 0;
	// Count of materials currently uploaded (CPU-side tracking)
	size_t materialCountCPU = 0;

	// --- Pending GPU uploads (to be executed on the render thread safe point) ---
	std::mutex                         pendingMeshUploadsMutex;
	std::vector<class MeshComponent *> pendingMeshUploads;        // meshes with staged data to copy

	// Enqueue mesh uploads collected on background/loading threads
	void EnqueueMeshUploads(const std::vector<class MeshComponent *> &meshes);
	// Execute pending mesh uploads on the render thread (called from Render after fence wait)
	void ProcessPendingMeshUploads();

	// Descriptor set layouts (declared before pools and sets)
	vk::raii::DescriptorSetLayout descriptorSetLayout            = nullptr;
	vk::raii::DescriptorSetLayout pbrDescriptorSetLayout         = nullptr;
	vk::raii::DescriptorSetLayout transparentDescriptorSetLayout = nullptr;
	vk::raii::PipelineLayout      pbrTransparentPipelineLayout   = nullptr;

	// The texture that will hold a snapshot of the opaque scene
	vk::raii::Image        opaqueSceneColorImage{nullptr};
	vk::raii::DeviceMemory opaqueSceneColorImageMemory{nullptr};        // <-- Standard Vulkan memory
	vk::raii::ImageView    opaqueSceneColorImageView{nullptr};
	vk::raii::Sampler      opaqueSceneColorSampler{nullptr};

	// A descriptor set for the opaque scene color texture. We will have one for each frame in flight
	// to match the swapchain images.
	std::vector<vk::raii::DescriptorSet> transparentDescriptorSets;
	// Fallback descriptor sets for opaque pass (binds a default SHADER_READ_ONLY texture as Set 1)
	std::vector<vk::raii::DescriptorSet> transparentFallbackDescriptorSets;

	// Ray Query composite descriptor sets: sample the rayQueryOutputImage in a fullscreen pass
	std::vector<vk::raii::DescriptorSet> rqCompositeDescriptorSets;
	// Fallback sampler for the RQ composite if no other sampler is available at init time
	vk::raii::Sampler rqCompositeSampler{nullptr};

	// Mesh resources
	struct MeshResources
	{
		// Device-local vertex/index buffers used for rendering
		vk::raii::Buffer                        vertexBuffer           = nullptr;
		std::unique_ptr<MemoryPool::Allocation> vertexBufferAllocation = nullptr;
		vk::raii::Buffer                        indexBuffer            = nullptr;
		std::unique_ptr<MemoryPool::Allocation> indexBufferAllocation  = nullptr;
		uint32_t                                indexCount             = 0;

		// Optional per-mesh staging buffers used when uploads are batched.
		// These are populated when createMeshResources(..., deferUpload=true) is used
		// and are consumed and cleared by preAllocateEntityResourcesBatch().
		vk::raii::Buffer       stagingVertexBuffer       = nullptr;
		vk::raii::DeviceMemory stagingVertexBufferMemory = nullptr;
		vk::DeviceSize         vertexBufferSizeBytes     = 0;

		vk::raii::Buffer       stagingIndexBuffer       = nullptr;
		vk::raii::DeviceMemory stagingIndexBufferMemory = nullptr;
		vk::DeviceSize         indexBufferSizeBytes     = 0;

		// Material index for ray query (extracted from entity name or MaterialMesh)
		int32_t materialIndex = -1;        // -1 = no material/default
	};
	std::unordered_map<MeshComponent *, MeshResources> meshResources;

	// Texture resources
	struct TextureResources
	{
		vk::raii::Image                         textureImage           = nullptr;
		std::unique_ptr<MemoryPool::Allocation> textureImageAllocation = nullptr;
		vk::raii::ImageView                     textureImageView       = nullptr;
		vk::raii::Sampler                       textureSampler         = nullptr;
		vk::Format                              format                 = vk::Format::eR8G8B8A8Srgb;        // Store texture format for proper color space handling
		uint32_t                                mipLevels              = 1;                                // Store number of mipmap levels
		// Hint: true if source texture appears to use alpha masking (any alpha < ~1.0)
		bool alphaMaskedHint = false;
	};
	std::unordered_map<std::string, TextureResources> textureResources;

	// Pending texture jobs that require GPU-side work. Worker threads
	// enqueue these jobs; the main thread drains them and performs the
	// actual LoadTexture/LoadTextureFromMemory calls.
	struct PendingTextureJob
	{
		enum class Type
		{
			FromFile,
			FromMemory
		} type;
		enum class Priority
		{
			Critical,
			NonCritical
		} priority;
		std::string                idOrPath;
		std::vector<unsigned char> data;        // only used for FromMemory
		int                        width    = 0;
		int                        height   = 0;
		int                        channels = 0;
	};

	std::mutex                     pendingTextureJobsMutex;
	std::condition_variable        pendingTextureCv;
	std::vector<PendingTextureJob> pendingTextureJobs;
	// Track outstanding critical texture jobs (for IsLoading)
	std::atomic<uint32_t> criticalJobsOutstanding{0};

	// Background uploader worker controls (multiple workers)
	std::atomic<bool>        stopUploadsWorker{false};
	std::vector<std::thread> uploadsWorkerThreads;

	// Track how many texture upload jobs have been scheduled vs completed
	// on the GPU side. Used only for UI feedback during streaming.
	std::atomic<uint32_t> uploadJobsTotal{0};
	std::atomic<uint32_t> uploadJobsCompleted{0};
	// When true, initial scene load is complete and the loading overlay should be hidden
	std::atomic<bool> initialLoadComplete{false};

	// Performance counters for texture uploads
	std::atomic<uint64_t> bytesUploadedTotal{0};
	// Streaming window start time in nanoseconds from steady_clock epoch (0 when inactive)
	std::atomic<uint64_t> uploadWindowStartNs{0};
	// Aggregate per-texture CPU upload durations (nanoseconds) and count
	std::atomic<uint64_t> totalUploadNs{0};
	std::atomic<uint32_t> uploadCount{0};

	// Reverse mapping from texture ID to entities that reference it. Used to
	// update descriptor sets when a streamed texture finishes uploading.
	std::mutex                                             textureUsersMutex;
	std::unordered_map<std::string, std::vector<Entity *>> textureToEntities;

	// Entities needing descriptor set refresh due to streamed textures
	std::mutex                   dirtyEntitiesMutex;
	std::unordered_set<Entity *> descriptorDirtyEntities;

	// Protect concurrent access to textureResources
	mutable std::shared_mutex textureResourcesMutex;

	// Texture aliasing: maps alias (canonical) IDs to actual loaded keys
	std::unordered_map<std::string, std::string> textureAliases;

	// Per-texture load de-duplication (serialize loads of the same texture ID only)
	mutable std::mutex              textureLoadStateMutex;
	std::condition_variable         textureLoadStateCv;
	std::unordered_set<std::string> texturesLoading;

	// Serialize GPU-side texture upload (image/buffer creation, transitions) to avoid driver/memory pool races
	mutable std::mutex textureUploadMutex;

	// Thread pool for background background tasks (textures, etc.)
	std::unique_ptr<ThreadPool> threadPool;
	// Mutex to protect threadPool access during initialization/cleanup
	mutable std::shared_mutex threadPoolMutex;

	// Texture loading progress (for UI)
	std::atomic<uint32_t> textureTasksScheduled{0};
	std::atomic<uint32_t> textureTasksCompleted{0};
	std::atomic<bool>     loadingFlag{false};

	// Default texture resources (used when no texture is provided)
	TextureResources defaultTextureResources;

	// Performance clamps (to reduce per-frame cost)
	static constexpr uint32_t MAX_ACTIVE_LIGHTS = 1024;        // Limit the number of lights processed per frame

	// Static lights loaded during model initialization
	std::vector<ExtractedLight> staticLights;

	// Dynamic lighting system using storage buffers
	struct LightStorageBuffer
	{
		vk::raii::Buffer                        buffer     = nullptr;
		std::unique_ptr<MemoryPool::Allocation> allocation = nullptr;
		void                                   *mapped     = nullptr;
		size_t                                  capacity   = 0;        // Current capacity in number of lights
		size_t                                  size       = 0;        // Current number of lights
	};
	std::vector<LightStorageBuffer> lightStorageBuffers;        // One per frame in flight

	// Entity resources (contains descriptor sets - must be declared before descriptor pool)
	struct EntityResources
	{
		std::vector<vk::raii::Buffer>                        uniformBuffers;
		std::vector<std::unique_ptr<MemoryPool::Allocation>> uniformBufferAllocations;
		std::vector<void *>                                  uniformBuffersMapped;
		std::vector<vk::raii::DescriptorSet>                 basicDescriptorSets;        // For basic pipeline
		std::vector<vk::raii::DescriptorSet>                 pbrDescriptorSets;          // For PBR pipeline

		// Instance buffer for instanced rendering
		vk::raii::Buffer                        instanceBuffer           = nullptr;
		std::unique_ptr<MemoryPool::Allocation> instanceBufferAllocation = nullptr;
		void                                   *instanceBufferMapped     = nullptr;

		// Tracks whether binding 0 (UBO) has been written at least once for each frame.
		// This lets us avoid re-writing descriptor binding 0 every frame and prevents
		// update-after-bind warnings while keeping initialization correct when a frame
		// first becomes current.
		std::vector<bool> uboBindingWritten;        // size = MAX_FRAMES_IN_FLIGHT

		// Tracks whether image bindings have been written at least once for each frame.
		// If false for the current frame at the safe point, we cold-initialize the
		// image bindings (PBR: b1..b5 [+b6 when applicable], Basic: b1) with either
		// real textures or shared defaults to avoid per-frame "black" flashes.
		std::vector<bool> pbrImagesWritten;          // size = MAX_FRAMES_IN_FLIGHT
		std::vector<bool> basicImagesWritten;        // size = MAX_FRAMES_IN_FLIGHT
	};
	std::unordered_map<Entity *, EntityResources> entityResources;

	// Descriptor pool (declared after entity resources to ensure proper destruction order)
	vk::raii::DescriptorPool descriptorPool = nullptr;

	// Current frame index
	uint32_t currentFrame = 0;

	// Queue family indices
	QueueFamilyIndices queueFamilyIndices;

	// Validation layers
	const std::vector<const char *> validationLayers = {
	    "VK_LAYER_KHRONOS_validation"};

	// Required device extensions
	const std::vector<const char *> requiredDeviceExtensions = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	// Optional device extensions
	const std::vector<const char *> optionalDeviceExtensions = {
	    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	    VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
	    VK_EXT_ATTACHMENT_FEEDBACK_LOOP_DYNAMIC_STATE_EXTENSION_NAME,
	    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
	    // Robustness and safety
	    VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
	    // Tile/local memory friendly dynamic rendering readback
	    VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME,
	    // Shader tile image for fast tile access
	    VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME,
	    // Ray query support for ray-traced rendering
	    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	    VK_KHR_RAY_QUERY_EXTENSION_NAME};

	// All device extensions (required + optional)
	std::vector<const char *> deviceExtensions;

	// Initialization flag
	bool initialized = false;
	// Whether VK_EXT_descriptor_indexing (update-after-bind) path is enabled
	bool descriptorIndexingEnabled = false;
	bool storageAfterBindEnabled   = false;
	// Feature toggles detected/enabled at device creation
	bool robustness2Enabled               = false;
	bool dynamicRenderingLocalReadEnabled = false;
	bool shaderTileImageEnabled           = false;
	bool rayQueryEnabled                  = false;
	bool accelerationStructureEnabled     = false;

	// When true and current render mode is RayQuery, the engine renders a static opaque scene:
	// - Animation/physics updates are suppressed by the Engine (input/Update hook)
	// - TLAS refit per-frame is skipped to avoid any animation-driven changes
	// - The AS is built once after loading completes
	// Default now OFF so animation is enabled again for AS (per user request)
	bool rayQueryStaticOnly = false;

	// (No debug-only TLAS filtering in production.)

	// Framebuffer resized flag (atomic to handle platform callback vs. render thread)
	std::atomic<bool> framebufferResized{false};
	// Guard to prevent descriptor updates while a command buffer is recording
	std::atomic<bool> isRecordingCmd{false};
	// Descriptor sets may be temporarily invalid during swapchain recreation; suppress updates then.
	std::atomic<bool> descriptorSetsValid{true};
	// Request flag for acceleration structure build (set by loading thread, cleared by render thread)
	std::atomic<bool> asBuildRequested{false};

	// Track last successfully built AS sizes to avoid rebuilding with a smaller subset
	// (e.g., during incremental streaming where not all meshes are ready yet).
	// We only accept AS builds that are monotonically non-decreasing in counts.
	size_t lastASBuiltBLASCount     = 0;
	size_t lastASBuiltInstanceCount = 0;

	// Freeze TLAS rebuilds after a full build to prevent regressions (e.g., animation-only TLAS)
	bool asFreezeAfterFullBuild = true;         // enable freezing behavior
	bool asFrozen               = false;        // once frozen, ignore rebuilds unless explicitly overridden
	// Optional developer override to allow rebuild while frozen
	bool asDevOverrideAllowRebuild = false;
	// Reason string for the last time a build was requested (for logging)
	std::string lastASBuildRequestReason;

	// Opportunistic rebuilds (when counts increase) can cause unintended TLAS churn during animation.
	// Leave this disabled by default; TLAS builds should be explicit (on mode switch / scene ready).
	bool asOpportunisticRebuildEnabled = false;

	// --- AS UPDATE/Refit state ---
	// Persistent TLAS instances buffer & order for UPDATE (refit)
	struct TlasInstanceRef
	{
		class Entity *entity{nullptr};
		uint32_t      instanceIndex{0};        // valid only when instanced==true
		bool          instanced{false};        // true when this TLAS entry comes from MeshComponent instancing
	};
	vk::raii::Buffer                        tlasInstancesBuffer{nullptr};
	std::unique_ptr<MemoryPool::Allocation> tlasInstancesAllocation;
	uint32_t                                tlasInstanceCount = 0;
	std::vector<TlasInstanceRef>            tlasInstanceOrder;        // order must match buffer instances

	// Scratch buffer for TLAS UPDATE operations
	vk::raii::Buffer                        tlasUpdateScratchBuffer{nullptr};
	std::unique_ptr<MemoryPool::Allocation> tlasUpdateScratchAllocation;

	// Maximum number of frames in flight
	const uint32_t MAX_FRAMES_IN_FLIGHT = 1u;

	// --- Performance & diagnostics ---
	// CPU-side frustum culling toggle and last-frame stats
	bool     enableFrustumCulling    = true;
	uint32_t lastCullingVisibleCount = 0;
	uint32_t lastCullingCulledCount  = 0;
	// Distance-based LOD (projected-size skip in pixels)
	bool  enableDistanceLOD            = true;
	float lodPixelThresholdOpaque      = 1.5f;
	float lodPixelThresholdTransparent = 2.5f;
	// Sampler anisotropy preference (clamped to device limits)
	float samplerMaxAnisotropy = 8.0f;
	// Upper bound on auto-generated mip levels (to avoid excessive VRAM use on huge textures)
	uint32_t maxAutoGeneratedMipLevels = 4;

	// --- Planar reflections (scaffolding) ---
	bool  enablePlanarReflections   = false;        // UI toggle to enable/disable planar reflections
	float reflectionResolutionScale = 0.5f;         // Scale relative to swapchain size
	// Cached per-frame reflection data used by UBO population
	// Current frame's reflection VP (for rendering the reflection pass)
	glm::mat4 currentReflectionVP{1.0f};
	glm::vec4 currentReflectionPlane{0.0f, 1.0f, 0.0f, 0.0f};
	// Per-frame stored reflection VP (written during reflection pass)
	std::vector<glm::mat4> reflectionVPs;        // size MAX_FRAMES_IN_FLIGHT
	// The VP to sample in the main pass (prev-frame VP to match prev-frame texture)
	glm::mat4 sampleReflectionVP{1.0f};
	bool      reflectionResourcesDirty = false;        // recreate reflection RTs at safe point

	// --- Ray query rendering options ---
	bool enableRayQueryReflections  = true;        // UI toggle to enable reflections in ray query mode
	bool enableRayQueryTransparency = true;        // UI toggle to enable transparency/refraction in ray query mode

	// === Watchdog system to detect application hangs ===
	// Atomic timestamp updated every frame - watchdog thread checks if stale
	std::atomic<std::chrono::steady_clock::time_point> lastFrameUpdateTime;
	std::thread                                        watchdogThread;
	std::atomic<bool>                                  watchdogRunning{false};

	// === Descriptor update deferral while recording ===
	struct PendingDescOp
	{
		Entity     *entity;
		std::string texPath;
		bool        usePBR;
		uint32_t    frameIndex;
		bool        imagesOnly;
	};
	std::mutex                 pendingDescMutex;
	std::vector<PendingDescOp> pendingDescOps;        // flushed at frame safe point
	std::atomic<bool>          descriptorRefreshPending{false};

	struct ReflectionRT
	{
		vk::raii::Image                         color{nullptr};
		std::unique_ptr<MemoryPool::Allocation> colorAlloc{nullptr};
		vk::raii::ImageView                     colorView{nullptr};
		vk::raii::Sampler                       colorSampler{nullptr};

		vk::raii::Image                         depth{nullptr};
		std::unique_ptr<MemoryPool::Allocation> depthAlloc{nullptr};
		vk::raii::ImageView                     depthView{nullptr};

		uint32_t width{0};
		uint32_t height{0};
	};
	std::vector<ReflectionRT> reflections;        // one per frame-in-flight

	// Private methods
	bool createInstance(const std::string &appName, bool enableValidationLayers);
	bool setupDebugMessenger(bool enableValidationLayers);
	bool createSurface();
	bool checkValidationLayerSupport() const;
	bool pickPhysicalDevice();
	void addSupportedOptionalExtensions();
	bool createLogicalDevice(bool enableValidationLayers);
	bool createSwapChain();
	bool createImageViews();
	bool setupDynamicRendering();
	bool createDescriptorSetLayout();
	bool createPBRDescriptorSetLayout();
	bool createGraphicsPipeline();

	bool createPBRPipeline();
	bool createLightingPipeline();
	bool createDepthPrepassPipeline();
	bool createForwardPlusPipelinesAndResources();

	// Ray query pipeline creation
	bool createRayQueryDescriptorSetLayout();
	bool createRayQueryPipeline();
	bool createRayQueryResources();
	// If updateOnlyCurrentFrame is true, only descriptor sets for currentFrame will be updated.
	// Use updateOnlyCurrentFrame=false during initialization/swapchain recreation when the device is idle.
	bool createOrResizeForwardPlusBuffers(uint32_t tilesX, uint32_t tilesY, uint32_t slicesZ, bool updateOnlyCurrentFrame = false);
	void updateForwardPlusParams(uint32_t frameIndex, const glm::mat4 &view, const glm::mat4 &proj, uint32_t lightCount, uint32_t tilesX, uint32_t tilesY, uint32_t slicesZ, float nearZ, float farZ);
	void dispatchForwardPlus(vk::raii::CommandBuffer &cmd, uint32_t tilesX, uint32_t tilesY, uint32_t slicesZ);
	// Ensure Forward+ compute descriptor set binding 0 (lights SSBO) is bound for a frame
	void refreshForwardPlusComputeLightsBindingForFrame(uint32_t frameIndex);
	bool createComputePipeline();
	void pushMaterialProperties(vk::CommandBuffer commandBuffer, const MaterialProperties &material) const;
	bool createCommandPool();

	// Shadow mapping methods
	bool createComputeCommandPool();
	bool createDepthResources();
	bool createTextureImage(const std::string &texturePath, TextureResources &resources);
	bool createTextureImageView(TextureResources &resources);
	bool createTextureSampler(TextureResources &resources);
	bool createDefaultTextureResources();
	bool createSharedDefaultPBRTextures();
	bool createMeshResources(MeshComponent *meshComponent, bool deferUpload = false);
	bool createUniformBuffers(Entity *entity);
	bool createDescriptorPool();
	bool createDescriptorSets(Entity *entity, const std::string &texturePath, bool usePBR = false);
	// Refresh only the currentFrame PBR descriptor set bindings that Forward+ relies on
	// (b6 = lights SSBO, b7 = tile headers, b8 = tile indices). Safe to call after
	// we've waited on the frame fence at the start of Render().
	void refreshPBRForwardPlusBindingsForFrame(uint32_t frameIndex);
	bool createCommandBuffers();
	bool createSyncObjects();

	void cleanupSwapChain();

	// Planar reflection helpers (initial scaffolding)
	bool createReflectionResources(uint32_t width, uint32_t height);
	void destroyReflectionResources();
	// Render the scene into the reflection RT (mirrored about a plane) — to be fleshed out next step
	void renderReflectionPass(vk::raii::CommandBuffer                    &cmd,
	                          const glm::vec4                            &planeWS,
	                          CameraComponent                            *camera,
	                          const std::vector<std::unique_ptr<Entity>> &entities);

	// Ensure Vulkan-Hpp dispatcher is initialized for the current thread when using RAII objects on worker threads
	void ensureThreadLocalVulkanInit() const;

	// ===================== Culling helpers =====================
	struct FrustumPlanes
	{
		// Plane equation ax + by + cz + d >= 0 considered inside
		glm::vec4 planes[6]{};        // 0=L,1=R,2=B,3=T,4=N,5=F
	};

	static FrustumPlanes extractFrustumPlanes(const glm::mat4 &vp);

	static void transformAABB(const glm::mat4 &M,
	                          const glm::vec3 &localMin,
	                          const glm::vec3 &localMax,
	                          glm::vec3       &outMin,
	                          glm::vec3       &outMax);

	static bool aabbIntersectsFrustum(const glm::vec3     &worldMin,
	                                  const glm::vec3     &worldMax,
	                                  const FrustumPlanes &frustum);
	void        recreateSwapChain();

	void updateUniformBuffer(uint32_t currentImage, Entity *entity, CameraComponent *camera);
	void updateUniformBuffer(uint32_t currentImage, Entity *entity, CameraComponent *camera, const glm::mat4 &customTransform);
	void updateUniformBufferInternal(uint32_t currentImage, Entity *entity, CameraComponent *camera, UniformBufferObject &ubo);

	vk::raii::ShaderModule createShaderModule(const std::vector<char> &code);

	QueueFamilyIndices      findQueueFamilies(const vk::raii::PhysicalDevice &device);
	SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice &device);
	bool                    isDeviceSuitable(vk::raii::PhysicalDevice &device);
	bool                    checkDeviceExtensionSupport(vk::raii::PhysicalDevice &device);

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
	vk::PresentModeKHR   chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
	vk::Extent2D         chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>                  createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	bool                                                                 createOpaqueSceneColorResources();
	void                                                                 createTransparentDescriptorSets();
	void                                                                 createTransparentFallbackDescriptorSets();
	std::pair<vk::raii::Buffer, std::unique_ptr<MemoryPool::Allocation>> createBufferPooled(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	void                                                                 copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size);

	std::pair<vk::raii::Image, vk::raii::DeviceMemory>                  createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
	std::pair<vk::raii::Image, std::unique_ptr<MemoryPool::Allocation>> createImagePooled(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, uint32_t mipLevels = 1, vk::SharingMode sharingMode = vk::SharingMode::eExclusive, const std::vector<uint32_t> &queueFamilies = {});
	void                                                                transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels = 1);
	void                                                                copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, const std::vector<vk::BufferImageCopy> &regions);
	// Extended: track stagedBytes for perf stats
	void uploadImageFromStaging(vk::Buffer                              staging,
	                            vk::Image                               image,
	                            vk::Format                              format,
	                            const std::vector<vk::BufferImageCopy> &regions,
	                            uint32_t                                mipLevels,
	                            vk::DeviceSize                          stagedBytes);

	vk::raii::ImageView createImageView(vk::raii::Image &image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels = 1);
	vk::Format          findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
	bool                hasStencilComponent(vk::Format format);

	std::vector<char> readFile(const std::string &filename);

	// Background uploader helpers
	void StartUploadsWorker(size_t workerCount = 0);
	void StopUploadsWorker();

	// Serialize descriptor writes vs command buffer recording to avoid mid-record updates during recording
	std::mutex renderRecordMutex;

	// (Descriptor API wrappers were considered but avoided here to keep RAII types intact.)

	// Upload perf getters
  public:
	uint64_t GetBytesUploadedTotal() const
	{
		return bytesUploadedTotal.load(std::memory_order_relaxed);
	}
	double GetAverageUploadMs() const
	{
		uint64_t ns  = totalUploadNs.load(std::memory_order_relaxed);
		uint32_t cnt = uploadCount.load(std::memory_order_relaxed);
		if (cnt == 0)
			return 0.0;
		return static_cast<double>(ns) / 1e6 / static_cast<double>(cnt);
	}
	double GetUploadThroughputMBps() const
	{
		uint64_t startNs = uploadWindowStartNs.load(std::memory_order_relaxed);
		if (startNs == 0)
			return 0.0;
		auto     now   = std::chrono::steady_clock::now().time_since_epoch();
		uint64_t nowNs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
		if (nowNs <= startNs)
			return 0.0;
		double seconds = static_cast<double>(nowNs - startNs) / 1e9;
		double mb      = static_cast<double>(bytesUploadedTotal.load(std::memory_order_relaxed)) / (1024.0 * 1024.0);
		return seconds > 0.0 ? (mb / seconds) : 0.0;
	}
};