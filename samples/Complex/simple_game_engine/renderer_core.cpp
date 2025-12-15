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
#include "renderer.h"
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <cstring>
#include <ranges>
#include <thread>
#include <algorithm>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE; // In a .cpp file

#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_platform.h>

// Debug callback for vk::raii
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackVkRaii(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        // Print a message to the console
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    } else {
        // Print a message to the console
        std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

// Watchdog thread function - monitors frame updates and aborts if application hangs
static void WatchdogThreadFunc(std::atomic<std::chrono::steady_clock::time_point>* lastFrameTime,
                               std::atomic<bool>* running) {
    std::cout << "[Watchdog] Started - will abort if no frame updates for 5+ seconds\n";
    
    while (running->load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (!running->load(std::memory_order_relaxed)) {
            break;  // Shutdown requested
        }
        
        // Check if frame timestamp was updated in the last 5 seconds
        // 5 seconds allows for heavy GPU workloads (ray query with 552 meshes + reflections/transparency)
        auto now = std::chrono::steady_clock::now();
        auto lastUpdate = lastFrameTime->load(std::memory_order_relaxed);
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count();
        
        if (elapsed >= 5) {
            // APPLICATION HAS HUNG - no frame updates for 5+ seconds
            std::cerr << "\n\n";
            std::cerr << "========================================\n";
            std::cerr << "WATCHDOG: APPLICATION HAS HUNG!\n";
            std::cerr << "========================================\n";
            std::cerr << "Last frame update was " << elapsed << " seconds ago.\n";
            std::cerr << "The render loop is not progressing.\n";
            std::cerr << "Aborting to generate stack trace...\n";
            std::cerr << "========================================\n\n";
            std::abort();  // Force crash with stack trace
        }
    }
    
    std::cout << "[Watchdog] Stopped\n";
}

// Renderer core implementation for the "Rendering Pipeline" chapter of the tutorial.
Renderer::Renderer(Platform* platform)
    : platform(platform) {
    // Initialize deviceExtensions with required extensions only
    // Optional extensions will be added later after checking device support
    deviceExtensions = requiredDeviceExtensions;
}

// Destructor
Renderer::~Renderer() {
    Cleanup();
}

// Initialize the renderer
bool Renderer::Initialize(const std::string& appName, bool enableValidationLayers) {
    vk::detail::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    // Create a Vulkan instance
    if (!createInstance(appName, enableValidationLayers)) {
        return false;
    }

    // Setup debug messenger
    if (!setupDebugMessenger(enableValidationLayers)) {
        return false;
    }

    // Create surface
    if (!createSurface()) {
        return false;
    }

    // Pick the physical device
    if (!pickPhysicalDevice()) {
        return false;
    }

    // Create logical device
    if (!createLogicalDevice(enableValidationLayers)) {
        return false;
    }

    // Initialize memory pool for efficient memory management
    try {
        memoryPool = std::make_unique<MemoryPool>(device, physicalDevice);
        if (!memoryPool->initialize()) {
            std::cerr << "Failed to initialize memory pool" << std::endl;
            return false;
        }

        // Optionally pre-allocate initial memory blocks for pools.
        // For large scenes (e.g., Bistro) on mid-range GPUs this can cause early OOM.
        // Skip pre-allocation to reduce peak memory pressure; blocks will be created on demand.
        // if (!memoryPool->preAllocatePools()) { /* non-fatal */ }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create memory pool: " << e.what() << std::endl;
        return false;
    }

    // Create swap chain
    if (!createSwapChain()) {
        return false;
    }

    // Create image views
    if (!createImageViews()) {
        return false;
    }

    // Setup dynamic rendering
    if (!setupDynamicRendering()) {
        return false;
    }

    // Create the descriptor set layout
    if (!createDescriptorSetLayout()) {
        return false;
    }

    // Create the graphics pipeline
    if (!createGraphicsPipeline()) {
        return false;
    }

    // Create PBR pipeline
    if (!createPBRPipeline()) {
        return false;
    }

    // Create the lighting pipeline
    if (!createLightingPipeline()) {
        std::cerr << "Failed to create lighting pipeline" << std::endl;
        return false;
    }

    // Create composite pipeline (fullscreen pass for off-screen → swapchain)
    if (!createCompositePipeline()) {
        std::cerr << "Failed to create composite pipeline" << std::endl;
        return false;
    }

    // Create compute pipeline
    if (!createComputePipeline()) {
        std::cerr << "Failed to create compute pipeline" << std::endl;
        return false;
    }

    // Ensure light storage buffers exist before creating Forward+ resources
    // so that compute descriptor binding 0 (lights SSBO) can be populated safely.
    if (!createOrResizeLightStorageBuffers(1)) {
        std::cerr << "Failed to create initial light storage buffers" << std::endl;
        return false;
    }

    // Create Forward+ compute and depth pre-pass pipelines/resources
    if (useForwardPlus) {
        if (!createForwardPlusPipelinesAndResources()) {
            std::cerr << "Failed to create Forward+ resources" << std::endl;
            return false;
        }
    }

    // Create ray query descriptor set layout and pipeline (but not resources yet - need descriptor pool first)
    if (!createRayQueryDescriptorSetLayout()) {
        std::cerr << "Failed to create ray query descriptor set layout" << std::endl;
        return false;
    }
    if (!createRayQueryPipeline()) {
        std::cerr << "Failed to create ray query pipeline" << std::endl;
        return false;
    }

    // Create the command pool
    if (!createCommandPool()) {
        return false;
    }

    // Create depth resources
    if (!createDepthResources()) {
        return false;
    }

    if (useForwardPlus) {
        if (!createDepthPrepassPipeline()) {
            return false;
        }
    }

    // Create the descriptor pool
    if (!createDescriptorPool()) {
        return false;
    }

    // Create ray query resources AFTER descriptor pool (needs pool for descriptor set allocation)
    if (!createRayQueryResources()) {
        std::cerr << "Failed to create ray query resources" << std::endl;
        return false;
    }
    
    // Note: Acceleration structure build is requested by scene_loading.cpp after entities load
    // No need to request it here during init

    // Light storage buffers were already created earlier to satisfy Forward+ binding requirements

    if (!createOpaqueSceneColorResources()) {
        return false;
    }

    createTransparentDescriptorSets();

    // Create default texture resources
    if (!createDefaultTextureResources()) {
        std::cerr << "Failed to create default texture resources" << std::endl;
        return false;
    }

    // Create fallback transparent descriptor sets (must occur after default textures exist)
    createTransparentFallbackDescriptorSets();

    // Create shared default PBR textures (to avoid creating hundreds of identical textures)
    if (!createSharedDefaultPBRTextures()) {
        std::cerr << "Failed to create shared default PBR textures" << std::endl;
        return false;
    }


    // Create command buffers
    if (!createCommandBuffers()) {
        return false;
    }

    // Create sync objects
    if (!createSyncObjects()) {
        return false;
    }

    // Initialize background thread pool for async tasks (textures, etc.) AFTER all Vulkan resources are ready
    try {
        // Size the thread pool based on hardware concurrency, clamped to a sensible range
        unsigned int hw = std::max(2u, std::min(8u, std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 4u));
        threadPool = std::make_unique<ThreadPool>(hw);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create thread pool: " << e.what() << std::endl;
        return false;
    }

    // Start background uploads worker now that queues/semaphores exist
    StartUploadsWorker();

    // Start watchdog thread to detect application hangs
    lastFrameUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
    watchdogRunning.store(true, std::memory_order_relaxed);
    watchdogThread = std::thread(WatchdogThreadFunc, &lastFrameUpdateTime, &watchdogRunning);

    initialized = true;
    return true;
}

void Renderer::ensureThreadLocalVulkanInit() const {
    // Initialize Vulkan-Hpp dispatcher per-thread; required for multi-threaded RAII usage
    static thread_local bool s_tlsInitialized = false;
    if (s_tlsInitialized) return;
    try {
        vk::detail::DynamicLoader dl;
        auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        if (vkGetInstanceProcAddr) {
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
        }
        if (*instance) {
            VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
        }
        if (*device) {
            VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
        }
        s_tlsInitialized = true;
    } catch (...) {
        // best-effort
    }
}

// Clean up renderer resources
void Renderer::Cleanup() {
    // Stop watchdog thread first to prevent false hang detection during shutdown
    if (watchdogRunning.load(std::memory_order_relaxed)) {
        watchdogRunning.store(false, std::memory_order_relaxed);
        if (watchdogThread.joinable()) {
            watchdogThread.join();
        }
    }

    // Ensure background workers are stopped before tearing down Vulkan resources
    StopUploadsWorker();

    // Disallow any further descriptor writes during shutdown.
    // This prevents late updates/frees racing against pool destruction.
    descriptorSetsValid.store(false, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lk(pendingDescMutex);
        pendingDescOps.clear();
        descriptorRefreshPending.store(false, std::memory_order_relaxed);
    }
    {
        std::unique_lock<std::shared_mutex> lock(threadPoolMutex);
        if (threadPool) {
            threadPool.reset();
        }
    }

    if (!initialized) {
        return;
    }

    std::cout << "Starting renderer cleanup..." << std::endl;

    // Wait for the device to be idle before cleaning up
    try { WaitIdle(); } catch (...) {}

    // 1) Clean up any swapchain-scoped resources first
    cleanupSwapChain();

    // 2) Clear per-entity resources (descriptor sets and buffers) while descriptor pools still exist
    for (auto &kv : entityResources) {
        auto &resources = kv.second;
        resources.basicDescriptorSets.clear();
        resources.pbrDescriptorSets.clear();
        resources.uniformBuffers.clear();
        resources.uniformBufferAllocations.clear();
        resources.uniformBuffersMapped.clear();
        resources.instanceBuffer = nullptr;
        resources.instanceBufferAllocation = nullptr;
        resources.instanceBufferMapped = nullptr;
    }
    entityResources.clear();

    // 3) Clear any global descriptor sets that are allocated from pools to avoid dangling refs
    transparentDescriptorSets.clear();
    transparentFallbackDescriptorSets.clear();
    compositeDescriptorSets.clear();
    computeDescriptorSets.clear();
    rqCompositeDescriptorSets.clear();
    
    // 3.5) Clear ray query descriptor sets BEFORE destroying descriptor pool
    // Without this, rayQueryDescriptorSets' RAII destructor tries to free them after
    // the pool is destroyed, causing "Invalid VkDescriptorPool Object" validation errors
    rayQueryDescriptorSets.clear();

    // Ray Query composite sampler/sets are allocated from the shared descriptor pool.
    // Ensure they are released before destroying the pool.
    rqCompositeSampler = nullptr;

    // 4) Destroy/Reset pipelines and pipeline layouts (graphics/compute/forward+)
    graphicsPipeline = nullptr;
    pbrGraphicsPipeline = nullptr;
    pbrBlendGraphicsPipeline = nullptr;
    pbrPremulBlendGraphicsPipeline = nullptr;
    pbrPrepassGraphicsPipeline = nullptr;
    glassGraphicsPipeline = nullptr;
    lightingPipeline = nullptr;
    compositePipeline = nullptr;
    forwardPlusPipeline = nullptr;
    depthPrepassPipeline = nullptr;

    pipelineLayout = nullptr;
    pbrPipelineLayout = nullptr;
    lightingPipelineLayout = nullptr;
    compositePipelineLayout = nullptr;
    pbrTransparentPipelineLayout = nullptr;
    forwardPlusPipelineLayout = nullptr;
    
    // 4.3) Ray query pipelines and layouts
    rayQueryPipeline = nullptr;
    rayQueryPipelineLayout = nullptr;

    // 4.5) Forward+ per-frame resources (including descriptor sets) must be released
    // BEFORE destroying descriptor pools to avoid vkFreeDescriptorSets with invalid pool
    for (auto &fp : forwardPlusPerFrame) {
        fp.tileHeaders = nullptr;
        fp.tileHeadersAlloc = nullptr;
        fp.tileLightIndices = nullptr;
        fp.tileLightIndicesAlloc = nullptr;
        fp.params = nullptr;
        fp.paramsAlloc = nullptr;
        fp.paramsMapped = nullptr;
        fp.debugOut = nullptr;
        fp.debugOutAlloc = nullptr;
        fp.probeOffscreen = nullptr;
        fp.probeOffscreenAlloc = nullptr;
        fp.probeSwapchain = nullptr;
        fp.probeSwapchainAlloc = nullptr;
        fp.computeSet = nullptr; // descriptor set allocated from compute/graphics pools
    }
    forwardPlusPerFrame.clear();

    // 5) Destroy descriptor set layouts and pools (compute + graphics)
    descriptorSetLayout = nullptr;
    pbrDescriptorSetLayout = nullptr;
    transparentDescriptorSetLayout = nullptr;
    compositeDescriptorSetLayout = nullptr;
    forwardPlusDescriptorSetLayout = nullptr;
    computeDescriptorSetLayout = nullptr;
    rayQueryDescriptorSetLayout = nullptr;

    // Pools last, after sets are cleared
    computeDescriptorPool = nullptr;
    descriptorPool = nullptr;

    // 6) Clear textures and aliases, including default resources
    {
        std::unique_lock<std::shared_mutex> lk(textureResourcesMutex);
        textureResources.clear();
        textureAliases.clear();
    }
    // Reset default texture resources
    defaultTextureResources.textureSampler = nullptr;
    defaultTextureResources.textureImageView = nullptr;
    defaultTextureResources.textureImage = nullptr;
    defaultTextureResources.textureImageAllocation = nullptr;

    // 7) Opaque scene color and related descriptors
    opaqueSceneColorSampler = nullptr;
    opaqueSceneColorImageView = nullptr;
    opaqueSceneColorImageMemory = nullptr;
    opaqueSceneColorImage = nullptr;
    
    // 7.5) Ray query output image and acceleration structures
    rayQueryOutputImageView = nullptr;
    rayQueryOutputImage = nullptr;
    rayQueryOutputImageAllocation = nullptr;
    
    // Clear acceleration structures (BLAS and TLAS buffers)
    blasStructures.clear();
    tlasStructure = AccelerationStructure{};

    // 8) (moved above) Forward+ per-frame buffers cleared prior to pool destruction

    // 9) Command buffers/pools
    commandBuffers.clear();
    commandPool = nullptr;
    computeCommandPool = nullptr;

    // 10) Sync objects
    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();
    uploadsTimeline = nullptr;

    // 11) Queues and surface (RAII handles will release upon reset; keep device alive until the end)
    graphicsQueue = nullptr;
    presentQueue = nullptr;
    computeQueue = nullptr;
    transferQueue = nullptr;
    surface = nullptr;

    // 12) Memory pool last
    memoryPool.reset();

    // Finally mark uninitialized
    initialized = false;
    std::cout << "Renderer cleanup completed." << std::endl;
}

// Create instance
bool Renderer::createInstance(const std::string& appName, bool enableValidationLayers) {
    try {
        // Create application info
        vk::ApplicationInfo appInfo{
            .pApplicationName = appName.c_str(),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Simple Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };

        // Get required extensions
        std::vector<const char*> extensions;

        // Add required extensions for GLFW
#if defined(PLATFORM_DESKTOP)
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        extensions.insert(extensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
#endif

        // Add debug extension if validation layers are enabled
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // Create instance info
        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        // Enable validation layers if requested
        vk::ValidationFeaturesEXT validationFeatures{};
        std::vector<vk::ValidationFeatureEnableEXT> enabledValidationFeatures;

        if (enableValidationLayers) {
            if (!checkValidationLayerSupport()) {
                std::cerr << "Validation layers requested, but not available" << std::endl;
                return false;
            }

            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // Keep validation output quiet by default (no DebugPrintf feature).
            // Ray Query debugPrintf/printf diagnostics are intentionally removed.

            validationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(enabledValidationFeatures.size());
            validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures.data();

            createInfo.pNext = &validationFeatures;
        }

        // Create instance
        instance = vk::raii::Instance(context, createInfo);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create instance: " << e.what() << std::endl;
        return false;
    }
}

// Setup debug messenger
bool Renderer::setupDebugMessenger(bool enableValidationLayers) {
    if (!enableValidationLayers) {
        return true;
    }

    try {
        // Create debug messenger info
        vk::DebugUtilsMessengerCreateInfoEXT createInfo{
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                              vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                              vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                              vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                          vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                          vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = debugCallbackVkRaii
        };

        // Create debug messenger
        debugMessenger = vk::raii::DebugUtilsMessengerEXT(instance, createInfo);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to set up debug messenger: " << e.what() << std::endl;
        return false;
    }
}

// Create surface
bool Renderer::createSurface() {
    try {
        // Create surface
        VkSurfaceKHR _surface;
        if (!platform->CreateVulkanSurface(*instance, &_surface)) {
            std::cerr << "Failed to create window surface" << std::endl;
            return false;
        }

        surface = vk::raii::SurfaceKHR(instance, _surface);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create surface: " << e.what() << std::endl;
        return false;
    }
}

// Pick a physical device
bool Renderer::pickPhysicalDevice() {
    try {
        // Get available physical devices
        std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

        if (devices.empty()) {
            std::cerr << "Failed to find GPUs with Vulkan support" << std::endl;
            return false;
        }

        // Prioritize discrete GPUs (like NVIDIA RTX 2080) over integrated GPUs (like Intel UHD Graphics)
        // First, collect all suitable devices with their suitability scores
        std::multimap<int, vk::raii::PhysicalDevice> suitableDevices;

        for (auto& _device : devices) {
            // Print device properties for debugging
            vk::PhysicalDeviceProperties deviceProperties = _device.getProperties();
            std::cout << "Checking device: " << deviceProperties.deviceName
                      << " (Type: " << vk::to_string(deviceProperties.deviceType) << ")" << std::endl;

            // Check if the device supports Vulkan 1.3
            bool supportsVulkan1_3 = deviceProperties.apiVersion >= VK_API_VERSION_1_3;
            if (!supportsVulkan1_3) {
                std::cout << "  - Does not support Vulkan 1.3" << std::endl;
                continue;
            }

            // Check queue families
            QueueFamilyIndices indices = findQueueFamilies(_device);
            bool supportsGraphics = indices.isComplete();
            if (!supportsGraphics) {
                std::cout << "  - Missing required queue families" << std::endl;
                continue;
            }

            // Check device extensions
            bool supportsAllRequiredExtensions = checkDeviceExtensionSupport(_device);
            if (!supportsAllRequiredExtensions) {
                std::cout << "  - Missing required extensions" << std::endl;
                continue;
            }

            // Check swap chain support
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_device);
            bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
            if (!swapChainAdequate) {
                std::cout << "  - Inadequate swap chain support" << std::endl;
                continue;
            }

            // Check for required features
            auto features = _device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features>();
            bool supportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering;
            if (!supportsRequiredFeatures) {
                std::cout << "  - Does not support required features (dynamicRendering)" << std::endl;
                continue;
            }

            // Calculate suitability score - prioritize discrete GPUs
            int score = 0;

            // Discrete GPUs get the highest priority (NVIDIA RTX 2080, AMD, etc.)
            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                score += 1000;
                std::cout << "  - Discrete GPU: +1000 points" << std::endl;
            }
            // Integrated GPUs get lower priority (Intel UHD Graphics, etc.)
            else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
                score += 100;
                std::cout << "  - Integrated GPU: +100 points" << std::endl;
            }

            // Add points for memory size (more VRAM is better)
            vk::PhysicalDeviceMemoryProperties memProperties = _device.getMemoryProperties();
            for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) {
                if (memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
                    // Add 1 point per GB of VRAM
                    score += static_cast<int>(memProperties.memoryHeaps[i].size / (1024 * 1024 * 1024));
                    break;
                }
            }

            std::cout << "  - Device is suitable with score: " << score << std::endl;
            suitableDevices.emplace(score, _device);
        }

        if (!suitableDevices.empty()) {
            // Select the device with the highest score (discrete GPU with most VRAM)
            physicalDevice = suitableDevices.rbegin()->second;
            vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
            std::cout << "Selected device: " << deviceProperties.deviceName
                      << " (Type: " << vk::to_string(deviceProperties.deviceType)
                      << ", Score: " << suitableDevices.rbegin()->first << ")" << std::endl;

            // Store queue family indices for the selected device
            queueFamilyIndices = findQueueFamilies(physicalDevice);

            // Add supported optional extensions
            addSupportedOptionalExtensions();

            return true;
        }
        std::cerr << "Failed to find a suitable GPU. Make sure your GPU supports Vulkan and has the required extensions." << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Failed to pick physical device: " << e.what() << std::endl;
        return false;
    }
}

// Add supported optional extensions
void Renderer::addSupportedOptionalExtensions() {
    try {
        // Get available extensions
        auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

        // Build a set of available extension names for quick lookup
        std::set<std::string> avail;
        for (const auto& e : availableExtensions) { avail.insert(e.extensionName); }

        // First, handle dependency: VK_EXT_attachment_feedback_loop_dynamic_state requires VK_EXT_attachment_feedback_loop_layout
        const char* dynState = VK_EXT_ATTACHMENT_FEEDBACK_LOOP_DYNAMIC_STATE_EXTENSION_NAME;
        const char* layoutReq = "VK_EXT_attachment_feedback_loop_layout";
        bool dynSupported = avail.contains(dynState);
        bool layoutSupported = avail.contains(layoutReq);
        for (const auto& optionalExt : optionalDeviceExtensions) {
            if (std::strcmp(optionalExt, dynState) == 0) {
                if (dynSupported && layoutSupported) {
                    deviceExtensions.push_back(dynState);
                    deviceExtensions.push_back(layoutReq);
                    std::cout << "Adding optional extension: " << dynState << std::endl;
                    std::cout << "Adding required-by-optional extension: " << layoutReq << std::endl;
                } else if (dynSupported && !layoutSupported) {
                    std::cout << "Skipping extension due to missing dependency: " << dynState << " requires " << layoutReq << std::endl;
                }
                continue; // handled
            }
            if (avail.contains(optionalExt)) {
                deviceExtensions.push_back(optionalExt);
                std::cout << "Adding optional extension: " << optionalExt << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to add optional extensions: " << e.what() << std::endl;
    }
}

// Create logical device
bool Renderer::createLogicalDevice(bool enableValidationLayers) {
    try {
        // Create queue create info for each unique queue family
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set uniqueQueueFamilies = {
            queueFamilyIndices.graphicsFamily.value(),
            queueFamilyIndices.presentFamily.value(),
            queueFamilyIndices.computeFamily.value(),
            queueFamilyIndices.transferFamily.value()
        };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            vk::DeviceQueueCreateInfo queueCreateInfo{
                .queueFamilyIndex = queueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
            };
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Enable required features
        auto features = physicalDevice.getFeatures2();
        features.features.samplerAnisotropy = vk::True;
        features.features.depthBiasClamp = vk::True;

        // Explicitly configure device features to prevent validation layer warnings
        // These features are required by extensions or other features, so we enable them explicitly

        // Timeline semaphore features (required for synchronization2)
        vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures;
        timelineSemaphoreFeatures.timelineSemaphore = vk::True;

        // Vulkan memory model features (required for some shader operations)
        vk::PhysicalDeviceVulkanMemoryModelFeatures memoryModelFeatures;
        memoryModelFeatures.vulkanMemoryModel = vk::True;
        memoryModelFeatures.vulkanMemoryModelDeviceScope = vk::True;

        // Buffer device address features (required for some buffer operations)
        vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures;
        bufferDeviceAddressFeatures.bufferDeviceAddress = vk::True;

        // 8-bit storage features (required for some shader storage operations)
        vk::PhysicalDevice8BitStorageFeatures storage8BitFeatures;
        storage8BitFeatures.storageBuffer8BitAccess = vk::True;

        // Enable Vulkan 1.3 features
        vk::PhysicalDeviceVulkan13Features vulkan13Features;
        vulkan13Features.dynamicRendering = vk::True;
        vulkan13Features.synchronization2 = vk::True;

        // Vulkan 1.1 features: shaderDrawParameters to satisfy SPIR-V DrawParameters capability
        vk::PhysicalDeviceVulkan11Features vulkan11Features{};
        vulkan11Features.shaderDrawParameters = vk::True;

        // Query extended feature support
        auto featureChain = physicalDevice.getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceDescriptorIndexingFeatures,
            vk::PhysicalDeviceRobustness2FeaturesEXT,
            vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR,
            vk::PhysicalDeviceShaderTileImageFeaturesEXT,
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
            vk::PhysicalDeviceRayQueryFeaturesKHR
        >();
        const auto& coreFeaturesSupported = featureChain.get<vk::PhysicalDeviceFeatures2>().features;
        const auto& indexingFeaturesSupported = featureChain.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
        const auto& robust2Supported = featureChain.get<vk::PhysicalDeviceRobustness2FeaturesEXT>();
        const auto& localReadSupported = featureChain.get<vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR>();
        const auto& tileImageSupported = featureChain.get<vk::PhysicalDeviceShaderTileImageFeaturesEXT>();
        const auto& accelerationStructureSupported = featureChain.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
        const auto& rayQuerySupported = featureChain.get<vk::PhysicalDeviceRayQueryFeaturesKHR>();

        // Ray Query shader uses indexing into a (large) sampled-image array.
        // Some drivers require this core feature to be explicitly enabled.
        if (coreFeaturesSupported.shaderSampledImageArrayDynamicIndexing) {
            features.features.shaderSampledImageArrayDynamicIndexing = vk::True;
        }

        // Prepare descriptor indexing features to enable if supported
        vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeaturesEnable{};
        descriptorIndexingEnabled = false;
        // Enable non-uniform indexing of sampled image arrays when supported — required for
        // `NonUniformResourceIndex()` in the ray-query shader to actually take effect.
        if (indexingFeaturesSupported.shaderSampledImageArrayNonUniformIndexing) {
            indexingFeaturesEnable.shaderSampledImageArrayNonUniformIndexing = vk::True;
            descriptorIndexingEnabled = true;
        }

        // These are not strictly required when writing a fully-populated descriptor array,
        // but enabling them when available avoids edge-case driver behavior for large arrays.
        if (descriptorIndexingEnabled) {
            if (indexingFeaturesSupported.descriptorBindingPartiallyBound) {
                indexingFeaturesEnable.descriptorBindingPartiallyBound = vk::True;
            }
            if (indexingFeaturesSupported.descriptorBindingUpdateUnusedWhilePending) {
                indexingFeaturesEnable.descriptorBindingUpdateUnusedWhilePending = vk::True;
            }
        }
        // Optionally enable UpdateAfterBind flags when supported (not strictly required for RQ textures)
        if (indexingFeaturesSupported.descriptorBindingSampledImageUpdateAfterBind)
            indexingFeaturesEnable.descriptorBindingSampledImageUpdateAfterBind = vk::True;
        if (indexingFeaturesSupported.descriptorBindingUniformBufferUpdateAfterBind)
            indexingFeaturesEnable.descriptorBindingUniformBufferUpdateAfterBind = vk::True;
        if (indexingFeaturesSupported.descriptorBindingUpdateUnusedWhilePending)
            indexingFeaturesEnable.descriptorBindingUpdateUnusedWhilePending = vk::True;

        // Prepare Robustness2 features if the extension is enabled and device supports
        auto hasRobust2 = std::find(deviceExtensions.begin(), deviceExtensions.end(), VK_EXT_ROBUSTNESS_2_EXTENSION_NAME) != deviceExtensions.end();
        vk::PhysicalDeviceRobustness2FeaturesEXT robust2Enable{};
        if (hasRobust2) {
            if (robust2Supported.robustBufferAccess2) robust2Enable.robustBufferAccess2 = vk::True;
            if (robust2Supported.robustImageAccess2)  robust2Enable.robustImageAccess2  = vk::True;
            if (robust2Supported.nullDescriptor)      robust2Enable.nullDescriptor      = vk::True;
        }

        // Prepare Dynamic Rendering Local Read features if extension is enabled and supported
        auto hasLocalRead = std::find(deviceExtensions.begin(), deviceExtensions.end(), VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME) != deviceExtensions.end();
        vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR localReadEnable{};
        if (hasLocalRead && localReadSupported.dynamicRenderingLocalRead) {
            localReadEnable.dynamicRenderingLocalRead = vk::True;
        }

        // Prepare Shader Tile Image features if extension is enabled and supported
        auto hasTileImage = std::find(deviceExtensions.begin(), deviceExtensions.end(), VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME) != deviceExtensions.end();
        vk::PhysicalDeviceShaderTileImageFeaturesEXT tileImageEnable{};
        if (hasTileImage) {
            if (tileImageSupported.shaderTileImageColorReadAccess)   tileImageEnable.shaderTileImageColorReadAccess   = vk::True;
            if (tileImageSupported.shaderTileImageDepthReadAccess)   tileImageEnable.shaderTileImageDepthReadAccess   = vk::True;
            if (tileImageSupported.shaderTileImageStencilReadAccess) tileImageEnable.shaderTileImageStencilReadAccess = vk::True;
        }

        // Prepare Acceleration Structure features if extension is enabled and supported
        auto hasAccelerationStructure = std::find(deviceExtensions.begin(), deviceExtensions.end(), VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) != deviceExtensions.end();
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureEnable{};
        if (hasAccelerationStructure && accelerationStructureSupported.accelerationStructure) {
            accelerationStructureEnable.accelerationStructure = vk::True;
        }

        // Prepare Ray Query features if extension is enabled and supported
        auto hasRayQuery = std::find(deviceExtensions.begin(), deviceExtensions.end(), VK_KHR_RAY_QUERY_EXTENSION_NAME) != deviceExtensions.end();
        vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryEnable{};
        if (hasRayQuery && rayQuerySupported.rayQuery) {
            rayQueryEnable.rayQuery = vk::True;
        }

        // Chain the feature structures together (build pNext chain explicitly)
        // Base
        features.pNext = &timelineSemaphoreFeatures;
        timelineSemaphoreFeatures.pNext = &memoryModelFeatures;
        memoryModelFeatures.pNext = &bufferDeviceAddressFeatures;
        bufferDeviceAddressFeatures.pNext = &storage8BitFeatures;
        storage8BitFeatures.pNext = &vulkan11Features; // link 1.1 first
        vulkan11Features.pNext = &vulkan13Features;    // then 1.3 features

        // Build tail chain starting at Vulkan 1.3 features
        vulkan13Features.pNext = nullptr;
        void** tailNext = reinterpret_cast<void**>(&vulkan13Features.pNext);
        if (descriptorIndexingEnabled) {
            indexingFeaturesEnable.pNext = nullptr;
            *tailNext = &indexingFeaturesEnable;
            tailNext = reinterpret_cast<void**>(&indexingFeaturesEnable.pNext);
        }
        if (hasRobust2) {
            robust2Enable.pNext = nullptr;
            *tailNext = &robust2Enable;
            tailNext = reinterpret_cast<void**>(&robust2Enable.pNext);
        }
        if (hasLocalRead) {
            localReadEnable.pNext = nullptr;
            *tailNext = &localReadEnable;
            tailNext = reinterpret_cast<void**>(&localReadEnable.pNext);
        }
        if (hasTileImage) {
            tileImageEnable.pNext = nullptr;
            *tailNext = &tileImageEnable;
            tailNext = reinterpret_cast<void**>(&tileImageEnable.pNext);
        }
        if (hasAccelerationStructure) {
            accelerationStructureEnable.pNext = nullptr;
            *tailNext = &accelerationStructureEnable;
            tailNext = reinterpret_cast<void**>(&accelerationStructureEnable.pNext);
        }
        if (hasRayQuery) {
            rayQueryEnable.pNext = nullptr;
            *tailNext = &rayQueryEnable;
            tailNext = reinterpret_cast<void**>(&rayQueryEnable.pNext);
        }

        // Record which features ended up enabled (for runtime decisions/tutorial diagnostics)
        robustness2Enabled = hasRobust2 && (
            robust2Enable.robustBufferAccess2 == vk::True ||
            robust2Enable.robustImageAccess2  == vk::True ||
            robust2Enable.nullDescriptor      == vk::True);
        dynamicRenderingLocalReadEnabled = hasLocalRead && (localReadEnable.dynamicRenderingLocalRead == vk::True);
        shaderTileImageEnabled = hasTileImage && (
            tileImageEnable.shaderTileImageColorReadAccess   == vk::True ||
            tileImageEnable.shaderTileImageDepthReadAccess   == vk::True ||
            tileImageEnable.shaderTileImageStencilReadAccess == vk::True);
        accelerationStructureEnabled = hasAccelerationStructure && (accelerationStructureEnable.accelerationStructure == vk::True);
        rayQueryEnabled = hasRayQuery && (rayQueryEnable.rayQuery == vk::True);

        // One-time startup diagnostics (Ray Query + texture array indexing)
        static bool printedFeatureDiag = false;
        if (!printedFeatureDiag) {
            printedFeatureDiag = true;
            std::cout << "[DeviceFeatures] shaderSampledImageArrayDynamicIndexing="
                      << (features.features.shaderSampledImageArrayDynamicIndexing == vk::True ? "ON" : "OFF")
                      << ", shaderSampledImageArrayNonUniformIndexing="
                      << (indexingFeaturesEnable.shaderSampledImageArrayNonUniformIndexing == vk::True ? "ON" : "OFF")
                      << ", descriptorIndexingEnabled="
                      << (descriptorIndexingEnabled ? "true" : "false")
                      << "\n";
        }

        // Create a device. Device layers are deprecated and ignored, so we
        // only configure extensions and features here; validation is enabled
        // via instance layers.
        vk::DeviceCreateInfo createInfo{
            .pNext = &features,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = nullptr // Using pNext for features
        };

        // Create the logical device
        device = vk::raii::Device(physicalDevice, createInfo);

        // Get queue handles
        graphicsQueue = vk::raii::Queue(device, queueFamilyIndices.graphicsFamily.value(), 0);
        presentQueue = vk::raii::Queue(device, queueFamilyIndices.presentFamily.value(), 0);
        computeQueue = vk::raii::Queue(device, queueFamilyIndices.computeFamily.value(), 0);
        transferQueue = vk::raii::Queue(device, queueFamilyIndices.transferFamily.value(), 0);

        // Create global timeline semaphore for uploads early (needed before default texture creation)
        vk::SemaphoreTypeCreateInfo typeInfo{
            .semaphoreType = vk::SemaphoreType::eTimeline,
            .initialValue = 0
        };
        vk::SemaphoreCreateInfo timelineCreateInfo{ .pNext = &typeInfo };
        uploadsTimeline = vk::raii::Semaphore(device, timelineCreateInfo);
        uploadTimelineLastSubmitted.store(0, std::memory_order_relaxed);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create logical device: " << e.what() << std::endl;
        return false;
    }
}

// Check validation layer support
bool Renderer::checkValidationLayerSupport() const {
    // Get available layers
    std::vector<vk::LayerProperties> availableLayers = context.enumerateInstanceLayerProperties();

    // Check if all requested layers are available
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}
