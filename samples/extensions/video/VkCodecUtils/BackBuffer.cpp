//
// Created by swinston on 12/12/24.
//

#include "BackBuffer.h"

#include <VulkanDeviceContext.h>

AcquireBuffer::AcquireBuffer()
	: m_semaphore(VkSemaphore())
	, m_fence(VkFence())
{
}

AcquireBuffer::~AcquireBuffer()
{
	if (m_semaphore) {
		VulkanDeviceContext::GetThe()->DestroySemaphore(VulkanDeviceContext::GetThe()->getDevice(), m_semaphore, nullptr);
		m_semaphore = VkSemaphore(0);
	}

	if (m_fence) {
		VulkanDeviceContext::GetThe()->DestroyFence(VulkanDeviceContext::GetThe()->getDevice(), m_fence, nullptr);
		m_fence = VkFence(0);
	}
}

VkResult AcquireBuffer::Create()
{
	VkSemaphoreCreateInfo sem_info = {};
	sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	// Fence for vkAcquireNextImageKHR must be unsignaled

	auto result = VulkanDeviceContext::GetThe()->CreateSemaphore(VulkanDeviceContext::GetThe()->getDevice(), &sem_info, nullptr, &m_semaphore);
	if (result != VK_SUCCESS) {
		return result;
	}
	result = VulkanDeviceContext::GetThe()->CreateFence(VulkanDeviceContext::GetThe()->getDevice(), &fence_info, nullptr, &m_fence);

	return result;
}

BackBuffer::BackBuffer()
	: m_imageIndex(0)
	, m_acquireBuffer(nullptr)
	, m_renderSemaphore(VkSemaphore())
	, m_lastFrameTime()
	, m_targetTimeDelta()
{
}

VkResult BackBuffer::Create()
{
	VkSemaphoreCreateInfo sem_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	auto result = VulkanDeviceContext::GetThe()->CreateSemaphore(VulkanDeviceContext::GetThe()->getDevice(), &sem_info, nullptr, &m_renderSemaphore);
	return result;
}

BackBuffer::~BackBuffer()
{
	if (m_renderSemaphore) {
		VulkanDeviceContext::GetThe()->DestroySemaphore(VulkanDeviceContext::GetThe()->getDevice(), m_renderSemaphore, nullptr);
		m_renderSemaphore = VK_NULL_HANDLE;
	}

	if (m_acquireBuffer) {
		delete m_acquireBuffer;
		m_acquireBuffer = nullptr;
	}
}