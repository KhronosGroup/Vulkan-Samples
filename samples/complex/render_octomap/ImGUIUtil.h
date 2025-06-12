//
// Created by Steven Winston on 4/14/24.
//

#ifndef ORB_SLAM3_IMGUI_H
#define ORB_SLAM3_IMGUI_H

#include "core/buffer.h"
#include "api_vulkan_sample.h"

#include "Screens/MapView.h"
#include <imgui.h>

// ----------------------------------------------------------------------------
// ImGUI class
// ----------------------------------------------------------------------------

class ImGUIUtil {
private:
    // Vulkan resources for rendering the UI
    VkSampler sampler;
    std::unique_ptr<vkb::core::BufferC> vertexBuffer;
    std::unique_ptr<vkb::core::BufferC> indexBuffer;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    VkDeviceMemory fontMemory = VK_NULL_HANDLE;
  std::unique_ptr<vkb::core::Image>     font_image;
  std::unique_ptr<vkb::core::ImageView> font_image_view;
    VkPipelineCache pipelineCache;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkPhysicalDeviceDriverProperties driverProperties = {};
    ApiVulkanSample *base;
    ImGuiStyle vulkanStyle;
    ImFont* montserratExtraBoldNormal;
    ImFont* montserratExtraBoldSmall;
    ImFont* montserratBoldNormal;
    ImFont* montserratRegularNormal;
    int selectedStyle = 0;
    float windowWidth, windowHeight;
    bool needsUpdateBuffers = false;
public:
    enum ViewState {
      LIVEMAPS_ACTIVE,
    }state;

    MapView MapsView;

    // UI params are set via push constants
    struct PushConstBlock {
        glm::vec2 scale;
        glm::vec2 translate;
    } pushConstBlock;

    explicit ImGUIUtil(ApiVulkanSample *_base);
    ~ImGUIUtil();

    // Initialize styles, keys, etc.
    void init(float width, float height);
    void setStyle(uint32_t index);
    static void TextColorAlign(int align, const ImVec4& col, const char* text, ...);

    // Initialize all Vulkan resources used by the ui
    void initResources(VkRenderPass renderPass, VkQueue copyQueue);

    // Starts a new imGui frame and sets up windows and ui elements
    bool newFrame(bool updateFrameGraph);

    // Update vertex and index buffer containing the imGui elements when required
    void updateBuffers();

    // Draw current imGui frame into a command buffer
    void drawFrame(VkCommandBuffer commandBuffer);

  static void handleKey(int key, int scancode, int action, int mode);

  static bool GetWantKeyCapture();

  static void charPressed(uint32_t key);
};

#endif //ORB_SLAM3_IMGUI_H
