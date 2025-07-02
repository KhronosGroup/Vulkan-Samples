/*
 * Copyright (C) 2016 Google, Inc.
 * Copyright 2020 NVIDIA Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

#ifndef FRAMEPROCESSOR_H
#define FRAMEPROCESSOR_H

#include <chrono>
#include <iostream>
#include <string>

#include "VkCodecUtils/ProgramConfig.h"
#include "VkCodecUtils/VkVideoRefCountBase.h"

class FrameProcessor : public VkVideoRefCountBase {
public:

    enum Key {
        // virtual keys
        KEY_SHUTDOWN,
        // physical keys
        KEY_UNKNOWN,
        KEY_ESC,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_PAGE_UP,
        KEY_PAGE_DOWN,
        KEY_SPACE,
    };

    FrameProcessor(const FrameProcessor &frameProcessor) = delete;
    FrameProcessor &operator=(const FrameProcessor &frameProcessor) = delete;

    virtual int AttachShell(int numBackBuffers) = 0;
    virtual void DetachShell() = 0;

    virtual int AttachSwapchain() = 0;
    virtual void DetachSwapchain() {}

    virtual int CreateFrameData(int count) = 0;
    virtual void DestroyFrameData() = 0;

    virtual bool OnKey(Key key) = 0;

    virtual bool OnFrame( int32_t           renderIndex,
                         uint32_t           waitSemaphoreCount,
                         const VkSemaphore* pWaitSemaphores,
                         uint32_t           signalSemaphoreCount,
                         const VkSemaphore* pSignalSemaphores) = 0;

    uint64_t GetTimeDiffNanoseconds(bool updateStartTime = true)
    {
        auto timeNow = std::chrono::steady_clock::now();
        auto diffNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(timeNow - start_time);
        if (updateStartTime) {
            start_time = timeNow;
        }
        return diffNanoseconds.count();
    }

    float GetFrameRateFps(bool &displayTimeNow, uint64_t* timeDiffNanoseconds = nullptr)
    {
        float fps = 0.0f;
        auto timeNow = std::chrono::steady_clock::now();
        auto diffMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - start_time);
        if (displayTimeNow || (diffMilliseconds.count() >= m_displayTimePeriodMilliseconds)) {
            uint64_t diffNanoseconds = GetTimeDiffNanoseconds();
            if (timeDiffNanoseconds) {
                *timeDiffNanoseconds = diffNanoseconds;
            }
            fps = static_cast<float>(static_cast<double>(m_profileFramesCount * 1000000000) / static_cast<double>(diffNanoseconds));
            m_profileFramesCount = 0;
            displayTimeNow = true;
        } else {
            if (timeDiffNanoseconds) {
                *timeDiffNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(timeNow - start_time).count();
            }
            m_profileFramesCount++;
            displayTimeNow = false;
        }
        return fps;
    }

protected:
    ~FrameProcessor() override = default;

    explicit FrameProcessor(bool verbose = false)
        : m_frameCount(0)
        , m_profileFramesCount(0)
        , m_displayTimePeriodMilliseconds(1000)
        , start_time (std::chrono::steady_clock::now())
        , m_verbose(verbose)
    {
        if (m_verbose) {
            std::cout << "The clock resolution of high_resolution_clock is: "
                    << static_cast<double>(std::chrono::high_resolution_clock::period::num) /
                                   std::chrono::high_resolution_clock::period::den << std::endl;
            std::cout << "The clock resolution of steady_clock is: "
                    << static_cast<double>(std::chrono::steady_clock::period::num) /
                                   std::chrono::steady_clock::period::den << std::endl;
            std::cout << "The clock resolution of system_clock is: "
                    << static_cast<double>(std::chrono::system_clock::period::num) /
                                   std::chrono::system_clock::period::den << std::endl;
        }
    }

	int64_t m_frameCount;
    int64_t m_profileFramesCount;
    const int64_t m_displayTimePeriodMilliseconds;
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    uint32_t m_verbose: 1;
};

#endif  // FRAMEPROCESSOR_H
