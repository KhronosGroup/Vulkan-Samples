/*
* Copyright 2023 NVIDIA Corporation.  All rights reserved.
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

#include "VkDecoderUtils/VideoStreamDemuxer.h"

VkResult FFmpegDemuxerCreate(const char *pFilePath,
                             VkVideoCodecOperationFlagBitsKHR codecType,
                             bool requiresStreamDemuxing,
                             int32_t defaultWidth,
                             int32_t defaultHeight,
                             int32_t defaultBitDepth,
                             VkSharedBaseObj<VideoStreamDemuxer>& videoStreamDemuxer);

VkResult ElementaryStreamCreate(const char *pFilePath,
                                VkVideoCodecOperationFlagBitsKHR codecType,
                                int32_t defaultWidth,
                                int32_t defaultHeight,
                                int32_t defaultBitDepth,
                                VkSharedBaseObj<VideoStreamDemuxer>& videoStreamDemuxer);

VkResult VideoStreamDemuxer::Create(const char *pFilePath,
                                    VkVideoCodecOperationFlagBitsKHR codecType,
                                    bool requiresStreamDemuxing,
                                    int32_t defaultWidth,
                                    int32_t defaultHeight,
                                    int32_t defaultBitDepth,
                                    VkSharedBaseObj<VideoStreamDemuxer>& videoStreamDemuxer)
{
    if (requiresStreamDemuxing || (codecType == VK_VIDEO_CODEC_OPERATION_NONE_KHR)) {
        return FFmpegDemuxerCreate(pFilePath,
                                   codecType,
                                   requiresStreamDemuxing,
                                   defaultWidth,
                                   defaultHeight,
                                   defaultBitDepth,
                                   videoStreamDemuxer);
    }  else {
        return ElementaryStreamCreate(pFilePath,
                                      codecType,
                                      defaultWidth,
                                      defaultHeight,
                                      defaultBitDepth,
                                      videoStreamDemuxer);
    }
}



