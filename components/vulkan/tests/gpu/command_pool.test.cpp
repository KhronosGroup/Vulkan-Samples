#include <catch2/catch_test_macros.hpp>

#include "macros.hpp"
#include <components/vulkan/context/context_builder.hpp>
#include <components/vulkan/pools/command_pool.hpp>
#include <components/vulkan/pools/fence_pool.hpp>
#include <components/vulkan/pools/memory_pool.hpp>

using namespace components;

inline vulkan::ContextPtr create_context(vulkan::QueuePtr *transfer_queue)
{
	// failed to initialise meta loader
	REQUIRE((vulkan::init_meta_loader() == VK_SUCCESS));

	vulkan::ContextBuilder builder;

	builder
	    .configure_instance()
	    .application_info(vulkan::default_application_info(VK_API_VERSION_1_2))
	    .enable_validation_layers()
	    .enable_debug_logger()
	    .done();

	builder
	    .select_gpu()
	    .score_device(
	        vulkan::scores::combined_scoring({
	            vulkan::scores::device_preference({VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU}),
	        }))
	    .done();

	builder
	    .configure_device()
	    .done();

	builder.request_queue(VK_QUEUE_TRANSFER_BIT, {}, transfer_queue);

	return builder.build();
}

TEST_CASE("create command pool", "[vulkan][pools]")
{
	vulkan::QueuePtr transfer_queue;
	auto             context = create_context(&transfer_queue);

	vulkan::CommandPool pool{context, transfer_queue};
}

TEST_CASE("staging buffer allocation", "[vulkan][pools]")
{
	vulkan::QueuePtr transfer_queue;
	auto             context = create_context(&transfer_queue);

	// hello triangle positions
	std::vector<float> positions{
	    {
	        -0.5f,
	        -0.5f,
	        0.0f,
	        0.5f,
	        -0.5f,
	        0.0f,
	        0.0f,
	        0.5f,
	        0.0f,
	    },
	};

	vulkan::MemoryPool memory_pool{context};

	// create staging buffer data

	std::weak_ptr<vulkan::Allocation> staging_allocation;

	{
		VkBufferCreateInfo buffer_create_info{};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size  = static_cast<VkDeviceSize>(sizeof(float) * positions.size());
		buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo allocation_create_info{};
		allocation_create_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		staging_allocation           = memory_pool.allocate(buffer_create_info, allocation_create_info);

		auto staging_buffer = staging_allocation.lock();

		REQUIRE(staging_buffer != nullptr);

		// upload staging buffer data

		staging_buffer->upload(positions);
	}

	// create GPU buffer

	std::weak_ptr<vulkan::Allocation> gpu_allocation;

	{
		VkBufferCreateInfo buffer_create_info{};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size  = static_cast<VkDeviceSize>(sizeof(float) * positions.size());
		buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo allocation_create_info{};
		allocation_create_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		gpu_allocation               = memory_pool.allocate(buffer_create_info, allocation_create_info);
	}

	vulkan::FencePool fence_pool{context};
	auto              fence = fence_pool.acquire_fence();

	vulkan::CommandPool command_pool{context, transfer_queue};

	auto cmd = command_pool.allocate_command_buffer();

	{
		// begin command buffer
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext            = nullptr;
		begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(cmd, &begin_info);
	}

	{
		// record copy command

		memory_pool.record_copy(cmd, staging_allocation, gpu_allocation);
	}

	{
		// end command buffer

		vkEndCommandBuffer(cmd);
	}

	{
		// submit command buffer

		VkSubmitInfo submit_info{};
		submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &cmd;

		vkQueueSubmit(transfer_queue->get_handle(), 1, &submit_info, fence);
	}

	fence_pool.reset_fence(fence);
}
