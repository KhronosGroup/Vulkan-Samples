#include "ray_queries.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "scene_graph/components/perspective_camera.h"
#include "scene_graph/components/material.h"
#include "rendering/subpasses/forward_subpass.h"

namespace
{
    static constexpr uint32_t MIN_THREAD_COUNT = 1;
    struct RequestFeature
    {
        vkb::PhysicalDevice &gpu;
        RequestFeature(vkb::PhysicalDevice &gpu) : gpu(gpu) {}

        template <typename T>
        RequestFeature& request(VkStructureType sType, VkBool32 T::*member)
        {
            auto &member_feature = gpu.request_extension_features<T>(sType);
            member_feature.*member = VK_TRUE;
            return *this;
        }
    };
}

void RayQueries::request_gpu_features(vkb::PhysicalDevice &gpu)
{
    RequestFeature(gpu)
            .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES, &VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress)
            .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR, &VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure)
            .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR, &VkPhysicalDeviceRayQueryFeaturesKHR::rayQuery)
            //.request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, &VkPhysicalDeviceRayTracingPipelineFeaturesKHR::rayTracingPipeline)
            ;
}

void RayQueries::prepare_render_context()
{
    max_thread_count = std::max(std::thread::hardware_concurrency(), MIN_THREAD_COUNT);
    get_render_context().prepare();
}




void RayQueries::draw(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
    auto &extent = render_target.get_extent();

    VkViewport viewport{};
    viewport.width    = static_cast<float>(extent.width);
    viewport.height   = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    command_buffer.set_viewport(0, {viewport});

    VkRect2D scissor{};
    scissor.extent = extent;
    command_buffer.set_scissor(0, {scissor});

    render_pipeline->draw(command_buffer, render_target);
    command_buffer.end_render_pass();
}


bool RayQueries::prepare(vkb::Platform &platform)
{
    if (!VulkanSample::prepare(platform))
    {
        return false;
    }

    load_scene("scenes/sponza/Sponza01.gltf");

    auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
    camera            = &camera_node.get_component<vkb::sg::Camera>();



    vkb::ShaderSource vert_shader("base.vert");
    vkb::ShaderSource frag_shader("base.frag");
    auto forward_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

    auto render_pipeline = vkb::RenderPipeline();
    render_pipeline.add_subpass(std::move(forward_subpass));
    render_pipeline.set_load_store(vkb::gbuffer::get_clear_all_store_swapchain());
    render_pipeline.set_clear_value(vkb::gbuffer::get_clear_value());
    set_render_pipeline(std::move(render_pipeline));

    gui = std::make_unique<vkb::Gui>(*this, platform.get_window());

    return true;
}



std::unique_ptr<vkb::VulkanSample> create_ray_queries()
{
    return std::make_unique<RayQueries>();
}
