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

RayQueries::RayQueries()
{
    title = "Ray queries";
    // SPIRV 1.4 requires Vulkan 1.1
    set_api_version(VK_API_VERSION_1_1);

    // Ray tracing related extensions required by this sample
    add_device_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

    // Required by VK_KHR_acceleration_structure
    add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    add_device_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

    // Required for VK_KHR_ray_tracing_pipeline
    add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

    // Required by VK_KHR_spirv_1_4
    add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

RayQueries::~RayQueries()
{
    if (device)
    {
        vertex_buffer.reset();
        index_buffer.reset();
    }
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

void RayQueries::build_command_buffers()
{

}


bool RayQueries::prepare(vkb::Platform &platform)
{
    if (!VulkanSample::prepare(platform))
    {
        return false;
    }

    camera.type = vkb::CameraType::FirstPerson;
    camera.set_perspective(60.0f, (float) width / (float) height, 0.1f, 512.0f);
    camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.set_translation(glm::vec3(0.0f, 1.5f, 0.f));

    vkb::ShaderSource vert_shader("ray_queries/ray_shadow.vert");
    vkb::ShaderSource frag_shader("ray_queries/ray_shadow.frag");

    load_scene();
    create_uniforms();

    throw -1;

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
            const float sponza_scale = 0.01f;
            const glm::mat3x4 transform                = glm::mat3x4{0.f, 0.f, sponza_scale, 4.3f,
                                                        sponza_scale, 0.f, 0.f, 0.f,
                                                        0.f, sponza_scale, 0.f, 9.5f};
            for (size_t i = 0; i < pts_.size(); ++i)
            {
                const auto translation = glm::vec3(transform[0][3], transform[1][3], transform[2][3]);
                const auto pt                     = glm::vec3(glm::mat4(transform) * glm::vec4(pts_[i], 1.f)) + translation;
                model.vertices[start_index + i].position = pt;
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

void RayQueries::create_uniforms()
{
    // Note that in contrast to a typical pipeline, our vertex/index buffer requires the acceleration structure build flag
    static constexpr VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    const auto vertex_buffer_size = model.vertices.size() * sizeof(model.vertices[0]);
    const auto index_buffer_size  = model.indices.size() * sizeof(model.indices[0]);
    vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
                                                        vertex_buffer_size,
                                                        buffer_usage_flags,
                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);
    index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
                                                       index_buffer_size,
                                                       buffer_usage_flags,
                                                       VMA_MEMORY_USAGE_CPU_TO_GPU);
    if (vertex_buffer_size){
        vertex_buffer->update(model.vertices.data(), vertex_buffer_size);
    }
    if (index_buffer_size){
        index_buffer->update(model.indices.data(), index_buffer_size);
    }

    uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
                                                         index_buffer_size,
                                                         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
    update_uniform_buffers();
}

void RayQueries::update_uniform_buffers()
{
    assert(!!uniform_buffer);
    uniform_buffer->update(&global_uniform, sizeof(global_uniform));
}

std::unique_ptr<vkb::VulkanSample> create_ray_queries()
{
    return std::make_unique<RayQueries>();
}
