//
// Created by swinston on 12/12/24.
//

#ifndef BACKBUFFER_H
#define BACKBUFFER_H

#include <chrono>
#include "vulkan/vulkan.h"

typedef enum BACK_BUFFER_STATE { BACK_BUFFER_INIT, BACK_BUFFER_PREPARE, BACK_BUFFER_IN_SWAPCHAIN, BACK_BUFFER_CANCELED } BACK_BUFFER_STATE;

struct AcquireBuffer {

	AcquireBuffer();
	~AcquireBuffer();
	VkResult Create();

	VkSemaphore                                                 m_semaphore;
	VkFence                                                     m_fence;
};

class BackBuffer {

public:
	BackBuffer();
	~BackBuffer();
	VkResult Create();


	AcquireBuffer* SetAcquireBuffer(uint32_t imageIndex, AcquireBuffer* acquireBuffer) {
		AcquireBuffer* oldAcquireBuffer = m_acquireBuffer;
		m_imageIndex = imageIndex;
		m_acquireBuffer = acquireBuffer;
		return oldAcquireBuffer;
	}

	const VkSemaphore* GetAcquireSemaphore() const {
		if (m_acquireBuffer)
		{
			return &m_acquireBuffer->m_semaphore;
		}
		return nullptr;
	}

	const VkSemaphore& GetRenderSemaphore() const {
		return m_renderSemaphore;
	}

	uint32_t GetImageIndex() const {
		return m_imageIndex;
	}

private:
	uint32_t                   m_imageIndex;

	AcquireBuffer*             m_acquireBuffer;
	VkSemaphore                m_renderSemaphore;

public:
	mutable std::chrono::nanoseconds                                    m_lastFrameTime;
	mutable std::chrono::time_point<std::chrono::high_resolution_clock> m_lastPresentTime;
	mutable std::chrono::nanoseconds                                    m_targetTimeDelta;
	mutable std::chrono::time_point<std::chrono::high_resolution_clock> m_framePresentAtTime;
};


#endif //BACKBUFFER_H
