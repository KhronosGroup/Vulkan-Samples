//
// Created by swinston on 3/12/23.
//

#ifndef VULKAN_SAMPLES_VIDEOPROCESSOR_H
#define VULKAN_SAMPLES_VIDEOPROCESSOR_H

#include "common/vk_common.h"

class DecodedFrame;

class VideoProcessor
{
	VkFormat GetFrameImageFormat(int32_t * width, int32_t * height, int32_t * bitDepth);
	int32_t GetWidth();
	int32_t GetHeight();
	int32_t GetBitDepth();
	size_t OutputFrameToFile(DecodedFrame* Frame);
	void Restart();
	bool StreamCompleted();
	int32_t ParserProcessNextDataChunk();
	int32_t GetNextFrame(DecodedFrame * Frame, bool* endOfStream);
	int32_t ReleaseDisplayedFrame(DecodedFrame * Frame);
	VkResult CreateParser(const char* filename,
	                      VkVideoCodecOperationFlagBitsKHR vkCodecType,
	                      uint32_t defaultMinBufferSize,
	                      uint32_t bufferOffsetAlignment,
	                      uint32_t bufferSizeAlignment);
	VkResult ParseVideoStreamData(const uint8_t* data, size_t size, size_t *VideoBytes, bool doPartialParsing, uint32_t flags, uint64_t timestamp);

};

#endif        // VULKAN_SAMPLES_VIDEOPROCESSOR_H
