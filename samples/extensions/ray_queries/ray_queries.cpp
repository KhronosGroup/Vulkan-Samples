#include "ray_queries.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"
#include "platform/platform.h"
#include "scene_graph/components/perspective_camera.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
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

    template <typename T>
    struct CopyBuffer
    {
        std::vector<T> operator()(std::unordered_map<std::string, vkb::core::Buffer> &buffers, const char *bufferName)
        {
            auto iter = buffers.find(bufferName);
            if (iter == buffers.cend())
            {
                return {};
            }
            auto &         buffer = iter->second;
            std::vector<T> out;

            const size_t sz = buffer.get_size();
            out.resize(sz / sizeof(T));
            const bool alreadyMapped = buffer.get_data() != nullptr;
            if (!alreadyMapped)
            {
                buffer.map();
            }
            memcpy(&out[0], buffer.get_data(), sz);
            if (!alreadyMapped)
            {
                buffer.unmap();
            }
            return out;
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

void RayQueries::render(float delta_time)
{

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

    auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
    camera            = &camera_node.get_component<vkb::sg::Camera>();

    vkb::ShaderSource vert_shader("ray_queries/ray_shadow.vert");
    vkb::ShaderSource frag_shader("ray_queries/ray_shadow.frag");

    load_scene();

    gui = std::make_unique<vkb::Gui>(*this, platform.get_window());

    return true;
}

void RayQueries::load_scene()
{
    model = {};
    vkb::GLTFLoader loader{*device};
    auto scene = loader.read_scene_from_file("scenes/sponza/Sponza01.gltf");

    for (auto &&mesh : scene->get_components<vkb::sg::Mesh>())
    {
        for (auto && sub_mesh : mesh->get_submeshes())
        {
            auto       pts_      = CopyBuffer<glm::vec3>{}(sub_mesh->vertex_buffers, "position");
            const auto normals_  = CopyBuffer<glm::vec3>{}(sub_mesh->vertex_buffers, "normal");

            const uint32_t start_index = model.vertices.size();
            model.vertices.resize(start_index + pts_.size());
            for (size_t i = 0; i < pts_.size(); ++i)
            {
                model.vertices[start_index + i].position = pts_[i];
                model.vertices[start_index + i].normal = i < normals_.size() ? normals_[i] : glm::vec3{0.f, 0.f, 0.f};
            }

            auto index_buffer = sub_mesh->index_buffer.get();
            if (index_buffer)
            {
                const size_t sz         = index_buffer->get_size();
                const size_t nTriangles = sz / sizeof(uint16_t) / 3;
                const size_t triangle_start_index = model.indices.size();
                model.indices.resize(triangle_start_index + nTriangles);
                auto ptr = index_buffer->get_data();
                assert(!!ptr);
                std::vector<uint16_t> tempBuffer(nTriangles * 3);
                memcpy(&tempBuffer[0], ptr, sz);
                for (size_t i = 0; i < nTriangles; ++i)
                {
                    model.indices[triangle_start_index + i] = {start_index + uint32_t(tempBuffer[3 * i]),
                                        start_index + uint32_t(tempBuffer[3 * i + 1]),
                                        start_index + uint32_t(tempBuffer[3 * i + 2])};
                }
            }
        }
    }


}

std::unique_ptr<vkb::VulkanSample> create_ray_queries()
{
    return std::make_unique<RayQueries>();
}
