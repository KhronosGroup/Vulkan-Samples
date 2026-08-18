/*
 * Copyright 2023-2024 NVIDIA Corporation.
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

#ifndef _VKVIDEOCORE_DECODEFRAMEBUFFERIF_H_
#define _VKVIDEOCORE_DECODEFRAMEBUFFERIF_H_

static const uint8_t InvalidImageTypeIdx = (uint8_t)-1;

class DecodeFrameBufferIf
{
public:
    enum { MAX_PER_FRAME_IMAGE_TYPES = 4};

    enum ImageTypeIdx : uint8_t  { IMAGE_TYPE_IDX_DECODE_DPB      = 0, // Used for DPB and coincide output
                                   IMAGE_TYPE_IDX_DECODE_OUT      = 1, // Used for separate output
                                   IMAGE_TYPE_IDX_LINEAR_OUT      = 2, // Used for linear output
                                   IMAGE_TYPE_IDX_FILM_GRAIN_OUT  = 3, // Used for film-grain output
                                   IMAGE_TYPE_IDX_FILTER_OUT      = 4, // Used for the filter output
                                   IMAGE_TYPE_IDX_INVALID         = (uint8_t)-1,
                                 };

    enum ImageType : uint8_t     { IMAGE_TYPE_MASK_DECODE_DPB      = (1 << IMAGE_TYPE_IDX_DECODE_DPB),
                                   IMAGE_TYPE_MASK_DECODE_OUT      = (1 << IMAGE_TYPE_IDX_DECODE_OUT),
                                   IMAGE_TYPE_MASK_LINEAR_OUT      = (1 <<  IMAGE_TYPE_IDX_LINEAR_OUT),
                                   IMAGE_TYPE_MASK_FILM_GRAIN_OUT  = (1 <<  IMAGE_TYPE_IDX_FILM_GRAIN_OUT),
                                   IMAGE_TYPE_MASK_FILTER_OUT      = (1 << IMAGE_TYPE_IDX_FILTER_OUT),
                                   IMAGE_TYPE_MASK_ALL             = ( IMAGE_TYPE_MASK_DECODE_DPB |
                                                                       IMAGE_TYPE_MASK_DECODE_OUT |
                                                                       IMAGE_TYPE_MASK_LINEAR_OUT |
                                                                       IMAGE_TYPE_MASK_FILM_GRAIN_OUT |
                                                                       IMAGE_TYPE_MASK_FILTER_OUT),
                                   IMAGE_TYPE_MASK_NONE            = 0,
                                 };


    struct ImageSpecsIndex {
        // decodeDpb type always require an image for the current setup DPB
        uint8_t decodeDpb;
        // decodeOut type always require an image.
        // Used with implementation not using DPB coincide and for AV1 film grain
        uint8_t decodeOut;
        // Can be dedicated or virtual equal to filterOut if the filter supports writing to a linear image
        uint8_t linearOut;
        // Virtual only, if FG is enabled for the current frame is equal to decodeOut.
        // AV1 film grain can also be done using the compute filter
        uint8_t filmGrainOut;
        uint8_t filterOut;
        // filterIn specifies the input of the filter, if enabled.
        // It is virtual only - no resource allocation for it. usually assigned to decodeDpb or decodeOut
        uint8_t filterIn;
        // displayOut specifies the output image for the display, if the presentation is enabled.
        // Virtual only - no resource allocation for it. it can be assigned to decodeDpb, decodeOut, linearOut, filterOut
        uint8_t displayOut;
        uint8_t reserved;

        ImageSpecsIndex()
        : decodeDpb(0)
        , decodeOut(InvalidImageTypeIdx)
        , linearOut(InvalidImageTypeIdx)
        , filmGrainOut(InvalidImageTypeIdx)
        , filterOut(InvalidImageTypeIdx)
        , filterIn(InvalidImageTypeIdx)
        , displayOut(0)
        , reserved(InvalidImageTypeIdx) { }
    };

    struct ImageViews {
        ImageViews()
        : view()
        , singleLevelView()
        , inUse(false) {}

        VkSharedBaseObj<VkImageResourceView>  view;
        VkSharedBaseObj<VkImageResourceView>  singleLevelView;
        uint32_t inUse : 1;

        bool GetImageResourceView(VkSharedBaseObj<VkImageResourceView>& imageResourceView)
        {
            if (!inUse) {
                return false;
            }

            if (singleLevelView != nullptr) {
                imageResourceView = singleLevelView;
                return true;
            }

            if (view != nullptr) {
                imageResourceView = view;
                return true;
            }

            return false;
        }
    };

};

#endif /* _VKVIDEOCORE_DECODEFRAMEBUFFERIF_H_ */
