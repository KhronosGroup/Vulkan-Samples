#include <catch2/catch_test_macros.hpp>

#include <components/vulkan/context/context_builder.hpp>

using namespace components;

TEST_CASE("create basic instance", "[vulkan][context]")
{
	// failed to initialise meta loader
	REQUIRE((vulkan::init_meta_loader() == VK_SUCCESS));

	vulkan::ContextBuilder builder;

	// clang-format off
  builder
    .configure_instance()
      .application_info(vulkan::default_application_info(VK_API_VERSION_1_2))
      .enable_validation_layers()
      .enable_debug_logger()
      .done()
    .select_gpu()
      .score_device(
        vulkan::scores::combined_scoring({
          vulkan::scores::device_preference({VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU}),
          vulkan::scores::has_queue(VK_QUEUE_GRAPHICS_BIT, 1),
        })
      )
      .done()
    .configure_device()
      .enable_queue(VK_QUEUE_GRAPHICS_BIT, 1)
      .done();
	// clang-format on

	vulkan::ContextPtr context = builder.build();
	REQUIRE(context != nullptr);
	REQUIRE(context->instance != VK_NULL_HANDLE);
	REQUIRE(context->gpu != VK_NULL_HANDLE);
	REQUIRE(context->device != VK_NULL_HANDLE);
}
