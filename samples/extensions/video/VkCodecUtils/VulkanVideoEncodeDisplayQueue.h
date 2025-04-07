/*
* Copyright 2020 NVIDIA Corporation.
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

#ifndef _VULKANVIDEOENCODEDISPLAYQUEUE_H_
#define _VULKANVIDEOENCODEDISPLAYQUEUE_H_

#include "VkCodecUtils/VkVideoRefCountBase.h"
#include "VkCodecUtils/VulkanVideoDisplayQueue.h"
#include "VkCodecUtils/VulkanEncoderInputFrame.h"

class FrameProcessor;
class VulkanDeviceContext;

VkResult CreateVulkanVideoEncodeDisplayQueue(const VulkanDeviceContext* vkDevCtx,
                                             int32_t defaultWidth, int32_t defaultHeight,
                                             int32_t defaultBitDepth, VkFormat defaultImageFormat,
                                             VkSharedBaseObj<VulkanVideoDisplayQueue<VulkanEncoderInputFrame>>& vulkanVideoEncodeDisplayQueue);


#endif /* _VULKANVIDEOENCODEDISPLAYQUEUE_H_ */
