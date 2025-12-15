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
#include "model_loader.h"
#include "mesh_component.h"
#include "renderer.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <set>
#include <tiny_gltf.h>

#include "mikktspace.h"

// This struct acts as a bridge between the C-style MikkTSpace callbacks
// and our C++ MaterialMesh vertex data. It's passed via the m_pUserData pointer.
struct MikkTSpaceInterface
{
	std::vector<Vertex>   *vertices;
	std::vector<uint32_t> *indices;
};

// These static callback functions are required by the MikkTSpace library.
// They are defined here at file-scope so they are not part of the ModelLoader class.
static int getNumFaces(const SMikkTSpaceContext *pContext)
{
	auto *userData = static_cast<MikkTSpaceInterface *>(pContext->m_pUserData);
	return static_cast<int>(userData->indices->size() / 3);
}

static int getNumVerticesOfFace(const SMikkTSpaceContext *pContext, const int iFace)
{
	return 3;
}

static void getPosition(const SMikkTSpaceContext *pContext, float fvPosOut[], const int iFace, const int iVert)
{
	auto            *userData = static_cast<MikkTSpaceInterface *>(pContext->m_pUserData);
	uint32_t         index    = (*userData->indices)[iFace * 3 + iVert];
	const glm::vec3 &pos      = (*userData->vertices)[index].position;
	fvPosOut[0]               = pos.x;
	fvPosOut[1]               = pos.y;
	fvPosOut[2]               = pos.z;
}

static void getNormal(const SMikkTSpaceContext *pContext, float fvNormOut[], const int iFace, const int iVert)
{
	auto            *userData = static_cast<MikkTSpaceInterface *>(pContext->m_pUserData);
	uint32_t         index    = (*userData->indices)[iFace * 3 + iVert];
	const glm::vec3 &norm     = (*userData->vertices)[index].normal;
	fvNormOut[0]              = norm.x;
	fvNormOut[1]              = norm.y;
	fvNormOut[2]              = norm.z;
}

static void getTexCoord(const SMikkTSpaceContext *pContext, float fvTexcOut[], const int iFace, const int iVert)
{
	auto            *userData = static_cast<MikkTSpaceInterface *>(pContext->m_pUserData);
	uint32_t         index    = (*userData->indices)[iFace * 3 + iVert];
	const glm::vec2 &uv       = (*userData->vertices)[index].texCoord;
	fvTexcOut[0]              = uv.x;
	fvTexcOut[1]              = uv.y;
}

static void setTSpaceBasic(const SMikkTSpaceContext *pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
{
	auto    *userData = static_cast<MikkTSpaceInterface *>(pContext->m_pUserData);
	uint32_t index    = (*userData->indices)[iFace * 3 + iVert];
	Vertex  &vert     = (*userData->vertices)[index];
	vert.tangent.x    = fvTangent[0];
	vert.tangent.y    = fvTangent[1];
	vert.tangent.z    = fvTangent[2];
	// Clamp handedness to +/-1 to avoid tiny floating deviations
	vert.tangent.w = (fSign >= 0.0f) ? 1.0f : -1.0f;
}

// KTX2 decoding for GLTF images
#include <ktx.h>

// Helper: load KTX2 file from disk into RGBA8 CPU buffer
static bool LoadKTX2FileToRGBA(const std::string &filePath, std::vector<uint8_t> &outData, int &width, int &height, int &channels)
{
	ktxTexture2   *ktxTex = nullptr;
	KTX_error_code result = ktxTexture2_CreateFromNamedFile(filePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);
	if (result != KTX_SUCCESS || !ktxTex)
	{
		return false;
	}
	bool needsTranscode = ktxTexture2_NeedsTranscoding(ktxTex);
	if (needsTranscode)
	{
		result = ktxTexture2_TranscodeBasis(ktxTex, KTX_TTF_RGBA32, 0);
		if (result != KTX_SUCCESS)
		{
			ktxTexture_Destroy((ktxTexture *) ktxTex);
			return false;
		}
	}
	width    = static_cast<int>(ktxTex->baseWidth);
	height   = static_cast<int>(ktxTex->baseHeight);
	channels = 4;
	ktx_size_t offset;
	ktxTexture_GetImageOffset((ktxTexture *) ktxTex, 0, 0, 0, &offset);
	const uint8_t *levelData = ktxTexture_GetData(reinterpret_cast<ktxTexture *>(ktxTex)) + offset;
	size_t         levelSize = needsTranscode ? static_cast<size_t>(width) * static_cast<size_t>(height) * 4 : ktxTexture_GetImageSize((ktxTexture *) ktxTex, 0);
	outData.resize(levelSize);
	std::memcpy(outData.data(), levelData, levelSize);
	ktxTexture_Destroy((ktxTexture *) ktxTex);
	return true;
}

// Emissive scaling factor to convert from Blender units to engine units
#define EMISSIVE_SCALE_FACTOR (1.0f / 638.0f)
#define LIGHT_SCALE_FACTOR (1.0f / 638.0f)

ModelLoader::~ModelLoader()
{
	// Destructor implementation
	models.clear();
	materials.clear();
}

bool ModelLoader::Initialize(Renderer *_renderer)
{
	renderer = _renderer;

	if (!renderer)
	{
		std::cerr << "ModelLoader::Initialize: Renderer is null" << std::endl;
		return false;
	}

	return true;
}

Model *ModelLoader::LoadGLTF(const std::string &filename)
{
	// Check if the model is already loaded
	auto it = models.find(filename);
	if (it != models.end())
	{
		return it->second.get();
	}

	// Create a new model
	auto model = std::make_unique<Model>(filename);

	// Parse the GLTF file
	if (!ParseGLTF(filename, model.get()))
	{
		std::cerr << "ModelLoader::LoadGLTF: Failed to parse GLTF file: " << filename << std::endl;
		return nullptr;
	}

	// Store the model
	models[filename] = std::move(model);

	return models[filename].get();
}

Model *ModelLoader::GetModel(const std::string &name)
{
	auto it = models.find(name);
	if (it != models.end())
	{
		return it->second.get();
	}
	return nullptr;
}

bool ModelLoader::ParseGLTF(const std::string &filename, Model *model)
{
	std::cout << "Parsing GLTF file: " << filename << std::endl;

	// Extract the directory path from the model file to use as a base path for textures
	std::filesystem::path modelPath(filename);
	std::filesystem::path baseDir         = std::filesystem::absolute(modelPath).parent_path();
	std::string           baseTexturePath = baseDir.string();
	if (!baseTexturePath.empty() && baseTexturePath.back() != '/')
	{
		baseTexturePath += "/";
	}
	std::cout << "Using base texture path: " << baseTexturePath << std::endl;

	// Create tinygltf loader
	tinygltf::Model    gltfModel;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;

	// Set up image loader: prefer KTX2 via libktx; fallback to stb for other formats
	loader.SetImageLoader([](tinygltf::Image *image, const int image_idx, std::string *err,
	                         std::string *warn, int req_width, int req_height,
	                         const unsigned char *bytes, int size, void *user_data) -> bool {
		// Try KTX2 first using libktx
		ktxTexture2   *ktxTex = nullptr;
		KTX_error_code result = ktxTexture2_CreateFromMemory(bytes, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);
		if (result == KTX_SUCCESS && ktxTex)
		{
			bool needsTranscode = ktxTexture2_NeedsTranscoding(ktxTex);
			if (needsTranscode)
			{
				result = ktxTexture2_TranscodeBasis(ktxTex, KTX_TTF_RGBA32, 0);
				if (result != KTX_SUCCESS)
				{
					if (err)
						*err = "Failed to transcode KTX2 image: " + std::to_string(result);
					ktxTexture_Destroy((ktxTexture *) ktxTex);
					return false;
				}
			}
			image->width      = static_cast<int>(ktxTex->baseWidth);
			image->height     = static_cast<int>(ktxTex->baseHeight);
			image->component  = 4;
			image->bits       = 8;
			image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;

			ktx_size_t offset;
			ktxTexture_GetImageOffset((ktxTexture *) ktxTex, 0, 0, 0, &offset);
			const uint8_t *levelData = ktxTexture_GetData(reinterpret_cast<ktxTexture *>(ktxTex)) + offset;
			size_t         levelSize = needsTranscode ? static_cast<size_t>(image->width) * static_cast<size_t>(image->height) * 4 : ktxTexture_GetImageSize((ktxTexture *) ktxTex, 0);
			image->image.resize(levelSize);
			std::memcpy(image->image.data(), levelData, levelSize);
			ktxTexture_Destroy((ktxTexture *) ktxTex);
			return true;
		}

		// Non-KTX images not supported by this loader per project simplification
		if (err)
		{
			*err = "Non-KTX2 images are not supported by the custom image loader (use KTX2).";
		}
		return false;
	},
	                      nullptr);

	// Load the GLTF file
	bool ret = false;
	if (filename.find(".glb") != std::string::npos)
	{
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filename);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);
	}

	if (!warn.empty())
	{
		std::cout << "GLTF Warning: " << warn << std::endl;
	}

	if (!err.empty())
	{
		std::cerr << "GLTF Error: " << err << std::endl;
		return false;
	}

	if (!ret)
	{
		std::cerr << "Failed to parse GLTF file: " << filename << std::endl;
		return false;
	}

	// Extract mesh data from the first mesh (for now, we'll handle multiple meshes later)
	if (gltfModel.meshes.empty())
	{
		std::cerr << "No meshes found in GLTF file" << std::endl;
		return false;
	}

	light_scale = 1.0f;
	// Test if generator is blender and apply the blender factor see the issue here: https://github.com/KhronosGroup/glTF/issues/2473
	if (gltfModel.asset.generator.find("blender") != std::string::npos)
	{
		std::cout << "Blender generator detected, applying blender factor" << std::endl;
		light_scale = EMISSIVE_SCALE_FACTOR;
	}

	// Track loaded textures to prevent loading the same texture multiple times
	std::set<std::string> loadedTextures;

	// Helper: lowercase a std::string (ASCII only)
	auto toLower = [](const std::string &s) {
		std::string out = s;
		std::ranges::transform(out, out.begin(),
		                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return out;
	};

	// Process materials first
	for (size_t i = 0; i < gltfModel.materials.size(); ++i)
	{
		const auto &gltfMaterial = gltfModel.materials[i];

		// Create PBR material
		auto material = std::make_unique<Material>(gltfMaterial.name.empty() ? ("material_" + std::to_string(i)) : gltfMaterial.name);

		// Extract PBR properties
		if (gltfMaterial.pbrMetallicRoughness.baseColorFactor.size() >= 3)
		{
			material->albedo = glm::vec3(
			    gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
			    gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
			    gltfMaterial.pbrMetallicRoughness.baseColorFactor[2]);
			if (gltfMaterial.pbrMetallicRoughness.baseColorFactor.size() >= 4)
			{
				material->alpha = static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]);
			}
		}
		material->metallic  = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
		material->roughness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);

		if (gltfMaterial.emissiveFactor.size() >= 3)
		{
			material->emissive = glm::vec3(
			    gltfMaterial.emissiveFactor[0],
			    gltfMaterial.emissiveFactor[1],
			    gltfMaterial.emissiveFactor[2]);
			material->emissive *= light_scale;
		}

		// Parse KHR_materials_emissive_strength extension
		auto extensionIt = gltfMaterial.extensions.find("KHR_materials_emissive_strength");
		if (extensionIt != gltfMaterial.extensions.end())
		{
			hasEmissiveStrengthExtension     = true;
			const tinygltf::Value &extension = extensionIt->second;
			if (extension.Has("emissiveStrength") && extension.Get("emissiveStrength").IsNumber())
			{
				material->emissiveStrength = static_cast<float>(extension.Get("emissiveStrength").Get<double>());
			}
		}
		else
		{
			material->emissiveStrength = 0.00058f;
		}

		// Alpha mode / cutoff
		material->alphaMode   = gltfMaterial.alphaMode.empty() ? std::string("OPAQUE") : gltfMaterial.alphaMode;
		material->alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);

		// Transmission (KHR_materials_transmission)
		auto transIt = gltfMaterial.extensions.find("KHR_materials_transmission");
		if (transIt != gltfMaterial.extensions.end())
		{
			const tinygltf::Value &ext = transIt->second;
			if (ext.Has("transmissionFactor") && ext.Get("transmissionFactor").IsNumber())
			{
				material->transmissionFactor = static_cast<float>(ext.Get("transmissionFactor").Get<double>());
			}
		}

		// Classify obvious architectural glass and liquid materials for
		// specialized rendering. This is a heuristic based primarily on
		// material name.
		{
			std::string lowerName = toLower(material->GetName());
			bool        nameSuggestsGlass =
			    (lowerName.find("glass") != std::string::npos) ||
			    (lowerName.find("window") != std::string::npos);

			bool probablyLiquid =
			    (lowerName.find("beer") != std::string::npos) ||
			    (lowerName.find("wine") != std::string::npos) ||
			    (lowerName.find("liquid") != std::string::npos);

			if (nameSuggestsGlass && !probablyLiquid)
			{
				material->isGlass = true;
			}

			if (probablyLiquid)
			{
				material->isLiquid = true;

				// Slightly boost liquid visibility.
				material->albedo *= 1.4f;
				material->albedo = glm::clamp(material->albedo, glm::vec3(0.0f), glm::vec3(4.0f));

				// Slightly reduce roughness so specular highlights from
				// lights help liquids stand out.
				material->roughness = glm::clamp(material->roughness * 0.8f, 0.0f, 1.0f);

				// Ensure the liquid is not fully transparent by default.
				material->alpha = glm::clamp(material->alpha * 1.2f, 0.15f, 1.0f);
			}
		}

		// Specular-Glossiness (KHR_materials_pbrSpecularGlossiness)
		auto sgIt = gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
		if (sgIt != gltfMaterial.extensions.end())
		{
			const tinygltf::Value &ext      = sgIt->second;
			material->useSpecularGlossiness = true;
			// diffuseFactor -> albedo and alpha
			if (ext.Has("diffuseFactor") && ext.Get("diffuseFactor").IsArray())
			{
				const auto &arr = ext.Get("diffuseFactor").Get<tinygltf::Value::Array>();
				if (arr.size() >= 3)
				{
					material->albedo = glm::vec3(
					    arr[0].IsNumber() ? static_cast<float>(arr[0].Get<double>()) : material->albedo.r,
					    arr[1].IsNumber() ? static_cast<float>(arr[1].Get<double>()) : material->albedo.g,
					    arr[2].IsNumber() ? static_cast<float>(arr[2].Get<double>()) : material->albedo.b);
					if (arr.size() >= 4 && arr[3].IsNumber())
					{
						material->alpha = static_cast<float>(arr[3].Get<double>());
					}
				}
			}
			// specularFactor (vec3)
			if (ext.Has("specularFactor") && ext.Get("specularFactor").IsArray())
			{
				const auto &arr = ext.Get("specularFactor").Get<tinygltf::Value::Array>();
				if (arr.size() >= 3)
				{
					material->specularFactor = glm::vec3(
					    arr[0].IsNumber() ? static_cast<float>(arr[0].Get<double>()) : material->specularFactor.r,
					    arr[1].IsNumber() ? static_cast<float>(arr[1].Get<double>()) : material->specularFactor.g,
					    arr[2].IsNumber() ? static_cast<float>(arr[2].Get<double>()) : material->specularFactor.b);
				}
			}
			// glossinessFactor (float)
			if (ext.Has("glossinessFactor") && ext.Get("glossinessFactor").IsNumber())
			{
				material->glossinessFactor = static_cast<float>(ext.Get("glossinessFactor").Get<double>());
			}

			// Load diffuseTexture into albedoTexturePath if present
			if (ext.Has("diffuseTexture") && ext.Get("diffuseTexture").IsObject())
			{
				const auto &diffObj = ext.Get("diffuseTexture");
				if (diffObj.Has("index") && diffObj.Get("index").IsInt())
				{
					int texIndex = diffObj.Get("index").Get<int>();
					if (texIndex >= 0 && texIndex < static_cast<int>(gltfModel.textures.size()))
					{
						const auto &texture    = gltfModel.textures[texIndex];
						int         imageIndex = -1;
						if (texture.source >= 0 && texture.source < static_cast<int>(gltfModel.images.size()))
						{
							imageIndex = texture.source;
						}
						else
						{
							auto extBasis = texture.extensions.find("KHR_texture_basisu");
							if (extBasis != texture.extensions.end())
							{
								const tinygltf::Value &e = extBasis->second;
								if (e.Has("source") && e.Get("source").IsInt())
								{
									int src = e.Get("source").Get<int>();
									if (src >= 0 && src < static_cast<int>(gltfModel.images.size()))
										imageIndex = src;
								}
							}
						}
						if (imageIndex >= 0)
						{
							const auto &image     = gltfModel.images[imageIndex];
							std::string textureId = "gltf_baseColor_" + std::to_string(texIndex);
							if (!image.image.empty())
							{
								renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
								material->albedoTexturePath = textureId;
							}
							else if (!image.uri.empty())
							{
								std::string filePath = baseTexturePath + image.uri;
								renderer->LoadTextureAsync(filePath);
								material->albedoTexturePath = filePath;
							}
						}
					}
				}
			}
			// Load specularGlossinessTexture into specGlossTexturePath and mirror to metallicRoughnessTexturePath (binding 2)
			if (ext.Has("specularGlossinessTexture") && ext.Get("specularGlossinessTexture").IsObject())
			{
				const auto &sgObj = ext.Get("specularGlossinessTexture");
				if (sgObj.Has("index") && sgObj.Get("index").IsInt())
				{
					int texIndex = sgObj.Get("index").Get<int>();
					if (texIndex >= 0 && texIndex < static_cast<int>(gltfModel.textures.size()))
					{
						const auto &texture = gltfModel.textures[texIndex];
						if (texture.source >= 0 && texture.source < static_cast<int>(gltfModel.images.size()))
						{
							std::string textureId = "gltf_specGloss_" + std::to_string(texIndex);
							const auto &image     = gltfModel.images[texture.source];
							if (!image.image.empty())
							{
								// Embedded image data (already decoded by tinygltf image loader)
								renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component, false);
								material->specGlossTexturePath         = textureId;
								material->metallicRoughnessTexturePath = textureId;        // reuse binding 2
							}
							else if (!image.uri.empty())
							{
								// External KTX2 file: offload libktx decode + upload to renderer worker threads
								std::string filePath = baseTexturePath + image.uri;
								renderer->RegisterTextureAlias(textureId, filePath);
								renderer->LoadTextureAsync(filePath);
								material->specGlossTexturePath         = textureId;
								material->metallicRoughnessTexturePath = textureId;        // reuse binding 2
							}
						}
					}
				}
			}
		}

		// Extract texture information and load embedded texture data
		if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
		{
			int texIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
			if (texIndex < gltfModel.textures.size())
			{
				const auto &texture    = gltfModel.textures[texIndex];
				int         imageIndex = -1;
				if (texture.source >= 0 && texture.source < gltfModel.images.size())
				{
					imageIndex = texture.source;
				}
				else
				{
					auto extIt = texture.extensions.find("KHR_texture_basisu");
					if (extIt != texture.extensions.end())
					{
						const tinygltf::Value &ext = extIt->second;
						if (ext.Has("source") && ext.Get("source").IsInt())
						{
							int src = ext.Get("source").Get<int>();
							if (src >= 0 && src < static_cast<int>(gltfModel.images.size()))
							{
								imageIndex = src;
							}
						}
					}
				}
				if (imageIndex >= 0)
				{
					std::string textureId       = "gltf_baseColor_" + std::to_string(texIndex);
					material->albedoTexturePath = textureId;

					// Load texture data (embedded or external)
					const auto &image = gltfModel.images[imageIndex];
					std::cout << "    Image data size: " << image.image.size() << ", URI: " << image.uri << std::endl;
					if (!image.image.empty())
					{
						// Always use memory-based upload (KTX2 already decoded by SetImageLoader)
						renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component, true);
						material->albedoTexturePath = textureId;
						std::cout << "    Scheduled base color texture upload from memory: " << textureId << std::endl;
					}
					else if (!image.uri.empty())
					{
						// Offload KTX2 file reading/upload to renderer thread pool
						std::string filePath = baseTexturePath + image.uri;
						renderer->RegisterTextureAlias(textureId, filePath);
						renderer->LoadTextureAsync(filePath, true);
						material->albedoTexturePath = textureId;
						std::cout << "    Scheduled base color KTX2 load from file: " << filePath << " (alias for " << textureId << ")" << std::endl;
					}
					else
					{
						std::cerr << "    Warning: No decoded image bytes for base color texture index " << texIndex << std::endl;
					}
				}
			}
		}

		if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
		{
			int texIndex = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
			if (texIndex < gltfModel.textures.size())
			{
				const auto &texture = gltfModel.textures[texIndex];
				if (texture.source >= 0 && texture.source < gltfModel.images.size())
				{
					std::string textureId                  = "gltf_texture_" + std::to_string(texIndex);
					material->metallicRoughnessTexturePath = textureId;

					// Load texture data (embedded or external)
					const auto &image = gltfModel.images[texture.source];
					if (!image.image.empty())
					{
						// Load embedded texture data asynchronously
						renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
						std::cout << "    Scheduled embedded metallic-roughness texture upload: " << textureId << std::endl;
					}
					else if (!image.uri.empty())
					{
						// Offload KTX2 file reading/upload to renderer thread pool
						std::string filePath = baseTexturePath + image.uri;
						renderer->RegisterTextureAlias(textureId, filePath);
						renderer->LoadTextureAsync(filePath);
						material->metallicRoughnessTexturePath = textureId;
						std::cout << "    Scheduled metallic-roughness KTX2 load from file: " << filePath << " (alias for " << textureId << ")" << std::endl;
					}
					else
					{
						std::cerr << "    Warning: No decoded bytes for metallic-roughness texture index " << texIndex << std::endl;
					}
				}
			}
		}

		if (gltfMaterial.normalTexture.index >= 0)
		{
			int texIndex = gltfMaterial.normalTexture.index;
			if (texIndex < gltfModel.textures.size())
			{
				const auto &texture    = gltfModel.textures[texIndex];
				int         imageIndex = -1;
				if (texture.source >= 0 && texture.source < gltfModel.images.size())
				{
					imageIndex = texture.source;
				}
				else
				{
					auto extIt = texture.extensions.find("KHR_texture_basisu");
					if (extIt != texture.extensions.end())
					{
						const tinygltf::Value &ext = extIt->second;
						if (ext.Has("source") && ext.Get("source").IsInt())
						{
							int src = ext.Get("source").Get<int>();
							if (src >= 0 && src < static_cast<int>(gltfModel.images.size()))
							{
								imageIndex = src;
							}
						}
					}
				}
				if (imageIndex >= 0)
				{
					std::string textureId       = "gltf_texture_" + std::to_string(texIndex);
					material->normalTexturePath = textureId;

					// Load texture data (embedded or external)
					const auto &image = gltfModel.images[imageIndex];
					if (!image.image.empty())
					{
						renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
						material->normalTexturePath = textureId;
						std::cout << "    Scheduled normal texture upload from memory: " << textureId
						          << " (" << image.width << "x" << image.height << ")" << std::endl;
					}
					else if (!image.uri.empty())
					{
						// Offload KTX2 file reading/upload to renderer thread pool
						std::string filePath = baseTexturePath + image.uri;
						renderer->RegisterTextureAlias(textureId, filePath);
						renderer->LoadTextureAsync(filePath);
						material->normalTexturePath = textureId;
						std::cout << "    Scheduled normal KTX2 load from file: " << filePath << " (alias for " << textureId << ")" << std::endl;
					}
					else
					{
						std::cerr << "    Warning: No decoded bytes for normal texture index " << texIndex << std::endl;
					}
				}
			}
		}

		if (gltfMaterial.occlusionTexture.index >= 0)
		{
			int texIndex = gltfMaterial.occlusionTexture.index;
			if (texIndex < gltfModel.textures.size())
			{
				const auto &texture = gltfModel.textures[texIndex];
				if (texture.source >= 0 && texture.source < gltfModel.images.size())
				{
					std::string textureId          = "gltf_texture_" + std::to_string(texIndex);
					material->occlusionTexturePath = textureId;

					// Load texture data (embedded or external)
					const auto &image = gltfModel.images[texture.source];
					if (!image.image.empty())
					{
						// Schedule embedded texture upload
						renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
						std::cout << "    Scheduled embedded occlusion texture upload: " << textureId
						          << " (" << image.width << "x" << image.height << ")" << std::endl;
					}
					else if (!image.uri.empty())
					{
						// Offload KTX2 file reading/upload to renderer thread pool
						std::string filePath = baseTexturePath + image.uri;
						renderer->RegisterTextureAlias(textureId, filePath);
						renderer->LoadTextureAsync(filePath);
						material->occlusionTexturePath = textureId;
						std::cout << "    Scheduled occlusion KTX2 load from file: " << filePath << " (alias for " << textureId << ")" << std::endl;
					}
					else
					{
						std::cerr << "    Warning: No decoded bytes for occlusion texture index " << texIndex << std::endl;
					}
				}
			}
		}

		if (gltfMaterial.emissiveTexture.index >= 0)
		{
			int texIndex = gltfMaterial.emissiveTexture.index;
			if (texIndex < gltfModel.textures.size())
			{
				const auto &texture = gltfModel.textures[texIndex];
				if (texture.source >= 0 && texture.source < gltfModel.images.size())
				{
					std::string textureId         = "gltf_texture_" + std::to_string(texIndex);
					material->emissiveTexturePath = textureId;

					// Load texture data (embedded or external)
					const auto &image = gltfModel.images[texture.source];
					if (!image.image.empty())
					{
						// Schedule embedded texture upload
						renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
						std::cout << "    Scheduled embedded emissive texture upload: " << textureId
						          << " (" << image.width << "x" << image.height << ")" << std::endl;
					}
					else if (!image.uri.empty())
					{
						// Offload KTX2 file reading/upload to renderer thread pool
						std::string filePath = baseTexturePath + image.uri;
						renderer->RegisterTextureAlias(textureId, filePath);
						renderer->LoadTextureAsync(filePath);
						material->emissiveTexturePath = textureId;
						std::cout << "    Scheduled emissive KTX2 load from file: " << filePath << " (alias for " << textureId << ")" << std::endl;
					}
					else
					{
						std::cerr << "    Warning: No decoded bytes for emissive texture index " << texIndex << std::endl;
					}
				}
			}
		}

		// Store the material
		materials[material->GetName()] = std::move(material);
	}

	// Handle KHR_materials_pbrSpecularGlossiness.diffuseTexture for baseColor when still missing
	for (size_t i = 0; i < gltfModel.materials.size(); ++i)
	{
		const auto &gltfMaterial = gltfModel.materials[i];
		std::string matName      = gltfMaterial.name.empty() ? ("material_" + std::to_string(i)) : gltfMaterial.name;
		auto        matIt        = materials.find(matName);
		if (matIt == materials.end())
			continue;
		Material *mat = matIt->second.get();
		if (!mat || !mat->albedoTexturePath.empty())
			continue;
		auto extIt = gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
		if (extIt != gltfMaterial.extensions.end())
		{
			const tinygltf::Value &ext = extIt->second;
			if (ext.Has("diffuseTexture") && ext.Get("diffuseTexture").IsObject())
			{
				const auto &diffObj = ext.Get("diffuseTexture");
				if (diffObj.Has("index") && diffObj.Get("index").IsInt())
				{
					int texIndex = diffObj.Get("index").Get<int>();
					if (texIndex >= 0 && texIndex < static_cast<int>(gltfModel.textures.size()))
					{
						const auto &texture    = gltfModel.textures[texIndex];
						int         imageIndex = -1;
						if (texture.source >= 0 && texture.source < static_cast<int>(gltfModel.images.size()))
						{
							imageIndex = texture.source;
						}
						else
						{
							auto extBasis = texture.extensions.find("KHR_texture_basisu");
							if (extBasis != texture.extensions.end())
							{
								const tinygltf::Value &e = extBasis->second;
								if (e.Has("source") && e.Get("source").IsInt())
								{
									int src = e.Get("source").Get<int>();
									if (src >= 0 && src < static_cast<int>(gltfModel.images.size()))
										imageIndex = src;
								}
							}
						}
						if (imageIndex >= 0)
						{
							const auto &image = gltfModel.images[imageIndex];
							std::string texIdOrPath;
							if (!image.uri.empty())
							{
								texIdOrPath = baseTexturePath + image.uri;
								// Schedule async load; libktx decoding will occur on renderer worker threads
								renderer->LoadTextureAsync(texIdOrPath, true);
								mat->albedoTexturePath = texIdOrPath;
								std::cout << "    Scheduled base color KTX2 file load (KHR_specGloss): " << texIdOrPath << std::endl;
							}
							if (mat->albedoTexturePath.empty() && !image.image.empty())
							{
								// Upload embedded image data (already decoded via our image loader when KTX2)
								texIdOrPath = "gltf_baseColor_" + std::to_string(texIndex);
								renderer->LoadTextureFromMemoryAsync(texIdOrPath, image.image.data(), image.width, image.height, image.component, true);
								mat->albedoTexturePath = texIdOrPath;
								std::cout << "    Scheduled base color texture upload from memory (KHR_specGloss): " << texIdOrPath << std::endl;
							}
						}
					}
				}
			}
		}
	}

	// Heuristic pass: fill missing baseColor (albedo) by deriving from normal map filenames
	// Many Bistro materials have no baseColorTexture index. When that happens, try inferring
	// the base color from the normal map by replacing common suffixes like _ddna -> _d/_c/_diffuse/_basecolor/_albedo.
	for (auto &kv : materials)
	{
		auto     &material = kv.second;
		Material *mat      = material.get();
		if (!mat)
			continue;
		if (!mat->albedoTexturePath.empty())
			continue;        // already set
		// Only attempt if we have an external normal texture path to derive from
		if (mat->normalTexturePath.empty())
			continue;
		const std::string &normalPath = mat->normalTexturePath;
		// Skip embedded IDs like gltf_* which were already handled by memory uploads
		if (normalPath.rfind("gltf_", 0) == 0)
			continue;

		std::string candidateBase = normalPath;
		std::string normalLower   = candidateBase;
		for (auto &ch : normalLower)
			ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		size_t pos = normalLower.find("_ddna");
		if (pos == std::string::npos)
		{
			// Try a few additional normal suffixes seen in the wild
			pos = normalLower.find("_n");
		}
		if (pos != std::string::npos)
		{
			static const char *suffixes[] = {"_d", "_c", "_cm", "_diffuse", "_basecolor", "_albedo"};
			for (const char *suf : suffixes)
			{
				std::string cand = candidateBase;
				cand.replace(pos, normalLower[pos] == '_' && normalLower.compare(pos, 5, "_ddna") == 0 ? 5 : 2, suf);
				// Ensure the file exists before attempting to load
				if (std::filesystem::exists(cand))
				{
					// Schedule async load; libktx decoding will occur on renderer worker threads
					renderer->LoadTextureAsync(cand, true);
					mat->albedoTexturePath = cand;
					std::cout << "    Scheduled derived base color KTX2 load from normal sibling: " << cand << std::endl;
					break;
				}
			}
		}
	}

	// Secondary heuristic: scan glTF images for base color by material-name match when still missing
	for (auto &[materialName, materialPtr] : materials)
	{
		Material *mat = materialPtr.get();
		if (!mat)
			continue;
		if (!mat->albedoTexturePath.empty())
			continue;        // already resolved
		// Try to find an image URI that looks like the base color for this material
		std::string materialNameLower = materialName;
		std::ranges::transform(materialNameLower, materialNameLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		for (const auto &image : gltfModel.images)
		{
			if (image.uri.empty())
				continue;
			std::string imageUri      = image.uri;
			std::string imageUriLower = imageUri;
			std::ranges::transform(imageUriLower, imageUriLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			bool looksBase = imageUriLower.find("basecolor") != std::string::npos ||
			                 imageUriLower.find("albedo") != std::string::npos ||
			                 imageUriLower.find("diffuse") != std::string::npos;
			if (!looksBase)
				continue;
			bool nameMatches = imageUriLower.find(materialNameLower) != std::string::npos;
			if (!nameMatches)
			{
				// Best-effort: try prefix of image name before '_' against material name
				size_t underscore = imageUriLower.find('_');
				if (underscore != std::string::npos)
				{
					std::string prefix = imageUriLower.substr(0, underscore);
					nameMatches        = materialNameLower.find(prefix) != std::string::npos;
				}
			}
			if (!nameMatches)
				continue;

			std::string textureId = baseTexturePath + imageUri;        // use path string as ID for cache
			if (!image.image.empty())
			{
				renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
				mat->albedoTexturePath = textureId;
				std::cout << "    Scheduled base color upload from memory (by name): " << textureId << std::endl;
				break;
			}
			else
			{
				// Fallback: offload KTX2 file load to renderer threads
				renderer->LoadTextureAsync(textureId);
				mat->albedoTexturePath = textureId;
				std::cout << "    Scheduled base color KTX2 load from file (by name): " << textureId << std::endl;
				break;
			}
		}
	}

	// Process cameras from the GLTF file
	if (!gltfModel.cameras.empty())
	{
		std::cout << "Found " << gltfModel.cameras.size() << " camera(s) in GLTF file" << std::endl;

		for (size_t i = 0; i < gltfModel.cameras.size(); ++i)
		{
			const auto &gltfCamera = gltfModel.cameras[i];
			std::cout << "  Camera " << i << ": " << gltfCamera.name << std::endl;

			// Store camera data in the model for later use
			CameraData cameraData;
			cameraData.name = gltfCamera.name.empty() ? ("camera_" + std::to_string(i)) : gltfCamera.name;

			if (gltfCamera.type == "perspective")
			{
				cameraData.isPerspective = true;
				cameraData.fov           = static_cast<float>(gltfCamera.perspective.yfov);
				cameraData.aspectRatio   = static_cast<float>(gltfCamera.perspective.aspectRatio);
				cameraData.nearPlane     = static_cast<float>(gltfCamera.perspective.znear);
				cameraData.farPlane      = static_cast<float>(gltfCamera.perspective.zfar);
				std::cout << "    Perspective camera: FOV=" << cameraData.fov
				          << ", Aspect=" << cameraData.aspectRatio
				          << ", Near=" << cameraData.nearPlane
				          << ", Far=" << cameraData.farPlane << std::endl;
			}
			else if (gltfCamera.type == "orthographic")
			{
				cameraData.isPerspective    = false;
				cameraData.orthographicSize = static_cast<float>(gltfCamera.orthographic.ymag);
				cameraData.nearPlane        = static_cast<float>(gltfCamera.orthographic.znear);
				cameraData.farPlane         = static_cast<float>(gltfCamera.orthographic.zfar);
				std::cout << "    Orthographic camera: Size=" << cameraData.orthographicSize
				          << ", Near=" << cameraData.nearPlane
				          << ", Far=" << cameraData.farPlane << std::endl;
			}

			// Find the node that uses this camera to get transform information
			for (const auto &node : gltfModel.nodes)
			{
				if (node.camera == static_cast<int>(i))
				{
					// Extract transform from node
					if (node.translation.size() == 3)
					{
						cameraData.position = glm::vec3(
						    static_cast<float>(node.translation[0]),
						    static_cast<float>(node.translation[1]),
						    static_cast<float>(node.translation[2]));
					}

					if (node.rotation.size() == 4)
					{
						cameraData.rotation = glm::quat(
						    static_cast<float>(node.rotation[3]),        // w
						    static_cast<float>(node.rotation[0]),        // x
						    static_cast<float>(node.rotation[1]),        // y
						    static_cast<float>(node.rotation[2])         // z
						);
					}

					std::cout << "    Position: (" << cameraData.position.x << ", "
					          << cameraData.position.y << ", " << cameraData.position.z << ")" << std::endl;
					break;
				}
			}

			model->cameras.push_back(cameraData);
		}
	}

	// Process animations from the GLTF file
	if (!gltfModel.animations.empty())
	{
		std::cout << "Found " << gltfModel.animations.size() << " animation(s) in GLTF file" << std::endl;

		std::vector<Animation> parsedAnimations;
		parsedAnimations.reserve(gltfModel.animations.size());

		for (size_t animIdx = 0; animIdx < gltfModel.animations.size(); ++animIdx)
		{
			const auto &gltfAnim = gltfModel.animations[animIdx];

			Animation anim;
			anim.name = gltfAnim.name.empty() ? ("animation_" + std::to_string(animIdx)) : gltfAnim.name;

			// Parse samplers
			anim.samplers.reserve(gltfAnim.samplers.size());
			for (const auto &gltfSampler : gltfAnim.samplers)
			{
				AnimationSampler sampler;

				// Parse interpolation type
				if (gltfSampler.interpolation == "STEP")
				{
					sampler.interpolation = AnimationInterpolation::Step;
				}
				else if (gltfSampler.interpolation == "CUBICSPLINE")
				{
					sampler.interpolation = AnimationInterpolation::CubicSpline;
				}
				else
				{
					sampler.interpolation = AnimationInterpolation::Linear;
				}

				// Read input (time) accessor
				if (gltfSampler.input >= 0 && gltfSampler.input < static_cast<int>(gltfModel.accessors.size()))
				{
					const auto &inputAccessor   = gltfModel.accessors[gltfSampler.input];
					const auto &inputBufferView = gltfModel.bufferViews[inputAccessor.bufferView];
					const auto &inputBuffer     = gltfModel.buffers[inputBufferView.buffer];

					const float *inputData = reinterpret_cast<const float *>(
					    &inputBuffer.data[inputBufferView.byteOffset + inputAccessor.byteOffset]);

					sampler.inputTimes.resize(inputAccessor.count);
					for (size_t i = 0; i < inputAccessor.count; ++i)
					{
						sampler.inputTimes[i] = inputData[i];
					}
				}

				// Read output (value) accessor
				if (gltfSampler.output >= 0 && gltfSampler.output < static_cast<int>(gltfModel.accessors.size()))
				{
					const auto &outputAccessor   = gltfModel.accessors[gltfSampler.output];
					const auto &outputBufferView = gltfModel.bufferViews[outputAccessor.bufferView];
					const auto &outputBuffer     = gltfModel.buffers[outputBufferView.buffer];

					const float *outputData = reinterpret_cast<const float *>(
					    &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset]);

					// Determine number of floats per element based on accessor type
					size_t componentsPerElement = 1;
					if (outputAccessor.type == TINYGLTF_TYPE_VEC3)
					{
						componentsPerElement = 3;
					}
					else if (outputAccessor.type == TINYGLTF_TYPE_VEC4)
					{
						componentsPerElement = 4;
					}

					size_t totalFloats = outputAccessor.count * componentsPerElement;
					sampler.outputValues.resize(totalFloats);
					for (size_t i = 0; i < totalFloats; ++i)
					{
						sampler.outputValues[i] = outputData[i];
					}
				}

				anim.samplers.push_back(std::move(sampler));
			}

			// Parse channels
			anim.channels.reserve(gltfAnim.channels.size());
			for (const auto &gltfChannel : gltfAnim.channels)
			{
				AnimationChannel channel;
				channel.samplerIndex = gltfChannel.sampler;
				channel.targetNode   = gltfChannel.target_node;

				// Parse target path
				if (gltfChannel.target_path == "translation")
				{
					channel.path = AnimationPath::Translation;
				}
				else if (gltfChannel.target_path == "rotation")
				{
					channel.path = AnimationPath::Rotation;
				}
				else if (gltfChannel.target_path == "scale")
				{
					channel.path = AnimationPath::Scale;
				}
				else if (gltfChannel.target_path == "weights")
				{
					channel.path = AnimationPath::Weights;
				}

				anim.channels.push_back(channel);
			}

			std::cout << "  Animation '" << anim.name << "': "
			          << anim.samplers.size() << " samplers, "
			          << anim.channels.size() << " channels, "
			          << "duration=" << anim.GetDuration() << "s" << std::endl;

			parsedAnimations.push_back(std::move(anim));
		}

		model->SetAnimations(parsedAnimations);
		std::cout << "Loaded " << parsedAnimations.size() << " animations into model" << std::endl;
	}

	// Collect all animated node indices from parsed animations
	std::set<int> animatedNodeIndices;
	for (const auto &anim : model->GetAnimations())
	{
		for (const auto &channel : anim.channels)
		{
			if (channel.targetNode >= 0)
			{
				animatedNodeIndices.insert(channel.targetNode);
			}
		}
	}
	if (!animatedNodeIndices.empty())
	{
		std::cout << "[Animation] Found " << animatedNodeIndices.size() << " unique animated node(s)" << std::endl;
	}

	// Process scene hierarchy to get node transforms for meshes
	std::map<int, std::vector<glm::mat4>> meshInstanceTransforms;        // Map from mesh index to all instance transforms
	std::unordered_map<int, glm::mat4>    animatedNodeTransforms;        // Map from animated node index to world transform
	std::unordered_map<int, int>          animatedNodeMeshes;            // Map from animated node index to mesh index

	// Helper function to calculate transform matrix from the GLTF node
	auto calculateNodeTransform = [](const tinygltf::Node &node) -> glm::mat4 {
		glm::mat4 transform;

		// Apply matrix if present
		if (node.matrix.size() == 16)
		{
			// GLTF matrices are column-major, the same as GLM
			transform = glm::mat4(
			    node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
			    node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
			    node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
			    node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
		}
		else
		{
			// Build transform from TRS components
			glm::mat4 translation = glm::mat4(1.0f);
			glm::mat4 rotation    = glm::mat4(1.0f);
			glm::mat4 scale       = glm::mat4(1.0f);

			// Translation
			if (node.translation.size() == 3)
			{
				translation = glm::translate(glm::mat4(1.0f), glm::vec3(
				                                                  static_cast<float>(node.translation[0]),
				                                                  static_cast<float>(node.translation[1]),
				                                                  static_cast<float>(node.translation[2])));
			}

			// Rotation (quaternion)
			if (node.rotation.size() == 4)
			{
				glm::quat quat(
				    static_cast<float>(node.rotation[3]),        // w
				    static_cast<float>(node.rotation[0]),        // x
				    static_cast<float>(node.rotation[1]),        // y
				    static_cast<float>(node.rotation[2])         // z
				);
				rotation = glm::mat4_cast(quat);
			}

			// Scale
			if (node.scale.size() == 3)
			{
				scale = glm::scale(glm::mat4(1.0f), glm::vec3(
				                                        static_cast<float>(node.scale[0]),
				                                        static_cast<float>(node.scale[1]),
				                                        static_cast<float>(node.scale[2])));
			}

			// Combine: T * R * S
			transform = translation * rotation * scale;
		}

		return transform;
	};

	// Recursive function to traverse scene hierarchy
	std::function<void(int, const glm::mat4 &)> traverseNode = [&](int nodeIndex, const glm::mat4 &parentTransform) {
		if (nodeIndex < 0 || nodeIndex >= gltfModel.nodes.size())
		{
			return;
		}

		const tinygltf::Node &node = gltfModel.nodes[nodeIndex];

		// Calculate this node's transform
		glm::mat4 nodeTransform  = calculateNodeTransform(node);
		glm::mat4 worldTransform = parentTransform * nodeTransform;

		// If this node has a mesh, add the transform to the instances list
		if (node.mesh >= 0 && node.mesh < gltfModel.meshes.size())
		{
			meshInstanceTransforms[node.mesh].push_back(worldTransform);
		}

		// If this node is animated, capture its world transform and mesh reference
		if (animatedNodeIndices.contains(nodeIndex))
		{
			animatedNodeTransforms[nodeIndex] = worldTransform;
			if (node.mesh >= 0)
			{
				animatedNodeMeshes[nodeIndex] = node.mesh;
				std::cout << "[Animation] Captured transform for animated node " << nodeIndex
				          << " (" << node.name << ") with mesh " << node.mesh << std::endl;
			}
			else
			{
				std::cout << "[Animation] Captured transform for animated node " << nodeIndex
				          << " (" << node.name << ") - no mesh" << std::endl;
			}
		}

		// Recursively process children
		for (int childIndex : node.children)
		{
			traverseNode(childIndex, worldTransform);
		}
	};

	// Process all scenes (typically there's only one default scene)
	if (!gltfModel.scenes.empty())
	{
		int defaultScene = gltfModel.defaultScene >= 0 ? gltfModel.defaultScene : 0;
		if (defaultScene < gltfModel.scenes.size())
		{
			const tinygltf::Scene &scene = gltfModel.scenes[defaultScene];

			// Traverse all root nodes in the scene
			for (int rootNodeIndex : scene.nodes)
			{
				traverseNode(rootNodeIndex, glm::mat4(1.0f));
			}
		}
	}

	// Store animated node transforms in the model for use by AnimationComponent
	if (!animatedNodeTransforms.empty())
	{
		model->SetAnimatedNodeTransforms(animatedNodeTransforms);
		std::cout << "[Animation] Stored " << animatedNodeTransforms.size()
		          << " animated node transform(s) in model" << std::endl;
	}

	// Store animated node mesh mappings for linking geometry entities to animations
	if (!animatedNodeMeshes.empty())
	{
		model->SetAnimatedNodeMeshes(animatedNodeMeshes);
		std::cout << "[Animation] Stored " << animatedNodeMeshes.size()
		          << " animated node mesh mapping(s) in model" << std::endl;
	}

	std::map<std::string, MaterialMesh> geometryMaterialMeshMap;        // Map from geometry+material hash to unique MaterialMesh

	// Helper function to create a geometry hash for deduplication
	auto createGeometryHash = [](const tinygltf::Primitive &primitive, int materialIndex) -> std::string {
		std::string hash = "mat_" + std::to_string(materialIndex);

		// Add primitive attribute hashes to ensure unique geometry identification
		if (primitive.indices >= 0)
		{
			hash += "_idx_" + std::to_string(primitive.indices);
		}

		for (const auto &[attrName, type] : primitive.attributes)
		{
			hash += "_" + attrName + "_" + std::to_string(type);
		}

		return hash;
	};

	// Process all meshes with improved instancing support
	for (size_t meshIndex = 0; meshIndex < gltfModel.meshes.size(); ++meshIndex)
	{
		const auto &mesh = gltfModel.meshes[meshIndex];

		// Check if this mesh has instances
		auto                   instanceIt = meshInstanceTransforms.find(static_cast<int>(meshIndex));
		std::vector<glm::mat4> instances;

		if (instanceIt == meshInstanceTransforms.end() || instanceIt->second.empty())
		{
			instances.emplace_back(1.0f);        // Identity transform at origin
		}
		else
		{
			instances = instanceIt->second;
		}

		// Process each primitive (material group) in this mesh
		for (const auto &primitive : mesh.primitives)
		{
			// Get the material index for this primitive
			int materialIndex = primitive.material;
			if (materialIndex < 0)
			{
				materialIndex = -1;        // Use -1 for primitives without materials
			}

			// Create a unique geometry hash for this primitive and material combination
			std::string geometryHash = createGeometryHash(primitive, materialIndex);

			// Check if we already have this exact geometry and material combination
			if (!geometryMaterialMeshMap.contains(geometryHash))
			{
				// Create a new MaterialMesh for this unique geometry and material combination
				MaterialMesh materialMesh;
				materialMesh.materialIndex   = materialIndex;
				materialMesh.sourceMeshIndex = static_cast<int>(meshIndex);        // Track source mesh for animations

				// Set material name
				if (materialIndex >= 0 && materialIndex < gltfModel.materials.size())
				{
					const auto &gltfMaterial  = gltfModel.materials[materialIndex];
					materialMesh.materialName = gltfMaterial.name.empty() ?
					                                ("material_" + std::to_string(materialIndex)) :
					                                gltfMaterial.name;
				}
				else
				{
					materialMesh.materialName = "no_material";
				}

				geometryMaterialMeshMap[geometryHash] = materialMesh;
			}

			MaterialMesh &materialMesh = geometryMaterialMeshMap[geometryHash];

			// Only process geometry if this MaterialMesh is empty (first time processing this geometry)
			if (materialMesh.vertices.empty())
			{
				auto vertexOffsetInMaterialMesh = static_cast<uint32_t>(materialMesh.vertices.size());

				// Get indices for this primitive (your existing code is correct)
				if (primitive.indices >= 0)
				{
					const tinygltf::Accessor   &indexAccessor   = gltfModel.accessors[primitive.indices];
					const tinygltf::BufferView &indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
					const tinygltf::Buffer     &indexBuffer     = gltfModel.buffers[indexBufferView.buffer];
					const void                 *indexData       = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
					if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						const auto *buf = static_cast<const uint16_t *>(indexData);
						for (size_t i = 0; i < indexAccessor.count; ++i)
						{
							materialMesh.indices.push_back(buf[i] + vertexOffsetInMaterialMesh);
						}
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
					{
						const auto *buf = static_cast<const uint32_t *>(indexData);
						for (size_t i = 0; i < indexAccessor.count; ++i)
						{
							materialMesh.indices.push_back(buf[i] + vertexOffsetInMaterialMesh);
						}
					}
				}

				// --- START: FINAL SAFE AND CORRECT VERTEX LOADING ---

				// Get the position accessor, which defines the vertex count.
				auto posIt = primitive.attributes.find("POSITION");
				if (posIt == primitive.attributes.end())
					continue;
				const tinygltf::Accessor &posAccessor = gltfModel.accessors[posIt->second];

				// Get data pointers and strides for all available attributes ONCE before the loop.
				const tinygltf::BufferView &posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
				const tinygltf::Buffer     &buffer        = gltfModel.buffers[posBufferView.buffer];
				const unsigned char        *pPositions    = &buffer.data[posBufferView.byteOffset + posAccessor.byteOffset];
				const size_t                posByteStride = posBufferView.byteStride == 0 ? sizeof(glm::vec3) : posBufferView.byteStride;

				const unsigned char *pNormals         = nullptr;
				size_t               normalByteStride = 0;
				auto                 normalIt         = primitive.attributes.find("NORMAL");
				if (normalIt != primitive.attributes.end())
				{
					const tinygltf::Accessor   &normalAccessor   = gltfModel.accessors[normalIt->second];
					const tinygltf::BufferView &normalBufferView = gltfModel.bufferViews[normalAccessor.bufferView];
					pNormals                                     = &gltfModel.buffers[normalBufferView.buffer].data[normalBufferView.byteOffset + normalAccessor.byteOffset];
					normalByteStride                             = normalBufferView.byteStride == 0 ? sizeof(glm::vec3) : normalBufferView.byteStride;
				}

				const unsigned char *pTexCoords         = nullptr;
				size_t               texCoordByteStride = 0;
				auto                 texCoordIt         = primitive.attributes.find("TEXCOORD_0");
				if (texCoordIt != primitive.attributes.end())
				{
					const tinygltf::Accessor   &texCoordAccessor   = gltfModel.accessors[texCoordIt->second];
					const tinygltf::BufferView &texCoordBufferView = gltfModel.bufferViews[texCoordAccessor.bufferView];
					pTexCoords                                     = &gltfModel.buffers[texCoordBufferView.buffer].data[texCoordBufferView.byteOffset + texCoordAccessor.byteOffset];
					texCoordByteStride                             = texCoordBufferView.byteStride == 0 ? sizeof(glm::vec2) : texCoordBufferView.byteStride;
				}

				const unsigned char *pTangents         = nullptr;
				size_t               tangentByteStride = 0;
				auto                 tangentIt         = primitive.attributes.find("TANGENT");
				bool                 hasTangents       = (tangentIt != primitive.attributes.end());
				if (hasTangents)
				{
					const tinygltf::Accessor   &tangentAccessor   = gltfModel.accessors[tangentIt->second];
					const tinygltf::BufferView &tangentBufferView = gltfModel.bufferViews[tangentAccessor.bufferView];
					pTangents                                     = &gltfModel.buffers[tangentBufferView.buffer].data[tangentBufferView.byteOffset + tangentAccessor.byteOffset];
					tangentByteStride                             = tangentBufferView.byteStride == 0 ? sizeof(glm::vec4) : tangentBufferView.byteStride;
				}

				// Append vertices for this primitive preserving prior vertices
				size_t baseVertex = materialMesh.vertices.size();
				materialMesh.vertices.resize(baseVertex + posAccessor.count);

				// Use a SINGLE, SAFE loop to load all vertex data.
				for (size_t i = 0; i < posAccessor.count; ++i)
				{
					auto &[position, normal, texCoord, tangent] = materialMesh.vertices[baseVertex + i];

					position = *reinterpret_cast<const glm::vec3 *>(pPositions + i * posByteStride);

					if (pNormals)
					{
						normal = *reinterpret_cast<const glm::vec3 *>(pNormals + i * normalByteStride);
					}
					else
					{
						normal = glm::vec3(0.0f, 0.0f, 1.0f);
					}
					// Normalize normals to ensure consistent magnitude
					if (glm::dot(normal, normal) > 0.0f)
					{
						normal = glm::normalize(normal);
					}
					else
					{
						normal = glm::vec3(0.0f, 0.0f, 1.0f);
					}

					if (pTexCoords)
					{
						texCoord = *reinterpret_cast<const glm::vec2 *>(pTexCoords + i * texCoordByteStride);
					}
					else
					{
						texCoord = glm::vec2(0.0f, 0.0f);
					}

					if (hasTangents && pTangents)
					{
						// Load glTF tangent and ensure it is normalized and orthogonal to the normal.
						glm::vec4 t4 = *reinterpret_cast<const glm::vec4 *>(pTangents + i * tangentByteStride);
						glm::vec3 T  = glm::vec3(t4);
						// Normalize tangent and make it orthogonal to normal to avoid skewed TBN
						if (glm::dot(T, T) > 0.0f)
						{
							T = glm::normalize(T);
							T = glm::normalize(T - normal * glm::dot(normal, T));
						}
						else
						{
							T = glm::vec3(1.0f, 0.0f, 0.0f);
						}
						float w = (t4.w >= 0.0f) ? 1.0f : -1.0f;        // clamp handedness to +/-1
						tangent = glm::vec4(T, w);
					}
					else
					{
						// No tangents in source: use a safe default tangent (T=+X, handedness=+1)
						tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
					}
				}

				// AFTER the mesh is fully built, generate tangents via MikkTSpace ONLY if the source mesh lacks glTF tangents.
				if (!hasTangents)
				{
					if (pNormals && pTexCoords && !materialMesh.indices.empty())
					{
						MikkTSpaceInterface mikkInterface;
						mikkInterface.vertices = &materialMesh.vertices;
						mikkInterface.indices  = &materialMesh.indices;

						SMikkTSpaceInterface sm_interface{};
						sm_interface.m_getNumFaces          = getNumFaces;
						sm_interface.m_getNumVerticesOfFace = getNumVerticesOfFace;
						sm_interface.m_getPosition          = getPosition;
						sm_interface.m_getNormal            = getNormal;
						sm_interface.m_getTexCoord          = getTexCoord;
						sm_interface.m_setTSpaceBasic       = setTSpaceBasic;

						SMikkTSpaceContext mikk_context{};
						mikk_context.m_pInterface = &sm_interface;
						mikk_context.m_pUserData  = &mikkInterface;

						if (genTangSpaceDefault(&mikk_context))
						{
							std::cout << "      Generated tangents (MikkTSpace) for material: " << materialMesh.materialName << std::endl;
						}
						else
						{
							std::cerr << "      Failed to generate tangents for material: " << materialMesh.materialName << std::endl;
						}
					}
					else
					{
						std::cout << "      Skipping tangent generation (missing normals, UVs, or indices) for material: " << materialMesh.materialName << std::endl;
					}
				}
				else
				{
					std::cout << "      Using glTF-provided tangents for material: " << materialMesh.materialName << std::endl;
				}
				// --- END: FINAL SAFE AND CORRECT VERTEX LOADING ---
			}

			// Add all instances to this MaterialMesh (both new and existing geometry)
			for (const glm::mat4 &instanceTransform : instances)
			{
				materialMesh.AddInstance(instanceTransform, static_cast<uint32_t>(materialIndex));
			}
		}
	}

	// Convert geometry-based material mesh map to vector
	std::vector<MaterialMesh> modelMaterialMeshes;
	for (auto &kv : geometryMaterialMeshMap)
	{
		modelMaterialMeshes.push_back(kv.second);
	}

	// Process texture loading for each MaterialMesh
	std::vector<Vertex>   combinedVertices;
	std::vector<uint32_t> combinedIndices;

	// Process texture loading for each MaterialMesh
	for (auto &materialMesh : modelMaterialMeshes)
	{
		int materialIndex = materialMesh.materialIndex;

		// Get ALL texture paths for this material (same as ParseGLTFDataOnly)
		if (materialIndex >= 0 && materialIndex < gltfModel.materials.size())
		{
			const auto &gltfMaterial = gltfModel.materials[materialIndex];

			// Extract base color texture
			if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
			{
				int texIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
				if (texIndex < gltfModel.textures.size())
				{
					const auto &texture    = gltfModel.textures[texIndex];
					int         imageIndex = -1;
					if (texture.source >= 0 && texture.source < gltfModel.images.size())
					{
						imageIndex = texture.source;
					}
					else
					{
						auto extIt = texture.extensions.find("KHR_texture_basisu");
						if (extIt != texture.extensions.end())
						{
							const tinygltf::Value &ext = extIt->second;
							if (ext.Has("source") && ext.Get("source").IsInt())
							{
								int src = ext.Get("source").Get<int>();
								if (src >= 0 && src < static_cast<int>(gltfModel.images.size()))
								{
									imageIndex = src;
								}
							}
						}
					}
					if (imageIndex >= 0)
					{
						std::string textureId             = "gltf_baseColor_" + std::to_string(texIndex);
						materialMesh.baseColorTexturePath = textureId;
						materialMesh.texturePath          = textureId;        // Keep for backward compatibility (now baseColor‑tagged)

						// Load texture data (embedded or external) with caching
						const auto &image = gltfModel.images[imageIndex];
						if (!image.image.empty())
						{
							if (!loadedTextures.contains(textureId))
							{
								renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component, true);
								loadedTextures.insert(textureId);
								std::cout << "      Scheduled baseColor texture upload: " << textureId
								          << " (" << image.width << "x" << image.height << ")" << std::endl;
							}
							else
							{
								std::cout << "      Using cached baseColor texture: " << textureId << std::endl;
							}
						}
						else
						{
							std::cerr << "      Warning: No decoded bytes for baseColor texture index " << texIndex << std::endl;
						}
					}
				}
			}
			else
			{
				// Since texture indices are -1, try to find external texture files by material name
				std::string materialName = materialMesh.materialName;

				// Look for external texture files that match this specific material (case-insensitive)
				for (const auto &image : gltfModel.images)
				{
					if (!image.uri.empty())
					{
						std::string imageUri = image.uri;
						// Lowercase copies for robust matching
						std::string imageUriLower = imageUri;
						std::ranges::transform(imageUriLower, imageUriLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
						std::string materialNameLower = materialName;
						std::ranges::transform(materialNameLower, materialNameLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

						// Check if this image belongs to this specific material based on naming patterns
						// Look for basecolor/albedo/diffuse textures that match the material name
						if ((imageUriLower.find("basecolor") != std::string::npos ||
						     imageUriLower.find("albedo") != std::string::npos ||
						     imageUriLower.find("diffuse") != std::string::npos) &&
						    (imageUriLower.find(materialNameLower) != std::string::npos ||
						     materialNameLower.find(imageUriLower.substr(0, imageUriLower.find('_'))) != std::string::npos))
						{
							// Use the relative path from the GLTF directory
							std::string textureId = baseTexturePath + imageUri;
							if (!image.image.empty())
							{
								renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
								materialMesh.baseColorTexturePath = textureId;
								materialMesh.texturePath          = textureId;
								std::cout << "      Scheduled baseColor upload from memory (heuristic): " << textureId << std::endl;
							}
							else
							{
								// Fallback: offload KTX2 file load to renderer worker threads
								renderer->LoadTextureAsync(textureId, true);
								materialMesh.baseColorTexturePath = textureId;
								materialMesh.texturePath          = textureId;
								std::cout << "      Scheduled baseColor KTX2 load from file (heuristic): " << textureId << std::endl;
							}
							break;
						}
					}
				}
			}

			// Extract normal texture
			if (gltfMaterial.normalTexture.index >= 0)
			{
				int texIndex = gltfMaterial.normalTexture.index;
				if (texIndex < gltfModel.textures.size())
				{
					const auto &texture = gltfModel.textures[texIndex];
					if (texture.source >= 0 && texture.source < gltfModel.images.size())
					{
						std::string textureId          = "gltf_texture_" + std::to_string(texIndex);
						materialMesh.normalTexturePath = textureId;

						// Load texture data (embedded or external)
						const auto &image = gltfModel.images[texture.source];
						if (!image.image.empty())
						{
							// Load embedded texture data
							renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
							std::cout << "      Scheduled embedded normal texture: " << textureId
							          << " (" << image.width << "x" << image.height << ")" << std::endl;
						}
						else if (!image.uri.empty())
						{
							// Fallback: offload KTX2 normal map load to renderer worker threads
							std::string filePath = baseTexturePath + image.uri;
							renderer->RegisterTextureAlias(textureId, filePath);
							renderer->LoadTextureAsync(filePath);
							materialMesh.normalTexturePath = textureId;
							std::cout << "    Scheduled normal KTX2 load from file: " << filePath << " (alias for " << textureId << ")" << std::endl;
						}
						else
						{
							std::cerr << "    Warning: No decoded bytes for normal texture index " << texIndex << std::endl;
						}
					}
				}
			}
			else
			{
				// Heuristic: search images for a normal texture for this material and load from memory
				std::string materialName = materialMesh.materialName;
				for (const auto &image : gltfModel.images)
				{
					if (!image.uri.empty())
					{
						std::string imageUri = image.uri;
						if (imageUri.find("Normal") != std::string::npos &&
						    (imageUri.find(materialName) != std::string::npos ||
						     materialName.find(imageUri.substr(0, imageUri.find('_'))) != std::string::npos))
						{
							std::string textureId = baseTexturePath + imageUri;
							if (!image.image.empty())
							{
								renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
								materialMesh.normalTexturePath = textureId;
								std::cout << "      Scheduled normal upload from memory (heuristic): " << textureId << std::endl;
							}
							else
							{
								std::cerr << "      Warning: Heuristic normal image has no decoded bytes: " << imageUri << std::endl;
							}
							break;
						}
					}
				}
			}

			// Extract metallic-roughness texture
			if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
			{
				int texIndex = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
				if (texIndex < gltfModel.textures.size())
				{
					const auto &texture = gltfModel.textures[texIndex];
					if (texture.source >= 0 && texture.source < gltfModel.images.size())
					{
						std::string textureId                     = "gltf_texture_" + std::to_string(texIndex);
						materialMesh.metallicRoughnessTexturePath = textureId;

						// Load texture data (embedded or external)
						const auto &image = gltfModel.images[texture.source];
						if (!image.image.empty())
						{
							renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
							materialMesh.metallicRoughnessTexturePath = textureId;
							std::cout << "      Scheduled metallic-roughness texture upload: " << textureId
							          << " (" << image.width << "x" << image.height << ")" << std::endl;
						}
						else
						{
							std::cerr << "      Warning: No decoded bytes for metallic-roughness texture index " << texIndex << std::endl;
						}
					}
				}
			}
			else
			{
				// Look for external metallic-roughness texture files that match this specific material
				std::string materialName = materialMesh.materialName;
				for (const auto &image : gltfModel.images)
				{
					if (!image.uri.empty())
					{
						std::string imageUri = image.uri;
						if ((imageUri.find("Metallic") != std::string::npos ||
						     imageUri.find("Roughness") != std::string::npos ||
						     imageUri.find("Specular") != std::string::npos) &&
						    (imageUri.find(materialName) != std::string::npos ||
						     materialName.find(imageUri.substr(0, imageUri.find('_'))) != std::string::npos))
						{
							std::string texturePath                   = baseTexturePath + imageUri;
							materialMesh.metallicRoughnessTexturePath = texturePath;
							std::cout << "      Found external metallic-roughness texture for " << materialName << ": " << texturePath << std::endl;
							break;
						}
					}
				}
			}

			// Extract occlusion texture
			if (gltfMaterial.occlusionTexture.index >= 0)
			{
				int texIndex = gltfMaterial.occlusionTexture.index;
				if (texIndex < gltfModel.textures.size())
				{
					const auto &texture = gltfModel.textures[texIndex];
					if (texture.source >= 0 && texture.source < gltfModel.images.size())
					{
						std::string textureId             = "gltf_texture_" + std::to_string(texIndex);
						materialMesh.occlusionTexturePath = textureId;

						// Load texture data (embedded or external)
						const auto &image = gltfModel.images[texture.source];
						if (!image.image.empty())
						{
							if (renderer->LoadTextureFromMemory(textureId, image.image.data(),
							                                    image.width, image.height, image.component))
							{
								materialMesh.occlusionTexturePath = textureId;
								std::cout << "      Loaded occlusion texture from memory: " << textureId
								          << " (" << image.width << "x" << image.height << ")" << std::endl;
							}
							else
							{
								std::cerr << "      Failed to load occlusion texture from memory: " << textureId << std::endl;
							}
						}
						else
						{
							std::cerr << "      Warning: No decoded bytes for occlusion texture index " << texIndex << std::endl;
						}
					}
				}
			}
			else
			{
				// Heuristic: search images for an occlusion texture for this material and load from memory
				std::string materialName = materialMesh.materialName;
				for (const auto &image : gltfModel.images)
				{
					if (!image.uri.empty())
					{
						std::string imageUri = image.uri;
						if ((imageUri.find("Occlusion") != std::string::npos ||
						     imageUri.find("AO") != std::string::npos) &&
						    (imageUri.find(materialName) != std::string::npos ||
						     materialName.find(imageUri.substr(0, imageUri.find('_'))) != std::string::npos))
						{
							std::string textureId = baseTexturePath + imageUri;
							if (!image.image.empty())
							{
								renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
								materialMesh.occlusionTexturePath = textureId;
								std::cout << "      Scheduled occlusion upload from memory (heuristic): " << textureId << std::endl;
							}
							else
							{
								std::cerr << "      Warning: Heuristic occlusion image has no decoded bytes: " << imageUri << std::endl;
							}
							break;
						}
					}
				}
			}

			// Extract emissive texture
			if (gltfMaterial.emissiveTexture.index >= 0)
			{
				int texIndex = gltfMaterial.emissiveTexture.index;
				if (texIndex < gltfModel.textures.size())
				{
					const auto &texture = gltfModel.textures[texIndex];
					if (texture.source >= 0 && texture.source < gltfModel.images.size())
					{
						std::string textureId            = "gltf_texture_" + std::to_string(texIndex);
						materialMesh.emissiveTexturePath = textureId;

						// Load texture data (embedded or external)
						const auto &image = gltfModel.images[texture.source];
						if (!image.image.empty())
						{
							// Load embedded texture data
							renderer->LoadTextureFromMemoryAsync(textureId, image.image.data(), image.width, image.height, image.component);
							std::cout << "      Scheduled embedded emissive texture: " << textureId
							          << " (" << image.width << "x" << image.height << ")" << std::endl;
						}
						else if (!image.uri.empty())
						{
							// Record external texture file path (loaded later by renderer)
							std::string texturePath          = baseTexturePath + image.uri;
							materialMesh.emissiveTexturePath = texturePath;
							std::cout << "      External emissive texture path: " << texturePath << std::endl;
						}
					}
				}
			}
			else
			{
				// Look for external emissive texture files that match this specific material
				std::string materialName = materialMesh.materialName;
				for (const auto &image : gltfModel.images)
				{
					if (!image.uri.empty())
					{
						std::string imageUri = image.uri;
						if ((imageUri.find("Emissive") != std::string::npos ||
						     imageUri.find("Emission") != std::string::npos) &&
						    (imageUri.find(materialName) != std::string::npos ||
						     materialName.find(imageUri.substr(0, imageUri.find('_'))) != std::string::npos))
						{
							std::string texturePath          = baseTexturePath + imageUri;
							materialMesh.emissiveTexturePath = texturePath;
							std::cout << "      Found external emissive texture for " << materialName << ": " << texturePath << std::endl;
							break;
						}
					}
				}
			}
		}

		// Add to combined mesh for backward compatibility (keep vertices in an original coordinate system)
		if (!materialMesh.instances.empty())
		{
			size_t vertexOffset = combinedVertices.size();

			// Instance transforms should be handled by the instancing system, not applied to vertex data
			for (const auto &vertex : materialMesh.vertices)
			{
				// Use vertices as-is without any transformation
				combinedVertices.push_back(vertex);
			}

			for (uint32_t index : materialMesh.indices)
			{
				combinedIndices.push_back(index + vertexOffset);
			}
		}
	}

	// Store material meshes for this model
	materialMeshes[filename] = modelMaterialMeshes;

	// Set the combined mesh data in the model for backward compatibility
	model->SetVertices(combinedVertices);
	model->SetIndices(combinedIndices);

	// Extract lights from the GLTF model
	std::cout << "Extracting lights from GLTF model..." << std::endl;

	// Extract punctual lights (KHR_lights_punctual extension)
	if (ExtractPunctualLights(gltfModel, filename))
	{
		std::cerr << "Warning: Failed to extract punctual lights from " << filename << std::endl;
	}

	std::cout << "GLTF model loaded successfully with " << combinedVertices.size() << " vertices and " << combinedIndices.size() << " indices" << std::endl;
	return true;
}

std::vector<ExtractedLight> ModelLoader::GetExtractedLights(const std::string &modelName) const
{
	std::vector<ExtractedLight> lights;

	// First, try to get punctual lights from the extracted lights storage
	auto lightIt = extractedLights.find(modelName);
	if (lightIt != extractedLights.end())
	{
		lights = lightIt->second;
		std::cout << "Found " << lights.size() << " punctual lights for model: " << modelName << std::endl;
	}

	// Now extract emissive materials as light sources
	auto materialMeshIt = materialMeshes.find(modelName);
	if (materialMeshIt != materialMeshes.end())
	{
		for (const auto &materialMesh : materialMeshIt->second)
		{
			// Get the material for this mesh
			auto materialIt = materials.find(materialMesh.materialName);
			if (materialIt != materials.end())
			{
				const Material *material = materialIt->second.get();

				// Check if this material has emissive properties (no threshold filtering)
				float emissiveIntensity = glm::length(material->emissive) * material->emissiveStrength;
				if (emissiveIntensity >= 0.1f)
				{
					// Calculate the center position and an approximate size of the emissive surface
					glm::vec3 center(0.0f);
					glm::vec3 minB(std::numeric_limits<float>::max());
					glm::vec3 maxB(-std::numeric_limits<float>::max());
					if (!materialMesh.vertices.empty())
					{
						for (const auto &vertex : materialMesh.vertices)
						{
							center += vertex.position;
							minB = glm::min(minB, vertex.position);
							maxB = glm::max(maxB, vertex.position);
						}
						center /= static_cast<float>(materialMesh.vertices.size());
					}
					glm::vec3 extent    = glm::max(maxB - minB, glm::vec3(0.0f));
					float     diag      = glm::length(extent);
					float     baseRange = std::max(0.5f * diag, 0.25f);        // base range in local units

					// Calculate a reasonable direction (average normal of the surface)
					glm::vec3 avgNormal(0.0f);
					if (!materialMesh.vertices.empty())
					{
						for (const auto &vertex : materialMesh.vertices)
						{
							avgNormal += vertex.normal;
						}
						avgNormal = glm::normalize(avgNormal / static_cast<float>(materialMesh.vertices.size()));
					}
					else
					{
						avgNormal = glm::vec3(0.0f, -1.0f, 0.0f);        // Default downward direction
					}

					// Create emissive light(s) transformed by each instance's model matrix
					if (!materialMesh.instances.empty())
					{
						for (const auto &inst : materialMesh.instances)
						{
							glm::mat4 M           = inst.getModelMatrix();
							glm::vec3 worldCenter = glm::vec3(M * glm::vec4(center, 1.0f));
							glm::mat3 normalMat   = glm::transpose(glm::inverse(glm::mat3(M)));
							glm::vec3 worldNormal = glm::normalize(normalMat * avgNormal);

							// Estimate a uniform scale factor from the instance transform
							float sx   = glm::length(glm::vec3(M[0]));
							float sy   = glm::length(glm::vec3(M[1]));
							float sz   = glm::length(glm::vec3(M[2]));
							float sMax = std::max(sx, std::max(sy, sz));
							// Slightly conservative halo; avoid massive ranges that wash out the scene
							float worldRange = baseRange * std::max(1.0f, sMax) * 1.25f;

							ExtractedLight emissiveLight;
							emissiveLight.type     = ExtractedLight::Type::Emissive;
							emissiveLight.position = worldCenter;
							// Separate chroma from intensity to avoid double-powering color and intensity
							glm::vec3 chroma    = material->emissive;
							float     chromaMag = glm::length(chroma);
							emissiveLight.color = (chromaMag > 1e-6f) ? (chroma / chromaMag) : chroma;
							float strength      = hasEmissiveStrengthExtension ? material->emissiveStrength : 1.0f;
							// Use a surface-area proxy from local bounds (diag^2) scaled by instance size, not range^2
							float areaProxy    = std::max(diag * diag * std::max(1.0f, sMax), 0.01f);
							float intensityRaw = strength * chromaMag * areaProxy * 0.08f;        // conservative scalar
							// Clamp to a reasonable band to avoid blowing out exposure
							emissiveLight.intensity      = glm::clamp(intensityRaw, 0.25f, 50.0f);
							emissiveLight.range          = worldRange;
							emissiveLight.sourceMaterial = material->GetName();
							emissiveLight.direction      = worldNormal;

							lights.push_back(emissiveLight);

							std::cout << "Created emissive light from material '" << material->GetName()
							          << "' at world position (" << worldCenter.x << ", " << worldCenter.y << ", " << worldCenter.z
							          << ") with intensity " << emissiveIntensity << std::endl;
						}
					}
					else
					{
						// No explicit instances; use identity transform
						ExtractedLight emissiveLight;
						emissiveLight.type     = ExtractedLight::Type::Emissive;
						emissiveLight.position = center;
						// Separate chroma from intensity
						glm::vec3 chroma             = material->emissive;
						float     chromaMag          = glm::length(chroma);
						emissiveLight.color          = (chromaMag > 1e-6f) ? (chroma / chromaMag) : chroma;
						float strength               = hasEmissiveStrengthExtension ? material->emissiveStrength : 1.0f;
						float worldRange             = baseRange * 1.25f;
						float areaProxy              = std::max(diag * diag, 0.01f);
						float intensityRaw           = strength * chromaMag * areaProxy * 0.08f;
						emissiveLight.intensity      = glm::clamp(intensityRaw, 0.25f, 50.0f);
						emissiveLight.range          = worldRange;
						emissiveLight.sourceMaterial = material->GetName();
						emissiveLight.direction      = avgNormal;

						lights.push_back(emissiveLight);

						std::cout << "Created emissive light from material '" << material->GetName()
						          << "' at position (" << center.x << ", " << center.y << ", " << center.z
						          << ") with intensity " << emissiveIntensity << std::endl;
					}
				}
			}
		}
	}

	std::cout << "Total lights extracted for model '" << modelName << "': " << lights.size()
	          << " (including emissive-derived lights)" << std::endl;

	return lights;
}

const std::vector<MaterialMesh> &ModelLoader::GetMaterialMeshes(const std::string &modelName) const
{
	auto it = materialMeshes.find(modelName);
	if (it != materialMeshes.end())
	{
		return it->second;
	}
	// Return a static empty vector to avoid creating temporary objects.
	static const std::vector<MaterialMesh> emptyVector;
	return emptyVector;
}

Material *ModelLoader::GetMaterial(const std::string &materialName) const
{
	auto it = materials.find(materialName);
	if (it != materials.end())
	{
		return it->second.get();
	}
	return nullptr;
}

const std::vector<Animation> &ModelLoader::GetAnimations(const std::string &modelName) const
{
	auto it = models.find(modelName);
	if (it != models.end() && it->second)
	{
		return it->second->GetAnimations();
	}
	// Return a static empty vector to avoid creating temporary objects.
	static const std::vector<Animation> emptyVector;
	return emptyVector;
}

bool ModelLoader::ExtractPunctualLights(const tinygltf::Model &gltfModel, const std::string &modelName)
{
	std::cout << "Extracting punctual lights from model: " << modelName << std::endl;

	std::vector<ExtractedLight> lights;

	// Check if the model has the KHR_lights_punctual extension
	auto extensionIt = gltfModel.extensions.find("KHR_lights_punctual");
	if (extensionIt != gltfModel.extensions.end())
	{
		std::cout << "  Found KHR_lights_punctual extension" << std::endl;

		// Parse the punctual lights from the extension
		const tinygltf::Value &extension = extensionIt->second;
		if (extension.Has("lights") && extension.Get("lights").IsArray())
		{
			const tinygltf::Value::Array &lightsArray = extension.Get("lights").Get<tinygltf::Value::Array>();

			for (size_t i = 0; i < lightsArray.size(); ++i)
			{
				const tinygltf::Value &lightValue = lightsArray[i];
				if (!lightValue.IsObject())
					continue;

				ExtractedLight light;

				// Parse light type
				if (lightValue.Has("type") && lightValue.Get("type").IsString())
				{
					std::string type = lightValue.Get("type").Get<std::string>();
					if (type == "directional")
					{
						light.type = ExtractedLight::Type::Directional;
					}
					else if (type == "point")
					{
						light.type = ExtractedLight::Type::Point;
					}
					else if (type == "spot")
					{
						light.type = ExtractedLight::Type::Spot;
					}
				}

				// Parse light color
				if (lightValue.Has("color") && lightValue.Get("color").IsArray())
				{
					const tinygltf::Value::Array &colorArray = lightValue.Get("color").Get<tinygltf::Value::Array>();
					if (colorArray.size() >= 3)
					{
						light.color = glm::vec3(
						    colorArray[0].IsNumber() ? static_cast<float>(colorArray[0].Get<double>()) : 1.0f,
						    colorArray[1].IsNumber() ? static_cast<float>(colorArray[1].Get<double>()) : 1.0f,
						    colorArray[2].IsNumber() ? static_cast<float>(colorArray[2].Get<double>()) : 1.0f);
					}
				}

				// Parse light intensity
				if (lightValue.Has("intensity") && lightValue.Get("intensity").IsNumber())
				{
					light.intensity = static_cast<float>(lightValue.Get("intensity").Get<double>()) * LIGHT_SCALE_FACTOR;
				}

				// Parse light range (for point and spotlights)
				if (lightValue.Has("range") && lightValue.Get("range").IsNumber())
				{
					light.range = static_cast<float>(lightValue.Get("range").Get<double>());
				}

				// Parse spotlights specific parameters
				if (light.type == ExtractedLight::Type::Spot && lightValue.Has("spot"))
				{
					const tinygltf::Value &spotValue = lightValue.Get("spot");
					if (spotValue.Has("innerConeAngle") && spotValue.Get("innerConeAngle").IsNumber())
					{
						light.innerConeAngle = static_cast<float>(spotValue.Get("innerConeAngle").Get<double>());
					}
					if (spotValue.Has("outerConeAngle") && spotValue.Get("outerConeAngle").IsNumber())
					{
						light.outerConeAngle = static_cast<float>(spotValue.Get("outerConeAngle").Get<double>());
					}
				}

				lights.push_back(light);
				std::cout << "    Parsed punctual light " << i << ": type=" << static_cast<int>(light.type)
				          << ", intensity=" << light.intensity << std::endl;
			}
		}
	}
	else
	{
		std::cout << "  No KHR_lights_punctual extension found" << std::endl;
	}

	// Compute world transforms for all nodes in the default scene
	std::vector<glm::mat4> nodeWorldTransforms(gltfModel.nodes.size(), glm::mat4(1.0f));

	auto calcLocal = [](const tinygltf::Node &n) -> glm::mat4 {
		// If matrix is provided, use it
		if (n.matrix.size() == 16)
		{
			glm::mat4 m(1.0f);
			for (int r = 0; r < 4; ++r)
			{
				for (int c = 0; c < 4; ++c)
				{
					m[c][r] = static_cast<float>(n.matrix[r * 4 + c]);
				}
			}
			return m;
		}
		// Otherwise compose TRS
		glm::mat4 T(1.0f), R(1.0f), S(1.0f);
		if (n.translation.size() == 3)
		{
			T = glm::translate(glm::mat4(1.0f), glm::vec3(
			                                        static_cast<float>(n.translation[0]),
			                                        static_cast<float>(n.translation[1]),
			                                        static_cast<float>(n.translation[2])));
		}
		if (n.rotation.size() == 4)
		{
			glm::quat q(
			    static_cast<float>(n.rotation[3]),
			    static_cast<float>(n.rotation[0]),
			    static_cast<float>(n.rotation[1]),
			    static_cast<float>(n.rotation[2]));
			R = glm::mat4_cast(q);
		}
		if (n.scale.size() == 3)
		{
			S = glm::scale(glm::mat4(1.0f), glm::vec3(
			                                    static_cast<float>(n.scale[0]),
			                                    static_cast<float>(n.scale[1]),
			                                    static_cast<float>(n.scale[2])));
		}
		return T * R * S;
	};

	std::function<void(int, const glm::mat4 &)> traverseNode = [&](int nodeIndex, const glm::mat4 &parent) {
		if (nodeIndex < 0 || nodeIndex >= static_cast<int>(gltfModel.nodes.size()))
			return;
		const tinygltf::Node &n        = gltfModel.nodes[nodeIndex];
		glm::mat4             local    = calcLocal(n);
		glm::mat4             world    = parent * local;
		nodeWorldTransforms[nodeIndex] = world;
		for (int child : n.children)
		{
			traverseNode(child, world);
		}
	};

	if (!gltfModel.scenes.empty())
	{
		int sceneIndex = gltfModel.defaultScene >= 0 ? gltfModel.defaultScene : 0;
		if (sceneIndex < static_cast<int>(gltfModel.scenes.size()))
		{
			const tinygltf::Scene &scene = gltfModel.scenes[sceneIndex];
			for (int root : scene.nodes)
			{
				traverseNode(root, glm::mat4(1.0f));
			}
		}
	}
	else
	{
		// Fallback: traverse all nodes as roots
		for (int i = 0; i < static_cast<int>(gltfModel.nodes.size()); ++i)
		{
			traverseNode(i, glm::mat4(1.0f));
		}
	}

	// Now assign positions and directions using world transforms
	for (size_t nodeIndex = 0; nodeIndex < gltfModel.nodes.size(); ++nodeIndex)
	{
		const auto &node = gltfModel.nodes[nodeIndex];
		if (node.extensions.contains("KHR_lights_punctual"))
		{
			const tinygltf::Value &nodeExtension = node.extensions.at("KHR_lights_punctual");
			if (nodeExtension.Has("light") && nodeExtension.Get("light").IsInt())
			{
				int lightIndex = nodeExtension.Get("light").Get<int>();
				if (lightIndex >= 0 && lightIndex < static_cast<int>(lights.size()))
				{
					const glm::mat4 &W = nodeWorldTransforms[nodeIndex];
					// Position from world transform origin
					glm::vec3 pos               = glm::vec3(W * glm::vec4(0, 0, 0, 1));
					lights[lightIndex].position = pos;

					// Direction for directional/spot: transform -Z
					if (lights[lightIndex].type == ExtractedLight::Type::Directional ||
					    lights[lightIndex].type == ExtractedLight::Type::Spot)
					{
						glm::mat3 rot                = glm::mat3(W);
						glm::vec3 dir                = glm::normalize(rot * glm::vec3(0.0f, 0.0f, -1.0f));
						lights[lightIndex].direction = dir;
					}

					std::cout << "    Light " << lightIndex << " positioned at ("
					          << lights[lightIndex].position.x << ", "
					          << lights[lightIndex].position.y << ", "
					          << lights[lightIndex].position.z << ")" << std::endl;
				}
			}
		}
	}

	// Store the extracted lights
	extractedLights[modelName] = lights;

	std::cout << "  Extracted " << lights.size() << " total lights from model" << std::endl;
	return lights.empty();
}
