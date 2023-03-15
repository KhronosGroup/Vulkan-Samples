//
// Created by swinston on 3/12/23.
//

#include "VideoProcessor.h"

VkFormat VideoProcessor::GetFrameImageFormat(int32_t *width, int32_t *height, int32_t *bitDepth)
{
	return VK_FORMAT_R32_UINT;
}
int32_t VideoProcessor::GetWidth()
{
	return 0;
}
int32_t VideoProcessor::GetHeight()
{
	return 0;
}
int32_t VideoProcessor::GetBitDepth()
{
	return 0;
}
size_t VideoProcessor::OutputFrameToFile(DecodedFrame *Frame)
{
	return 0;
}
void VideoProcessor::Restart()
{
}
bool VideoProcessor::StreamCompleted()
{
	return false;
}
int32_t VideoProcessor::ParserProcessNextDataChunk()
{
	return 0;
}
int32_t VideoProcessor::GetNextFrame(DecodedFrame *Frame, bool *endOfStream)
{
	return 0;
}
int32_t VideoProcessor::ReleaseDisplayedFrame(DecodedFrame *Frame)
{
	return 0;
}
VkResult VideoProcessor::CreateParser(const char *filename, VkVideoCodecOperationFlagBitsKHR vkCodecType, uint32_t defaultMinBufferSize, uint32_t bufferOffsetAlignment, uint32_t bufferSizeAlignment)
{
	return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}
VkResult VideoProcessor::ParseVideoStreamData(const uint8_t *data, size_t size, size_t *VideoBytes, bool doPartialParsing, uint32_t flags, uint64_t timestamp)
{
	if (!vkParser) {
		assert(!"Parser not initialized!");
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	VkParserSourceDataPacket packet = { 0 };
	packet.payload = pData;
	packet.payload_size = size;
	packet.flags = flags;
	if (timestamp) {
		packet.flags |= VK_PARSER_PKT_TIMESTAMP;
	}
	packet.timestamp = timestamp;
	if (!pData || size == 0) {
		packet.flags |= VK_PARSER_PKT_ENDOFSTREAM;
	}

	return m_vkParser->ParseVideoData(&packet, pnVideoBytes, doPartialParsing);

}
