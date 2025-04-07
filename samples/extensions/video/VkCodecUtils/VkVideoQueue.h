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

#ifndef _VKCODECUTILS_VKVIDEOQUEUE_H_
#define _VKCODECUTILS_VKVIDEOQUEUE_H_

#include <vulkan/vulkan.h>
#include "VkCodecUtils/VkVideoRefCountBase.h"

template<class FrameDataType>
class VkVideoQueue : public VkVideoRefCountBase {
public:

    virtual bool     IsValid()    const = 0;
    virtual int32_t  GetWidth()       const = 0;
    virtual int32_t  GetHeight()      const = 0;
    virtual int32_t  GetBitDepth()    const = 0;
    virtual VkFormat GetFrameImageFormat(int32_t* pWidth,
                                         int32_t* pHeight,
                                         int32_t* pBitDepth) const = 0;
    virtual int32_t GetNextFrame(FrameDataType* pFrame, bool* endOfStream) = 0;
    virtual int32_t ReleaseFrame(FrameDataType* pDisplayedFrame) = 0;
	~VkVideoQueue() override = default;
};

#endif /* _VKCODECUTILS_VKVIDEOQUEUE_H_ */
