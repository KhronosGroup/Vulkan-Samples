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
#include "memory_pool.h"
#include <algorithm>
#include <iostream>
#include <vulkan/vulkan.hpp>

MemoryPool::MemoryPool(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice) :
    device(device), physicalDevice(physicalDevice)
{
}

MemoryPool::~MemoryPool()
{
	// RAII will handle cleanup automatically
	std::lock_guard lock(poolMutex);
	pools.clear();
}

bool MemoryPool::initialize()
{
	std::lock_guard lock(poolMutex);

	try
	{
		// Configure default pool settings based on typical usage patterns

		// Vertex buffer pool: Large allocations, device-local (increased for large models like bistro)
		configurePool(
		    PoolType::VERTEX_BUFFER,
		    128 * 1024 * 1024,        // 128MB blocks (doubled)
		    4096,                     // 4KB allocation units
		    vk::MemoryPropertyFlagBits::eDeviceLocal);

		// Index buffer pool: Medium allocations, device-local (increased for large models like bistro)
		configurePool(
		    PoolType::INDEX_BUFFER,
		    64 * 1024 * 1024,        // 64MB blocks (doubled)
		    2048,                    // 2KB allocation units
		    vk::MemoryPropertyFlagBits::eDeviceLocal);

		// Uniform buffer pool: Small allocations, host-visible
		// Use 64-byte alignment to match nonCoherentAtomSize and prevent validation errors
		configurePool(
		    PoolType::UNIFORM_BUFFER,
		    4 * 1024 * 1024,        // 4MB blocks
		    64,                     // 64B allocation units (aligned to nonCoherentAtomSize)
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		// Staging buffer pool: Variable allocations, host-visible
		// Use 64-byte alignment to match nonCoherentAtomSize and prevent validation errors
		configurePool(
		    PoolType::STAGING_BUFFER,
		    16 * 1024 * 1024,        // 16MB blocks
		    64,                      // 64B allocation units (aligned to nonCoherentAtomSize)
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		// Texture image pool: Use moderate block sizes to reduce allocation failures on mid-range GPUs
		configurePool(
		    PoolType::TEXTURE_IMAGE,
		    64 * 1024 * 1024,        // 64MB blocks (smaller blocks reduce contiguous allocation pressure)
		    4096,                    // 4KB allocation units
		    vk::MemoryPropertyFlagBits::eDeviceLocal);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to initialize memory pool: " << e.what() << std::endl;
		return false;
	}
}

void MemoryPool::configurePool(
    const PoolType                poolType,
    const vk::DeviceSize          blockSize,
    const vk::DeviceSize          allocationUnit,
    const vk::MemoryPropertyFlags properties)
{
	PoolConfig config;
	config.blockSize      = blockSize;
	config.allocationUnit = allocationUnit;
	config.properties     = properties;

	poolConfigs[poolType] = config;
}

uint32_t MemoryPool::findMemoryType(const uint32_t typeFilter, const vk::MemoryPropertyFlags properties) const
{
	const vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) &&
		    (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}

std::unique_ptr<MemoryPool::MemoryBlock> MemoryPool::createMemoryBlock(PoolType poolType, vk::DeviceSize size, vk::MemoryAllocateFlags allocFlags)
{
	auto configIt = poolConfigs.find(poolType);
	if (configIt == poolConfigs.end())
	{
		throw std::runtime_error("Pool type not configured");
	}

	const PoolConfig &config = configIt->second;

	// Use the larger of the requested size or configured block size
	const vk::DeviceSize blockSize = std::max(size, config.blockSize);

	// Create a dummy buffer to get memory requirements for the memory type
	vk::BufferCreateInfo bufferInfo{
	    .size  = blockSize,
	    .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer |
	             vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc |
	             vk::BufferUsageFlagBits::eTransferDst,
	    .sharingMode = vk::SharingMode::eExclusive};

	vk::raii::Buffer       dummyBuffer(device, bufferInfo);
	vk::MemoryRequirements memRequirements = dummyBuffer.getMemoryRequirements();

	uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, config.properties);

	// Allocate the memory block using the device-required size
	vk::MemoryAllocateInfo allocInfo{
	    .allocationSize  = memRequirements.size,
	    .memoryTypeIndex = memoryTypeIndex};

	// Add allocation flags (e.g., VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT) if needed
	vk::MemoryAllocateFlagsInfo flagsInfo{};
	if (allocFlags != vk::MemoryAllocateFlags{})
	{
		flagsInfo.flags = allocFlags;
		allocInfo.pNext = &flagsInfo;
	}

	// Create MemoryBlock with proper initialization to avoid default constructor issues
	auto block = std::unique_ptr<MemoryBlock>(new MemoryBlock{
	    .memory          = vk::raii::DeviceMemory(device, allocInfo),
	    .size            = memRequirements.size,
	    .used            = 0,
	    .memoryTypeIndex = memoryTypeIndex,
	    .isMapped        = false,
	    .mappedPtr       = nullptr,
	    .freeList        = {},
	    .allocationUnit  = config.allocationUnit});

	// Map memory if it's host-visible
	block->isMapped = (config.properties & vk::MemoryPropertyFlagBits::eHostVisible) != vk::MemoryPropertyFlags{};
	if (block->isMapped)
	{
		block->mappedPtr = block->memory.mapMemory(0, memRequirements.size);
	}
	else
	{
		block->mappedPtr = nullptr;
	}

	// Initialize a free list based on the actual allocated size
	const size_t numUnits = static_cast<size_t>(block->size / config.allocationUnit);
	block->freeList.resize(numUnits, true);        // All units initially free

	return block;
}

std::unique_ptr<MemoryPool::MemoryBlock> MemoryPool::createMemoryBlockWithType(PoolType poolType, vk::DeviceSize size, uint32_t memoryTypeIndex, vk::MemoryAllocateFlags allocFlags)
{
	auto configIt = poolConfigs.find(poolType);
	if (configIt == poolConfigs.end())
	{
		throw std::runtime_error("Pool type not configured");
	}
	const PoolConfig &config = configIt->second;

	// Allocate the memory block with the exact requested size
	vk::MemoryAllocateInfo allocInfo{
	    .allocationSize  = size,
	    .memoryTypeIndex = memoryTypeIndex};

	// Add allocation flags (e.g., VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT) if needed
	vk::MemoryAllocateFlagsInfo flagsInfo{};
	if (allocFlags != vk::MemoryAllocateFlags{})
	{
		flagsInfo.flags = allocFlags;
		allocInfo.pNext = &flagsInfo;
	}

	// Determine properties from the chosen memory type
	const auto memProps = physicalDevice.getMemoryProperties();
	if (memoryTypeIndex >= memProps.memoryTypeCount)
	{
		throw std::runtime_error("Invalid memoryTypeIndex for createMemoryBlockWithType");
	}
	const vk::MemoryPropertyFlags typeProps = memProps.memoryTypes[memoryTypeIndex].propertyFlags;

	auto block = std::unique_ptr<MemoryBlock>(new MemoryBlock{
	    .memory          = vk::raii::DeviceMemory(device, allocInfo),
	    .size            = size,
	    .used            = 0,
	    .memoryTypeIndex = memoryTypeIndex,
	    .isMapped        = false,
	    .mappedPtr       = nullptr,
	    .freeList        = {},
	    .allocationUnit  = config.allocationUnit});

	block->isMapped = (typeProps & vk::MemoryPropertyFlagBits::eHostVisible) != vk::MemoryPropertyFlags{};
	if (block->isMapped)
	{
		block->mappedPtr = block->memory.mapMemory(0, size);
	}

	const size_t numUnits = static_cast<size_t>(block->size / config.allocationUnit);
	block->freeList.resize(numUnits, true);

	return block;
}

std::pair<MemoryPool::MemoryBlock *, size_t> MemoryPool::findSuitableBlock(PoolType poolType, vk::DeviceSize size, vk::DeviceSize alignment)
{
	auto poolIt = pools.find(poolType);
	if (poolIt == pools.end())
	{
		poolIt = pools.try_emplace(poolType).first;
	}

	auto             &poolBlocks = poolIt->second;
	const PoolConfig &config     = poolConfigs[poolType];

	// Calculate required units (accounting for size alignment)
	const vk::DeviceSize alignedSize   = ((size + alignment - 1) / alignment) * alignment;
	const size_t         requiredUnits = static_cast<size_t>((alignedSize + config.allocationUnit - 1) / config.allocationUnit);

	// Search existing blocks for sufficient free space with proper offset alignment
	for (const auto &block : poolBlocks)
	{
		const vk::DeviceSize unit       = config.allocationUnit;
		const size_t         totalUnits = block->freeList.size();

		size_t i = 0;
		while (i < totalUnits)
		{
			// Ensure starting unit produces an offset aligned to 'alignment'
			vk::DeviceSize startOffset = static_cast<vk::DeviceSize>(i) * unit;
			if ((alignment > 0) && (startOffset % alignment != 0))
			{
				// Advance i to the next unit that aligns with 'alignment'
				const vk::DeviceSize remainder    = startOffset % alignment;
				const vk::DeviceSize advanceBytes = alignment - remainder;
				const size_t         advanceUnits = static_cast<size_t>((advanceBytes + unit - 1) / unit);
				i += std::max<size_t>(advanceUnits, 1);
				continue;
			}

			// From aligned i, check for consecutive free units
			size_t consecutiveFree = 0;
			size_t j               = i;
			while (j < totalUnits && block->freeList[j] && consecutiveFree < requiredUnits)
			{
				++consecutiveFree;
				++j;
			}

			if (consecutiveFree >= requiredUnits)
			{
				return {block.get(), i};
			}

			// Move past the checked range
			i = (j > i) ? j : (i + 1);
		}
	}

	// No suitable block found; create a new one on demand (no hard limits, allowed during rendering)
	try
	{
		auto newBlock = createMemoryBlock(poolType, alignedSize);
		poolBlocks.push_back(std::move(newBlock));
		std::cout << "Created new memory block (pool type: "
		          << static_cast<int>(poolType) << ")" << std::endl;
		return {poolBlocks.back().get(), 0};
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create new memory block: " << e.what() << std::endl;
		return {nullptr, 0};
	}
}

std::unique_ptr<MemoryPool::Allocation> MemoryPool::allocate(PoolType poolType, vk::DeviceSize size, vk::DeviceSize alignment)
{
	std::lock_guard<std::mutex> lock(poolMutex);

	auto [block, startUnit] = findSuitableBlock(poolType, size, alignment);
	if (!block)
	{
		return nullptr;
	}

	const PoolConfig &config = poolConfigs[poolType];

	// Calculate required units (accounting for alignment)
	const vk::DeviceSize alignedSize   = ((size + alignment - 1) / alignment) * alignment;
	const size_t         requiredUnits = (alignedSize + config.allocationUnit - 1) / config.allocationUnit;

	// Mark units as used
	for (size_t i = startUnit; i < startUnit + requiredUnits; ++i)
	{
		block->freeList[i] = false;
	}

	// Create allocation info
	auto allocation             = std::make_unique<Allocation>();
	allocation->memory          = *block->memory;
	allocation->offset          = startUnit * config.allocationUnit;
	allocation->size            = alignedSize;
	allocation->memoryTypeIndex = block->memoryTypeIndex;
	allocation->isMapped        = block->isMapped;
	allocation->mappedPtr       = block->isMapped ?
	                                  static_cast<char *>(block->mappedPtr) + allocation->offset :
	                                  nullptr;

	block->used += alignedSize;

	return allocation;
}

void MemoryPool::deallocate(std::unique_ptr<Allocation> allocation)
{
	if (!allocation)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(poolMutex);

	// Find the block that contains this allocation
	for (auto &[poolType, poolBlocks] : pools)
	{
		const PoolConfig &config = poolConfigs[poolType];

		for (auto &block : poolBlocks)
		{
			if (*block->memory == allocation->memory)
			{
				// Calculate which units to free
				size_t startUnit = allocation->offset / config.allocationUnit;
				size_t numUnits  = (allocation->size + config.allocationUnit - 1) / config.allocationUnit;

				// Mark units as free
				for (size_t i = startUnit; i < startUnit + numUnits; ++i)
				{
					if (i < block->freeList.size())
					{
						block->freeList[i] = true;
					}
				}

				block->used -= allocation->size;
				return;
			}
		}
	}

	std::cerr << "Warning: Could not find memory block for deallocation" << std::endl;
}

std::pair<vk::raii::Buffer, std::unique_ptr<MemoryPool::Allocation>> MemoryPool::createBuffer(
    const vk::DeviceSize          size,
    const vk::BufferUsageFlags    usage,
    const vk::MemoryPropertyFlags properties)
{
	// Determine a pool type based on usage and properties
	PoolType poolType = PoolType::VERTEX_BUFFER;

	// Check for host-visible requirements first (for instance buffers and staging)
	if (properties & vk::MemoryPropertyFlagBits::eHostVisible)
	{
		poolType = PoolType::STAGING_BUFFER;
	}
	else if (usage & vk::BufferUsageFlagBits::eVertexBuffer)
	{
		poolType = PoolType::VERTEX_BUFFER;
	}
	else if (usage & vk::BufferUsageFlagBits::eIndexBuffer)
	{
		poolType = PoolType::INDEX_BUFFER;
	}
	else if (usage & vk::BufferUsageFlagBits::eUniformBuffer)
	{
		poolType = PoolType::UNIFORM_BUFFER;
	}

	// Create the buffer
	const vk::BufferCreateInfo bufferInfo{
	    .size        = size,
	    .usage       = usage,
	    .sharingMode = vk::SharingMode::eExclusive};

	vk::raii::Buffer buffer(device, bufferInfo);

	// Get memory requirements
	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

	// Check if buffer requires device address support (for ray tracing)
	const bool needsDeviceAddress = (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) != vk::BufferUsageFlags{};

	std::unique_ptr<Allocation> allocation;

	if (needsDeviceAddress)
	{
		// Buffers with device address usage require VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT flag
		// Create a dedicated memory block for this buffer (similar to image allocation)
		uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		std::lock_guard<std::mutex> lock(poolMutex);
		auto                        poolIt = pools.find(poolType);
		if (poolIt == pools.end())
		{
			poolIt = pools.try_emplace(poolType).first;
		}
		auto &poolBlocks = poolIt->second;
		auto  block      = createMemoryBlockWithType(poolType, memRequirements.size, memoryTypeIndex,
		                                             vk::MemoryAllocateFlagBits::eDeviceAddress);

		// Prepare allocation that uses the new block from offset 0
		allocation                  = std::make_unique<Allocation>();
		allocation->memory          = *block->memory;
		allocation->offset          = 0;
		allocation->size            = memRequirements.size;
		allocation->memoryTypeIndex = memoryTypeIndex;
		allocation->isMapped        = block->isMapped;
		allocation->mappedPtr       = block->mappedPtr;

		// Mark the entire block as used
		block->used        = memRequirements.size;
		const size_t units = block->freeList.size();
		for (size_t i = 0; i < units; ++i)
		{
			block->freeList[i] = false;
		}

		// Keep the block owned by the pool for lifetime management
		poolBlocks.push_back(std::move(block));
	}
	else
	{
		// Normal pooled allocation path
		allocation = allocate(poolType, memRequirements.size, memRequirements.alignment);
		if (!allocation)
		{
			throw std::runtime_error("Failed to allocate memory from pool");
		}
	}

	// Bind memory to buffer
	buffer.bindMemory(allocation->memory, allocation->offset);

	return {std::move(buffer), std::move(allocation)};
}

std::pair<vk::raii::Image, std::unique_ptr<MemoryPool::Allocation>> MemoryPool::createImage(
    uint32_t                     width,
    uint32_t                     height,
    vk::Format                   format,
    vk::ImageTiling              tiling,
    vk::ImageUsageFlags          usage,
    vk::MemoryPropertyFlags      properties,
    uint32_t                     mipLevels,
    vk::SharingMode              sharingMode,
    const std::vector<uint32_t> &queueFamilyIndices)
{
	// Create the image
	vk::ImageCreateInfo imageInfo{
	    .imageType     = vk::ImageType::e2D,
	    .format        = format,
	    .extent        = {width, height, 1},
	    .mipLevels     = std::max(1u, mipLevels),
	    .arrayLayers   = 1,
	    .samples       = vk::SampleCountFlagBits::e1,
	    .tiling        = tiling,
	    .usage         = usage,
	    .sharingMode   = sharingMode,
	    .initialLayout = vk::ImageLayout::eUndefined};

	// If concurrent sharing is requested, provide queue family indices
	std::vector<uint32_t> fam = queueFamilyIndices;
	if (sharingMode == vk::SharingMode::eConcurrent && !fam.empty())
	{
		imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(fam.size());
		imageInfo.pQueueFamilyIndices   = fam.data();
	}

	vk::raii::Image image(device, imageInfo);

	// Get memory requirements for this image
	vk::MemoryRequirements memRequirements = image.getMemoryRequirements();

	// Pick a memory type compatible with this image
	uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	// Create a dedicated memory block for this image with the exact type and size
	std::unique_ptr<Allocation> allocation;
	{
		std::lock_guard<std::mutex> lock(poolMutex);
		auto                        poolIt = pools.find(PoolType::TEXTURE_IMAGE);
		if (poolIt == pools.end())
		{
			poolIt = pools.try_emplace(PoolType::TEXTURE_IMAGE).first;
		}
		auto &poolBlocks = poolIt->second;
		auto  block      = createMemoryBlockWithType(PoolType::TEXTURE_IMAGE, memRequirements.size, memoryTypeIndex);

		// Prepare allocation that uses the new block from offset 0
		allocation                  = std::make_unique<Allocation>();
		allocation->memory          = *block->memory;
		allocation->offset          = 0;
		allocation->size            = memRequirements.size;
		allocation->memoryTypeIndex = memoryTypeIndex;
		allocation->isMapped        = block->isMapped;
		allocation->mappedPtr       = block->mappedPtr;

		// Mark the entire block as used
		block->used        = memRequirements.size;
		const size_t units = block->freeList.size();
		for (size_t i = 0; i < units; ++i)
		{
			block->freeList[i] = false;
		}

		// Keep the block owned by the pool for lifetime management and deallocation support
		poolBlocks.push_back(std::move(block));
	}

	// Bind memory to image
	image.bindMemory(allocation->memory, allocation->offset);

	return {std::move(image), std::move(allocation)};
}

std::pair<vk::DeviceSize, vk::DeviceSize> MemoryPool::getMemoryUsage(PoolType poolType) const
{
	std::lock_guard<std::mutex> lock(poolMutex);

	auto poolIt = pools.find(poolType);
	if (poolIt == pools.end())
	{
		return {0, 0};
	}

	vk::DeviceSize used  = 0;
	vk::DeviceSize total = 0;

	for (const auto &block : poolIt->second)
	{
		used += block->used;
		total += block->size;
	}

	return {used, total};
}

std::pair<vk::DeviceSize, vk::DeviceSize> MemoryPool::getTotalMemoryUsage() const
{
	std::lock_guard<std::mutex> lock(poolMutex);

	vk::DeviceSize totalUsed      = 0;
	vk::DeviceSize totalAllocated = 0;

	for (const auto &[poolType, poolBlocks] : pools)
	{
		for (const auto &block : poolBlocks)
		{
			totalUsed += block->used;
			totalAllocated += block->size;
		}
	}

	return {totalUsed, totalAllocated};
}

bool MemoryPool::preAllocatePools()
{
	std::lock_guard<std::mutex> lock(poolMutex);

	try
	{
		std::cout << "Pre-allocating initial memory blocks for pools..." << std::endl;

		// Pre-allocate at least one block for each pool type
		for (const auto &[poolType, config] : poolConfigs)
		{
			auto poolIt = pools.find(poolType);
			if (poolIt == pools.end())
			{
				poolIt = pools.try_emplace(poolType).first;
			}

			auto &poolBlocks = poolIt->second;
			if (poolBlocks.empty())
			{
				// Create initial block for this pool type
				auto newBlock = createMemoryBlock(poolType, config.blockSize);
				poolBlocks.push_back(std::move(newBlock));
				std::cout << "  Pre-allocated block for pool type " << static_cast<int>(poolType) << std::endl;
			}
		}

		std::cout << "Memory pool pre-allocation completed successfully" << std::endl;
		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to pre-allocate memory pools: " << e.what() << std::endl;
		return false;
	}
}

void MemoryPool::setRenderingActive(bool active)
{
	std::lock_guard lock(poolMutex);
	renderingActive = active;
}

bool MemoryPool::isRenderingActive() const
{
	std::lock_guard<std::mutex> lock(poolMutex);
	return renderingActive;
}
