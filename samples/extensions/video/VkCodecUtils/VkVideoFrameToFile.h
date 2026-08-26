/*
* Copyright 2024 NVIDIA Corporation.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef _VKCODECUTILS_VKVIDEOFRAMETOFILE_H_
#define _VKCODECUTILS_VKVIDEOFRAMETOFILE_H_

#include "nvidia_utils/vulkan/ycbcrvkinfo.h"

class VkVideoFrameToFile {

public:

    VkVideoFrameToFile()
        : m_outputFile(),
          m_pLinearMemory()
        , m_allocationSize()
        , m_firstFrame(true) {}

    ~VkVideoFrameToFile()
    {
        if (m_pLinearMemory) {
            delete[] m_pLinearMemory;
            m_pLinearMemory = nullptr;
        }

        if (m_outputFile) {
            fclose(m_outputFile);
            m_outputFile = nullptr;
        }
    }

    uint8_t* EnsureAllocation(const VulkanDeviceContext* vkDevCtx,
                              VkSharedBaseObj<VkImageResource>& imageResource) {

        if (m_outputFile == nullptr) {
            return nullptr;
        }

        VkDeviceSize imageMemorySize = imageResource->GetImageDeviceMemorySize();

        if ((m_pLinearMemory == nullptr) || (imageMemorySize > m_allocationSize)) {

            if (m_outputFile) {
                fflush(m_outputFile);
            }

            if (m_pLinearMemory != nullptr) {
                delete[] m_pLinearMemory;
                m_pLinearMemory = nullptr;
            }

            // Allocate the memory that will be dumped to file directly.
            m_allocationSize = (size_t)(imageMemorySize);
            m_pLinearMemory = new uint8_t[m_allocationSize];
            if (m_pLinearMemory == nullptr) {
                return nullptr;
            }
            assert(m_pLinearMemory != nullptr);
        }
        return m_pLinearMemory;
    }

    FILE* AttachFile(const char* fileName) {

        if (m_outputFile) {
            fclose(m_outputFile);
            m_outputFile = nullptr;
        }

        if (fileName != nullptr) {
            m_outputFile = fopen(fileName, "wb");
            if (m_outputFile) {
                return m_outputFile;
            }
        }

        return nullptr;
    }

    bool IsFileStreamValid() const
    {
        return m_outputFile != nullptr;
    }

    operator bool() const {
        return IsFileStreamValid();
    }

    size_t WriteDataToFile(size_t offset, size_t size)
    {
        return fwrite(m_pLinearMemory + offset, size, 1, m_outputFile);
    }

    size_t GetMaxFrameSize() {
        return m_allocationSize;
    }

    size_t WriteFrameToFileY4M(size_t offset, size_t size, size_t width, size_t height, const VkMpFormatInfo *mpInfo)
    {
        // Output Frame.
        if (m_firstFrame != false) {
            m_firstFrame = false;
            fprintf(m_outputFile, "YUV4MPEG2 ");
            fprintf(m_outputFile, "W%i H%i ", (int)width, (int)height);
            m_height = height;
            m_width = width;
            fprintf(m_outputFile, "F24:1 ");
            fprintf(m_outputFile, "Ip ");
            fprintf(m_outputFile, "A1:1 ");
            if (mpInfo->planesLayout.secondaryPlaneSubsampledX == false) {
                fprintf(m_outputFile, "C444");
            } else {
                fprintf(m_outputFile, "C420");
            }

            if (mpInfo->planesLayout.bpp != YCBCRA_8BPP) {
                fprintf(m_outputFile, "p16");
            }

            fprintf(m_outputFile, "\n");
        }

        fprintf(m_outputFile, "FRAME");
        if ((m_width != width) || (m_height != height)) {
            fprintf(m_outputFile, " ");
            fprintf(m_outputFile, "W%i H%i", (int)width, (int)height);
            m_height = height;
            m_width = width;
        }

        fprintf(m_outputFile, "\n");
        return WriteDataToFile(offset, size);
    }

private:
    FILE*    m_outputFile;
    uint8_t* m_pLinearMemory;
    size_t   m_allocationSize;
    bool     m_firstFrame;
    size_t   m_height;
    size_t   m_width;
};



#endif /* _VKCODECUTILS_VKVIDEOFRAMETOFILE_H_ */
