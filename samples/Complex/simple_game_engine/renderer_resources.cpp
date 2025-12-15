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
#include "renderer.h"
#include "model_loader.h"
#include "mesh_component.h"
#include "transform_component.h"
#include <fstream>
#include <stdexcept>
#include <array>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <functional>
#include <algorithm>

// stb_image dependency removed; all GLTF textures are uploaded via memory path from ModelLoader.

// KTX2 support
#include <ktx.h>
#include <ranges>

// This file contains resource-related methods from the Renderer class

// Define shared default PBR texture identifiers (static constants)
const std::string Renderer::SHARED_DEFAULT_ALBEDO_ID = "__shared_default_albedo__";
const std::string Renderer::SHARED_DEFAULT_NORMAL_ID = "__shared_default_normal__";
const std::string Renderer::SHARED_DEFAULT_METALLIC_ROUGHNESS_ID = "__shared_default_metallic_roughness__";
const std::string Renderer::SHARED_DEFAULT_OCCLUSION_ID = "__shared_default_occlusion__";
const std::string Renderer::SHARED_DEFAULT_EMISSIVE_ID = "__shared_default_emissive__";
const std::string Renderer::SHARED_BRIGHT_RED_ID = "__shared_bright_red__";

// Create depth resources
bool Renderer::createDepthResources() {
    try {
        // Find depth format
        vk::Format depthFormat = findDepthFormat();

        // Create depth image using memory pool
        auto [depthImg, depthImgAllocation] = createImagePooled(
            swapChainExtent.width,
            swapChainExtent.height,
            depthFormat,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        depthImage = std::move(depthImg);
        depthImageAllocation = std::move(depthImgAllocation);

        // Create depth image view
        depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

        // Transition depth image layout
        transitionImageLayout(
            *depthImage,
            depthFormat,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        );

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create depth resources: " << e.what() << std::endl;
        return false;
    }
}

// Helper: coerce an sRGB/UNORM variant of a given VkFormat while preserving block type where possible
static vk::Format CoerceFormatSRGB(vk::Format fmt, bool wantSRGB) {
    switch (fmt) {
        case vk::Format::eR8G8B8A8Unorm: return wantSRGB ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
        case vk::Format::eR8G8B8A8Srgb:  return wantSRGB ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;

        case vk::Format::eBc1RgbUnormBlock:  return wantSRGB ? vk::Format::eBc1RgbSrgbBlock  : vk::Format::eBc1RgbUnormBlock;
        case vk::Format::eBc1RgbSrgbBlock:   return wantSRGB ? vk::Format::eBc1RgbSrgbBlock  : vk::Format::eBc1RgbUnormBlock;
        case vk::Format::eBc1RgbaUnormBlock: return wantSRGB ? vk::Format::eBc1RgbaSrgbBlock : vk::Format::eBc1RgbaUnormBlock;
        case vk::Format::eBc1RgbaSrgbBlock:  return wantSRGB ? vk::Format::eBc1RgbaSrgbBlock : vk::Format::eBc1RgbaUnormBlock;

        case vk::Format::eBc2UnormBlock: return wantSRGB ? vk::Format::eBc2SrgbBlock : vk::Format::eBc2UnormBlock;
        case vk::Format::eBc2SrgbBlock:  return wantSRGB ? vk::Format::eBc2SrgbBlock : vk::Format::eBc2UnormBlock;

        case vk::Format::eBc3UnormBlock: return wantSRGB ? vk::Format::eBc3SrgbBlock : vk::Format::eBc3UnormBlock;
        case vk::Format::eBc3SrgbBlock:  return wantSRGB ? vk::Format::eBc3SrgbBlock : vk::Format::eBc3UnormBlock;

        case vk::Format::eBc7UnormBlock: return wantSRGB ? vk::Format::eBc7SrgbBlock : vk::Format::eBc7UnormBlock;
        case vk::Format::eBc7SrgbBlock:  return wantSRGB ? vk::Format::eBc7SrgbBlock : vk::Format::eBc7UnormBlock;

        default: return fmt;
    }
}

// Create texture image
bool Renderer::createTextureImage(const std::string& texturePath_, TextureResources& resources) {
    try {
        ensureThreadLocalVulkanInit();
        const std::string textureId = ResolveTextureId(texturePath_);
        // Check if texture already exists
        {
            std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
            auto it = textureResources.find(textureId);
            if (it != textureResources.end()) {
                // Texture already loaded and cached; leave cache intact and return success
                return true;
            }
        }

        // Resolve on-disk path (may differ from logical ID)
        std::string resolvedPath = textureId;

        // Ensure command pool is initialized before any GPU work
        if (!*commandPool) {
            std::cerr << "createTextureImage: commandPool not initialized yet for '" << textureId << "'" << std::endl;
            return false;
        }

        // Per-texture de-duplication (serialize loads of the same texture ID only)
        {
            std::unique_lock<std::mutex> lk(textureLoadStateMutex);
            while (texturesLoading.contains(textureId)) {
                textureLoadStateCv.wait(lk);
            }
        }
        // Double-check cache after the wait
        {
            std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
            auto it2 = textureResources.find(textureId);
            if (it2 != textureResources.end()) {
                return true;
            }
        }
        // Mark as loading and ensure we notify on all exit paths
        {
            std::lock_guard<std::mutex> lk(textureLoadStateMutex);
            texturesLoading.insert(textureId);
        }
        auto _loadingGuard = std::unique_ptr<void, std::function<void(void*)>>(reinterpret_cast<void *>(1), [this, textureId](void*){
            std::lock_guard<std::mutex> lk(textureLoadStateMutex);
            texturesLoading.erase(textureId);
            textureLoadStateCv.notify_all();
        });

        // Check if this is a KTX2 file
        bool isKtx2 = resolvedPath.find(".ktx2") != std::string::npos;

        // If it's a KTX2 texture but the path doesn't exist, try common fallback filename variants
        if (isKtx2) {
            std::filesystem::path origPath(resolvedPath);
            if (!std::filesystem::exists(origPath)) {
                std::string fname = origPath.filename().string();
                std::string dir = origPath.parent_path().string();
                auto tryCandidate = [&](const std::string& candidateName) -> bool {
                    std::filesystem::path cand = std::filesystem::path(dir) / candidateName;
                    if (std::filesystem::exists(cand)) {
                        std::cout << "Resolved missing texture '" << resolvedPath << "' to existing file '" << cand.string() << "'" << std::endl;
                        resolvedPath = cand.string();
                        return true;
                    }
                    return false;
                };
                // Known suffix variants near the end of filename before extension
                // Examples: *_c.ktx2, *_d.ktx2, *_cm.ktx2, *_diffuse.ktx2, *_basecolor.ktx2, *_albedo.ktx2
                std::vector<std::string> suffixes = {"_c", "_d", "_cm", "_diffuse", "_basecolor", "_albedo"};
                // If filename matches one known suffix, try others
                for (const auto& s : suffixes) {
                    std::string key = s + ".ktx2";
                    if (fname.size() > key.size() && fname.rfind(key) == fname.size() - key.size()) {
                        std::string prefix = fname.substr(0, fname.size() - key.size());
                        for (const auto& alt : suffixes) {
                            if (alt == s) continue;
                            std::string candName = prefix + alt + ".ktx2";
                            if (tryCandidate(candName)) { isKtx2 = true; break; }
                        }
                        break; // Only replace last suffix occurrence
                    }
                }
            }
        }

        int texWidth, texHeight, texChannels;
        unsigned char* pixels = nullptr;
        ktxTexture2* ktxTex = nullptr;
        vk::DeviceSize imageSize;

        // Track KTX2 transcoding state across the function scope (BasisU only)
        bool wasTranscoded = false;
        // Track KTX2 header-provided VkFormat (0 == VK_FORMAT_UNDEFINED)
        uint32_t headerVkFormatRaw = 0;

        uint32_t mipLevels = 1;
        std::vector<vk::BufferImageCopy> copyRegions;

        if (isKtx2) {
            // Load KTX2 file
            KTX_error_code result = ktxTexture2_CreateFromNamedFile(resolvedPath.c_str(),
                                                                   KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                                                   &ktxTex);
            if (result != KTX_SUCCESS) {
                // Retry with sibling suffix variants if file exists but cannot be parsed/opened
                std::filesystem::path origPath(resolvedPath);
                std::string fname = origPath.filename().string();
                std::string dir = origPath.parent_path().string();
                auto tryLoad = [&](const std::string& candidateName) -> bool {
                    std::filesystem::path cand = std::filesystem::path(dir) / candidateName;
                    if (std::filesystem::exists(cand)) {
                        std::string candStr = cand.string();
                        std::cout << "Retrying KTX2 load with sibling candidate '" << candStr << "' for original '" << resolvedPath << "'" << std::endl;
                        result = ktxTexture2_CreateFromNamedFile(candStr.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);
                        if (result == KTX_SUCCESS) {
                            resolvedPath = candStr; // Use the successfully opened candidate
                            return true;
                        }
                    }
                    return false;
                };
                // Known suffix variants near the end of filename before extension
                std::vector<std::string> suffixes = {"_c", "_d", "_cm", "_diffuse", "_basecolor", "_albedo"};
                for (const auto& s : suffixes) {
                    std::string key = s + ".ktx2";
                    if (fname.size() > key.size() && fname.rfind(key) == fname.size() - key.size()) {
                        std::string prefix = fname.substr(0, fname.size() - key.size());
                        bool loaded = false;
                        for (const auto& alt : suffixes) {
                            if (alt == s) continue;
                            std::string candName = prefix + alt + ".ktx2";
                            if (tryLoad(candName)) { loaded = true; break; }
                        }
                        if (loaded) break;
                    }
                }
            }

            // Bail out if we still failed to load
            if (result != KTX_SUCCESS || ktxTex == nullptr) {
                std::cerr << "Failed to load KTX2 texture: " << resolvedPath << " (error: " << result << ")" << std::endl;
                return false;
            }

            // Read header-provided vkFormat (if already GPU-compressed/transcoded offline)
            headerVkFormatRaw = static_cast<uint32_t>(ktxTex->vkFormat);

            // Check if the texture needs BasisU transcoding; prefer GPU-compressed targets to save VRAM
            wasTranscoded = ktxTexture2_NeedsTranscoding(ktxTex);
            if (wasTranscoded) {
                // Select a compressed target supported by the device (prefer BC7 RGBA, then BC3 RGBA, then BC1 RGB)
                auto supportsFormat = [&](vk::Format f) {
                    auto props = physicalDevice.getFormatProperties(f);
                    return static_cast<bool>(props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
                };
                bool wantSrgb = (Renderer::determineTextureFormat(resolvedPath) == vk::Format::eR8G8B8A8Srgb);
                KTX_error_code tcErr = KTX_SUCCESS;
                if (supportsFormat(vk::Format::eBc7UnormBlock) || supportsFormat(vk::Format::eBc7SrgbBlock)) {
                    tcErr = ktxTexture2_TranscodeBasis(ktxTex, KTX_TTF_BC7_RGBA, 0);
                } else if (supportsFormat(vk::Format::eBc3UnormBlock) || supportsFormat(vk::Format::eBc3SrgbBlock)) {
                    tcErr = ktxTexture2_TranscodeBasis(ktxTex, KTX_TTF_BC3_RGBA, 0);
                } else if (supportsFormat(vk::Format::eBc1RgbUnormBlock) || supportsFormat(vk::Format::eBc1RgbSrgbBlock)) {
                    tcErr = ktxTexture2_TranscodeBasis(ktxTex, KTX_TTF_BC1_RGB, 0);
                } else {
                    // Fallback to RGBA32 if no BC formats are supported
                    tcErr = ktxTexture2_TranscodeBasis(ktxTex, KTX_TTF_RGBA32, 0);
                }
                if (tcErr != KTX_SUCCESS) {
                    std::cerr << "Failed to transcode KTX2 BasisU texture: " << resolvedPath << " (error: " << tcErr << ")" << std::endl;
                    ktxTexture_Destroy((ktxTexture*)ktxTex);
                    return false;
                }
            }

            texWidth = ktxTex->baseWidth;
            texHeight = ktxTex->baseHeight;
            texChannels = 4; // logical channels; compressed size handled below
            // Disable mipmapping for now - memory pool only supports single mip level
            // TODO: Implement proper mipmap support in memory pool
            mipLevels = 1;

            // Calculate size for base level only (use libktx for correct size incl. compression)
            imageSize = ktxTexture_GetImageSize((ktxTexture*)ktxTex, 0);

            // Create single copy region for base level only
            copyRegions.push_back({
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .imageOffset = {0, 0, 0},
                .imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1}
            });
        } else {
            // Non-KTX texture loading via file path is disabled to simplify pipeline.
            std::cerr << "Unsupported non-KTX2 texture path: " << textureId << std::endl;
            return false;
        }

        // Create staging buffer
        auto [stagingBuffer, stagingBufferMemory] = createBuffer(
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        // Copy pixel data to staging buffer
        void* data = stagingBufferMemory.mapMemory(0, imageSize);

        if (isKtx2) {
            // Copy KTX2 texture data for base level only (level 0), regardless of transcode target
            ktx_size_t offset = 0;
            ktxTexture_GetImageOffset((ktxTexture*)ktxTex, 0, 0, 0, &offset);
            const void* levelData = ktxTexture_GetData(reinterpret_cast<ktxTexture *>(ktxTex)) + offset;
            size_t levelSize = ktxTexture_GetImageSize((ktxTexture*)ktxTex, 0);
            memcpy(data, levelData, levelSize);
        } else {
            // Copy regular image data
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        }

        stagingBufferMemory.unmapMemory();


        // Determine appropriate texture format
        vk::Format textureFormat;
        const bool wantSRGB = (Renderer::determineTextureFormat(textureId) == vk::Format::eR8G8B8A8Srgb);
        bool alphaMaskedHint = false;
        if (isKtx2) {
            // If the KTX2 provided a valid VkFormat and we did NOT transcode, respect its block type
            // but coerce the sRGB/UNORM variant based on texture usage (baseColor vs data maps)
            if (!wasTranscoded) {
                VkFormat headerFmt = static_cast<VkFormat>(headerVkFormatRaw);
                if (headerFmt != VK_FORMAT_UNDEFINED) {
                    textureFormat = CoerceFormatSRGB(static_cast<vk::Format>(headerFmt), wantSRGB);
                } else {
                    textureFormat = wantSRGB ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
                }
                // Can't easily scan alpha in compressed formats here; leave hint at default false
            } else {
                // We transcoded; choose a Vulkan format matching the transcode target (we requested BC7/BC3/BC1 or RGBA32 fallback)
                // There is no direct query from KTX for chosen VkFormat after transcoding, so infer by capabilities using our preference order.
                bool wantSRGB2 = wantSRGB;
                if (physicalDevice.getFormatProperties(vk::Format::eBc7UnormBlock).optimalTilingFeatures != vk::FormatFeatureFlags{}) {
                    textureFormat = wantSRGB2 ? vk::Format::eBc7SrgbBlock : vk::Format::eBc7UnormBlock;
                } else if (physicalDevice.getFormatProperties(vk::Format::eBc3UnormBlock).optimalTilingFeatures != vk::FormatFeatureFlags{}) {
                    textureFormat = wantSRGB2 ? vk::Format::eBc3SrgbBlock : vk::Format::eBc3UnormBlock;
                } else if (physicalDevice.getFormatProperties(vk::Format::eBc1RgbUnormBlock).optimalTilingFeatures != vk::FormatFeatureFlags{}) {
                    textureFormat = wantSRGB2 ? vk::Format::eBc1RgbSrgbBlock : vk::Format::eBc1RgbUnormBlock;
                } else {
                    // Fallback to uncompressed RGBA
                    textureFormat = wantSRGB2 ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
                    // We have CPU-visible RGBA data; detect alpha for masked hint
                    ktx_size_t offsetScan = 0;
                    ktxTexture_GetImageOffset((ktxTexture*)ktxTex, 0, 0, 0, &offsetScan);
                    const uint8_t* rgba = ktxTexture_GetData(reinterpret_cast<ktxTexture *>(ktxTex)) + offsetScan;
                    size_t pixelCount = static_cast<size_t>(texWidth) * static_cast<size_t>(texHeight);
                    for (size_t i = 0; i < pixelCount; ++i) { if (rgba[i * 4 + 3] < 250) { alphaMaskedHint = true; break; } }
                }
            }
        } else {
            textureFormat = wantSRGB ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
        }

        // Now that we're done reading libktx data, destroy the KTX texture to avoid leaks
        if (isKtx2 && ktxTex) {
            ktxTexture_Destroy((ktxTexture*)ktxTex);
            ktxTex = nullptr;
        }

        // Create texture image using memory pool
        bool differentFamilies = queueFamilyIndices.graphicsFamily.value() != queueFamilyIndices.transferFamily.value();
        std::vector<uint32_t> families;
        if (differentFamilies) {
            families = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.transferFamily.value() };
        }
        // Decide mip count and usage (cap to limit to reduce VRAM)
        if (wasTranscoded && (textureFormat == vk::Format::eR8G8B8A8Srgb || textureFormat == vk::Format::eR8G8B8A8Unorm)) {
            // We can generate mips for uncompressed textures
            uint32_t fullMips = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
            mipLevels = std::max(1u, std::min(fullMips, maxAutoGeneratedMipLevels));
        } else {
            // For compressed or header-provided compressed formats, rely on the base level only
            mipLevels = 1;
        }

        vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
        if (mipLevels > 1) {
            usageFlags |= vk::ImageUsageFlagBits::eTransferSrc; // needed for blit generation
        }

        // Create image with OOM fallback: retry with mipLevels=1 and reduced usage if needed
        try {
            auto [textureImg, textureImgAllocation] = createImagePooled(
                texWidth,
                texHeight,
                textureFormat,
                vk::ImageTiling::eOptimal,
                usageFlags,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                /*mipLevels*/ mipLevels,
                differentFamilies ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
                families
            );
            resources.textureImage = std::move(textureImg);
            resources.textureImageAllocation = std::move(textureImgAllocation);
        } catch (const std::exception& e) {
            std::cerr << "Image allocation failed (" << resolvedPath << "): " << e.what() << ". Retrying with mipLevels=1..." << std::endl;
            // Retry with a single mip level and no TRANSFER_SRC usage to reduce memory pressure
            mipLevels = 1;
            usageFlags &= ~vk::ImageUsageFlagBits::eTransferSrc;
            auto [textureImg2, textureImgAllocation2] = createImagePooled(
                texWidth,
                texHeight,
                textureFormat,
                vk::ImageTiling::eOptimal,
                usageFlags,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                /*mipLevels*/ mipLevels,
                differentFamilies ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
                families
            );
            resources.textureImage = std::move(textureImg2);
            resources.textureImageAllocation = std::move(textureImgAllocation2);
        }

        // GPU upload for this texture (base level)
        uploadImageFromStaging(*stagingBuffer, *resources.textureImage, textureFormat, copyRegions, mipLevels, imageSize);

        // Generate mip chain if requested (only for uncompressed RGBA textures)
        if (mipLevels > 1 && (textureFormat == vk::Format::eR8G8B8A8Srgb || textureFormat == vk::Format::eR8G8B8A8Unorm)) {
            generateMipmaps(*resources.textureImage, textureFormat, texWidth, texHeight, mipLevels);
        }

        // Store the format and mipLevels for createTextureImageView
        resources.format = textureFormat;
        resources.mipLevels = mipLevels;
        resources.alphaMaskedHint = alphaMaskedHint;

        // Create texture image view
        if (!createTextureImageView(resources)) {
            return false;
        }

        // Create texture sampler
        if (!createTextureSampler(resources)) {
            return false;
        }

        // Add to texture resources map (guarded)
        {
            std::unique_lock<std::shared_mutex> texLock(textureResourcesMutex);
            textureResources[textureId] = std::move(resources);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create texture image: " << e.what() << std::endl;
        return false;
    }
}

// Create texture image view
bool Renderer::createTextureImageView(TextureResources& resources) {
    try {
        resources.textureImageView = createImageView(
            resources.textureImage,
            resources.format, // Use the stored format instead of hardcoded sRGB
            vk::ImageAspectFlagBits::eColor,
            resources.mipLevels // Use the stored mipLevels
        );
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create texture image view: " << e.what() << std::endl;
        return false;
    }
}

// Create shared default PBR textures (to avoid creating hundreds of identical textures)
bool Renderer::createSharedDefaultPBRTextures() {
    try {
        unsigned char translucentPixel[4] = {128, 128, 128, 125}; // 50% alpha
        if (!LoadTextureFromMemory(SHARED_DEFAULT_ALBEDO_ID, translucentPixel, 1, 1, 4)) {
            std::cerr << "Failed to create shared default albedo texture" << std::endl;
            return false;
        }

        // Create shared default normal texture (flat normal)
        unsigned char normalPixel[4] = {128, 128, 255, 255}; // (0.5, 0.5, 1.0, 1.0) in 0-255 range
        if (!LoadTextureFromMemory(SHARED_DEFAULT_NORMAL_ID, normalPixel, 1, 1, 4)) {
            std::cerr << "Failed to create shared default normal texture" << std::endl;
            return false;
        }

        // Create shared default metallic-roughness texture (non-metallic, fully rough)
        unsigned char metallicRoughnessPixel[4] = {0, 255, 0, 255}; // (unused, roughness=1.0, metallic=0.0, alpha=1.0)
        if (!LoadTextureFromMemory(SHARED_DEFAULT_METALLIC_ROUGHNESS_ID, metallicRoughnessPixel, 1, 1, 4)) {
            std::cerr << "Failed to create shared default metallic-roughness texture" << std::endl;
            return false;
        }

        // Create shared default occlusion texture (white - no occlusion)
        unsigned char occlusionPixel[4] = {255, 255, 255, 255};
        if (!LoadTextureFromMemory(SHARED_DEFAULT_OCCLUSION_ID, occlusionPixel, 1, 1, 4)) {
            std::cerr << "Failed to create shared default occlusion texture" << std::endl;
            return false;
        }

        // Create shared default emissive texture (black - no emission)
        unsigned char emissivePixel[4] = {0, 0, 0, 255};
        if (!LoadTextureFromMemory(SHARED_DEFAULT_EMISSIVE_ID, emissivePixel, 1, 1, 4)) {
            std::cerr << "Failed to create shared default emissive texture" << std::endl;
            return false;
        }

        // Create shared bright red texture for ball visibility
        unsigned char brightRedPixel[4] = {255, 0, 0, 255}; // Bright red (R=255, G=0, B=0, A=255)
        if (!LoadTextureFromMemory(SHARED_BRIGHT_RED_ID, brightRedPixel, 1, 1, 4)) {
            std::cerr << "Failed to create shared bright red texture" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create shared default PBR textures: " << e.what() << std::endl;
        return false;
    }
}

// Create default texture resources (1x1 white texture)
bool Renderer::createDefaultTextureResources() {
    try {
        // Create a 1x1 white texture
        const uint32_t width = 1;
        const uint32_t height = 1;
        const uint32_t pixelSize = 4; // RGBA
        const std::vector<uint8_t> pixels = {255, 255, 255, 255}; // White pixel (RGBA)

        // Create staging buffer
        vk::DeviceSize imageSize = width * height * pixelSize;
        auto [stagingBuffer, stagingBufferMemory] = createBuffer(
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        // Copy pixel data to staging buffer
        void* data = stagingBufferMemory.mapMemory(0, imageSize);
        memcpy(data, pixels.data(), static_cast<size_t>(imageSize));
        stagingBufferMemory.unmapMemory();

        // Create texture image using memory pool
        auto [textureImg, textureImgAllocation] = createImagePooled(
            width,
            height,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        defaultTextureResources.textureImage = std::move(textureImg);
        defaultTextureResources.textureImageAllocation = std::move(textureImgAllocation);

        // Transition image layout for copy
        transitionImageLayout(
            *defaultTextureResources.textureImage,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal
        );

        // Copy buffer to image
        std::vector<vk::BufferImageCopy> regions = {{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1}
        }};
        copyBufferToImage(
            *stagingBuffer,
            *defaultTextureResources.textureImage,
            width,
            height,
            regions
        );

        // Transition image layout for shader access
        transitionImageLayout(
            *defaultTextureResources.textureImage,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );

        // Create texture image view
        defaultTextureResources.textureImageView = createImageView(
            defaultTextureResources.textureImage,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageAspectFlagBits::eColor
        );

        // Create texture sampler
        return createTextureSampler(defaultTextureResources);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create default texture resources: " << e.what() << std::endl;
        return false;
    }
}

// Create texture sampler
bool Renderer::createTextureSampler(TextureResources& resources) {
    try {
        ensureThreadLocalVulkanInit();
        // Get physical device properties
        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

        // Create sampler with mipmapping + anisotropy (clamped to device limit)
        float deviceMaxAniso = properties.limits.maxSamplerAnisotropy;
        float desiredAniso = std::clamp(samplerMaxAnisotropy, 1.0f, deviceMaxAniso);
        float maxLod = resources.mipLevels > 1 ? static_cast<float>(resources.mipLevels - 1) : 0.0f;
        vk::SamplerCreateInfo samplerInfo{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eRepeat,
            .addressModeV = vk::SamplerAddressMode::eRepeat,
            .addressModeW = vk::SamplerAddressMode::eRepeat,
            .mipLodBias = 0.0f,
            .anisotropyEnable = desiredAniso > 1.0f ? VK_TRUE : VK_FALSE,
            .maxAnisotropy = desiredAniso,
            .compareEnable = VK_FALSE,
            .compareOp = vk::CompareOp::eAlways,
            .minLod = 0.0f,
            .maxLod = maxLod,
            .borderColor = vk::BorderColor::eIntOpaqueBlack,
            .unnormalizedCoordinates = VK_FALSE
        };

        resources.textureSampler = vk::raii::Sampler(device, samplerInfo);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create texture sampler: " << e.what() << std::endl;
        return false;
    }
}

// Load texture from file (public wrapper for createTextureImage)
bool Renderer::LoadTexture(const std::string& texturePath) {
    ensureThreadLocalVulkanInit();
    if (texturePath.empty()) {
        std::cerr << "LoadTexture: Empty texture path provided" << std::endl;
        return false;
    }

    // Resolve aliases (canonical ID -> actual key)
    const std::string resolvedId = ResolveTextureId(texturePath);

    // Check if texture is already loaded
    {
        std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
        auto it = textureResources.find(resolvedId);
        if (it != textureResources.end()) {
            // Texture already loaded
            return true;
        }
    }

    // Create temporary texture resources (unused output; cache will be populated internally)
    TextureResources tempResources;

    // Use existing createTextureImage method (it inserts into textureResources on success) if it's a KTX2 path; otherwise fall back to memory path below
    bool success = false;
    if (resolvedId.size() > 5 && resolvedId.rfind(".ktx2") == resolvedId.size() - 5) {
        success = createTextureImage(resolvedId, tempResources);
        if (success) return true;
        // Fall through to raw-memory path if KTX load failed
    }

    if (!success) {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
    }

    return success;
}

// Determine appropriate texture format based on texture type
vk::Format Renderer::determineTextureFormat(const std::string& textureId) {
    // Determine sRGB vs Linear in a case-insensitive way
    std::string idLower = textureId;
    std::ranges::transform(idLower, idLower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    // BaseColor/Albedo/Diffuse & SpecGloss RGB should be sRGB for proper gamma correction
    if (idLower.find("basecolor") != std::string::npos ||
        idLower.find("base_color") != std::string::npos ||
        idLower.find("albedo") != std::string::npos ||
        idLower.find("diffuse") != std::string::npos ||
        idLower.find("specgloss") != std::string::npos ||
        idLower.find("specularglossiness") != std::string::npos ||
        textureId == Renderer::SHARED_DEFAULT_ALBEDO_ID) {
        return vk::Format::eR8G8B8A8Srgb;
    }

    // Emissive is color data and should be sampled in sRGB
    if (idLower.find("emissive") != std::string::npos ||
        textureId == Renderer::SHARED_DEFAULT_EMISSIVE_ID) {
        return vk::Format::eR8G8B8A8Srgb;
    }

    // Shared bright red (ball) is a color texture; ensure sRGB for vivid appearance
    if (textureId == Renderer::SHARED_BRIGHT_RED_ID) {
        return vk::Format::eR8G8B8A8Srgb;
    }

    // All other PBR textures (normal, metallic-roughness, occlusion) should be linear
    // because they contain non-color data that shouldn't be gamma corrected
    return vk::Format::eR8G8B8A8Unorm;
}

// Load texture from raw image data in memory
bool Renderer::LoadTextureFromMemory(const std::string& textureId, const unsigned char* imageData,
                                    int width, int height, int channels) {
    ensureThreadLocalVulkanInit();
    const std::string resolvedId = ResolveTextureId(textureId);
    std::cout << "[LoadTextureFromMemory] start id=" << textureId << " -> resolved=" << resolvedId << " size=" << width << "x" << height << " ch=" << channels << std::endl;
    if (resolvedId.empty() || !imageData || width <= 0 || height <= 0 || channels <= 0) {
        std::cerr << "LoadTextureFromMemory: Invalid parameters" << std::endl;
        return false;
    }

    // Check if texture is already loaded
    {
        std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
        auto it = textureResources.find(resolvedId);
        if (it != textureResources.end()) {
            // Texture already loaded
            return true;
        }
    }

    // Per-texture de-duplication (serialize loads of the same texture ID only)
    {
        std::unique_lock<std::mutex> lk(textureLoadStateMutex);
        while (texturesLoading.contains(resolvedId)) {
            textureLoadStateCv.wait(lk);
        }
    }
    // Double-check cache after the wait
    {
        std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
        auto it2 = textureResources.find(resolvedId);
        if (it2 != textureResources.end()) {
            return true;
        }
    }
    // Mark as loading and ensure we notify on all exit paths
    {
        std::lock_guard<std::mutex> lk(textureLoadStateMutex);
        texturesLoading.insert(resolvedId);
    }
    auto _loadingGuard = std::unique_ptr<void, std::function<void(void*)>>(reinterpret_cast<void *>(1), [this, resolvedId](void*){
        std::lock_guard<std::mutex> lk(textureLoadStateMutex);
        texturesLoading.erase(resolvedId);
        textureLoadStateCv.notify_all();
    });

    try {
        TextureResources resources;

        // Calculate image size (ensure 4 channels for RGBA)
        int targetChannels = 4; // Always use RGBA for consistency
        vk::DeviceSize imageSize = width * height * targetChannels;

        // Create a staging buffer
        auto [stagingBuffer, stagingBufferMemory] = createBuffer(
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        // Copy and convert pixel data to staging buffer
        void* data = stagingBufferMemory.mapMemory(0, imageSize);
        auto* stagingData = static_cast<unsigned char*>(data);

        if (channels == 4) {
            // Already RGBA, direct copy
            memcpy(stagingData, imageData, imageSize);
        } else if (channels == 3) {
            // RGB to RGBA conversion
            for (int i = 0; i < width * height; ++i) {
                stagingData[i * 4 + 0] = imageData[i * 3 + 0]; // R
                stagingData[i * 4 + 1] = imageData[i * 3 + 1]; // G
                stagingData[i * 4 + 2] = imageData[i * 3 + 2]; // B
                stagingData[i * 4 + 3] = 255; // A
            }
        } else if (channels == 2) {
            // Grayscale + Alpha to RGBA conversion
            for (int i = 0; i < width * height; ++i) {
                stagingData[i * 4 + 0] = imageData[i * 2 + 0]; // R (grayscale)
                stagingData[i * 4 + 1] = imageData[i * 2 + 0]; // G (grayscale)
                stagingData[i * 4 + 2] = imageData[i * 2 + 0]; // B (grayscale)
                stagingData[i * 4 + 3] = imageData[i * 2 + 1]; // A (alpha)
            }
        } else if (channels == 1) {
            // Grayscale to RGBA conversion
            for (int i = 0; i < width * height; ++i) {
                stagingData[i * 4 + 0] = imageData[i]; // R
                stagingData[i * 4 + 1] = imageData[i]; // G
                stagingData[i * 4 + 2] = imageData[i]; // B
                stagingData[i * 4 + 3] = 255; // A
            }
        } else {
            std::cerr << "LoadTextureFromMemory: Unsupported channel count: " << channels << std::endl;
            stagingBufferMemory.unmapMemory();
            return false;
        }

        // Analyze alpha to set alphaMaskedHint (treat as masked if any pixel alpha < ~1.0)
        bool alphaMaskedHint = false;
        for (int i = 0, n = width * height; i < n; ++i) {
            if (stagingData[i * 4 + 3] < 250) { alphaMaskedHint = true; break; }
        }

        stagingBufferMemory.unmapMemory();

        // Determine the appropriate texture format based on the texture type
        vk::Format textureFormat = determineTextureFormat(textureId);

        // Create texture image using memory pool (with optional mipmap generation)
        bool differentFamilies = queueFamilyIndices.graphicsFamily.value() != queueFamilyIndices.transferFamily.value();
        std::vector<uint32_t> families;
        if (differentFamilies) {
            families = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.transferFamily.value() };
        }
        // Decide mip count and usage for memory textures; cap to reduce VRAM pressure
        uint32_t mipLevels = 1;
        if (width > 1 && height > 1) {
            uint32_t full = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
            mipLevels = std::max(1u, std::min(full, maxAutoGeneratedMipLevels));
        }
        vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
        if (mipLevels > 1) usageFlags |= vk::ImageUsageFlagBits::eTransferSrc;

        // OOM-resilient allocation
        try {
            auto [textureImg, textureImgAllocation] = createImagePooled(
                width,
                height,
                textureFormat,
                vk::ImageTiling::eOptimal,
                usageFlags,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                mipLevels,
                differentFamilies ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
                families
            );

            resources.textureImage = std::move(textureImg);
            resources.textureImageAllocation = std::move(textureImgAllocation);
        } catch (const std::exception& e) {
            std::cerr << "Image allocation failed (memory texture): " << e.what() << ". Retrying with mipLevels=1..." << std::endl;
            mipLevels = 1;
            usageFlags &= ~vk::ImageUsageFlagBits::eTransferSrc;
            auto [textureImg, textureImgAllocation] = createImagePooled(
                width,
                height,
                textureFormat,
                vk::ImageTiling::eOptimal,
                usageFlags,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                mipLevels,
                differentFamilies ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
                families
            );
            resources.textureImage = std::move(textureImg);
            resources.textureImageAllocation = std::move(textureImgAllocation);
        }

        // GPU upload. Copy buffer to image in a single submit.
        std::vector<vk::BufferImageCopy> regions = {{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1}
        }};
        uploadImageFromStaging(*stagingBuffer, *resources.textureImage, textureFormat, regions, mipLevels, imageSize);

        // Generate mip chain if requested and format is uncompressed RGBA
        if (mipLevels > 1 && (textureFormat == vk::Format::eR8G8B8A8Srgb || textureFormat == vk::Format::eR8G8B8A8Unorm)) {
            generateMipmaps(*resources.textureImage, textureFormat, width, height, mipLevels);
        }

        // Store the format for createTextureImageView
        resources.format = textureFormat;
        resources.mipLevels = mipLevels;
        resources.alphaMaskedHint = alphaMaskedHint;

        // Use resolvedId as the cache key to avoid duplicates
        const std::string& cacheId = resolvedId;

        // Create texture image view
        resources.textureImageView = createImageView(
            resources.textureImage,
            textureFormat,
            vk::ImageAspectFlagBits::eColor,
            mipLevels
        );

        // Create texture sampler
        if (!createTextureSampler(resources)) {
            return false;
        }

        // Add to texture resources map (guarded)
        {
            std::unique_lock<std::shared_mutex> texLock(textureResourcesMutex);
            textureResources[cacheId] = std::move(resources);
        }

        std::cout << "Successfully loaded texture from memory: " << cacheId
                  << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to load texture from memory: " << e.what() << std::endl;
        return false;
    }
}

// Create mesh resources
bool Renderer::createMeshResources(MeshComponent* meshComponent, bool deferUpload) {
    ensureThreadLocalVulkanInit();
    try {
        // If resources already exist, no need to recreate them.
        auto it = meshResources.find(meshComponent);
        if (it != meshResources.end()) {
            return true;
        }

        // Get mesh data
        const auto& vertices = meshComponent->GetVertices();
        const auto& indices = meshComponent->GetIndices();

        if (vertices.empty() || indices.empty()) {
            std::cerr << "Mesh has no vertices or indices" << std::endl;
            return false;
        }

        // --- 1. Create and fill per-mesh staging buffers on the host ---
        vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        auto [stagingVertexBuffer, stagingVertexBufferMemory] = createBuffer(
            vertexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        void* vertexData = stagingVertexBufferMemory.mapMemory(0, vertexBufferSize);
        std::memcpy(vertexData, vertices.data(), static_cast<size_t>(vertexBufferSize));
        stagingVertexBufferMemory.unmapMemory();

        vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
        auto [stagingIndexBuffer, stagingIndexBufferMemory] = createBuffer(
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        void* indexData = stagingIndexBufferMemory.mapMemory(0, indexBufferSize);
        std::memcpy(indexData, indices.data(), static_cast<size_t>(indexBufferSize));
        stagingIndexBufferMemory.unmapMemory();

        // --- 2. Create device-local vertex and index buffers via the memory pool ---
        // Add ray tracing flags: eShaderDeviceAddress for vkGetBufferDeviceAddress and
        // eAccelerationStructureBuildInputReadOnlyKHR for acceleration structure building
        auto [vertexBuffer, vertexBufferAllocation] = createBufferPooled(
            vertexBufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        auto [indexBuffer, indexBufferAllocation] = createBufferPooled(
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        // --- 3. Either copy now (legacy path) or defer copies for batched submission ---
        MeshResources resources;
        resources.vertexBuffer = std::move(vertexBuffer);
        resources.vertexBufferAllocation = std::move(vertexBufferAllocation);
        resources.indexBuffer = std::move(indexBuffer);
        resources.indexBufferAllocation = std::move(indexBufferAllocation);
        resources.indexCount = static_cast<uint32_t>(indices.size());

        if (deferUpload) {
            // Keep staging buffers alive and record their sizes; copies will be
            // performed later by preAllocateEntityResourcesBatch().
            resources.stagingVertexBuffer = std::move(stagingVertexBuffer);
            resources.stagingVertexBufferMemory = std::move(stagingVertexBufferMemory);
            resources.vertexBufferSizeBytes = vertexBufferSize;

            resources.stagingIndexBuffer = std::move(stagingIndexBuffer);
            resources.stagingIndexBufferMemory = std::move(stagingIndexBufferMemory);
            resources.indexBufferSizeBytes = indexBufferSize;
        } else {
            // Immediate upload path used by preAllocateEntityResources() and other
            // small-object callers. This preserves existing behaviour.
            copyBuffer(stagingVertexBuffer, resources.vertexBuffer, vertexBufferSize);
            copyBuffer(stagingIndexBuffer, resources.indexBuffer, indexBufferSize);
            // staging* buffers are RAII objects and will be destroyed on scope exit.
        }

        // Add to mesh resources map
        meshResources[meshComponent] = std::move(resources);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create mesh resources: " << e.what() << std::endl;
        return false;
    }
}

// Create uniform buffers
bool Renderer::createUniformBuffers(Entity* entity) {
    ensureThreadLocalVulkanInit();
    try {
        // Check if entity resources already exist
        auto it = entityResources.find(entity);
        if (it != entityResources.end()) {
            return true;
        }

        // Create entity resources
        EntityResources resources;

        // Create uniform buffers using memory pool
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto [buffer, bufferAllocation] = createBufferPooled(
                bufferSize,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );

            // Use the memory pool's mapped pointer if available
            void* mappedMemory = bufferAllocation->mappedPtr;
            if (!mappedMemory) {
                std::cerr << "Warning: Uniform buffer allocation is not mapped" << std::endl;
            }

            resources.uniformBuffers.emplace_back(std::move(buffer));
            resources.uniformBufferAllocations.emplace_back(std::move(bufferAllocation));
            resources.uniformBuffersMapped.emplace_back(mappedMemory);
        }

        // Create instance buffer for all entities (shaders always expect instance data)
        auto* meshComponent = entity->GetComponent<MeshComponent>();
        if (meshComponent) {
            std::vector<InstanceData> instanceData;

            // CRITICAL FIX: Check if entity has any instance data first
            if (meshComponent->GetInstanceCount() > 0) {
                // Use existing instance data from GLTF loading (whether 1 or many instances)
                instanceData = meshComponent->GetInstances();
            } else {
                // Create single instance data using IDENTITY matrix to avoid double-transform with UBO.model
                InstanceData singleInstance;
                singleInstance.setModelMatrix(glm::mat4(1.0f));
                instanceData = {singleInstance};
            }

            vk::DeviceSize instanceBufferSize = sizeof(InstanceData) * instanceData.size();

            auto [instanceBuffer, instanceBufferAllocation] = createBufferPooled(
                instanceBufferSize,
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );

            // Copy instance data to buffer
            void* instanceMappedMemory = instanceBufferAllocation->mappedPtr;
            if (instanceMappedMemory) {
                std::memcpy(instanceMappedMemory, instanceData.data(), instanceBufferSize);
            } else {
                std::cerr << "Warning: Instance buffer allocation is not mapped" << std::endl;
            }

            resources.instanceBuffer = std::move(instanceBuffer);
            resources.instanceBufferAllocation = std::move(instanceBufferAllocation);
            resources.instanceBufferMapped = instanceMappedMemory;
        }

        // Add to entity resources map
        entityResources[entity] = std::move(resources);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create uniform buffers: " << e.what() << std::endl;
        return false;
    }
}

// Create descriptor pool
bool Renderer::createDescriptorPool() {
    try {
        // Calculate pool sizes for all Bistro materials plus additional entities
        // The Bistro model creates many more entities than initially expected
        // Each entity needs descriptor sets for both basic and PBR pipelines
        // PBR pipeline needs 7 descriptors per set (1 UBO + 5 PBR textures + 1 shadow map array with 16 shadow maps)
        // Basic pipeline needs 2 descriptors per set (1 UBO + 1 texture)
        const uint32_t maxEntities = 20000; // Increased to 20k entities to handle large scenes like Bistro reliably
        const uint32_t maxDescriptorSets = MAX_FRAMES_IN_FLIGHT * maxEntities * 2; // 2 pipeline types per entity

        // Calculate descriptor counts
        // UBO descriptors: 1 per descriptor set
        const uint32_t uboDescriptors = maxDescriptorSets;
        // Texture descriptors: Basic pipeline uses 1, PBR uses 21 (5 PBR textures + 16 shadow maps)
        // Allocate for worst case: all entities using PBR (21 texture descriptors each)
        const uint32_t textureDescriptors = MAX_FRAMES_IN_FLIGHT * maxEntities * 21;
        // Storage buffer descriptors: PBR pipeline uses 1 light storage buffer per descriptor set
        // Only PBR entities need storage buffers, so allocate for all entities using PBR
        // Storage buffers used per PBR descriptor set:
        //  - Binding 6: light storage buffer
        //  - Binding 7: Forward+ tile headers buffer
        //  - Binding 8: Forward+ tile indices buffer
        const uint32_t storageBufferDescriptors = MAX_FRAMES_IN_FLIGHT * maxEntities * 3u;
        
        // Acceleration structure descriptors: Ray query needs 1 TLAS descriptor per frame
        const uint32_t accelerationStructureDescriptors = MAX_FRAMES_IN_FLIGHT;
        
        // Storage image descriptors: Ray query needs 1 output image descriptor per frame
        const uint32_t storageImageDescriptors = MAX_FRAMES_IN_FLIGHT;

        // Reserve extra combined image sampler capacity for Ray Query binding 6 (baseColor texture array)
        const uint32_t rqTexDescriptors = MAX_FRAMES_IN_FLIGHT * RQ_MAX_TEX;
        std::array<vk::DescriptorPoolSize, 5> poolSizes = {
            vk::DescriptorPoolSize{
                .type = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = uboDescriptors
            },
            vk::DescriptorPoolSize{
                .type = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = textureDescriptors + rqTexDescriptors
            },
            vk::DescriptorPoolSize{
                .type = vk::DescriptorType::eStorageBuffer,
                .descriptorCount = storageBufferDescriptors
            },
            vk::DescriptorPoolSize{
                .type = vk::DescriptorType::eAccelerationStructureKHR,
                .descriptorCount = accelerationStructureDescriptors
            },
            vk::DescriptorPoolSize{
                .type = vk::DescriptorType::eStorageImage,
                .descriptorCount = storageImageDescriptors
            }
        };

        // Create descriptor pool
        vk::DescriptorPoolCreateFlags poolFlags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        if (descriptorIndexingEnabled) {
            poolFlags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
        }
        vk::DescriptorPoolCreateInfo poolInfo{
            .flags = poolFlags,
            .maxSets = maxDescriptorSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()
        };

        descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create descriptor pool: " << e.what() << std::endl;
        return false;
    }
}

// Create descriptor sets
bool Renderer::createDescriptorSets(Entity* entity, const std::string& texturePath, bool usePBR) {
    // Resolve alias before taking the shared lock to avoid nested shared_lock on the same mutex
    const std::string resolvedTexturePath = ResolveTextureId(texturePath);
    std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
    try {
        auto entityIt = entityResources.find(entity);
        if (entityIt == entityResources.end()) return false;

        vk::DescriptorSetLayout selectedLayout = usePBR ? *pbrDescriptorSetLayout : *descriptorSetLayout;
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, selectedLayout);
        vk::DescriptorSetAllocateInfo allocInfo{ .descriptorPool = *descriptorPool, .descriptorSetCount = MAX_FRAMES_IN_FLIGHT, .pSetLayouts = layouts.data() };

        auto& targetDescriptorSets = usePBR ? entityIt->second.pbrDescriptorSets : entityIt->second.basicDescriptorSets;
        if (targetDescriptorSets.empty()) {
            std::lock_guard<std::mutex> lk(descriptorMutex);
            // Allocate into a temporary owning container, then move the individual RAII sets into our vector.
            // (Avoid assigning `vk::raii::DescriptorSets` directly into `std::vector<vk::raii::DescriptorSet>`.)
            auto sets = vk::raii::DescriptorSets(device, allocInfo);
            targetDescriptorSets.clear();
            targetDescriptorSets.reserve(sets.size());
            for (auto &s : sets) {
                targetDescriptorSets.emplace_back(std::move(s));
            }
        }

        // CRITICAL FIX: Validate that allocated descriptor sets have valid handles before use.
        // The error "Invalid VkDescriptorSet Object 0x131a000000131a" indicates a corrupted
        // or freed descriptor set handle. This can happen if:
        // 1. Descriptor pool is exhausted and allocation fails silently
        // 2. Descriptor set was destroyed elsewhere before this function
        // 3. Memory corruption or race condition
        // Checking validity prevents SIGSEGV crash when Vulkan tries to access invalid handles.
        if (targetDescriptorSets.empty() || targetDescriptorSets.size() < MAX_FRAMES_IN_FLIGHT) {
            std::cerr << "ERROR: Descriptor set allocation failed for entity " << entity->GetName() 
                      << " (usePBR=" << usePBR << "). Descriptor pool may be exhausted." << std::endl;
            return false;
        }

        // Only initialize the current frame's descriptor set at runtime to avoid
        // updating descriptor sets that may be in use by pending command buffers.
        // Other frames will be initialized at their own safe points.
        size_t startIndex = static_cast<size_t>(currentFrame);
        size_t endIndex = startIndex + 1;
        for (size_t i = startIndex; i < endIndex; i++) {
            // Validate descriptor set handle before dereferencing to prevent crash
            // Check if the underlying VkDescriptorSet handle is valid (not null/default)
            vk::DescriptorSet handleCheck = *targetDescriptorSets[i];
            if (handleCheck == vk::DescriptorSet{}) {
                std::cerr << "ERROR: Invalid descriptor set handle for entity " << entity->GetName() 
                          << " frame " << i << " (usePBR=" << usePBR << ")" << std::endl;
                return false;
            }
            vk::DescriptorBufferInfo bufferInfo{ .buffer = *entityIt->second.uniformBuffers[i], .range = sizeof(UniformBufferObject) };

            if (usePBR) {
                // Build descriptor writes dynamically to avoid writing unused bindings
                std::vector<vk::WriteDescriptorSet> descriptorWrites;
                std::array<vk::DescriptorImageInfo, 5> imageInfos;
                // CRITICAL FIX: Buffer infos must remain in scope until updateDescriptorSets completes.
                // Previously these were declared inside nested scopes, causing dangling pointers
                // when descriptorWrites held pointers to them after they went out of scope.
                vk::DescriptorBufferInfo lightBufferInfo;
                vk::DescriptorBufferInfo headersInfo;
                vk::DescriptorBufferInfo indicesInfo;

                descriptorWrites.push_back({ .dstSet = *targetDescriptorSets[i], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &bufferInfo });

                auto meshComponent = entity->GetComponent<MeshComponent>();
                std::vector<std::string> pbrTexturePaths = { /* ... same as before ... */ };
                // ... (logic to get texture paths is the same)
                {
                    const std::string legacyPath = (meshComponent ? meshComponent->GetTexturePath() : std::string());
                    const std::string baseColorPath = (meshComponent && !meshComponent->GetBaseColorTexturePath().empty())
                                                      ? meshComponent->GetBaseColorTexturePath()
                                                      : (!legacyPath.empty() ? legacyPath : SHARED_DEFAULT_ALBEDO_ID);
                    const std::string mrPath = (meshComponent && !meshComponent->GetMetallicRoughnessTexturePath().empty())
                                               ? meshComponent->GetMetallicRoughnessTexturePath()
                                               : SHARED_DEFAULT_METALLIC_ROUGHNESS_ID;
                    const std::string normalPath = (meshComponent && !meshComponent->GetNormalTexturePath().empty())
                                                   ? meshComponent->GetNormalTexturePath()
                                                   : SHARED_DEFAULT_NORMAL_ID;
                    const std::string occlusionPath = (meshComponent && !meshComponent->GetOcclusionTexturePath().empty())
                                                      ? meshComponent->GetOcclusionTexturePath()
                                                      : SHARED_DEFAULT_OCCLUSION_ID;
                    const std::string emissivePath = (meshComponent && !meshComponent->GetEmissiveTexturePath().empty())
                                                    ? meshComponent->GetEmissiveTexturePath()
                                                    : SHARED_DEFAULT_EMISSIVE_ID;

                    pbrTexturePaths = { baseColorPath, mrPath, normalPath, occlusionPath, emissivePath };
                }


                for (int j = 0; j < 5; j++) {
                    const auto resolvedBindingPath = ResolveTextureId(pbrTexturePaths[j]);
                    auto textureIt = textureResources.find(resolvedBindingPath);
                    TextureResources* texRes = (textureIt != textureResources.end()) ? &textureIt->second : &defaultTextureResources;
                    imageInfos[j] = { .sampler = *texRes->textureSampler, .imageView = *texRes->textureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
                    descriptorWrites.push_back({ .dstSet = *targetDescriptorSets[i], .dstBinding = static_cast<uint32_t>(j + 1), .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &imageInfos[j] });
                }

                lightBufferInfo = vk::DescriptorBufferInfo{ .buffer = *lightStorageBuffers[i].buffer, .range = VK_WHOLE_SIZE };
                descriptorWrites.push_back({ .dstSet = *targetDescriptorSets[i], .dstBinding = 6, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &lightBufferInfo });

                // CRITICAL FIX: PBR descriptor set layout ALWAYS requires bindings 7/8 (tileHeaders, tileLightIndices).
                // We MUST bind valid buffers (real or dummy) to satisfy Vulkan validation.
                // Creating dummy buffers here ensures bindings are never left uninitialized.
                
                // Ensure Forward+ per-frame array exists
                if (forwardPlusPerFrame.empty()) {
                    forwardPlusPerFrame.resize(MAX_FRAMES_IN_FLIGHT);
                }
                
                // Ensure tile headers buffer exists (binding 7) - create minimal dummy if needed
                if (i < forwardPlusPerFrame.size()) {
                    auto &f = forwardPlusPerFrame[i];
                    if (f.tileHeaders == nullptr) {
                        vk::DeviceSize minSize = sizeof(uint32_t) * 4; // Single TileHeader {offset, count, pad0, pad1}
                        auto [buf, alloc] = createBufferPooled(minSize,
                            vk::BufferUsageFlagBits::eStorageBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                        f.tileHeaders = std::move(buf);
                        f.tileHeadersAlloc = std::move(alloc);
                        if (f.tileHeadersAlloc && f.tileHeadersAlloc->mappedPtr) {
                            std::memset(f.tileHeadersAlloc->mappedPtr, 0, minSize);
                        }
                    }
                    headersInfo = vk::DescriptorBufferInfo{ .buffer = *f.tileHeaders, .offset = 0, .range = VK_WHOLE_SIZE };
                }
                
                // Ensure tile light indices buffer exists (binding 8) - create minimal dummy if needed
                if (i < forwardPlusPerFrame.size()) {
                    auto &f = forwardPlusPerFrame[i];
                    if (f.tileLightIndices == nullptr) {
                        vk::DeviceSize minSize = sizeof(uint32_t) * 4; // Minimal array of 4 uints
                        auto [buf, alloc] = createBufferPooled(minSize,
                            vk::BufferUsageFlagBits::eStorageBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                        f.tileLightIndices = std::move(buf);
                        f.tileLightIndicesAlloc = std::move(alloc);
                        if (f.tileLightIndicesAlloc && f.tileLightIndicesAlloc->mappedPtr) {
                            std::memset(f.tileLightIndicesAlloc->mappedPtr, 0, minSize);
                        }
                    }
                    indicesInfo = vk::DescriptorBufferInfo{ .buffer = *f.tileLightIndices, .offset = 0, .range = VK_WHOLE_SIZE };
                }
                
                // Now both headersInfo and indicesInfo have valid buffers (never nullptr)
                descriptorWrites.push_back({ .dstSet = *targetDescriptorSets[i], .dstBinding = 7, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &headersInfo });
                descriptorWrites.push_back({ .dstSet = *targetDescriptorSets[i], .dstBinding = 8, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &indicesInfo });

                {
                    std::lock_guard<std::mutex> lk(descriptorMutex);
                    device.updateDescriptorSets(descriptorWrites, {});
                }
            } else { // Basic Pipeline
                // ... (this part remains the same)
                 auto textureIt = textureResources.find(resolvedTexturePath);
                TextureResources* texRes = (textureIt != textureResources.end()) ? &textureIt->second : &defaultTextureResources;
                vk::DescriptorImageInfo imageInfo{ .sampler = *texRes->textureSampler, .imageView = *texRes->textureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
                std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
                    vk::WriteDescriptorSet{ .dstSet = *targetDescriptorSets[i], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &bufferInfo },
                    vk::WriteDescriptorSet{ .dstSet = *targetDescriptorSets[i], .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &imageInfo }
                };
                {
                    std::lock_guard<std::mutex> lk(descriptorMutex);
                    device.updateDescriptorSets(descriptorWrites, {});
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create descriptor sets for " << entity->GetName() << ": " << e.what() << std::endl;
        return false;
    }
}

// Pre-allocate all Vulkan resources for an entity during scene loading
bool Renderer::preAllocateEntityResources(Entity* entity) {
    try {
        // Get the mesh component
        auto meshComponent = entity->GetComponent<MeshComponent>();
        if (!meshComponent) {
            std::cerr << "Entity " << entity->GetName() << " has no mesh component" << std::endl;
            return false;
        }

        // Ensure local AABB is available for debug/probes
        meshComponent->RecomputeLocalAABB();

        // 1. Create mesh resources (vertex/index buffers)
        if (!createMeshResources(meshComponent)) {
            std::cerr << "Failed to create mesh resources for entity: " << entity->GetName() << std::endl;
            return false;
        }

        // 2. Create uniform buffers
        if (!createUniformBuffers(entity)) {
            std::cerr << "Failed to create uniform buffers for entity: " << entity->GetName() << std::endl;
            return false;
        }

        // Initialize per-frame UBO and image binding write flags
        {
            auto it = entityResources.find(entity);
            if (it != entityResources.end()) {
                it->second.uboBindingWritten.assign(MAX_FRAMES_IN_FLIGHT, false);
                it->second.pbrImagesWritten.assign(MAX_FRAMES_IN_FLIGHT, false);
                it->second.basicImagesWritten.assign(MAX_FRAMES_IN_FLIGHT, false);
            }
        }


        // 3. Pre-allocate BOTH basic and PBR descriptor sets
        std::string texturePath = meshComponent->GetTexturePath();
        // Fallback: if legacy texturePath is empty, use PBR baseColor texture
        if (texturePath.empty()) {
            const std::string& baseColor = meshComponent->GetBaseColorTexturePath();
            if (!baseColor.empty()) {
                texturePath = baseColor;
            }
        }

        // Create basic descriptor sets
        if (!createDescriptorSets(entity, texturePath, false)) {
            std::cerr << "Failed to create basic descriptor sets for entity: " << entity->GetName() << std::endl;
            return false;
        }

        // Create PBR descriptor sets
        if (!createDescriptorSets(entity, texturePath, true)) {
            std::cerr << "Failed to create PBR descriptor sets for entity: " << entity->GetName() << std::endl;
            return false;
        }
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to pre-allocate resources for entity " << entity->GetName() << ": " << e.what() << std::endl;
        return false;
    }
}

// Pre-allocate Vulkan resources for a batch of entities, batching mesh uploads
bool Renderer::preAllocateEntityResourcesBatch(const std::vector<Entity*>& entities) {
    ensureThreadLocalVulkanInit();
    try {
        // --- 1. For all entities, create mesh resources with deferred uploads ---
        std::vector<MeshComponent*> meshesNeedingUpload;
        meshesNeedingUpload.reserve(entities.size());

        for (Entity* entity : entities) {
            if (!entity) {
                continue;
            }

            auto meshComponent = entity->GetComponent<MeshComponent>();
            if (!meshComponent) {
                continue;
            }

            // Ensure local AABB is available for debug/probes
            meshComponent->RecomputeLocalAABB();

            if (!createMeshResources(meshComponent, true)) {
                std::cerr << "Failed to create mesh resources for entity (batch): "
                          << entity->GetName() << std::endl;
                return false;
            }

            auto it = meshResources.find(meshComponent);
            if (it == meshResources.end()) {
                continue;
            }
            MeshResources& res = it->second;

            // Only schedule meshes that still have staged data pending upload
            if (res.vertexBufferSizeBytes > 0 && res.indexBufferSizeBytes > 0) {
                meshesNeedingUpload.push_back(meshComponent);
            }
        }

        // --- 2. Defer all GPU copies to the render thread safe point ---
        if (!meshesNeedingUpload.empty()) {
            EnqueueMeshUploads(meshesNeedingUpload);
        }

        // --- 3. Create uniform buffers and descriptor sets per entity ---
        for (Entity* entity : entities) {
            if (!entity) {
                continue;
            }

            auto meshComponent = entity->GetComponent<MeshComponent>();
            if (!meshComponent) {
                continue;
            }

            if (!createUniformBuffers(entity)) {
                std::cerr << "Failed to create uniform buffers for entity (batch): "
                          << entity->GetName() << std::endl;
                return false;
            }

            std::string texturePath = meshComponent->GetTexturePath();
            // Fallback: if legacy texturePath is empty, use PBR baseColor texture
            if (texturePath.empty()) {
                const std::string& baseColor = meshComponent->GetBaseColorTexturePath();
                if (!baseColor.empty()) {
                    texturePath = baseColor;
                }
            }

            if (!createDescriptorSets(entity, texturePath, false)) {
                std::cerr << "Failed to create basic descriptor sets for entity (batch): "
                          << entity->GetName() << std::endl;
                return false;
            }

            if (!createDescriptorSets(entity, texturePath, true)) {
                std::cerr << "Failed to create PBR descriptor sets for entity (batch): "
                          << entity->GetName() << std::endl;
                return false;
            }
        }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to batch pre-allocate resources for entities: " << e.what() << std::endl;
        return false;
    }
}

// Enqueue a set of meshes to upload on the render thread (safe point)
void Renderer::EnqueueMeshUploads(const std::vector<MeshComponent*>& meshes) {
    if (meshes.empty()) return;
    std::lock_guard<std::mutex> lk(pendingMeshUploadsMutex);
    // Avoid duplicates by using a temporary set of current entries
    for (MeshComponent* m : meshes) {
        if (!m) continue;
        pendingMeshUploads.push_back(m);
    }
}

// Execute pending mesh uploads on the render thread after the per-frame fence wait
void Renderer::ProcessPendingMeshUploads() {
    // Grab the list atomically
    std::vector<MeshComponent*> toProcess;
    {
        std::lock_guard<std::mutex> lk(pendingMeshUploadsMutex);
        if (pendingMeshUploads.empty()) return;
        toProcess.swap(pendingMeshUploads);
    }

    // Filter to meshes that still have staged data
    std::vector<MeshComponent*> needsCopy;
    needsCopy.reserve(toProcess.size());
    for (auto* meshComponent : toProcess) {
        auto it = meshResources.find(meshComponent);
        if (it == meshResources.end()) continue;
        const MeshResources& res = it->second;
        if (res.vertexBufferSizeBytes > 0 || res.indexBufferSizeBytes > 0) {
            needsCopy.push_back(meshComponent);
        }
    }
    if (needsCopy.empty()) return;

    // Record copies on GRAPHICS queue to avoid cross-queue hazards while stabilizing
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
    };
    vk::raii::CommandPool tempPool(device, poolInfo);

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = *tempPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    vk::raii::CommandBuffers cbs(device, allocInfo);
    vk::raii::CommandBuffer& cb = cbs[0];
    cb.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    for (auto* meshComponent : needsCopy) {
        auto it = meshResources.find(meshComponent);
        if (it == meshResources.end()) continue;
        MeshResources& res = it->second;
        if (res.vertexBufferSizeBytes > 0 && res.stagingVertexBuffer != nullptr && res.vertexBuffer != nullptr) {
            vk::BufferCopy region{ .srcOffset = 0, .dstOffset = 0, .size = res.vertexBufferSizeBytes };
            cb.copyBuffer(*res.stagingVertexBuffer, *res.vertexBuffer, region);
        }
        if (res.indexBufferSizeBytes > 0 && res.stagingIndexBuffer != nullptr && res.indexBuffer != nullptr) {
            vk::BufferCopy region{ .srcOffset = 0, .dstOffset = 0, .size = res.indexBufferSizeBytes };
            cb.copyBuffer(*res.stagingIndexBuffer, *res.indexBuffer, region);
        }
    }

    cb.end();

    // Submit and wait on the GRAPHICS queue (single-threaded via queueMutex)
    vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*cb };
    vk::raii::Fence fence(device, vk::FenceCreateInfo{});
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        graphicsQueue.submit(submitInfo, *fence);
    }
    (void)device.waitForFences({ *fence }, VK_TRUE, UINT64_MAX);

    // Clear staging once copies are complete
    for (auto* meshComponent : needsCopy) {
        auto it = meshResources.find(meshComponent);
        if (it == meshResources.end()) continue;
        MeshResources& res = it->second;
        res.stagingVertexBuffer = nullptr;
        res.stagingVertexBufferMemory = nullptr;
        res.vertexBufferSizeBytes = 0;
        res.stagingIndexBuffer = nullptr;
        res.stagingIndexBufferMemory = nullptr;
        res.indexBufferSizeBytes = 0;
    }

    // Now that more meshes are READY (uploads finished), request a TLAS rebuild so
    // non‑instanced and previously missing meshes are included in the acceleration structure.
    // This is safe at the render‑thread safe point and avoids partial TLAS builds.
    asDevOverrideAllowRebuild = true; // allow rebuild even if frozen
    RequestAccelerationStructureBuild("uploads completed");
}

// Recreate instance buffer for an entity (e.g., after clearing instances for animation)
bool Renderer::recreateInstanceBuffer(Entity* entity) {
    ensureThreadLocalVulkanInit();
    try {
        // Find the entity in entityResources
        auto it = entityResources.find(entity);
        if (it == entityResources.end()) {
            std::cerr << "Entity " << entity->GetName() << " not found in entityResources" << std::endl;
            return false;
        }

        EntityResources& resources = it->second;

        // Create a single instance with identity matrix
        InstanceData singleInstance;
        singleInstance.setModelMatrix(glm::mat4(1.0f));
        std::vector<InstanceData> instanceData = {singleInstance};

        vk::DeviceSize instanceBufferSize = sizeof(InstanceData) * instanceData.size();

        // Create new instance buffer using memory pool
        auto [instanceBuffer, instanceBufferAllocation] = createBufferPooled(
            instanceBufferSize,
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        // Copy instance data to buffer
        void* instanceMappedMemory = instanceBufferAllocation->mappedPtr;
        if (instanceMappedMemory) {
            std::memcpy(instanceMappedMemory, instanceData.data(), instanceBufferSize);
        } else {
            std::cerr << "Warning: Instance buffer allocation is not mapped" << std::endl;
        }

        // Wait for GPU to finish using the old instance buffer before destroying it.
        // External synchronization required (VVL): serialize against queue submits/present.
        WaitIdle();

        // Replace the old instance buffer with the new one
        resources.instanceBuffer = std::move(instanceBuffer);
        resources.instanceBufferAllocation = std::move(instanceBufferAllocation);
        resources.instanceBufferMapped = instanceMappedMemory;

        std::cout << "[Animation] Recreated instance buffer for entity '" << entity->GetName() 
                  << "' with single identity instance" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to recreate instance buffer for entity " << entity->GetName() 
                  << ": " << e.what() << std::endl;
        return false;
    }
}

// Create buffer using memory pool for efficient allocation
std::pair<vk::raii::Buffer, std::unique_ptr<MemoryPool::Allocation>> Renderer::createBufferPooled(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties) {
    try {
        if (!memoryPool) {
            throw std::runtime_error("Memory pool not initialized");
        }

        // Use memory pool for allocation
        auto [buffer, allocation] = memoryPool->createBuffer(size, usage, properties);

        return {std::move(buffer), std::move(allocation)};

    } catch (const std::exception& e) {
        std::cerr << "Failed to create buffer with memory pool: " << e.what() << std::endl;
        throw;
    }
}

// Legacy createBuffer function - now strictly enforces memory pool usage
std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Renderer::createBuffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties) {

    // This function should only be used for temporary staging buffers during resource creation
    // All persistent resources should use createBufferPooled directly

    if (!memoryPool) {
        throw std::runtime_error("Memory pool not available - cannot create buffer");
    }


    // Only allow direct allocation for staging buffers (temporary, host-visible)
    if (!(properties & vk::MemoryPropertyFlagBits::eHostVisible)) {
        std::cerr << "ERROR: Legacy createBuffer should only be used for staging buffers!" << std::endl;
        throw std::runtime_error("Legacy createBuffer used for non-staging buffer");
    }

    try {
        vk::BufferCreateInfo bufferInfo{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };

        vk::raii::Buffer buffer(device, bufferInfo);

        // Allocate memory directly for staging buffers only
        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

        // Align allocation size to nonCoherentAtomSize (64 bytes) to prevent validation errors
        // VUID-VkMappedMemoryRange-size-01390 requires memory flush sizes to be multiples of nonCoherentAtomSize
        const vk::DeviceSize nonCoherentAtomSize = 64; // Typical value, should query from device properties
        vk::DeviceSize alignedSize = ((memRequirements.size + nonCoherentAtomSize - 1) / nonCoherentAtomSize) * nonCoherentAtomSize;

        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = alignedSize,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
        };

        vk::raii::DeviceMemory bufferMemory(device, allocInfo);

        // Bind memory to buffer
        buffer.bindMemory(*bufferMemory, 0);

        return {std::move(buffer), std::move(bufferMemory)};

    } catch (const std::exception& e) {
        std::cerr << "Failed to create staging buffer: " << e.what() << std::endl;
        throw;
    }
}

void Renderer::createTransparentDescriptorSets() {
    // We need one descriptor set per frame in flight for this resource
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *transparentDescriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts = layouts.data()
    };

    {
        // Serialize allocation vs other descriptor ops
        std::lock_guard<std::mutex> lk(descriptorMutex);
        transparentDescriptorSets = vk::raii::DescriptorSets(device, allocInfo);
    }

    // Update each descriptor set to point to our single off-screen opaque color image
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorImageInfo imageInfo{
            .sampler = *opaqueSceneColorSampler,
            .imageView = *opaqueSceneColorImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet descriptorWrite{
            .dstSet = *transparentDescriptorSets[i],
            .dstBinding = 0, // Binding 0 in Set 1
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        };

        {
            std::lock_guard<std::mutex> lk(descriptorMutex);
            device.updateDescriptorSets(descriptorWrite, nullptr);
        }
    }
}

void Renderer::createTransparentFallbackDescriptorSets() {
    // Allocate one descriptor set per frame in flight using the same layout (single combined image sampler at binding 0)
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *transparentDescriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts = layouts.data()
    };

    {
        std::lock_guard<std::mutex> lk(descriptorMutex);
        transparentFallbackDescriptorSets = vk::raii::DescriptorSets(device, allocInfo);
    }

    // Point each set to the default texture, which is guaranteed to be in SHADER_READ_ONLY_OPTIMAL when used in the opaque pass
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorImageInfo imageInfo{
            .sampler = *defaultTextureResources.textureSampler,
            .imageView = *defaultTextureResources.textureImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet descriptorWrite{
            .dstSet = *transparentFallbackDescriptorSets[i],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        };

        {
            std::lock_guard<std::mutex> lk(descriptorMutex);
            device.updateDescriptorSets(descriptorWrite, nullptr);
        }
    }
}

bool Renderer::createOpaqueSceneColorResources() {
    try {
        // Create the image
        auto [image, allocation] = createImagePooled(
            swapChainExtent.width,
            swapChainExtent.height,
            swapChainImageFormat, // Use the same format as the swapchain
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc, // <-- Note the new usage flags
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        opaqueSceneColorImage = std::move(image);
        // We don't need a member for the allocation, it's managed by the unique_ptr

        // Create the image view
        opaqueSceneColorImageView = createImageView(opaqueSceneColorImage, swapChainImageFormat, vk::ImageAspectFlagBits::eColor);

        // Create the sampler
        vk::SamplerCreateInfo samplerInfo{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .addressModeU = vk::SamplerAddressMode::eClampToEdge,
            .addressModeV = vk::SamplerAddressMode::eClampToEdge,
            .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        };
        opaqueSceneColorSampler = vk::raii::Sampler(device, samplerInfo);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create opaque scene color resources: " << e.what() << std::endl;
        return false;
    }
}

// Copy buffer
void Renderer::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
    ensureThreadLocalVulkanInit();
    try {
        // Create a temporary transient command pool and command buffer to isolate per-thread usage (transfer family)
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamilyIndices.transferFamily.value()
        };
        vk::raii::CommandPool tempPool(device, poolInfo);
        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = *tempPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        vk::raii::CommandBuffers commandBuffers(device, allocInfo);
        vk::raii::CommandBuffer& commandBuffer = commandBuffers[0];

        // Begin command buffer
        vk::CommandBufferBeginInfo beginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };

        commandBuffer.begin(beginInfo);

        // Copy buffer
        vk::BufferCopy copyRegion{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size
        };

        commandBuffer.copyBuffer(*srcBuffer, *dstBuffer, copyRegion);

        // End command buffer
        commandBuffer.end();

        // Submit command buffer
        vk::SubmitInfo submitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffer
        };

        // Use mutex to ensure thread-safe access to transfer queue
        vk::raii::Fence fence(device, vk::FenceCreateInfo{});
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            transferQueue.submit(submitInfo, *fence);
        }
        [[maybe_unused]] auto fenceResult2 = device.waitForFences({*fence}, VK_TRUE, UINT64_MAX);
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy buffer: " << e.what() << std::endl;
        throw;
    }
}

// Create image
std::pair<vk::raii::Image, vk::raii::DeviceMemory> Renderer::createImage(
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties) {
    try {
        // Create image
        vk::ImageCreateInfo imageInfo{
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = {width, height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = tiling,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined
        };

        vk::raii::Image image(device, imageInfo);

        // Allocate memory
        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
        };

        vk::raii::DeviceMemory imageMemory(device, allocInfo);

        // Bind memory to image
        image.bindMemory(*imageMemory, 0);

        return {std::move(image), std::move(imageMemory)};
    } catch (const std::exception& e) {
        std::cerr << "Failed to create image: " << e.what() << std::endl;
        throw;
    }
}

// Create image using memory pool for efficient allocation
std::pair<vk::raii::Image, std::unique_ptr<MemoryPool::Allocation>> Renderer::createImagePooled(
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    uint32_t mipLevels,
    vk::SharingMode sharingMode,
    const std::vector<uint32_t>& queueFamilies) {
    try {
        if (!memoryPool) {
            throw std::runtime_error("Memory pool not initialized");
        }

        // Use memory pool for allocation (mipmap support limited by memory pool API)
        auto [image, allocation] = memoryPool->createImage(width,
                                                          height,
                                                          format,
                                                          tiling,
                                                          usage,
                                                          properties,
                                                          mipLevels,
                                                          sharingMode,
                                                          queueFamilies);

        return {std::move(image), std::move(allocation)};

    } catch (const std::exception& e) {
        std::cerr << "Failed to create image with memory pool: " << e.what() << std::endl;
        throw;
    }
}

// Create an image view
vk::raii::ImageView Renderer::createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
    try {
        ensureThreadLocalVulkanInit();
        // Create image view
        vk::ImageViewCreateInfo viewInfo{
            .image = *image,
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = aspectFlags,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        return { device, viewInfo };
    } catch (const std::exception& e) {
        std::cerr << "Failed to create image view: " << e.what() << std::endl;
        throw;
    }
}

// Transition image layout
void Renderer::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels) {
    ensureThreadLocalVulkanInit();
    try {
        // Create a temporary transient command pool and command buffer to isolate per-thread usage
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
        };
        vk::raii::CommandPool tempPool(device, poolInfo);
        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = *tempPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        vk::raii::CommandBuffers commandBuffers(device, allocInfo);
        vk::raii::CommandBuffer& commandBuffer = commandBuffers[0];

        // Begin command buffer
        vk::CommandBufferBeginInfo beginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };

        commandBuffer.begin(beginInfo);

        // Create an image barrier (Sync2)
        vk::ImageMemoryBarrier2 barrier2{
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = format == vk::Format::eD32Sfloat || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint
                    ? vk::ImageAspectFlagBits::eDepth
                    : vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        // Set stage and access masks based on layouts
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            barrier2.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            barrier2.srcAccessMask = vk::AccessFlagBits2::eNone;
            barrier2.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
            barrier2.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
        } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier2.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
            barrier2.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
            barrier2.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
            barrier2.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            barrier2.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            barrier2.srcAccessMask = vk::AccessFlagBits2::eNone;
            barrier2.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
            barrier2.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilReadOnlyOptimal) {
            // Support for shadow map creation: transition from undefined to read-only depth layout
            barrier2.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            barrier2.srcAccessMask = vk::AccessFlagBits2::eNone;
            barrier2.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
            barrier2.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eGeneral) {
            // Support for compute shader storage images: transition from undefined to general layout
            barrier2.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            barrier2.srcAccessMask = vk::AccessFlagBits2::eNone;
            barrier2.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
            barrier2.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            // Support for textures that skip staging buffer (e.g., preloaded, generated, or default textures)
            // Transition directly from undefined to shader read-only for sampling
            barrier2.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            barrier2.srcAccessMask = vk::AccessFlagBits2::eNone;
            barrier2.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
            barrier2.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        } else {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        // Add a barrier to command buffer (Sync2)
        vk::DependencyInfo depInfo{
            .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier2
        };
        commandBuffer.pipelineBarrier2(depInfo);
        std::cout << "[transitionImageLayout] recorded barrier image=" << (void*)image << " old=" << static_cast<int>(oldLayout) << " new=" << static_cast<int>(newLayout) << std::endl;

        // End command buffer
        commandBuffer.end();

        // CRITICAL FIX: Signal timeline semaphore after layout transition to ensure render loop
        // waits for ALL texture operations (initialization + streaming) before sampling.
        // Without this, textures transitioned via this function (e.g., default textures during init)
        // can be sampled before their layout transitions complete, causing validation errors:
        // "expects SHADER_READ_ONLY_OPTIMAL--instead, current layout is UNDEFINED"
        vk::raii::Fence fence(device, vk::FenceCreateInfo{});
        bool canSignalTimeline = uploadsTimeline != nullptr;
        uint64_t signalValue = 0;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            vk::SubmitInfo submitInfo{};
            if (canSignalTimeline) {
                signalValue = uploadTimelineLastSubmitted.fetch_add(1, std::memory_order_relaxed) + 1;
                vk::TimelineSemaphoreSubmitInfo timelineInfo{
                    .signalSemaphoreValueCount = 1,
                    .pSignalSemaphoreValues = &signalValue
                };
                submitInfo.pNext = &timelineInfo;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &*uploadsTimeline;
            }
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &*commandBuffer;
            graphicsQueue.submit(submitInfo, *fence);
        }
        [[maybe_unused]] auto fenceResult3 = device.waitForFences({*fence}, VK_TRUE, UINT64_MAX);
    } catch (const std::exception& e) {
        std::cerr << "Failed to transition image layout: " << e.what() << std::endl;
        throw;
    }
}

// Copy buffer to image
void Renderer::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, const std::vector<vk::BufferImageCopy>& regions) {
    ensureThreadLocalVulkanInit();
    try {
        // Create a temporary transient command pool for the GRAPHICS queue to avoid cross-queue races
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
        };
        vk::raii::CommandPool tempPool(device, poolInfo);
        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = *tempPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        vk::raii::CommandBuffers commandBuffers(device, allocInfo);
        vk::raii::CommandBuffer& commandBuffer = commandBuffers[0];

        // Begin command buffer
        vk::CommandBufferBeginInfo beginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };

        commandBuffer.begin(beginInfo);

        // Copy buffer to image using provided regions
        commandBuffer.copyBufferToImage(
            buffer,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            regions
        );
        std::cout << "[copyBufferToImage] recorded copy img=" << (void*)image << std::endl;

        // End command buffer
        commandBuffer.end();

        // CRITICAL FIX: Signal timeline semaphore after buffer copy to ensure render loop
        // waits for ALL texture uploads (initialization + streaming) before sampling.
        // Without this, default textures copied via this function during initialization can be
        // sampled before the copy completes, causing validation errors and flickering.
        vk::raii::Fence fence(device, vk::FenceCreateInfo{});
        bool canSignalTimeline = uploadsTimeline != nullptr;
        uint64_t signalValue = 0;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            vk::SubmitInfo submitInfo{};
            if (canSignalTimeline) {
                signalValue = uploadTimelineLastSubmitted.fetch_add(1, std::memory_order_relaxed) + 1;
                vk::TimelineSemaphoreSubmitInfo timelineInfo{
                    .signalSemaphoreValueCount = 1,
                    .pSignalSemaphoreValues = &signalValue
                };
                submitInfo.pNext = &timelineInfo;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &*uploadsTimeline;
            }
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &*commandBuffer;
            graphicsQueue.submit(submitInfo, *fence);
        }
        [[maybe_unused]] auto fenceResult4 = device.waitForFences({*fence}, VK_TRUE, UINT64_MAX);
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy buffer to image: " << e.what() << std::endl;
        throw;
    }
}

// Create or resize light storage buffers to accommodate the given number of lights
bool Renderer::createOrResizeLightStorageBuffers(size_t lightCount) {
    try {
        // Ensure we have storage buffers for each frame in flight
        if (lightStorageBuffers.size() != MAX_FRAMES_IN_FLIGHT) {
            lightStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        }

        // Check if we need to resize buffers
        bool needsResize = false;
        for (auto& buffer : lightStorageBuffers) {
            if (buffer.capacity < lightCount) {
                needsResize = true;
                break;
            }
        }

        if (!needsResize) {
            return true; // Buffers are already large enough
        }

        // Calculate new capacity (with some headroom for growth)
        size_t newCapacity = std::max(lightCount * 2, static_cast<size_t>(64));
        vk::DeviceSize bufferSize = sizeof(LightData) * newCapacity;

        // Wait for device to be idle before destroying old buffers to prevent validation errors.
        // External synchronization required (VVL): serialize against queue submits/present.
        WaitIdle();

        // Create new buffers for each frame
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            auto& buffer = lightStorageBuffers[i];

            // Clean up old buffer if it exists (now safe after waitIdle)
            if (buffer.allocation) {
                buffer.buffer = nullptr;
                buffer.allocation.reset();
                buffer.mapped = nullptr;
            }

            // Create new storage buffer
            auto [newBuffer, newAllocation] = createBufferPooled(
                bufferSize,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );

            // Get the mapped pointer from the allocation
            void* mapped = newAllocation->mappedPtr;

            // Store the new buffer
            buffer.buffer = std::move(newBuffer);
            buffer.allocation = std::move(newAllocation);
            buffer.mapped = mapped;
            buffer.capacity = newCapacity;
            buffer.size = 0;
        }

        // Update all existing descriptor sets to reference the new light storage buffers
        updateAllDescriptorSetsWithNewLightBuffers();

        // Also refresh Forward+ compute descriptor sets (binding 0) so compute reads valid buffers
        try {
            if (!forwardPlusPerFrame.empty()) {
                for (size_t i = 0; i < forwardPlusPerFrame.size() && i < lightStorageBuffers.size(); ++i) {
                    if (forwardPlusPerFrame[i].computeSet == nullptr) continue;
                    if (lightStorageBuffers[i].buffer == nullptr) continue;
                    vk::DescriptorBufferInfo lightsInfo{ .buffer = *lightStorageBuffers[i].buffer, .offset = 0, .range = VK_WHOLE_SIZE };
                    vk::WriteDescriptorSet write{
                        .dstSet = *forwardPlusPerFrame[i].computeSet,
                        .dstBinding = 0,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = vk::DescriptorType::eStorageBuffer,
                        .pBufferInfo = &lightsInfo
                    };
                    {
                        std::lock_guard<std::mutex> lk(descriptorMutex);
                        device.updateDescriptorSets(write, {});
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to update Forward+ compute descriptors after light buffer resize: " << e.what() << std::endl;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create or resize light storage buffers: " << e.what() << std::endl;
        return false;
    }
}

// Update all existing descriptor sets with new light storage buffer references
void Renderer::updateAllDescriptorSetsWithNewLightBuffers(bool allFrames) {
    try {
        if (!descriptorSetsValid.load(std::memory_order_relaxed)) return;
        if (isRecordingCmd.load(std::memory_order_relaxed)) return;
        // Iterate through all entity resources and update their PBR descriptor sets
        for (auto& kv : entityResources) {
            auto& resources = kv.second;
            // Only update PBR descriptor sets (they have light buffer bindings)
            if (!resources.pbrDescriptorSets.empty()) {
                size_t beginFrame = allFrames ? 0 : static_cast<size_t>(currentFrame);
                size_t endFrame   = allFrames ? resources.pbrDescriptorSets.size() : (beginFrame + 1);
                for (size_t i = beginFrame; i < endFrame && i < resources.pbrDescriptorSets.size() && i < lightStorageBuffers.size(); ++i) {
                    // Skip if this set looks invalid/uninitialized
                    if (!(*resources.pbrDescriptorSets[i])) continue;
                    if (i < lightStorageBuffers.size() && *lightStorageBuffers[i].buffer) {
                        // Create descriptor write for light storage buffer (binding 7)
                        vk::DescriptorBufferInfo lightBufferInfo{
                            .buffer = *lightStorageBuffers[i].buffer,
                            .offset = 0,
                            .range = VK_WHOLE_SIZE
                        };

                        vk::WriteDescriptorSet descriptorWrite{
                            .dstSet = *resources.pbrDescriptorSets[i],
                            .dstBinding = 6,
                            .dstArrayElement = 0,
                            .descriptorCount = 1,
                            .descriptorType = vk::DescriptorType::eStorageBuffer,
                            .pBufferInfo = &lightBufferInfo
                        };

                        // Update the descriptor set
                        {
                            std::lock_guard<std::mutex> lk(descriptorMutex);
                            device.updateDescriptorSets(descriptorWrite, {});
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to update descriptor sets with new light buffers: " << e.what() << std::endl;
    }
}

// Refresh only current frame's PBR descriptor bindings used by Forward+
// Safe to call after waiting on inFlightFences[currentFrame] and before command recording.
void Renderer::refreshPBRForwardPlusBindingsForFrame(uint32_t frameIndex) {
    try {
        if (frameIndex >= MAX_FRAMES_IN_FLIGHT) return;
        if (!descriptorSetsValid.load(std::memory_order_relaxed)) return;
        if (isRecordingCmd.load(std::memory_order_relaxed)) return;

        // Resolve current frame Forward+ buffers
        vk::Buffer headersBuf{};
        vk::Buffer indicesBuf{};
        if (frameIndex < forwardPlusPerFrame.size()) {
            auto &f = forwardPlusPerFrame[frameIndex];
            if (!(f.tileHeaders == nullptr)) headersBuf = *f.tileHeaders;
            if (!(f.tileLightIndices == nullptr)) indicesBuf = *f.tileLightIndices;
        }

        // Resolve current frame lights buffer
        vk::Buffer lightsBuf{};
        if (frameIndex < lightStorageBuffers.size() && !(lightStorageBuffers[frameIndex].buffer == nullptr)) {
            lightsBuf = *lightStorageBuffers[frameIndex].buffer;
        }

        // CRITICAL FIX: PBR descriptor set layout ALWAYS declares bindings 6/7/8 (see renderer_pipelines.cpp lines 112-135),
        // so these bindings MUST be valid even when Forward+ is disabled. If real Forward+ buffers don't exist,
        // we must ensure minimal dummy buffers are bound to satisfy the descriptor set layout requirements.
        // Without valid bindings, Vulkan validation reports: "descriptor [binding X] is being used in draw but has never been updated"
        
        // Ensure lights buffer exists (binding 6) - create minimal dummy if needed
        if (!lightsBuf) {
            // Lazily create a minimal lights buffer (single LightData element) for use when Forward+ is disabled
            if (lightStorageBuffers.empty()) {
                lightStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
            }
            if (frameIndex < lightStorageBuffers.size() && lightStorageBuffers[frameIndex].buffer == nullptr) {
                vk::DeviceSize minSize = sizeof(LightData); // Single light element
                auto [buf, alloc] = createBufferPooled(minSize,
                    vk::BufferUsageFlagBits::eStorageBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                lightStorageBuffers[frameIndex].buffer = std::move(buf);
                lightStorageBuffers[frameIndex].allocation = std::move(alloc);
                lightStorageBuffers[frameIndex].mapped = lightStorageBuffers[frameIndex].allocation->mappedPtr;
                lightStorageBuffers[frameIndex].capacity = 1;
                lightStorageBuffers[frameIndex].size = 0;
                // Zero-initialize to prevent garbage data
                if (lightStorageBuffers[frameIndex].mapped) {
                    std::memset(lightStorageBuffers[frameIndex].mapped, 0, minSize);
                }
            }
            if (frameIndex < lightStorageBuffers.size() && !(lightStorageBuffers[frameIndex].buffer == nullptr)) {
                lightsBuf = *lightStorageBuffers[frameIndex].buffer;
            }
        }

        // Ensure tile headers buffer exists (binding 7) - create minimal dummy if needed
        if (!headersBuf) {
            if (forwardPlusPerFrame.empty()) {
                forwardPlusPerFrame.resize(MAX_FRAMES_IN_FLIGHT);
            }
            if (frameIndex < forwardPlusPerFrame.size()) {
                auto &f = forwardPlusPerFrame[frameIndex];
                if (f.tileHeaders == nullptr) {
                    vk::DeviceSize minSize = sizeof(uint32_t) * 4; // Single TileHeader {offset, count, pad0, pad1}
                    auto [buf, alloc] = createBufferPooled(minSize,
                        vk::BufferUsageFlagBits::eStorageBuffer,
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                    f.tileHeaders = std::move(buf);
                    f.tileHeadersAlloc = std::move(alloc);
                    if (f.tileHeadersAlloc && f.tileHeadersAlloc->mappedPtr) {
                        std::memset(f.tileHeadersAlloc->mappedPtr, 0, minSize);
                    }
                }
                if (!(f.tileHeaders == nullptr)) headersBuf = *f.tileHeaders;
            }
        }

        // Ensure tile light indices buffer exists (binding 8) - create minimal dummy if needed
        if (!indicesBuf) {
            if (forwardPlusPerFrame.empty()) {
                forwardPlusPerFrame.resize(MAX_FRAMES_IN_FLIGHT);
            }
            if (frameIndex < forwardPlusPerFrame.size()) {
                auto &f = forwardPlusPerFrame[frameIndex];
                if (f.tileLightIndices == nullptr) {
                    vk::DeviceSize minSize = sizeof(uint32_t) * 4; // Minimal array of 4 uints
                    auto [buf, alloc] = createBufferPooled(minSize,
                        vk::BufferUsageFlagBits::eStorageBuffer,
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                    f.tileLightIndices = std::move(buf);
                    f.tileLightIndicesAlloc = std::move(alloc);
                    if (f.tileLightIndicesAlloc && f.tileLightIndicesAlloc->mappedPtr) {
                        std::memset(f.tileLightIndicesAlloc->mappedPtr, 0, minSize);
                    }
                }
                if (!(f.tileLightIndices == nullptr)) indicesBuf = *f.tileLightIndices;
            }
        }

        std::vector<vk::WriteDescriptorSet> writes;
        vk::DescriptorBufferInfo lightsInfo{};
        vk::DescriptorBufferInfo headersInfo{};
        vk::DescriptorBufferInfo indicesInfo{};
        vk::DescriptorBufferInfo fragDbgInfo{};

        // At this point, all three critical buffers (lights, headers, indices) should exist (real or dummy)
        if (lightsBuf) {
            lightsInfo = vk::DescriptorBufferInfo{ .buffer = lightsBuf, .offset = 0, .range = VK_WHOLE_SIZE };
        }
        // Current frame fragment debug buffer (reuse compute debugOut) - this one is optional
        if (frameIndex < forwardPlusPerFrame.size()) {
            auto &fpf = forwardPlusPerFrame[frameIndex];
            if (!(fpf.debugOut == nullptr)) {
                fragDbgInfo = vk::DescriptorBufferInfo{ .buffer = *fpf.debugOut, .offset = 0, .range = VK_WHOLE_SIZE };
            }
        }
        if (headersBuf) {
            headersInfo = vk::DescriptorBufferInfo{ .buffer = headersBuf, .offset = 0, .range = VK_WHOLE_SIZE };
        }
        if (indicesBuf) {
            indicesInfo = vk::DescriptorBufferInfo{ .buffer = indicesBuf, .offset = 0, .range = VK_WHOLE_SIZE };
        }

        // Binding 10: reflection sampler — always bind fallback texture while reflection pass is disabled
        // The reflection rendering pass is currently disabled (commented out in renderer_rendering.cpp
        // lines 1194-1203), so we must not bind any reflection RTs that may exist but contain stale data.
        // When reflection rendering is re-enabled, restore the conditional logic to bind previous frame's RT.
        vk::DescriptorImageInfo reflInfo{};
        reflInfo = vk::DescriptorImageInfo{ .sampler = *defaultTextureResources.textureSampler, .imageView = *defaultTextureResources.textureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };

        for (auto &kv : entityResources) {
            auto &res = kv.second;
            if (res.pbrDescriptorSets.empty() || frameIndex >= res.pbrDescriptorSets.size()) continue;
            
            // CRITICAL: Validate descriptor set handle before using it
            // This prevents "Invalid VkDescriptorSet Object" errors when sets have been freed/invalidated
            if (!(*res.pbrDescriptorSets[frameIndex])) {
                std::cerr << "Warning: Invalid descriptor set handle for entity at frame " << frameIndex << ", skipping" << std::endl;
                continue;
            }

            // Binding 6: lights SSBO - ALWAYS bind (required by layout)
            if (lightsBuf) {
                writes.push_back(vk::WriteDescriptorSet{ .dstSet = *res.pbrDescriptorSets[frameIndex], .dstBinding = 6, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &lightsInfo });
            }
            // Binding 7: tile headers - ALWAYS bind (required by layout)
            if (headersBuf) {
                writes.push_back(vk::WriteDescriptorSet{ .dstSet = *res.pbrDescriptorSets[frameIndex], .dstBinding = 7, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &headersInfo });
            }
            // Binding 8: tile indices - ALWAYS bind (required by layout)
            if (indicesBuf) {
                writes.push_back(vk::WriteDescriptorSet{ .dstSet = *res.pbrDescriptorSets[frameIndex], .dstBinding = 8, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &indicesInfo });
            }
            // Binding 9: fragment debug output buffer (optional - only bind if exists)
            if (fragDbgInfo.buffer) {
                writes.push_back(vk::WriteDescriptorSet{ .dstSet = *res.pbrDescriptorSets[frameIndex], .dstBinding = 9, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &fragDbgInfo });
            }
            // Binding 10: reflection sampler - ALWAYS bind (required by layout)
            writes.push_back(vk::WriteDescriptorSet{ .dstSet = *res.pbrDescriptorSets[frameIndex], .dstBinding = 10, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &reflInfo });
        }

        if (!writes.empty()) {
            std::lock_guard<std::mutex> lk(descriptorMutex);
            device.updateDescriptorSets(writes, {});
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to refresh PBR Forward+ bindings for frame " << frameIndex << ": " << e.what() << std::endl;
    }
}

// Update the light storage buffer with current light data
bool Renderer::updateLightStorageBuffer(uint32_t frameIndex, const std::vector<ExtractedLight>& lights) {
    try {
        // Ensure buffers are large enough and properly initialized
        if (!createOrResizeLightStorageBuffers(lights.size())) {
            return false;
        }

        // Now check frame index after buffers are properly initialized
        if (frameIndex >= lightStorageBuffers.size()) {
            std::cerr << "Invalid frame index for light storage buffer update: " << frameIndex
                      << " >= " << lightStorageBuffers.size() << std::endl;
            return false;
        }

        auto& buffer = lightStorageBuffers[frameIndex];
        if (!buffer.mapped) {
            std::cerr << "Light storage buffer not mapped" << std::endl;
            return false;
        }

        // Convert ExtractedLight data to LightData format
        auto* lightData = static_cast<LightData*>(buffer.mapped);
        for (size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];

            // For directional lights, store direction in position field (they don't need position)
            // For other lights, store position
            if (light.type == ExtractedLight::Type::Directional) {
                lightData[i].position = glm::vec4(light.direction, 0.0f); // w=0 indicates direction
            } else {
                lightData[i].position = glm::vec4(light.position, 1.0f); // w=1 indicates position
            }

            lightData[i].color = glm::vec4(light.color * light.intensity, 1.0f);

            // Calculate light space matrix for shadow mapping
            glm::mat4 lightProjection, lightView;
            if (light.type == ExtractedLight::Type::Directional) {
                float orthoSize = 50.0f;
                lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
                lightView = glm::lookAt(light.position, light.position + light.direction, glm::vec3(0.0f, 1.0f, 0.0f));
            } else {
                lightProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light.range);
                lightView = glm::lookAt(light.position, light.position + light.direction, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            lightData[i].lightSpaceMatrix = lightProjection * lightView;

            // Set light type
            switch (light.type) {
                case ExtractedLight::Type::Point:
                    lightData[i].lightType = 0;
                    break;
                case ExtractedLight::Type::Directional:
                    lightData[i].lightType = 1;
                    break;
                case ExtractedLight::Type::Spot:
                    lightData[i].lightType = 2;
                    break;
                case ExtractedLight::Type::Emissive:
                    lightData[i].lightType = 3;
                    break;
            }

            // Set other light properties
            lightData[i].range = light.range;
            lightData[i].innerConeAngle = light.innerConeAngle;
            lightData[i].outerConeAngle = light.outerConeAngle;
        }

        // Update buffer size
        buffer.size = lights.size();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to update light storage buffer: " << e.what() << std::endl;
        return false;
    }
}


// Asynchronous texture loading implementations using ThreadPool
std::future<bool> Renderer::LoadTextureAsync(const std::string& texturePath, bool critical) {
    if (texturePath.empty()) {
        return std::async(std::launch::deferred, [] { return false; });
    }
    // Schedule a CPU-light job that enqueues a pending GPU upload to be
    // processed later on the main thread. This avoids submitting Vulkan
    // command buffers from worker threads, which can confuse GPU-assisted
    // validation.
    textureTasksScheduled.fetch_add(1, std::memory_order_relaxed);
    uploadJobsTotal.fetch_add(1, std::memory_order_relaxed);
    auto task = [this, texturePath, critical]() {
        PendingTextureJob job;
        job.type = PendingTextureJob::Type::FromFile;
        job.priority = critical ? PendingTextureJob::Priority::Critical
                                : PendingTextureJob::Priority::NonCritical;
        job.idOrPath = texturePath;
        {
            std::lock_guard<std::mutex> lk(pendingTextureJobsMutex);
            pendingTextureJobs.emplace_back(std::move(job));
        }
        pendingTextureCv.notify_one();
        if (critical) {
            criticalJobsOutstanding.fetch_add(1, std::memory_order_relaxed);
        }
        textureTasksCompleted.fetch_add(1, std::memory_order_relaxed);
        return true;
    };

    std::shared_lock<std::shared_mutex> lock(threadPoolMutex);
    if (!threadPool) {
        return std::async(std::launch::async, task);
    }
    return threadPool->enqueue(task);
}

std::future<bool> Renderer::LoadTextureFromMemoryAsync(const std::string& textureId, const unsigned char* imageData,
                              int width, int height, int channels, bool critical) {
    if (!imageData || textureId.empty() || width <= 0 || height <= 0 || channels <= 0) {
        return std::async(std::launch::deferred, [] { return false; });
    }
    // Copy the source bytes so the caller can free/modify their buffer immediately
    size_t srcSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
    std::vector<unsigned char> dataCopy(srcSize);
    std::memcpy(dataCopy.data(), imageData, srcSize);

    textureTasksScheduled.fetch_add(1, std::memory_order_relaxed);
    uploadJobsTotal.fetch_add(1, std::memory_order_relaxed);
    auto task = [this, textureId, data = std::move(dataCopy), width, height, channels, critical]() mutable {
        PendingTextureJob job;
        job.type = PendingTextureJob::Type::FromMemory;
        job.priority = critical ? PendingTextureJob::Priority::Critical
                                : PendingTextureJob::Priority::NonCritical;
        job.idOrPath = textureId;
        job.data = std::move(data);
        job.width = width;
        job.height = height;
        job.channels = channels;
        {
            std::lock_guard<std::mutex> lk(pendingTextureJobsMutex);
            pendingTextureJobs.emplace_back(std::move(job));
        }
        pendingTextureCv.notify_one();
        if (critical) {
            criticalJobsOutstanding.fetch_add(1, std::memory_order_relaxed);
        }
        textureTasksCompleted.fetch_add(1, std::memory_order_relaxed);
        return true;
    };

    std::shared_lock<std::shared_mutex> lock(threadPoolMutex);
    if (!threadPool) {
        return std::async(std::launch::async, std::move(task));
    }
    return threadPool->enqueue(std::move(task));
}

void Renderer::WaitForAllTextureTasks() {
    // Simple blocking wait: spin until all scheduled texture tasks have completed.
    // This is only intended for use during initial scene loading where a short
    // stall is acceptable to ensure descriptor sets see all real textures.
    for (;;) {
        uint32_t scheduled = textureTasksScheduled.load(std::memory_order_relaxed);
        uint32_t completed = textureTasksCompleted.load(std::memory_order_relaxed);
        if (scheduled == 0 || completed >= scheduled) {
            break;
        }
        // Sleep briefly to yield CPU while background texture jobs finish
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Start background worker threads that drain pending texture jobs and perform GPU uploads
void Renderer::StartUploadsWorker(size_t workerCount) {
    stopUploadsWorker.store(false, std::memory_order_relaxed);
    if (workerCount == 0) {
        unsigned int hw = std::thread::hardware_concurrency();
        // Heuristic: at least 2 workers, at most 4, and not exceeding half of HW threads
        unsigned int target = std::max(2u, std::min(4u, hw > 0 ? hw / 2 : 2u));
        workerCount = static_cast<size_t>(target);
    }
    uploadsWorkerThreads.reserve(workerCount);
    for (size_t t = 0; t < workerCount; ++t) {
        uploadsWorkerThreads.emplace_back([this]() {
            ensureThreadLocalVulkanInit();
            while (!stopUploadsWorker.load(std::memory_order_relaxed)) {
                // Wait for work or stop signal
                {
                    std::unique_lock<std::mutex> lk(pendingTextureJobsMutex);
                    pendingTextureCv.wait(lk, [this]() {
                        return stopUploadsWorker.load(std::memory_order_relaxed) || !pendingTextureJobs.empty();
                    });
                }
                if (stopUploadsWorker.load(std::memory_order_relaxed)) break;

                // Drain a batch of jobs
                std::vector<PendingTextureJob> batch;
                {
                    std::lock_guard<std::mutex> lk(pendingTextureJobsMutex);
                    const size_t maxBatch = 16; // simple batch size to limit command overhead
                    const size_t take = std::min(maxBatch, pendingTextureJobs.size());
                    batch.reserve(take);
                    for (size_t i = 0; i < take; ++i) {
                        batch.emplace_back(std::move(pendingTextureJobs.back()));
                        pendingTextureJobs.pop_back();
                    }
                }

            // Process critical jobs first
            auto isCritical = [](const PendingTextureJob& j) { return j.priority == PendingTextureJob::Priority::Critical; };
            std::stable_sort(batch.begin(), batch.end(), [&](const PendingTextureJob& a, const PendingTextureJob& b){
                return isCritical(a) && !isCritical(b);
            });

            // Try to batch FromMemory jobs together for a single transfer submit
            std::vector<PendingTextureJob> memJobs;
            for (auto& j : batch) if (j.type == PendingTextureJob::Type::FromMemory) memJobs.push_back(std::move(j));
            // Remove moved jobs from batch
            batch.erase(std::remove_if(batch.begin(), batch.end(), [](const PendingTextureJob& j){ return j.type == PendingTextureJob::Type::FromMemory; }), batch.end());

            if (!memJobs.empty()) {
                try {
                    // Process batched memory uploads with a single submit
                    // Fallback to per-job if batching fails for any reason
                    auto processSingle = [&](const PendingTextureJob& job){
                        (void)LoadTextureFromMemory(job.idOrPath,
                                                    job.data.data(),
                                                    job.width,
                                                    job.height,
                                                    job.channels);
                        OnTextureUploaded(job.idOrPath);
                        if (job.priority == PendingTextureJob::Priority::Critical) {
                            criticalJobsOutstanding.fetch_sub(1, std::memory_order_relaxed);
                        }
                        uploadJobsCompleted.fetch_add(1, std::memory_order_relaxed);
                    };

                    // Build staging buffers and images without submitting yet
                    struct Item { std::string id; vk::raii::Buffer staging; std::unique_ptr<MemoryPool::Allocation> stagingAlloc; std::vector<uint8_t> tmp; uint32_t w, h; vk::Format format; std::vector<vk::BufferImageCopy> regions; uint32_t mipLevels; vk::raii::Image image; std::unique_ptr<MemoryPool::Allocation> imageAlloc; };
                    std::vector<Item> items;
                    items.reserve(memJobs.size());

                    for (auto& job : memJobs) {
                        try {
                            // Create staging buffer and copy data
                            const vk::DeviceSize imgSize = static_cast<vk::DeviceSize>(job.width * job.height * 4);
                            auto [stagingBuf, stagingAlloc] = createBufferPooled(imgSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                            void* mapped = stagingAlloc->mappedPtr;
                            // Convert to RGBA if not already
                            std::vector<uint8_t> rgba;
                            rgba.resize(static_cast<size_t>(imgSize));
                            const uint8_t* src = job.data.data();
                            if (job.channels == 4) {
                                std::memcpy(rgba.data(), src, static_cast<size_t>(imgSize));
                            } else if (job.channels == 3) {
                                for (int y = 0; y < job.height; ++y) {
                                    for (int x = 0; x < job.width; ++x) {
                                        size_t si = (y * job.width + x) * 3;
                                        size_t di = (y * job.width + x) * 4;
                                        rgba[di+0] = src[si+0];
                                        rgba[di+1] = src[si+1];
                                        rgba[di+2] = src[si+2];
                                        rgba[di+3] = 255;
                                    }
                                }
                            } else if (job.channels == 1) {
                                for (int i = 0, n = job.width*job.height; i < n; ++i) {
                                    uint8_t v = src[i];
                                    size_t di = i*4;
                                    rgba[di+0] = v; rgba[di+1] = v; rgba[di+2] = v; rgba[di+3] = 255;
                                }
                            } else {
                                // unsupported layout, fallback to single path which will handle it
                                processSingle(job);
                                continue;
                            }
                            std::memcpy(mapped, rgba.data(), static_cast<size_t>(imgSize));
                            // Persistent mapping via memory pool; no explicit unmap needed here

                            // Create image (concurrent sharing if needed)
                            bool differentFamilies = queueFamilyIndices.graphicsFamily.value() != queueFamilyIndices.transferFamily.value();
                            std::vector<uint32_t> families;
                            if (differentFamilies) families = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.transferFamily.value() };
                            vk::Format texFormat = determineTextureFormat(job.idOrPath);
                            auto [image, imageAlloc] = createImagePooled(job.width, job.height, texFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, 1, differentFamilies ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive, families);

                            // Prepare one region
                            std::vector<vk::BufferImageCopy> regions{vk::BufferImageCopy{
                                .bufferOffset = 0,
                                .bufferRowLength = 0,
                                .bufferImageHeight = 0,
                                .imageSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
                                .imageOffset = {0,0,0},
                                .imageExtent = { static_cast<uint32_t>(job.width), static_cast<uint32_t>(job.height), 1 }
                            }};

                            items.push_back(Item{ job.idOrPath, std::move(stagingBuf), std::move(stagingAlloc), std::move(rgba), static_cast<uint32_t>(job.width), static_cast<uint32_t>(job.height), texFormat, std::move(regions), 1, std::move(image), std::move(imageAlloc) });
                        } catch (const std::exception& e) {
                            std::cerr << "Batch prepare failed for '" << job.idOrPath << "': " << e.what() << ". Falling back to single." << std::endl;
                            processSingle(job);
                        }
                    }

                    if (!items.empty()) {
                        // Record a single command buffer for all items
                        vk::CommandPoolCreateInfo poolInfo{ .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = queueFamilyIndices.transferFamily.value() };
                        vk::raii::CommandPool tempPool(device, poolInfo);
                        vk::CommandBufferAllocateInfo allocInfo{ .commandPool = *tempPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
                        vk::raii::CommandBuffers cbs(device, allocInfo);
                        vk::raii::CommandBuffer& cb = cbs[0];
                        cb.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

                        for (auto& it : items) {
                            // Transition undefined->transfer dst (Sync2)
                            vk::ImageMemoryBarrier2 toDst2{
                                .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
                                .srcAccessMask = vk::AccessFlagBits2::eNone,
                                .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
                                .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
                                .oldLayout = vk::ImageLayout::eUndefined,
                                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .image = *it.image,
                                .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
                            };
                            vk::DependencyInfo depToDst{ .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &toDst2 };
                            cb.pipelineBarrier2(depToDst);

                            cb.copyBufferToImage(*it.staging, *it.image, vk::ImageLayout::eTransferDstOptimal, it.regions);

                            // Transition to shader-read (Sync2)
                            vk::ImageMemoryBarrier2 toShader2{
                                .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                                .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                                .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
                                .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
                                .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                                .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .image = *it.image,
                                .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
                            };
                            vk::DependencyInfo depToShader{ .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &toShader2 };
                            cb.pipelineBarrier2(depToShader);
                        }

                        cb.end();

                        vk::raii::Fence fence(device, vk::FenceCreateInfo{});
                        uint64_t signalValue = 0;
                        bool canSignal = uploadsTimeline != nullptr;
                        {
                            std::lock_guard<std::mutex> lock(queueMutex);
                            vk::SubmitInfo submit{};
                            if (canSignal) {
                                signalValue = uploadTimelineLastSubmitted.fetch_add(1, std::memory_order_relaxed) + 1;
                                vk::TimelineSemaphoreSubmitInfo timelineInfo{ .signalSemaphoreValueCount = 1, .pSignalSemaphoreValues = &signalValue };
                                submit.pNext = &timelineInfo;
                                submit.signalSemaphoreCount = 1;
                                submit.pSignalSemaphores = &*uploadsTimeline;
                            }
                            submit.commandBufferCount = 1;
                            submit.pCommandBuffers = &*cb;
                            transferQueue.submit(submit, *fence);
                        }
                        device.waitForFences({*fence}, VK_TRUE, UINT64_MAX);

                        // Perf accounting for the batch
                        uint64_t batchBytes = 0;
                        for (auto& it : items) batchBytes += static_cast<uint64_t>(it.w) * it.h * 4ull;
                        bytesUploadedTotal.fetch_add(batchBytes, std::memory_order_relaxed);
                        uploadCount.fetch_add(static_cast<uint32_t>(items.size()), std::memory_order_relaxed);

                        // Finalize resources and notify
                        for (auto& it : items) {
                            // Store in textureResources
                            TextureResources res; res.textureImage = std::move(it.image); res.textureImageAllocation = std::move(it.imageAlloc); res.format = it.format; res.mipLevels = it.mipLevels; res.alphaMaskedHint = false; // heuristic omitted in batch
                            // Create sampler/view
                            createTextureSampler(res);
                            res.textureImageView = createImageView(res.textureImage, res.format, vk::ImageAspectFlagBits::eColor, res.mipLevels);
                            {
                                std::unique_lock<std::shared_mutex> lk(textureResourcesMutex);
                                textureResources[it.id] = std::move(res);
                            }
                            OnTextureUploaded(it.id);
                            // Update counters
                            uploadJobsCompleted.fetch_add(1, std::memory_order_relaxed);
                        }
                        // Decrement outstanding critical jobs if any
                        for (auto& job : memJobs) if (job.priority == PendingTextureJob::Priority::Critical) criticalJobsOutstanding.fetch_sub(1, std::memory_order_relaxed);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "UploadsWorker: batch processing failed: " << e.what() << std::endl;
                    // Fallback: per-job processing
                    for (auto& job : memJobs) {
                        try {
                            (void)LoadTextureFromMemory(job.idOrPath,
                                                        job.data.data(),
                                                        job.width,
                                                        job.height,
                                                        job.channels);
                            OnTextureUploaded(job.idOrPath);
                            if (job.priority == PendingTextureJob::Priority::Critical) {
                                criticalJobsOutstanding.fetch_sub(1, std::memory_order_relaxed);
                            }
                            uploadJobsCompleted.fetch_add(1, std::memory_order_relaxed);
                        } catch (...) {}
                    }
                }
            }

            // Process remaining non-memory jobs individually
            for (auto& job : batch) {
                try {
                    if (job.type == PendingTextureJob::Type::FromFile) {
                        (void)LoadTexture(job.idOrPath);
                        OnTextureUploaded(job.idOrPath);
                        if (job.priority == PendingTextureJob::Priority::Critical) {
                            criticalJobsOutstanding.fetch_sub(1, std::memory_order_relaxed);
                        }
                        uploadJobsCompleted.fetch_add(1, std::memory_order_relaxed);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "UploadsWorker: failed to process job for '" << job.idOrPath << "': " << e.what() << std::endl;
                }
            }
            }
        });
    }
}

void Renderer::StopUploadsWorker() {
    stopUploadsWorker.store(true, std::memory_order_relaxed);
    pendingTextureCv.notify_all();
    for (auto &th : uploadsWorkerThreads) {
        if (th.joinable()) th.join();
    }
    uploadsWorkerThreads.clear();
}

void Renderer::RegisterTextureUser(const std::string& textureId, Entity* entity) {
    if (textureId.empty() || !entity) return;

    // Always register under the canonical resolved ID so that lookups from
    // descriptor creation and upload completion (which also use
    // ResolveTextureId) are consistent.
    std::string canonicalId = ResolveTextureId(textureId);
    if (canonicalId.empty()) {
        canonicalId = textureId;
    }

    std::lock_guard<std::mutex> lk(textureUsersMutex);
    textureToEntities[canonicalId].push_back(entity);
}

void Renderer::OnTextureUploaded(const std::string& textureId) {
    // Resolve alias to canonical ID used for tracking and descriptor
    // creation. RegisterTextureUser also stores under this canonical ID.
    std::string canonicalId = ResolveTextureId(textureId);
    if (canonicalId.empty()) {
        canonicalId = textureId;
    }

    std::vector<Entity*> users;
    {
        std::lock_guard<std::mutex> lk(textureUsersMutex);
        auto it = textureToEntities.find(canonicalId);
        if (it == textureToEntities.end()) {
            return;
        }
        users = it->second;
    }

    // Always defer descriptor updates to the safe point at the start of Render()
    // (after the in-flight fence for the current frame has been signaled).
    // This avoids UPDATE_AFTER_BIND violations and mid-recording invalidation.
    // If descriptor indexing / UPDATE_AFTER_BIND is enabled, we still prefer
    // this safer path for consistency across devices.
    for (Entity* entity : users) {
        if (!entity) continue;
        MarkEntityDescriptorsDirty(entity);
    }
}

void Renderer::MarkEntityDescriptorsDirty(Entity* entity) {
    if (!entity) return;
    std::lock_guard<std::mutex> lk(dirtyEntitiesMutex);
    descriptorDirtyEntities.insert(entity);
}

bool Renderer::updateDescriptorSetsForFrame(Entity* entity,
                                            const std::string& texturePath,
                                            bool usePBR,
                                            uint32_t frameIndex,
                                            bool imagesOnly,
                                            bool uboOnly) {
    if (!entity) return false;
    if (!descriptorSetsValid.load(std::memory_order_relaxed)) {
        // Descriptor sets are being recreated; skip updates for now
        return false;
    }
    // Defer descriptor writes if the command buffer is currently being recorded.
    if (isRecordingCmd.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> qlk(pendingDescMutex);
        pendingDescOps.push_back(PendingDescOp{entity, texturePath, usePBR, frameIndex, imagesOnly});
        descriptorRefreshPending.store(true, std::memory_order_relaxed);
        return true;
    }
    std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
    auto entityIt = entityResources.find(entity);
    if (entityIt == entityResources.end()) return false;

    // Ensure we have a valid UBO for this frame before attempting descriptor writes
    if (frameIndex >= entityIt->second.uniformBuffers.size() ||
        frameIndex >= entityIt->second.uniformBuffersMapped.size() ||
        *entityIt->second.uniformBuffers[frameIndex] == vk::Buffer{}) {
        // Missing UBO for this frame; skip to avoid writing invalid descriptors
        return false;
    }

    vk::DescriptorSetLayout selectedLayout = usePBR ? *pbrDescriptorSetLayout : *descriptorSetLayout;
    // Ensure descriptor sets exist for this entity
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, selectedLayout);
    vk::DescriptorSetAllocateInfo allocInfo{ .descriptorPool = *descriptorPool, .descriptorSetCount = MAX_FRAMES_IN_FLIGHT, .pSetLayouts = layouts.data() };
    auto& targetDescriptorSets = usePBR ? entityIt->second.pbrDescriptorSets : entityIt->second.basicDescriptorSets;
    bool newlyAllocated = false;
    if (targetDescriptorSets.empty()) {
        std::lock_guard<std::mutex> lk(descriptorMutex);
        targetDescriptorSets = vk::raii::DescriptorSets(device, allocInfo);
        newlyAllocated = true;
    }
    if (frameIndex >= targetDescriptorSets.size()) return false;

    vk::DescriptorBufferInfo bufferInfo{ .buffer = *entityIt->second.uniformBuffers[frameIndex], .range = sizeof(UniformBufferObject) };

    // Ensure uboBindingWritten vector is sized
    if (entityIt->second.uboBindingWritten.size() != MAX_FRAMES_IN_FLIGHT) {
        entityIt->second.uboBindingWritten.assign(MAX_FRAMES_IN_FLIGHT, false);
    }

    if (usePBR) {
        // We'll fill descriptor writes. Binding 0 (UBO) is written only when explicitly requested (uboOnly)
        // or when doing a full update (imagesOnly == false). For imagesOnly updates we must NOT touch UBO
        // to avoid update-after-bind hazards.
        std::vector<vk::WriteDescriptorSet> writes;
        std::array<vk::DescriptorImageInfo, 5> imageInfos;
        // Optionally write only the UBO (binding 0) — used at safe point to initialize per-frame sets once
        if (uboOnly) {
            // Avoid re-writing if we already initialized this frame's UBO binding
            if (!entityIt->second.uboBindingWritten[frameIndex]) {
                writes.push_back({ .dstSet = *targetDescriptorSets[frameIndex], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &bufferInfo });
                {
                    std::lock_guard<std::mutex> lk(descriptorMutex);
                    device.updateDescriptorSets(writes, {});
                }
                entityIt->second.uboBindingWritten[frameIndex] = true;
            }
            return true;
        }

        // For full updates (imagesOnly == false), include UBO write; for imagesOnly, skip it
        if (!imagesOnly) {
            writes.push_back({ .dstSet = *targetDescriptorSets[frameIndex], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &bufferInfo });
        }

        auto meshComponent = entity->GetComponent<MeshComponent>();
        // Determine PBR texture paths in the same manner as createDescriptorSets
        std::string legacyPath = (meshComponent ? meshComponent->GetTexturePath() : std::string());
        const std::string baseColorPath = (meshComponent && !meshComponent->GetBaseColorTexturePath().empty())
                                          ? meshComponent->GetBaseColorTexturePath()
                                          : (!legacyPath.empty() ? legacyPath : SHARED_DEFAULT_ALBEDO_ID);
        const std::string mrPath = (meshComponent && !meshComponent->GetMetallicRoughnessTexturePath().empty())
                                   ? meshComponent->GetMetallicRoughnessTexturePath()
                                   : SHARED_DEFAULT_METALLIC_ROUGHNESS_ID;
        const std::string normalPath = (meshComponent && !meshComponent->GetNormalTexturePath().empty())
                                       ? meshComponent->GetNormalTexturePath()
                                       : SHARED_DEFAULT_NORMAL_ID;
        const std::string occlusionPath = (meshComponent && !meshComponent->GetOcclusionTexturePath().empty())
                                          ? meshComponent->GetOcclusionTexturePath()
                                          : SHARED_DEFAULT_OCCLUSION_ID;
        const std::string emissivePath = (meshComponent && !meshComponent->GetEmissiveTexturePath().empty())
                                         ? meshComponent->GetEmissiveTexturePath()
                                         : SHARED_DEFAULT_EMISSIVE_ID;
        std::array<std::string,5> pbrTexturePaths = { baseColorPath, mrPath, normalPath, occlusionPath, emissivePath };

        for (int j = 0; j < 5; ++j) {
            const auto resolvedBindingPath = ResolveTextureId(pbrTexturePaths[j]);
            auto textureIt = textureResources.find(resolvedBindingPath);
            TextureResources* texRes = (textureIt != textureResources.end()) ? &textureIt->second : &defaultTextureResources;
            imageInfos[j] = { .sampler = *texRes->textureSampler, .imageView = *texRes->textureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
            writes.push_back({ .dstSet = *targetDescriptorSets[frameIndex], .dstBinding = static_cast<uint32_t>(j + 1), .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &imageInfos[j] });
        }
        // Ensure Forward+ light buffer (binding 6) is written for the current frame when available.
        // Do this even on imagesOnly updates so set 0 is fully valid for PBR shading.
        if (frameIndex < lightStorageBuffers.size() && *lightStorageBuffers[frameIndex].buffer) {
            vk::DescriptorBufferInfo lightBufferInfo{ .buffer = *lightStorageBuffers[frameIndex].buffer, .range = VK_WHOLE_SIZE };
            writes.push_back({ .dstSet = *targetDescriptorSets[frameIndex], .dstBinding = 6, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &lightBufferInfo });
        }
        {
            std::lock_guard<std::mutex> lk(descriptorMutex);
            device.updateDescriptorSets(writes, {});
        }
        // CRITICAL FIX: Only mark UBO as written if we actually wrote it (not during imagesOnly updates)
        if (!imagesOnly) {
            entityIt->second.uboBindingWritten[frameIndex] = true;
        }
    } else {
        const std::string resolvedTexturePath = ResolveTextureId(texturePath);
        auto textureIt = textureResources.find(resolvedTexturePath);
        TextureResources* texRes = (textureIt != textureResources.end()) ? &textureIt->second : &defaultTextureResources;
        vk::DescriptorImageInfo imageInfo{ .sampler = *texRes->textureSampler, .imageView = *texRes->textureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
        if (imagesOnly && !newlyAllocated) {
            std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
                vk::WriteDescriptorSet{ .dstSet = *targetDescriptorSets[frameIndex], .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &imageInfo }
            };
            {
                std::lock_guard<std::mutex> lk(descriptorMutex);
                device.updateDescriptorSets(descriptorWrites, {});
            }
        } else {
            // If uboOnly is requested for basic pipeline, only write binding 0
            if (uboOnly) {
                std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
                    vk::WriteDescriptorSet{ .dstSet = *targetDescriptorSets[frameIndex], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &bufferInfo }
                };
                {
                    std::lock_guard<std::mutex> lk(descriptorMutex);
                    device.updateDescriptorSets(descriptorWrites, {});
                }
                entityIt->second.uboBindingWritten[frameIndex] = true;
                return true;
            }
            std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
                vk::WriteDescriptorSet{ .dstSet = *targetDescriptorSets[frameIndex], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &bufferInfo },
                vk::WriteDescriptorSet{ .dstSet = *targetDescriptorSets[frameIndex], .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &imageInfo }
            };
            {
                std::lock_guard<std::mutex> lk(descriptorMutex);
                device.updateDescriptorSets(descriptorWrites, {});
            }
            entityIt->second.uboBindingWritten[frameIndex] = true;
        }
    }
    return true;
}

void Renderer::ProcessDirtyDescriptorsForFrame(uint32_t frameIndex) {
    std::vector<Entity*> dirty;
    {
        std::lock_guard<std::mutex> lk(dirtyEntitiesMutex);
        if (descriptorDirtyEntities.empty()) return;
        dirty.reserve(descriptorDirtyEntities.size());
        for (auto* e : descriptorDirtyEntities) dirty.push_back(e);
        descriptorDirtyEntities.clear();
    }

    for (Entity* entity : dirty) {
        if (!entity) continue;
        auto meshComponent = entity->GetComponent<MeshComponent>();
        if (!meshComponent) continue;
        // Resolve a texture path to pass for the basic pipeline
        std::string basicTexPath = meshComponent->GetTexturePath();
        if (basicTexPath.empty()) basicTexPath = meshComponent->GetBaseColorTexturePath();
        // Update strategy:
        // - Only update the current frame here at the safe point.
        //   Other frames will be updated at their own safe points to avoid UPDATE_AFTER_BIND violations.
        updateDescriptorSetsForFrame(entity, basicTexPath, false, frameIndex, /*imagesOnly=*/true);
        updateDescriptorSetsForFrame(entity, basicTexPath, true,  frameIndex, /*imagesOnly=*/true);
        // Do not touch descriptors for other frames while their command buffers may be pending.
    }
}

void Renderer::ProcessPendingTextureJobs(uint32_t maxJobs,
                                         bool includeCritical,
                                         bool includeNonCritical) {
    // If the background uploads worker is running, it will handle draining
    // texture jobs. Keep this function as a safe no-op for render-thread code
    // paths that still call it.
    if (!uploadsWorkerThreads.empty() && !stopUploadsWorker.load(std::memory_order_relaxed)) {
        return;
    }
    // Drain the pending job list under lock into a local vector, then
    // perform a bounded number of texture loads (including Vulkan work)
    // on this thread. This must be called from the main/render thread.
    std::vector<PendingTextureJob> jobs;
    {
        std::lock_guard<std::mutex> lk(pendingTextureJobsMutex);
        if (pendingTextureJobs.empty()) {
            return;
        }
        jobs.swap(pendingTextureJobs);
    }

    std::vector<PendingTextureJob> remaining;
    remaining.reserve(jobs.size());

    uint32_t processed = 0;
    for (auto& job : jobs) {
        const bool isCritical = (job.priority == PendingTextureJob::Priority::Critical);
        if (processed < maxJobs &&
            ((isCritical && includeCritical) || (!isCritical && includeNonCritical))) {
            switch (job.type) {
                case PendingTextureJob::Type::FromFile:
                    // LoadTexture will resolve aliases and perform full GPU upload
                    LoadTexture(job.idOrPath);
                    break;
                case PendingTextureJob::Type::FromMemory:
                    // LoadTextureFromMemory will create GPU resources for this ID
                    LoadTextureFromMemory(job.idOrPath,
                                          job.data.data(),
                                          job.width,
                                          job.height,
                                          job.channels);
                    break;
            }
            // Refresh descriptors for entities that use this texture so
            // streaming uploads become visible in the scene.
            OnTextureUploaded(job.idOrPath);
            if (isCritical) {
                criticalJobsOutstanding.fetch_sub(1, std::memory_order_relaxed);
            }
            uploadJobsCompleted.fetch_add(1, std::memory_order_relaxed);
            ++processed;
        } else {
            remaining.emplace_back(std::move(job));
        }
    }

    if (!remaining.empty()) {
        std::lock_guard<std::mutex> lk(pendingTextureJobsMutex);
        // Append remaining jobs back to the pending queue
        pendingTextureJobs.insert(pendingTextureJobs.end(),
                                  std::make_move_iterator(remaining.begin()),
                                  std::make_move_iterator(remaining.end()));
    }
}


// Record both layout transitions and the copy in a single submission with a fence
void Renderer::uploadImageFromStaging(vk::Buffer staging,
                                      vk::Image image,
                                      vk::Format format,
                                      const std::vector<vk::BufferImageCopy>& regions,
                                      uint32_t mipLevels,
                                      vk::DeviceSize stagedBytes) {
    ensureThreadLocalVulkanInit();
    try {
        // Start perf window on first upload
        if (uploadWindowStartNs.load(std::memory_order_relaxed) == 0) {
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            uint64_t nowNs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
            uploadWindowStartNs.store(nowNs, std::memory_order_relaxed);
        }
        auto t0 = std::chrono::steady_clock::now();

        // Use a temporary transient command pool for the TRANSFER queue family
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamilyIndices.transferFamily.value()
        };
        vk::raii::CommandPool tempPool(device, poolInfo);
        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = *tempPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        vk::raii::CommandBuffers cbs(device, allocInfo);
        vk::raii::CommandBuffer& cb = cbs[0];

        vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        cb.begin(beginInfo);

        // Barrier: Undefined -> TransferDstOptimal (base level only) (Sync2)
        vk::ImageMemoryBarrier2 toTransfer2{
            .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
            .srcAccessMask = vk::AccessFlagBits2::eNone,
            .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = (format == vk::Format::eD32Sfloat || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
                               ? vk::ImageAspectFlagBits::eDepth
                               : vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vk::DependencyInfo depToTransfer{ .dependencyFlags = vk::DependencyFlagBits::eByRegion, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &toTransfer2 };
        cb.pipelineBarrier2(depToTransfer);

        // Copy
        cb.copyBufferToImage(staging, image, vk::ImageLayout::eTransferDstOptimal, regions);

        // After copy, if we'll generate mips, keep level 0 in TRANSFER_SRC. Else transition to SHADER_READ_ONLY. (Sync2)
        vk::ImageMemoryBarrier2 postCopy2{
            .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .dstAccessMask = vk::AccessFlagBits2::eNone,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = (mipLevels > 1) ? vk::ImageLayout::eTransferSrcOptimal : vk::ImageLayout::eShaderReadOnlyOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = (format == vk::Format::eD32Sfloat || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
                               ? vk::ImageAspectFlagBits::eDepth
                               : vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        // IMPORTANT: This command buffer is recorded for the TRANSFER queue family.
        // Do not use FragmentShader (or any graphics stage) in dstStageMask here, or VVL will complain.
        vk::DependencyInfo depPostCopy{ .dependencyFlags = vk::DependencyFlagBits::eByRegion, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &postCopy2 };
        cb.pipelineBarrier2(depPostCopy);

        cb.end();

        // Submit once on the TRANSFER queue; signal uploads timeline if available
        vk::raii::Fence fence(device, vk::FenceCreateInfo{});
        bool canSignalTimeline = uploadsTimeline != nullptr;
        uint64_t signalValue = 0;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            vk::SubmitInfo submit{};
            if (canSignalTimeline) {
                signalValue = uploadTimelineLastSubmitted.fetch_add(1, std::memory_order_relaxed) + 1;
                vk::TimelineSemaphoreSubmitInfo timelineInfo{
                    .signalSemaphoreValueCount = 1,
                    .pSignalSemaphoreValues = &signalValue
                };
                submit.pNext = &timelineInfo;
                submit.signalSemaphoreCount = 1;
                submit.pSignalSemaphores = &*uploadsTimeline;
            }
            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &*cb;

            transferQueue.submit(submit, *fence);
        }
        [[maybe_unused]] auto fenceResult5 = device.waitForFences({*fence}, VK_TRUE, UINT64_MAX);

        // Perf accounting
        auto t1 = std::chrono::steady_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        totalUploadNs.fetch_add(static_cast<uint64_t>(ns), std::memory_order_relaxed);
        uploadCount.fetch_add(1, std::memory_order_relaxed);
        if (stagedBytes > 0) {
            bytesUploadedTotal.fetch_add(static_cast<uint64_t>(stagedBytes), std::memory_order_relaxed);
        }
    } catch (const std::exception& e) {
        std::cerr << "uploadImageFromStaging failed: " << e.what() << std::endl;
        throw;
    }
}

// Generate full mip chain with linear blits (RGBA formats). Assumes level 0 is in TRANSFER_SRC_OPTIMAL.
void Renderer::generateMipmaps(vk::Image image,
                               vk::Format format,
                               int32_t texWidth,
                               int32_t texHeight,
                               uint32_t mipLevels) {
    ensureThreadLocalVulkanInit();
    // Verify format supports linear blit
    auto props = physicalDevice.getFormatProperties(format);
    if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear) == vk::FormatFeatureFlags{}) {
        return; // no linear filter support; skip
    }

    vk::CommandPoolCreateInfo poolInfo{ .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool tempPool(device, poolInfo);
    vk::CommandBufferAllocateInfo allocInfo{ .commandPool = *tempPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
    vk::raii::CommandBuffers cbs(device, allocInfo);
    vk::raii::CommandBuffer& cb = cbs[0];
    cb.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    int32_t mipW = texWidth;
    int32_t mipH = texHeight;
    for (uint32_t i = 1; i < mipLevels; ++i) {
        // Transition level i to TRANSFER_DST (Sync2)
        vk::ImageMemoryBarrier2 toDst2{ .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe, .srcAccessMask = vk::AccessFlagBits2::eNone, .dstStageMask = vk::PipelineStageFlagBits2::eTransfer, .dstAccessMask = vk::AccessFlagBits2::eTransferWrite, .oldLayout = vk::ImageLayout::eUndefined, .newLayout = vk::ImageLayout::eTransferDstOptimal, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = image, .subresourceRange = { vk::ImageAspectFlagBits::eColor, i, 1, 0, 1 } };
        vk::DependencyInfo depToDst{ .dependencyFlags = vk::DependencyFlagBits::eByRegion, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &toDst2 };
        cb.pipelineBarrier2(depToDst);

        // Blit from i-1 to i
        vk::ImageBlit blit{};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{ mipW, mipH, 1 };
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{ std::max(1, mipW/2), std::max(1, mipH/2), 1 };
        cb.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

        // Transition previous level to SHADER_READ_ONLY (Sync2)
        vk::ImageMemoryBarrier2 prevToRead2{ .srcStageMask = vk::PipelineStageFlagBits2::eTransfer, .srcAccessMask = vk::AccessFlagBits2::eTransferRead, .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader, .dstAccessMask = vk::AccessFlagBits2::eShaderRead, .oldLayout = vk::ImageLayout::eTransferSrcOptimal, .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = image, .subresourceRange = { vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1 } };
        vk::DependencyInfo depPrevToRead{ .dependencyFlags = vk::DependencyFlagBits::eByRegion, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &prevToRead2 };
        cb.pipelineBarrier2(depPrevToRead);

        mipW = std::max(1, mipW / 2);
        mipH = std::max(1, mipH / 2);
    }
    // Transition last level to SHADER_READ_ONLY (Sync2)
    vk::ImageMemoryBarrier2 lastToRead2{ .srcStageMask = vk::PipelineStageFlagBits2::eTransfer, .srcAccessMask = vk::AccessFlagBits2::eTransferWrite, .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader, .dstAccessMask = vk::AccessFlagBits2::eShaderRead, .oldLayout = vk::ImageLayout::eTransferDstOptimal, .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = image, .subresourceRange = { vk::ImageAspectFlagBits::eColor, mipLevels - 1, 1, 0, 1 } };
    vk::DependencyInfo depLastToRead{ .dependencyFlags = vk::DependencyFlagBits::eByRegion, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &lastToRead2 };
    cb.pipelineBarrier2(depLastToRead);

    cb.end();
    
    // CRITICAL FIX: Signal timeline semaphore after mipmap generation to ensure render loop
    // waits for BOTH upload (transfer queue) AND mipmap generation (graphics queue) to complete.
    // Without this, textures can be sampled before mipmaps are fully generated, causing
    // validation errors: "expects SHADER_READ_ONLY_OPTIMAL--instead, current layout is UNDEFINED"
    // This happens because uploadImageFromStaging signals timeline N, but generateMipmaps runs
    // on graphics queue without updating timeline, creating a race where rendering waits for N
    // (upload complete) but not for mipmap generation (which may still be running).
    vk::raii::Fence fence(device, vk::FenceCreateInfo{});
    bool canSignalTimeline = uploadsTimeline != nullptr;
    uint64_t signalValue = 0;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        vk::SubmitInfo submit{};
        if (canSignalTimeline) {
            signalValue = uploadTimelineLastSubmitted.fetch_add(1, std::memory_order_relaxed) + 1;
            vk::TimelineSemaphoreSubmitInfo timelineInfo{
                .signalSemaphoreValueCount = 1,
                .pSignalSemaphoreValues = &signalValue
            };
            submit.pNext = &timelineInfo;
            submit.signalSemaphoreCount = 1;
            submit.pSignalSemaphores = &*uploadsTimeline;
        }
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &*cb;
        graphicsQueue.submit(submit, *fence);
    }
    (void)device.waitForFences({ *fence }, VK_TRUE, UINT64_MAX);
}
