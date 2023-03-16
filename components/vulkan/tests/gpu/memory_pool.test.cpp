#include <catch2/catch_test_macros.hpp>

#include <components/vulkan/context/context_builder.hpp>
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

TEST_CASE("create memory pool", "[vulkan][pools]")
{
	vulkan::QueuePtr transfer_queue;
	auto             context = create_context(&transfer_queue);

	{
		vulkan::MemoryPool pool{context};
	}
}

TEST_CASE("create buffer allocation", "[vulkan][pools]")
{
	vulkan::QueuePtr transfer_queue;
	auto             context = create_context(&transfer_queue);

	{
		vulkan::MemoryPool pool{context};

		VkBufferCreateInfo buffer_create_info{};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size  = 1024;
		buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VmaAllocationCreateInfo allocation_create_info{};
		allocation_create_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		auto allocation = pool.allocate(buffer_create_info, allocation_create_info);

		REQUIRE(allocation.lock() != nullptr);
		REQUIRE(pool.allocation_count() == 1);

		pool.free(allocation);

		REQUIRE(pool.allocation_count() == 0);
	}
}