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
#include "scene_loading.h"
#include "animation_component.h"
#include "camera_component.h"
#include "engine.h"
#include "mesh_component.h"
#include "transform_component.h"
#include <filesystem>
#include <glm/gtx/matrix_decompose.hpp>
#include <iostream>

/**
 * @brief Calculate bounding box dimensions for a MaterialMesh.
 * @param materialMesh The MaterialMesh to analyze.
 * @return The size of the bounding box (max - min for each axis).
 */
glm::vec3 CalculateBoundingBoxSize(const MaterialMesh &materialMesh)
{
	if (materialMesh.vertices.empty())
	{
		return glm::vec3(0.0f);
	}

	glm::vec3 minBounds = materialMesh.vertices[0].position;
	glm::vec3 maxBounds = materialMesh.vertices[0].position;

	for (const auto &vertex : materialMesh.vertices)
	{
		minBounds = glm::min(minBounds, vertex.position);
		maxBounds = glm::max(maxBounds, vertex.position);
	}

	return maxBounds - minBounds;
}

/**
 * @brief Load a GLTF model synchronously on the main thread.
 * @return success or fail on loading the GLTF model.
 * @param engine The engine to create entities in.
 * @param modelPath The path to the GLTF model file.
 * @param position The position to place the model (default: origin with slight Y offset).
 * @param rotation The rotation to apply to the model (default: no rotation).
 * @param scale The scale to apply to the model (default: unit scale).
 */
bool LoadGLTFModel(Engine *engine, const std::string &modelPath,
                   const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
{
	// Get the model loader and renderer
	ModelLoader *modelLoader = engine->GetModelLoader();
	Renderer    *renderer    = engine->GetRenderer();

	if (!modelLoader || !renderer)
	{
		std::cerr << "Error: ModelLoader or Renderer is null" << std::endl;
		if (renderer)
		{
			renderer->SetLoading(false);
		}
		return false;
	}
	// Ensure loading flag is cleared on any exit from this function
	struct LoadingGuard
	{
		Renderer *r;
		~LoadingGuard()
		{
			r->SetLoading(false);
		}
	} loadingGuard{renderer};

	// Extract model name from file path for entity naming
	std::filesystem::path modelFilePath(modelPath);
	std::string           modelName = modelFilePath.stem().string();        // Get filename without extension

	try
	{
		// Load the complete GLTF model with all textures and lighting on the main thread
		Model *loadedModel = modelLoader->LoadGLTF(modelPath);
		if (!loadedModel)
		{
			std::cerr << "Failed to load GLTF model: " << modelPath << std::endl;
			return false;
		}

		std::cout << "Successfully loaded GLTF model with all textures and lighting: " << modelPath << std::endl;

		// Extract lights from the model and transform them to world space
		std::vector<ExtractedLight> extractedLights = modelLoader->GetExtractedLights(modelPath);

		// Create a transformation matrix from position, rotation, and scale
		glm::mat4 transformMatrix = glm::mat4(1.0f);
		transformMatrix           = glm::translate(transformMatrix, position);
		transformMatrix           = glm::rotate(transformMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		transformMatrix           = glm::rotate(transformMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		transformMatrix           = glm::rotate(transformMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		transformMatrix           = glm::scale(transformMatrix, scale);

		// Transform all light positions from local model space to world space
		// Also transform the light direction (for directional lights)
		glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(transformMatrix)));
		for (auto &light : extractedLights)
		{
			glm::vec4 worldPos = transformMatrix * glm::vec4(light.position, 1.0f);
			light.position     = glm::vec3(worldPos);
			light.direction    = glm::normalize(normalMatrix * light.direction);
		}

		renderer->SetStaticLights(extractedLights);

		// Extract and apply cameras from the GLTF model
		const std::vector<CameraData> &cameras = loadedModel->GetCameras();
		if (!cameras.empty())
		{
			const CameraData &gltfCamera = cameras[0];        // Use the first camera

			// Find or create a camera entity to replace the default one
			Entity *cameraEntity = engine->GetEntity("Camera");
			if (!cameraEntity)
			{
				// Create a new camera entity if none exists
				cameraEntity = engine->CreateEntity("Camera");
				if (cameraEntity)
				{
					cameraEntity->AddComponent<TransformComponent>();
					cameraEntity->AddComponent<CameraComponent>();
				}
			}

			if (cameraEntity)
			{
				// Update the camera transform with GLTF data
				auto *cameraTransform = cameraEntity->GetComponent<TransformComponent>();
				if (cameraTransform)
				{
					// Apply the transformation matrix to the camera position
					glm::vec4 worldPos = transformMatrix * glm::vec4(gltfCamera.position, 1.0f);
					cameraTransform->SetPosition(glm::vec3(worldPos));

					// Apply rotation from GLTF camera
					glm::vec3 eulerAngles = glm::eulerAngles(gltfCamera.rotation);
					cameraTransform->SetRotation(eulerAngles);
				}

				// Update the camera component with GLTF properties
				auto *camera = cameraEntity->GetComponent<CameraComponent>();
				if (camera)
				{
					camera->ForceViewMatrixUpdate();        // Only sets viewMatrixDirty flag, doesn't change camera orientation
					if (gltfCamera.isPerspective)
					{
						camera->SetFieldOfView(glm::degrees(gltfCamera.fov));        // Convert radians to degrees
						camera->SetClipPlanes(gltfCamera.nearPlane, gltfCamera.farPlane);
						if (gltfCamera.aspectRatio > 0.0f)
						{
							camera->SetAspectRatio(gltfCamera.aspectRatio);
						}
					}
					else
					{
						// Handle orthographic camera if needed
						camera->SetProjectionType(CameraComponent::ProjectionType::Orthographic);
						camera->SetOrthographicSize(gltfCamera.orthographicSize, gltfCamera.orthographicSize);
						camera->SetClipPlanes(gltfCamera.nearPlane, gltfCamera.farPlane);
					}

					// Set this as the active camera
					engine->SetActiveCamera(camera);
				}
			}
		}

		// Get the material meshes from the loaded model
		const std::vector<MaterialMesh> &materialMeshes = modelLoader->GetMaterialMeshes(modelPath);
		if (materialMeshes.empty())
		{
			std::cerr << "No material meshes found in loaded model: " << modelPath << std::endl;
			return false;
		}

		// Collect all geometry entities so we can batch Vulkan uploads for their meshes
		std::vector<Entity *> geometryEntities;
		geometryEntities.reserve(materialMeshes.size());

		for (const auto &materialMesh : materialMeshes)
		{
			// Create an entity name based on model and material
			std::string entityName = modelName + "_Material_" + std::to_string(materialMesh.materialIndex) +
			                         "_" + materialMesh.materialName;

			if (Entity *materialEntity = engine->CreateEntity(entityName))
			{
				// Add a transform component with provided parameters
				auto *transform = materialEntity->AddComponent<TransformComponent>();
				transform->SetPosition(position);
				transform->SetRotation(glm::radians(rotation));
				transform->SetScale(scale);

				// Add a mesh component with material-specific data
				auto *mesh = materialEntity->AddComponent<MeshComponent>();
				mesh->SetVertices(materialMesh.vertices);
				mesh->SetIndices(materialMesh.indices);

				if (materialMesh.GetInstanceCount() > 0)
				{
					const std::vector<InstanceData> &instances = materialMesh.instances;
					for (const auto &instanceData : instances)
					{
						// Reconstruct the transformation matrix from InstanceData column vectors
						glm::mat4 instanceMatrix = instanceData.getModelMatrix();
						mesh->AddInstance(instanceMatrix, static_cast<uint32_t>(materialMesh.materialIndex));
					}
				}

				// Set ALL PBR texture paths for this material
				// Set primary texture path for backward compatibility
				if (!materialMesh.texturePath.empty())
				{
					mesh->SetTexturePath(materialMesh.texturePath);
				}

				// Set all PBR texture paths
				if (!materialMesh.baseColorTexturePath.empty())
				{
					mesh->SetBaseColorTexturePath(materialMesh.baseColorTexturePath);
				}
				if (!materialMesh.normalTexturePath.empty())
				{
					mesh->SetNormalTexturePath(materialMesh.normalTexturePath);
				}
				if (!materialMesh.metallicRoughnessTexturePath.empty())
				{
					mesh->SetMetallicRoughnessTexturePath(materialMesh.metallicRoughnessTexturePath);
				}
				if (!materialMesh.occlusionTexturePath.empty())
				{
					mesh->SetOcclusionTexturePath(materialMesh.occlusionTexturePath);
				}
				if (!materialMesh.emissiveTexturePath.empty())
				{
					mesh->SetEmissiveTexturePath(materialMesh.emissiveTexturePath);
				}

				// Fallback: Use material DB (from ModelLoader) if any PBR texture is still missing
				if (modelLoader)
				{
					Material *mat = modelLoader->GetMaterial(materialMesh.materialName);
					if (mat)
					{
						if (mesh->GetBaseColorTexturePath().empty() && !mat->albedoTexturePath.empty())
						{
							mesh->SetBaseColorTexturePath(mat->albedoTexturePath);
						}
						if (mesh->GetNormalTexturePath().empty() && !mat->normalTexturePath.empty())
						{
							mesh->SetNormalTexturePath(mat->normalTexturePath);
						}
						if (mesh->GetMetallicRoughnessTexturePath().empty() && !mat->metallicRoughnessTexturePath.empty())
						{
							mesh->SetMetallicRoughnessTexturePath(mat->metallicRoughnessTexturePath);
						}
						if (mesh->GetOcclusionTexturePath().empty() && !mat->occlusionTexturePath.empty())
						{
							mesh->SetOcclusionTexturePath(mat->occlusionTexturePath);
						}
						if (mesh->GetEmissiveTexturePath().empty() && !mat->emissiveTexturePath.empty())
						{
							mesh->SetEmissiveTexturePath(mat->emissiveTexturePath);
						}
					}
				}

				// Register all effective texture IDs this mesh uses so that when
				// textures finish streaming in, the renderer can refresh
				// descriptor sets for the appropriate entities. This must
				// happen *after* material fallbacks so we see the final IDs.
				if (renderer)
				{
					auto registerTex = [&](const std::string &texId) {
						if (!texId.empty())
						{
							renderer->RegisterTextureUser(texId, materialEntity);
						}
					};

					registerTex(mesh->GetTexturePath());
					registerTex(mesh->GetBaseColorTexturePath());
					registerTex(mesh->GetNormalTexturePath());
					registerTex(mesh->GetMetallicRoughnessTexturePath());
					registerTex(mesh->GetOcclusionTexturePath());
					registerTex(mesh->GetEmissiveTexturePath());
				}

				// Track this entity for batched Vulkan resource pre-allocation later
				geometryEntities.push_back(materialEntity);

				// Create physics body for collision with balls, but only for geometry
				// that is reasonably close to the ground plane. This avoids creating
				// expensive mesh colliders for high-up roofs and distant details.
				PhysicsSystem *physicsSystem = engine->GetPhysicsSystem();
				if (physicsSystem)
				{
					auto *mc = materialEntity->GetComponent<MeshComponent>();
					if (mc && !mc->GetVertices().empty() && !mc->GetIndices().empty())
					{
						// Compute a simple Y-range in WORLD space using the entity transform
						// and the mesh's local AABB if available; otherwise approximate from vertices.
						glm::vec3 minWS(std::numeric_limits<float>::max());
						glm::vec3 maxWS(-std::numeric_limits<float>::max());

						auto     *xform = materialEntity->GetComponent<TransformComponent>();
						glm::mat4 model = xform ? xform->GetModelMatrix() : glm::mat4(1.0f);

						if (mc->HasLocalAABB())
						{
							glm::vec3 localMin = mc->GetLocalAABBMin();
							glm::vec3 localMax = mc->GetLocalAABBMax();

							// Transform the 8 corners of the local AABB to world space
							for (int ix = 0; ix < 2; ++ix)
							{
								for (int iy = 0; iy < 2; ++iy)
								{
									for (int iz = 0; iz < 2; ++iz)
									{
										glm::vec3 corner(
										    ix ? localMax.x : localMin.x,
										    iy ? localMax.y : localMin.y,
										    iz ? localMax.z : localMin.z);
										glm::vec3 cWS = glm::vec3(model * glm::vec4(corner, 1.0f));
										minWS         = glm::min(minWS, cWS);
										maxWS         = glm::max(maxWS, cWS);
									}
								}
							}
						}
						else
						{
							// Fallback: compute bounds directly from vertices in world space
							const auto &verts = mc->GetVertices();
							for (const auto &v : verts)
							{
								glm::vec3 pWS = glm::vec3(model * glm::vec4(v.position, 1.0f));
								minWS         = glm::min(minWS, pWS);
								maxWS         = glm::max(maxWS, pWS);
							}
						}

						// If we have a valid Y range and the mesh comes within 6 meters of the ground,
						// create a physics body. Otherwise, skip it to save startup time and memory.
						const float groundY               = 0.0f;
						const float maxDistanceFromGround = 6.0f;
						bool        nearGround            = (minWS.y <= groundY + maxDistanceFromGround);

						if (nearGround)
						{
							physicsSystem->EnqueueRigidBodyCreation(
							    materialEntity,
							    CollisionShape::Mesh,
							    0.0f,         // mass 0 = static
							    true,         // kinematic
							    0.15f,        // restitution
							    0.5f          // friction
							);
							std::cout << "Queued physics body for near-ground geometry entity: " << entityName << std::endl;
						}
						else
						{
							std::cout << "Skipped physics body for high/remote entity: " << entityName
							          << " (minY=" << minWS.y << ")" << std::endl;
						}
					}
					else
					{
						std::cerr << "Skipping physics body for entity (no geometry): " << entityName << std::endl;
					}
				}
			}
			else
			{
				std::cerr << "Failed to create entity for material " << materialMesh.materialName << std::endl;
			}
		}

		// Pre-allocate Vulkan resources for all geometry entities in a single batched pass
		if (!geometryEntities.empty())
		{
			if (!renderer->preAllocateEntityResourcesBatch(geometryEntities))
			{
				std::cerr << "Failed to pre-allocate resources for one or more geometry entities in batch" << std::endl;
				// For now, continue; individual entities may still be partially usable
			}
		}

		// Set up animations if the model has any
		const std::vector<Animation> &animations = loadedModel->GetAnimations();
		std::cout << "[Animation] Model has " << animations.size() << " animation(s)" << std::flush << std::endl;
		if (!animations.empty())
		{
			std::cout << "[Animation] Setting up " << animations.size() << " animation(s) for playback" << std::flush << std::endl;

			// Create an animation controller entity
			Entity *animController = engine->CreateEntity(modelName + "_AnimController");
			if (animController)
			{
				auto *animTransform = animController->AddComponent<TransformComponent>();
				animTransform->SetPosition(position);

				auto *animComponent = animController->AddComponent<AnimationComponent>();
				animComponent->SetAnimations(animations);

				// Build node-to-entity mapping using actual glTF node indices
				// Get animated node mesh mappings to link geometry entities to animated nodes
				const auto &animatedNodeMeshes = loadedModel->GetAnimatedNodeMeshes();

				// Get the base transforms for animated nodes
				const auto &animatedNodeTransforms = loadedModel->GetAnimatedNodeTransforms();

				std::cout << "[Animation] Processing " << animatedNodeMeshes.size() << " animated nodes" << std::endl;

				// Build nodeToEntity mapping by creating or finding entities for each animated node
				std::unordered_map<int, Entity *> nodeToEntity;
				std::unordered_map<int, int>      meshUsageCount;        // Track how many times each mesh is used

				// First pass: count how many animated nodes use each mesh
				for (const auto &[nodeIndex, meshIndex] : animatedNodeMeshes)
				{
					meshUsageCount[meshIndex]++;
				}

				// Second pass: create entities for animated nodes
				for (const auto &[nodeIndex, meshIndex] : animatedNodeMeshes)
				{
					std::cout << "[Animation] Processing animated node " << nodeIndex << " with mesh " << meshIndex << std::endl;

					// Find a MaterialMesh with this sourceMeshIndex
					const MaterialMesh *sourceMaterialMesh    = nullptr;
					size_t              sourceMaterialMeshIdx = 0;
					for (size_t i = 0; i < materialMeshes.size(); ++i)
					{
						if (materialMeshes[i].sourceMeshIndex == meshIndex)
						{
							sourceMaterialMesh    = &materialMeshes[i];
							sourceMaterialMeshIdx = i;
							break;
						}
					}

					if (!sourceMaterialMesh)
					{
						std::cerr << "[Animation] WARNING: No MaterialMesh found for animated node "
						          << nodeIndex << " (mesh " << meshIndex << ")" << std::endl;
						continue;
					}

					Entity *nodeEntity = nullptr;

					// If this is the first animated node using this mesh, use the existing entity
					// For subsequent nodes, create new entities
					bool isFirstUse = (nodeToEntity.size() == 0 ||
					                   std::none_of(nodeToEntity.begin(), nodeToEntity.end(),
					                                [meshIndex, &animatedNodeMeshes](const auto &pair) {
						                                auto it = animatedNodeMeshes.find(pair.first);
						                                return it != animatedNodeMeshes.end() && it->second == meshIndex;
					                                }));

					if (isFirstUse && sourceMaterialMeshIdx < geometryEntities.size())
					{
						// Reuse existing entity for first animated node with this mesh
						nodeEntity = geometryEntities[sourceMaterialMeshIdx];
						std::cout << "[Animation] Reusing existing entity for first node " << nodeIndex << std::endl;

						// CRITICAL: Clear any instance data from the reused entity
						// If this mesh was set up for instanced rendering, we need to convert it
						// to a single non-instanced entity for animation
						auto *mesh = nodeEntity->GetComponent<MeshComponent>();
						if (mesh && mesh->GetInstanceCount() > 0)
						{
							size_t instanceCount = mesh->GetInstanceCount();
							mesh->ClearInstances();
							std::cout << "[Animation] Cleared " << instanceCount
							          << " instances from reused entity for animation" << std::endl;

							// Recreate the GPU instance buffer with a single identity instance
							// The old buffer still had multiple instances, so we need to update it
							if (renderer && !renderer->recreateInstanceBuffer(nodeEntity))
							{
								std::cerr << "[Animation] Failed to recreate instance buffer for reused entity" << std::endl;
							}
						}
					}
					else
					{
						// Create a new entity for this animated node (duplicate geometry)
						std::string entityName = modelName + "_AnimNode_" + std::to_string(nodeIndex) +
						                         "_Material_" + std::to_string(sourceMaterialMesh->materialIndex);
						nodeEntity = engine->CreateEntity(entityName);

						if (nodeEntity)
						{
							// Add transform component (will be set below)
							nodeEntity->AddComponent<TransformComponent>();

							// Clone the mesh component from the source MaterialMesh
							auto *mesh = nodeEntity->AddComponent<MeshComponent>();
							mesh->SetVertices(sourceMaterialMesh->vertices);
							mesh->SetIndices(sourceMaterialMesh->indices);

							// Copy all texture paths
							if (!sourceMaterialMesh->baseColorTexturePath.empty())
								mesh->SetBaseColorTexturePath(sourceMaterialMesh->baseColorTexturePath);
							if (!sourceMaterialMesh->normalTexturePath.empty())
								mesh->SetNormalTexturePath(sourceMaterialMesh->normalTexturePath);
							if (!sourceMaterialMesh->metallicRoughnessTexturePath.empty())
								mesh->SetMetallicRoughnessTexturePath(sourceMaterialMesh->metallicRoughnessTexturePath);
							if (!sourceMaterialMesh->occlusionTexturePath.empty())
								mesh->SetOcclusionTexturePath(sourceMaterialMesh->occlusionTexturePath);
							if (!sourceMaterialMesh->emissiveTexturePath.empty())
								mesh->SetEmissiveTexturePath(sourceMaterialMesh->emissiveTexturePath);

							// Register textures with renderer
							if (renderer)
							{
								auto registerTex = [&](const std::string &texId) {
									if (!texId.empty())
										renderer->RegisterTextureUser(texId, nodeEntity);
								};
								registerTex(mesh->GetBaseColorTexturePath());
								registerTex(mesh->GetNormalTexturePath());
								registerTex(mesh->GetMetallicRoughnessTexturePath());
								registerTex(mesh->GetOcclusionTexturePath());
								registerTex(mesh->GetEmissiveTexturePath());
							}

							// Pre-allocate resources for this new entity
							if (renderer && !renderer->preAllocateEntityResources(nodeEntity))
							{
								std::cerr << "[Animation] Failed to pre-allocate resources for " << entityName << std::endl;
							}

							std::cout << "[Animation] Created new entity '" << entityName << "' for node " << nodeIndex << std::endl;
						}
					}

					if (nodeEntity)
					{
						// Apply the base transform from the glTF node to this entity
						auto transformIt = animatedNodeTransforms.find(nodeIndex);
						if (transformIt != animatedNodeTransforms.end())
						{
							const glm::mat4 &nodeTransform = transformIt->second;

							// Decompose the matrix into position, rotation, and scale
							glm::vec3 nodePosition, nodeScale, skew;
							glm::quat nodeRotation;
							glm::vec4 perspective;
							glm::decompose(nodeTransform, nodeScale, nodeRotation, nodePosition, skew, perspective);

							// Apply the node's local transform to the entity
							auto *transform = nodeEntity->GetComponent<TransformComponent>();
							if (transform)
							{
								transform->SetPosition(nodePosition);
								transform->SetRotation(glm::eulerAngles(nodeRotation));
								transform->SetScale(nodeScale);
								std::cout << "[Animation] Applied base transform to entity '" << nodeEntity->GetName()
								          << "' - pos(" << nodePosition.x << "," << nodePosition.y << "," << nodePosition.z << ")" << std::endl;
							}
						}

						nodeToEntity[nodeIndex] = nodeEntity;
						std::cout << "[Animation] Linked entity '" << nodeEntity->GetName()
						          << "' to animated node " << nodeIndex << std::endl;
					}
				}

				animComponent->SetNodeToEntityMap(nodeToEntity);

				std::cout << "[Animation] Node-to-entity mapping has " << nodeToEntity.size()
				          << " entries (of " << animatedNodeMeshes.size() << " animated nodes)" << std::endl;

				// Auto-play the first animation
				if (!animations.empty())
				{
					animComponent->Play(0, true);        // Play first animation, looping
					std::cout << "Auto-playing animation: " << animations[0].name
					          << " (duration: " << animations[0].GetDuration() << "s)" << std::endl;
				}
			}
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error loading GLTF model: " << e.what() << std::endl;
		return false;
	}

	// Request acceleration structure build at next safe frame point
	// Don't build here in background thread to avoid threading issues with command pools
	if (renderer && renderer->GetRayQueryEnabled() && renderer->GetAccelerationStructureEnabled())
	{
		std::cout << "Requesting acceleration structure build for loaded scene..." << std::endl;
		renderer->RequestAccelerationStructureBuild();
	}

	return true;
}

/**
 * @brief Load a GLTF model with default transform values.
 * @param engine The engine to create entities in.
 * @param modelPath The path to the GLTF model file.
 */
void LoadGLTFModel(Engine *engine, const std::string &modelPath)
{
	// Use default transform values: slight Y offset, no rotation, unit scale
	LoadGLTFModel(engine, modelPath, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
}
