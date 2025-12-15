/* Copyright (c) 2025 Holochip Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
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
#include "imgui_system.h"
#include "audio_system.h"
#include "renderer.h"

// Include ImGui headers
#include "imgui/imgui.h"

#include <iostream>

// This implementation corresponds to the GUI chapter in the tutorial:
// @see en/Building_a_Simple_Engine/GUI/02_imgui_setup.adoc

ImGuiSystem::ImGuiSystem()
{
	// Constructor implementation
}

ImGuiSystem::~ImGuiSystem()
{
	// Destructor implementation
	Cleanup();
}

bool ImGuiSystem::Initialize(Renderer *renderer, uint32_t width, uint32_t height)
{
	if (initialized)
	{
		return true;
	}

	this->renderer = renderer;
	this->width    = width;
	this->height   = height;

	// Create ImGui context
	context = ImGui::CreateContext();
	if (!context)
	{
		std::cerr << "Failed to create ImGui context" << std::endl;
		return false;
	}

	// Configure ImGui
	ImGuiIO &io = ImGui::GetIO();
	// Set display size
	io.DisplaySize             = ImVec2(static_cast<float>(width), static_cast<float>(height));
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// Set up ImGui style
	ImGui::StyleColorsDark();

	// Create Vulkan resources
	if (!createResources())
	{
		std::cerr << "Failed to create ImGui Vulkan resources" << std::endl;
		Cleanup();
		return false;
	}

	// Initialize per-frame buffers containers
	if (renderer)
	{
		uint32_t frames = renderer->GetMaxFramesInFlight();
		vertexBuffers.clear();
		vertexBuffers.reserve(frames);
		vertexBufferMemories.clear();
		vertexBufferMemories.reserve(frames);
		indexBuffers.clear();
		indexBuffers.reserve(frames);
		indexBufferMemories.clear();
		indexBufferMemories.reserve(frames);
		for (uint32_t i = 0; i < frames; ++i)
		{
			vertexBuffers.emplace_back(nullptr);
			vertexBufferMemories.emplace_back(nullptr);
			indexBuffers.emplace_back(nullptr);
			indexBufferMemories.emplace_back(nullptr);
		}
		vertexCounts.assign(frames, 0);
		indexCounts.assign(frames, 0);
	}

	initialized = true;
	return true;
}

void ImGuiSystem::Cleanup()
{
	if (!initialized)
	{
		return;
	}

	// Wait for the device to be idle before cleaning up
	if (renderer)
	{
		renderer->WaitIdle();
	}
	// Destroy ImGui context
	if (context)
	{
		ImGui::DestroyContext(context);
		context = nullptr;
	}

	initialized = false;
}

void ImGuiSystem::SetAudioSystem(AudioSystem *audioSystem)
{
	this->audioSystem = audioSystem;

	// Load the grass-step-right.wav file and create audio source
	if (audioSystem)
	{
		if (audioSystem->LoadAudio("../Assets/grass-step-right.wav", "grass_step"))
		{
			audioSource = audioSystem->CreateAudioSource("grass_step");
			if (audioSource)
			{
				audioSource->SetPosition(audioSourceX, audioSourceY, audioSourceZ);
				audioSource->SetVolume(0.8f);
				audioSource->SetLoop(true);
				std::cout << "Audio source created and configured for HRTF demo" << std::endl;
			}
		}

		// Also create a debug ping source for testing
		debugPingSource = audioSystem->CreateDebugPingSource("debug_ping");
		if (debugPingSource)
		{
			debugPingSource->SetPosition(audioSourceX, audioSourceY, audioSourceZ);
			debugPingSource->SetVolume(0.8f);
			debugPingSource->SetLoop(true);
			std::cout << "Debug ping source created for audio debugging" << std::endl;
		}
	}
}

void ImGuiSystem::NewFrame()
{
	if (!initialized)
	{
		return;
	}

	// Reset the flag at the start of each frame
	frameAlreadyRendered = false;

	ImGui::NewFrame();

	// Loading overlay: show only a fullscreen progress bar while the model
	// itself is loading. Once the scene is ready and geometry is visible,
	// we no longer block the view with a full-screen progress bar.
	if (renderer)
	{
		const uint32_t scheduled    = renderer->GetTextureTasksScheduled();
		const uint32_t completed    = renderer->GetTextureTasksCompleted();
		const bool     modelLoading = renderer->IsLoading();
		if (modelLoading)
		{
			ImGuiIO &io = ImGui::GetIO();
			// Suppress right-click while loading
			if (io.MouseDown[1])
				io.MouseDown[1] = false;

			const ImVec2 dispSize = io.DisplaySize;

			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(dispSize);
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
			                         ImGuiWindowFlags_NoResize |
			                         ImGuiWindowFlags_NoMove |
			                         ImGuiWindowFlags_NoScrollbar |
			                         ImGuiWindowFlags_NoCollapse |
			                         ImGuiWindowFlags_NoSavedSettings |
			                         ImGuiWindowFlags_NoBringToFrontOnFocus |
			                         ImGuiWindowFlags_NoNav;

			if (ImGui::Begin("##LoadingOverlay", nullptr, flags))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				// Center the progress elements
				const float barWidth = dispSize.x * 0.8f;
				const float barX     = (dispSize.x - barWidth) * 0.5f;
				const float barY     = dispSize.y * 0.45f;
				ImGui::SetCursorPos(ImVec2(barX, barY));
				ImGui::BeginGroup();
				float frac = (scheduled > 0) ? (float) completed / (float) scheduled : 0.0f;
				ImGui::ProgressBar(frac, ImVec2(barWidth, 0.0f));
				ImGui::Dummy(ImVec2(0.0f, 10.0f));
				ImGui::SetCursorPosX(barX);
				ImGui::Text("Loading scene...");
				ImGui::EndGroup();
				ImGui::PopStyleVar();
			}
			ImGui::End();
			ImGui::Render();
			frameAlreadyRendered = true;
			return;
		}
	}

	// --- Streaming status: small progress indicator in the upper-right ---
	// Once the scene is visible, textures may continue streaming to the GPU.
	// Show a compact progress bar in the top-right while there are still
	// outstanding texture tasks, and hide it once everything is fully loaded.
	if (renderer)
	{
		const uint32_t uploadTotal  = renderer->GetUploadJobsTotal();
		const uint32_t uploadDone   = renderer->GetUploadJobsCompleted();
		const bool     modelLoading = renderer->IsLoading();

		if (!modelLoading && uploadTotal > 0 && uploadDone < uploadTotal)
		{
			ImGuiIO     &io       = ImGui::GetIO();
			const ImVec2 dispSize = io.DisplaySize;

			const float  windowWidth  = std::min(260.0f, dispSize.x * 0.35f);
			const float  windowHeight = 120.0f;
			const ImVec2 winPos(dispSize.x - windowWidth - 10.0f, 10.0f);

			ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
			                         ImGuiWindowFlags_NoResize |
			                         ImGuiWindowFlags_NoMove |
			                         ImGuiWindowFlags_NoScrollbar |
			                         ImGuiWindowFlags_NoSavedSettings |
			                         ImGuiWindowFlags_NoCollapse;

			if (ImGui::Begin("##StreamingTextures", nullptr, flags))
			{
				ImGui::TextUnformatted("Streaming textures to GPU");
				float frac = (uploadTotal > 0) ? (float) uploadDone / (float) uploadTotal : 0.0f;
				ImGui::ProgressBar(frac, ImVec2(-1.0f, 0.0f));

				// Perf counters
				const double mbps    = renderer->GetUploadThroughputMBps();
				const double avgMs   = renderer->GetAverageUploadMs();
				const double totalMB = (double) renderer->GetBytesUploadedTotal() / (1024.0 * 1024.0);
				ImGui::Text("Throughput: %.1f MB/s", mbps);
				ImGui::SameLine();
				ImGui::Text("Avg upload: %.2f ms/tex", avgMs);
				ImGui::Text("Total uploaded: %.1f MB", totalMB);
			}
			ImGui::End();
		}
	}

	// Create HRTF Audio Control UI
	ImGui::Begin("HRTF Audio Controls");
	ImGui::Text("3D Audio Position Control");

	// Audio source selection
	ImGui::Separator();
	ImGui::Text("Audio Source Selection:");

	static bool useDebugPing = false;
	if (ImGui::Checkbox("Use Debug Ping (800Hz sine wave)", &useDebugPing))
	{
		// Stop current audio
		if (audioSource && audioSource->IsPlaying())
		{
			audioSource->Stop();
		}
		if (debugPingSource && debugPingSource->IsPlaying())
		{
			debugPingSource->Stop();
		}
		std::cout << "Switched to " << (useDebugPing ? "debug ping" : "file audio") << " source" << std::endl;
	}

	// Display current audio source position
	ImGui::Text("Audio Source Position: (%.2f, %.2f, %.2f)", audioSourceX, audioSourceY, audioSourceZ);
	ImGui::Text("Current Source: %s", useDebugPing ? "Debug Ping (800Hz)" : "grass-step-right.wav");

	// Directional control buttons
	ImGui::Separator();
	ImGui::Text("Directional Controls:");

	// Get current active source
	AudioSource *currentSource = useDebugPing ? debugPingSource : audioSource;

	// Up button
	if (ImGui::Button("Up"))
	{
		audioSourceY += 0.5f;
		if (currentSource)
		{
			currentSource->SetPosition(audioSourceX, audioSourceY, audioSourceZ);
		}
		std::cout << (useDebugPing ? "Debug ping" : "Audio") << " moved up to (" << audioSourceX << ", " << audioSourceY << ", " << audioSourceZ << ")" << std::endl;
	}

	// Left and Right buttons on same line
	if (ImGui::Button("Left"))
	{
		audioSourceX -= 0.5f;
		if (currentSource)
		{
			currentSource->SetPosition(audioSourceX, audioSourceY, audioSourceZ);
		}
		std::cout << (useDebugPing ? "Debug ping" : "Audio") << " moved left to (" << audioSourceX << ", " << audioSourceY << ", " << audioSourceZ << ")" << std::endl;
	}
	ImGui::SameLine();
	if (ImGui::Button("Right"))
	{
		audioSourceX += 0.5f;
		if (currentSource)
		{
			currentSource->SetPosition(audioSourceX, audioSourceY, audioSourceZ);
		}
		std::cout << (useDebugPing ? "Debug ping" : "Audio") << " moved right to (" << audioSourceX << ", " << audioSourceY << ", " << audioSourceZ << ")" << std::endl;
	}

	// Down button
	if (ImGui::Button("Down"))
	{
		audioSourceY -= 0.5f;
		if (currentSource)
		{
			currentSource->SetPosition(audioSourceX, audioSourceY, audioSourceZ);
		}
		std::cout << (useDebugPing ? "Debug ping" : "Audio") << " moved down to (" << audioSourceX << ", " << audioSourceY << ", " << audioSourceZ << ")" << std::endl;
	}

	// Audio playback controls
	ImGui::Separator();
	ImGui::Text("Playback Controls:");

	// Play button
	if (ImGui::Button("Play"))
	{
		if (currentSource)
		{
			currentSource->Play();
			if (audioSystem)
			{
				audioSystem->FlushOutput();
			}
			if (useDebugPing)
			{
				std::cout << "Started playing debug ping (800Hz sine wave) with HRTF processing" << std::endl;
			}
			else
			{
				std::cout << "Started playing grass-step-right.wav with HRTF processing" << std::endl;
			}
		}
		else
		{
			std::cout << "No audio source available - audio system not initialized" << std::endl;
		}
	}
	ImGui::SameLine();

	// Stop button
	if (ImGui::Button("Stop"))
	{
		if (currentSource)
		{
			currentSource->Stop();
			if (useDebugPing)
			{
				std::cout << "Stopped debug ping playback" << std::endl;
			}
			else
			{
				std::cout << "Stopped audio playback" << std::endl;
			}
		}
	}

	// Additional info
	ImGui::Separator();
	if (audioSystem && audioSystem->IsHRTFEnabled())
	{
		ImGui::Text("HRTF Processing: ENABLED");
		ImGui::Text("Use directional buttons to move the audio source in 3D space");
		ImGui::Text("You should hear the audio move around you!");

		// HRTF Processing Mode: GPU only (checkbox removed)
		ImGui::Separator();
		ImGui::Text("HRTF Processing Mode:");
		ImGui::Text("Current Mode: Vulkan shader processing (GPU)");
	}
	else
	{
		ImGui::Text("HRTF Processing: DISABLED");
	}

	// Ball Debugging Controls
	ImGui::Separator();
	ImGui::Text("Ball Debugging Controls:");

	if (ImGui::Checkbox("Ball-Only Rendering", &ballOnlyRenderingEnabled))
	{
		std::cout << "Ball-only rendering " << (ballOnlyRenderingEnabled ? "enabled" : "disabled") << std::endl;
	}
	ImGui::SameLine();
	if (ImGui::Button("?##BallOnlyHelp"))
	{
		// Help tooltip will be shown on hover
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("When enabled, only balls will be rendered.\nAll other geometry (bistro scene) will be hidden.");
	}

	if (ImGui::Checkbox("Camera Track Ball", &cameraTrackingEnabled))
	{
		std::cout << "Camera tracking " << (cameraTrackingEnabled ? "enabled" : "disabled") << std::endl;
	}
	ImGui::SameLine();
	if (ImGui::Button("?##CameraTrackHelp"))
	{
		// Help tooltip will be shown on hover
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("When enabled, camera will automatically\nfollow and look at the ball.");
	}

	// Status display
	if (ballOnlyRenderingEnabled)
	{
		ImGui::Text("Status: Only balls are being rendered");
	}
	else
	{
		ImGui::Text("Status: All geometry is being rendered");
	}

	if (cameraTrackingEnabled)
	{
		ImGui::Text("Camera: Tracking ball automatically");
	}
	else
	{
		ImGui::Text("Camera: Manual control (WASD + mouse)");
	}

	// Texture loading progress
	if (renderer)
	{
		const uint32_t scheduled = renderer->GetTextureTasksScheduled();
		const uint32_t completed = renderer->GetTextureTasksCompleted();
		if (scheduled > 0 && completed < scheduled)
		{
			ImGui::Separator();
			float frac = scheduled ? (float) completed / (float) scheduled : 1.0f;
			ImGui::Text("Loading textures: %u / %u", completed, scheduled);
			ImGui::ProgressBar(frac, ImVec2(-FLT_MIN, 0.0f));
			ImGui::Text("You can continue interacting while textures stream in...");
		}
	}

	ImGui::End();
}

void ImGuiSystem::Render(vk::raii::CommandBuffer &commandBuffer, uint32_t frameIndex)
{
	if (!initialized)
	{
		return;
	}

	// End the frame and prepare for rendering
	ImGui::Render();

	// Update vertex and index buffers for this frame
	updateBuffers(frameIndex);

	// Record rendering commands
	ImDrawData *drawData = ImGui::GetDrawData();
	if (!drawData || drawData->CmdListsCount == 0)
	{
		return;
	}

	try
	{
		// Bind the pipeline
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

		// Set viewport
		vk::Viewport viewport;
		viewport.width    = ImGui::GetIO().DisplaySize.x;
		viewport.height   = ImGui::GetIO().DisplaySize.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		commandBuffer.setViewport(0, {viewport});

		// Set push constants
		struct PushConstBlock
		{
			float scale[2];
			float translate[2];
		} pushConstBlock{};

		pushConstBlock.scale[0]     = 2.0f / ImGui::GetIO().DisplaySize.x;
		pushConstBlock.scale[1]     = 2.0f / ImGui::GetIO().DisplaySize.y;
		pushConstBlock.translate[0] = -1.0f;
		pushConstBlock.translate[1] = -1.0f;

		commandBuffer.pushConstants<PushConstBlock>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, pushConstBlock);

		// Bind vertex and index buffers for this frame
		std::array<vk::Buffer, 1>     vertexBuffersArr = {*vertexBuffers[frameIndex]};
		std::array<vk::DeviceSize, 1> offsets          = {};
		commandBuffer.bindVertexBuffers(0, vertexBuffersArr, offsets);
		commandBuffer.bindIndexBuffer(*indexBuffers[frameIndex], 0, vk::IndexType::eUint16);

		// Render command lists
		int vertexOffset = 0;
		int indexOffset  = 0;

		for (int i = 0; i < drawData->CmdListsCount; i++)
		{
			const ImDrawList *cmdList = drawData->CmdLists[i];

			for (int j = 0; j < cmdList->CmdBuffer.Size; j++)
			{
				const ImDrawCmd *pcmd = &cmdList->CmdBuffer[j];

				// Set scissor rectangle
				vk::Rect2D scissor;
				scissor.offset.x      = std::max(static_cast<int32_t>(pcmd->ClipRect.x), 0);
				scissor.offset.y      = std::max(static_cast<int32_t>(pcmd->ClipRect.y), 0);
				scissor.extent.width  = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissor.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
				commandBuffer.setScissor(0, {scissor});

				// Bind descriptor set (font texture)
				commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, {*descriptorSet}, {});

				// Draw
				commandBuffer.drawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}

			vertexOffset += cmdList->VtxBuffer.Size;
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to render ImGui: " << e.what() << std::endl;
	}
}

void ImGuiSystem::HandleMouse(float x, float y, uint32_t buttons)
{
	if (!initialized)
	{
		return;
	}

	ImGuiIO &io = ImGui::GetIO();

	// Update mouse position
	io.MousePos = ImVec2(x, y);

	// Update mouse buttons
	io.MouseDown[0] = (buttons & 0x01) != 0;        // Left button
	io.MouseDown[1] = (buttons & 0x02) != 0;        // Right button
	io.MouseDown[2] = (buttons & 0x04) != 0;        // Middle button
}

void ImGuiSystem::HandleKeyboard(uint32_t key, bool pressed)
{
	if (!initialized)
	{
		return;
	}

	ImGuiIO &io = ImGui::GetIO();

	// Update key state
	if (key < 512)
	{
		io.KeysDown[key] = pressed;
	}

	// Update modifier keys
	// Using GLFW key codes instead of Windows-specific VK_* constants
	io.KeyCtrl  = io.KeysDown[341] || io.KeysDown[345];        // Left/Right Control
	io.KeyShift = io.KeysDown[340] || io.KeysDown[344];        // Left/Right Shift
	io.KeyAlt   = io.KeysDown[342] || io.KeysDown[346];        // Left/Right Alt
	io.KeySuper = io.KeysDown[343] || io.KeysDown[347];        // Left/Right Super
}

void ImGuiSystem::HandleChar(uint32_t c)
{
	if (!initialized)
	{
		return;
	}

	ImGuiIO &io = ImGui::GetIO();
	io.AddInputCharacter(c);
}

void ImGuiSystem::HandleResize(uint32_t width, uint32_t height)
{
	if (!initialized)
	{
		return;
	}

	this->width  = width;
	this->height = height;

	ImGuiIO &io    = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
}

bool ImGuiSystem::WantCaptureKeyboard() const
{
	if (!initialized)
	{
		return false;
	}

	return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiSystem::WantCaptureMouse() const
{
	if (!initialized)
	{
		return false;
	}

	return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiSystem::createResources()
{
	// Create all Vulkan resources needed for ImGui rendering
	if (!createFontTexture())
	{
		return false;
	}

	if (!createDescriptorSetLayout())
	{
		return false;
	}

	if (!createDescriptorPool())
	{
		return false;
	}

	if (!createDescriptorSet())
	{
		return false;
	}

	if (!createPipelineLayout())
	{
		return false;
	}

	if (!createPipeline())
	{
		return false;
	}

	return true;
}

bool ImGuiSystem::createFontTexture()
{
	// Get font texture from ImGui
	ImGuiIO       &io = ImGui::GetIO();
	unsigned char *fontData;
	int            texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
	vk::DeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

	try
	{
		// Create the font image
		vk::ImageCreateInfo imageInfo;
		imageInfo.imageType     = vk::ImageType::e2D;
		imageInfo.format        = vk::Format::eR8G8B8A8Unorm;
		imageInfo.extent.width  = static_cast<uint32_t>(texWidth);
		imageInfo.extent.height = static_cast<uint32_t>(texHeight);
		imageInfo.extent.depth  = 1;
		imageInfo.mipLevels     = 1;
		imageInfo.arrayLayers   = 1;
		imageInfo.samples       = vk::SampleCountFlagBits::e1;
		imageInfo.tiling        = vk::ImageTiling::eOptimal;
		imageInfo.usage         = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
		imageInfo.sharingMode   = vk::SharingMode::eExclusive;
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;

		const vk::raii::Device &device = renderer->GetRaiiDevice();
		fontImage                      = vk::raii::Image(device, imageInfo);

		// Allocate memory for the image
		vk::MemoryRequirements memRequirements = fontImage.getMemoryRequirements();

		vk::MemoryAllocateInfo allocInfo;
		allocInfo.allocationSize  = memRequirements.size;
		allocInfo.memoryTypeIndex = renderer->FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

		fontMemory = vk::raii::DeviceMemory(device, allocInfo);
		fontImage.bindMemory(*fontMemory, 0);

		// Create a staging buffer for uploading the font data
		vk::BufferCreateInfo bufferInfo;
		bufferInfo.size        = uploadSize;
		bufferInfo.usage       = vk::BufferUsageFlagBits::eTransferSrc;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		vk::raii::Buffer stagingBuffer(device, bufferInfo);

		vk::MemoryRequirements stagingMemRequirements = stagingBuffer.getMemoryRequirements();

		vk::MemoryAllocateInfo stagingAllocInfo;
		stagingAllocInfo.allocationSize  = stagingMemRequirements.size;
		stagingAllocInfo.memoryTypeIndex = renderer->FindMemoryType(stagingMemRequirements.memoryTypeBits,
		                                                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		vk::raii::DeviceMemory stagingBufferMemory(device, stagingAllocInfo);
		stagingBuffer.bindMemory(*stagingBufferMemory, 0);

		// Copy font data to staging buffer
		void *data = stagingBufferMemory.mapMemory(0, uploadSize);
		memcpy(data, fontData, uploadSize);
		stagingBufferMemory.unmapMemory();

		// Transition image layout and copy data
		renderer->TransitionImageLayout(*fontImage, vk::Format::eR8G8B8A8Unorm,
		                                vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		renderer->CopyBufferToImage(*stagingBuffer, *fontImage,
		                            static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		renderer->TransitionImageLayout(*fontImage, vk::Format::eR8G8B8A8Unorm,
		                                vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		// Staging buffer and memory will be automatically cleaned up by RAII

		// Create image view
		vk::ImageViewCreateInfo viewInfo;
		viewInfo.image                           = *fontImage;
		viewInfo.viewType                        = vk::ImageViewType::e2D;
		viewInfo.format                          = vk::Format::eR8G8B8A8Unorm;
		viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
		viewInfo.subresourceRange.baseMipLevel   = 0;
		viewInfo.subresourceRange.levelCount     = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount     = 1;

		fontView = vk::raii::ImageView(device, viewInfo);

		// Create sampler
		vk::SamplerCreateInfo samplerInfo;
		samplerInfo.magFilter               = vk::Filter::eLinear;
		samplerInfo.minFilter               = vk::Filter::eLinear;
		samplerInfo.mipmapMode              = vk::SamplerMipmapMode::eLinear;
		samplerInfo.addressModeU            = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeV            = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeW            = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.mipLodBias              = 0.0f;
		samplerInfo.anisotropyEnable        = VK_FALSE;
		samplerInfo.maxAnisotropy           = 1.0f;
		samplerInfo.compareEnable           = VK_FALSE;
		samplerInfo.compareOp               = vk::CompareOp::eAlways;
		samplerInfo.minLod                  = 0.0f;
		samplerInfo.maxLod                  = 0.0f;
		samplerInfo.borderColor             = vk::BorderColor::eFloatOpaqueWhite;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		fontSampler = vk::raii::Sampler(device, samplerInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create font texture: " << e.what() << std::endl;
		return false;
	}
}

bool ImGuiSystem::createDescriptorSetLayout()
{
	try
	{
		vk::DescriptorSetLayoutBinding binding;
		binding.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
		binding.descriptorCount = 1;
		binding.stageFlags      = vk::ShaderStageFlagBits::eFragment;
		binding.binding         = 0;

		vk::DescriptorSetLayoutCreateInfo layoutInfo;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings    = &binding;

		const vk::raii::Device &device = renderer->GetRaiiDevice();
		descriptorSetLayout            = vk::raii::DescriptorSetLayout(device, layoutInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create descriptor set layout: " << e.what() << std::endl;
		return false;
	}
}

bool ImGuiSystem::createDescriptorPool()
{
	try
	{
		vk::DescriptorPoolSize poolSize;
		poolSize.type            = vk::DescriptorType::eCombinedImageSampler;
		poolSize.descriptorCount = 1;

		vk::DescriptorPoolCreateInfo poolInfo;
		poolInfo.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
		poolInfo.maxSets       = 1;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes    = &poolSize;

		const vk::raii::Device &device = renderer->GetRaiiDevice();
		descriptorPool                 = vk::raii::DescriptorPool(device, poolInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create descriptor pool: " << e.what() << std::endl;
		return false;
	}
}

bool ImGuiSystem::createDescriptorSet()
{
	try
	{
		vk::DescriptorSetAllocateInfo allocInfo;
		allocInfo.descriptorPool     = *descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts        = &(*descriptorSetLayout);

		const vk::raii::Device  &device = renderer->GetRaiiDevice();
		vk::raii::DescriptorSets descriptorSets(device, allocInfo);
		descriptorSet = std::move(descriptorSets[0]);        // Store the first (and only) descriptor set
		std::cout << "ImGui created descriptor set with handle: " << *descriptorSet << std::endl;

		// Update descriptor set
		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView   = *fontView;
		imageInfo.sampler     = *fontSampler;

		vk::WriteDescriptorSet writeSet;
		writeSet.dstSet          = *descriptorSet;
		writeSet.descriptorCount = 1;
		writeSet.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
		writeSet.pImageInfo      = &imageInfo;
		writeSet.dstBinding      = 0;

		device.updateDescriptorSets({writeSet}, {});

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create descriptor set: " << e.what() << std::endl;
		return false;
	}
}

bool ImGuiSystem::createPipelineLayout()
{
	try
	{
		// Push constant range for the transformation matrix
		vk::PushConstantRange pushConstantRange;
		pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
		pushConstantRange.offset     = 0;
		pushConstantRange.size       = sizeof(float) * 4;        // 2 floats for scale, 2 floats for translate

		// Create pipeline layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
		pipelineLayoutInfo.setLayoutCount         = 1;
		pipelineLayoutInfo.pSetLayouts            = &(*descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

		const vk::raii::Device &device = renderer->GetRaiiDevice();
		pipelineLayout                 = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create pipeline layout: " << e.what() << std::endl;
		return false;
	}
}

bool ImGuiSystem::createPipeline()
{
	try
	{
		// Load shaders
		vk::raii::ShaderModule shaderModule = renderer->CreateShaderModule("shaders/imgui.spv");

		// Shader stage creation
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
		vertShaderStageInfo.stage  = vk::ShaderStageFlagBits::eVertex;
		vertShaderStageInfo.module = *shaderModule;
		vertShaderStageInfo.pName  = "VSMain";

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
		fragShaderStageInfo.stage  = vk::ShaderStageFlagBits::eFragment;
		fragShaderStageInfo.module = *shaderModule;
		fragShaderStageInfo.pName  = "PSMain";

		std::array shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

		// Vertex input
		vk::VertexInputBindingDescription bindingDescription;
		bindingDescription.binding   = 0;
		bindingDescription.stride    = sizeof(ImDrawVert);
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;

		std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;
		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format   = vk::Format::eR32G32Sfloat;
		attributeDescriptions[0].offset   = offsetof(ImDrawVert, pos);

		attributeDescriptions[1].binding  = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format   = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset   = offsetof(ImDrawVert, uv);

		attributeDescriptions[2].binding  = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format   = vk::Format::eR8G8B8A8Unorm;
		attributeDescriptions[2].offset   = offsetof(ImDrawVert, col);

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		vertexInputInfo.vertexBindingDescriptionCount   = 1;
		vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

		// Input assembly
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
		inputAssembly.topology               = vk::PrimitiveTopology::eTriangleList;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// Viewport and scissor
		vk::PipelineViewportStateCreateInfo viewportState;
		viewportState.viewportCount = 1;
		viewportState.scissorCount  = 1;
		viewportState.pViewports    = nullptr;        // Dynamic state
		viewportState.pScissors     = nullptr;        // Dynamic state

		// Rasterization
		vk::PipelineRasterizationStateCreateInfo rasterizer;
		rasterizer.depthClampEnable        = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode             = vk::PolygonMode::eFill;
		rasterizer.lineWidth               = 1.0f;
		rasterizer.cullMode                = vk::CullModeFlagBits::eNone;
		rasterizer.frontFace               = vk::FrontFace::eCounterClockwise;
		rasterizer.depthBiasEnable         = VK_FALSE;

		// Multisampling
		vk::PipelineMultisampleStateCreateInfo multisampling;
		multisampling.sampleShadingEnable  = VK_FALSE;
		multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

		// Depth and stencil testing
		vk::PipelineDepthStencilStateCreateInfo depthStencil;
		depthStencil.depthTestEnable       = VK_FALSE;
		depthStencil.depthWriteEnable      = VK_FALSE;
		depthStencil.depthCompareOp        = vk::CompareOp::eLessOrEqual;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable     = VK_FALSE;

		// Color blending
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.colorWriteMask =
		    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		colorBlendAttachment.blendEnable         = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.colorBlendOp        = vk::BlendOp::eAdd;
		colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		colorBlendAttachment.alphaBlendOp        = vk::BlendOp::eAdd;

		vk::PipelineColorBlendStateCreateInfo colorBlending;
		colorBlending.logicOpEnable   = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments    = &colorBlendAttachment;

		// Dynamic state
		std::vector<vk::DynamicState> dynamicStates = {
		    vk::DynamicState::eViewport,
		    vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo dynamicState;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates    = dynamicStates.data();

		vk::Format depthFormat = renderer->findDepthFormat();
		// Create the graphics pipeline with dynamic rendering
		vk::PipelineRenderingCreateInfo renderingInfo;
		renderingInfo.colorAttachmentCount    = 1;
		vk::Format colorFormat                = renderer->GetSwapChainImageFormat();        // Get the actual swapchain format
		renderingInfo.pColorAttachmentFormats = &colorFormat;
		renderingInfo.depthAttachmentFormat   = depthFormat;

		vk::GraphicsPipelineCreateInfo pipelineInfo;
		pipelineInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages             = shaderStages.data();
		pipelineInfo.pVertexInputState   = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState      = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState   = &multisampling;
		pipelineInfo.pDepthStencilState  = &depthStencil;
		pipelineInfo.pColorBlendState    = &colorBlending;
		pipelineInfo.pDynamicState       = &dynamicState;
		pipelineInfo.layout              = *pipelineLayout;
		pipelineInfo.pNext               = &renderingInfo;
		pipelineInfo.basePipelineHandle  = nullptr;

		const vk::raii::Device &device = renderer->GetRaiiDevice();
		pipeline                       = vk::raii::Pipeline(device, nullptr, pipelineInfo);
		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create graphics pipeline: " << e.what() << std::endl;
		return false;
	}
}

void ImGuiSystem::updateBuffers(uint32_t frameIndex)
{
	ImDrawData *drawData = ImGui::GetDrawData();
	if (!drawData || drawData->CmdListsCount == 0)
	{
		return;
	}

	try
	{
		const vk::raii::Device &device = renderer->GetRaiiDevice();

		// Calculate required buffer sizes
		vk::DeviceSize vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
		vk::DeviceSize indexBufferSize  = drawData->TotalIdxCount * sizeof(ImDrawIdx);

		// Resize buffers if needed for this frame
		if (frameIndex >= vertexCounts.size())
			return;        // Safety

		if (drawData->TotalVtxCount > vertexCounts[frameIndex])
		{
			// Clean up old buffer
			vertexBuffers[frameIndex]        = vk::raii::Buffer(nullptr);
			vertexBufferMemories[frameIndex] = vk::raii::DeviceMemory(nullptr);

			// Create new vertex buffer
			vk::BufferCreateInfo bufferInfo;
			bufferInfo.size        = vertexBufferSize;
			bufferInfo.usage       = vk::BufferUsageFlagBits::eVertexBuffer;
			bufferInfo.sharingMode = vk::SharingMode::eExclusive;

			vertexBuffers[frameIndex] = vk::raii::Buffer(device, bufferInfo);

			vk::MemoryRequirements memRequirements = vertexBuffers[frameIndex].getMemoryRequirements();

			vk::MemoryAllocateInfo allocInfo;
			allocInfo.allocationSize  = memRequirements.size;
			allocInfo.memoryTypeIndex = renderer->FindMemoryType(memRequirements.memoryTypeBits,
			                                                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			vertexBufferMemories[frameIndex] = vk::raii::DeviceMemory(device, allocInfo);
			vertexBuffers[frameIndex].bindMemory(*vertexBufferMemories[frameIndex], 0);
			vertexCounts[frameIndex] = drawData->TotalVtxCount;
		}

		if (drawData->TotalIdxCount > indexCounts[frameIndex])
		{
			// Clean up old buffer
			indexBuffers[frameIndex]        = vk::raii::Buffer(nullptr);
			indexBufferMemories[frameIndex] = vk::raii::DeviceMemory(nullptr);

			// Create new index buffer
			vk::BufferCreateInfo bufferInfo;
			bufferInfo.size        = indexBufferSize;
			bufferInfo.usage       = vk::BufferUsageFlagBits::eIndexBuffer;
			bufferInfo.sharingMode = vk::SharingMode::eExclusive;

			indexBuffers[frameIndex] = vk::raii::Buffer(device, bufferInfo);

			vk::MemoryRequirements memRequirements = indexBuffers[frameIndex].getMemoryRequirements();

			vk::MemoryAllocateInfo allocInfo;
			allocInfo.allocationSize  = memRequirements.size;
			allocInfo.memoryTypeIndex = renderer->FindMemoryType(memRequirements.memoryTypeBits,
			                                                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			indexBufferMemories[frameIndex] = vk::raii::DeviceMemory(device, allocInfo);
			indexBuffers[frameIndex].bindMemory(*indexBufferMemories[frameIndex], 0);
			indexCounts[frameIndex] = drawData->TotalIdxCount;
		}

		// Upload data to buffers for this frame
		void *vtxMappedMemory = vertexBufferMemories[frameIndex].mapMemory(0, vertexBufferSize);
		void *idxMappedMemory = indexBufferMemories[frameIndex].mapMemory(0, indexBufferSize);

		ImDrawVert *vtxDst = static_cast<ImDrawVert *>(vtxMappedMemory);
		ImDrawIdx  *idxDst = static_cast<ImDrawIdx *>(idxMappedMemory);

		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList *cmdList = drawData->CmdLists[n];
			memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmdList->VtxBuffer.Size;
			idxDst += cmdList->IdxBuffer.Size;
		}

		vertexBufferMemories[frameIndex].unmapMemory();
		indexBufferMemories[frameIndex].unmapMemory();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to update buffers: " << e.what() << std::endl;
	}
}
