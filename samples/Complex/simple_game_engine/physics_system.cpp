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
#include "physics_system.h"
#include "entity.h"
#include "mesh_component.h"
#include "renderer.h"
#include "transform_component.h"
#include <iostream>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>

// Concrete implementation of RigidBody
class ConcreteRigidBody final : public RigidBody
{
  public:
	ConcreteRigidBody(Entity *entity, CollisionShape shape, float mass) :
	    entity(entity), shape(shape), mass(mass)
	{
		// Initialize with the entity's transform if available
		if (entity)
		{
			// Get the position, rotation, and scale from the entity's transform component
			if (auto *transform = entity->GetComponent<TransformComponent>())
			{
				position = transform->GetPosition();
				rotation = glm::quat(transform->GetRotation());        // Convert from Euler angles to quaternion
				scale    = transform->GetScale();
			}
			else
			{
				// Fallback to defaults if no transform component
				position = glm::vec3(0.0f);
				rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);        // Identity quaternion
				scale    = glm::vec3(1.0f);
			}
		}
	}

	~ConcreteRigidBody() override = default;

	void SetPosition(const glm::vec3 &_position) override
	{
		position = _position;

		// Update entity transform component for visual representation
		if (entity)
		{
			if (auto *transform = entity->GetComponent<TransformComponent>())
			{
				transform->SetPosition(_position);
			}
		}
	}

	void SetRotation(const glm::quat &_rotation) override
	{
		rotation = _rotation;

		// Update entity transform component for visual representation
		if (entity)
		{
			if (auto *transform = entity->GetComponent<TransformComponent>())
			{
				// Convert quaternion to Euler angles for the transform component
				glm::vec3 eulerAngles = glm::eulerAngles(_rotation);
				transform->SetRotation(eulerAngles);
			}
		}
	}

	void SetScale(const glm::vec3 &_scale) override
	{
		scale = _scale;
	}

	void SetMass(float _mass) override
	{
		mass = _mass;
	}

	void SetRestitution(float _restitution) override
	{
		restitution = _restitution;
	}

	void SetFriction(float _friction) override
	{
		friction = _friction;
	}

	void ApplyForce(const glm::vec3 &force, const glm::vec3 &localPosition) override
	{
		// In a real implementation, this would apply the force to the rigid body
		linearVelocity += force / mass;
	}

	void ApplyImpulse(const glm::vec3 &impulse, const glm::vec3 &localPosition) override
	{
		// In a real implementation, this would apply the impulse to the rigid body
		linearVelocity += impulse / mass;
	}

	void SetLinearVelocity(const glm::vec3 &velocity) override
	{
		linearVelocity = velocity;
	}

	void SetAngularVelocity(const glm::vec3 &velocity) override
	{
		angularVelocity = velocity;
	}

	[[nodiscard]] glm::vec3 GetPosition() const override
	{
		return position;
	}

	[[nodiscard]] glm::quat GetRotation() const override
	{
		return rotation;
	}

	[[nodiscard]] glm::vec3 GetLinearVelocity() const override
	{
		return linearVelocity;
	}

	[[nodiscard]] glm::vec3 GetAngularVelocity() const override
	{
		return angularVelocity;
	}

	void SetKinematic(bool _kinematic) override
	{
		// Prevent balls from being set as kinematic - they should always be dynamic
		if (entity && entity->GetName().find("Ball_") == 0 && _kinematic)
		{
			return;
		}

		kinematic = _kinematic;
	}

	[[nodiscard]] bool IsKinematic() const override
	{
		return kinematic;
	}

	[[nodiscard]] Entity *GetEntity() const
	{
		return entity;
	}

	[[nodiscard]] CollisionShape GetShape() const
	{
		return shape;
	}

	[[nodiscard]] float GetMass() const
	{
		return mass;
	}

	[[nodiscard]] float GetInverseMass() const
	{
		return mass > 0.0f ? 1.0f / mass : 0.0f;
	}

	[[nodiscard]] float GetRestitution() const
	{
		return restitution;
	}

	[[nodiscard]] float GetFriction() const
	{
		return friction;
	}

  private:
	Entity        *entity = nullptr;
	CollisionShape shape;

	glm::vec3 position = glm::vec3(0.0f);
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);        // Identity quaternion
	glm::vec3 scale    = glm::vec3(1.0f);

	glm::vec3 linearVelocity  = glm::vec3(0.0f);
	glm::vec3 angularVelocity = glm::vec3(0.0f);

	float mass        = 1.0f;
	float restitution = 0.5f;
	float friction    = 0.5f;

	bool kinematic        = false;
	bool markedForRemoval = false;        // Flag to mark physics body for removal

	friend class PhysicsSystem;
};

PhysicsSystem::~PhysicsSystem()
{
	// Destructor implementation
	if (initialized && gpuAccelerationEnabled)
	{
		CleanupVulkanResources();
	}
	rigidBodies.clear();
}

bool PhysicsSystem::Initialize()
{
	// Enforce GPU-only physics. If GPU resources cannot be initialized, initialization fails.

	// Renderer must be set for GPU compute physics
	if (!renderer)
	{
		std::cerr << "PhysicsSystem::Initialize: Renderer is not set. GPU-only physics cannot proceed." << std::endl;
		return false;
	}

	// Always keep GPU acceleration enabled (CPU fallback is not allowed)
	gpuAccelerationEnabled = true;

	// Initialize Vulkan resources; fail hard if not available
	if (!InitializeVulkanResources())
	{
		std::cerr << "PhysicsSystem::Initialize: Failed to initialize Vulkan resources for physics (GPU-only)." << std::endl;
		return false;
	}

	initialized = true;
	return true;
}

void PhysicsSystem::Update(std::chrono::milliseconds deltaTime)
{
	// Drain any pending rigid body creations queued from background threads
	std::vector<PendingCreation> toCreate;
	{
		std::lock_guard<std::mutex> lk(pendingMutex);
		if (!pendingCreations.empty())
		{
			toCreate.swap(pendingCreations);
		}
	}
	for (const auto &pc : toCreate)
	{
		if (!pc.entity)
			continue;

		// Check size limit with proper locking (CreateRigidBody will acquire the lock again, but that's safe)
		{
			std::lock_guard<std::mutex> lock(rigidBodiesMutex);
			if (rigidBodies.size() >= maxGPUObjects)
				break;        // avoid oversubscription
		}

		RigidBody *rb = CreateRigidBody(pc.entity, pc.shape, pc.mass);
		if (rb)
		{
			rb->SetKinematic(pc.kinematic);
			rb->SetRestitution(pc.restitution);
			rb->SetFriction(pc.friction);
		}
	}

	// GPU-ONLY physics - NO CPU fallback available

	// Check if GPU physics is properly initialized and available
	bool canUseGPUPhysics = false;
	{
		std::lock_guard<std::mutex> lock(rigidBodiesMutex);
		canUseGPUPhysics = (rigidBodies.size() <= maxGPUObjects);
	}

	if (initialized && gpuAccelerationEnabled && renderer && canUseGPUPhysics)
	{
		// Debug: Log that we're using GPU physics
		static bool gpuPhysicsLogged = false;
		if (!gpuPhysicsLogged)
		{
			gpuPhysicsLogged = true;
		}
		SimulatePhysicsOnGPU(deltaTime);
	}
	else
	{
		// NO CPU FALLBACK - GPU physics must work, or physics is disabled
		static bool noFallbackLogged = false;
		if (!noFallbackLogged)
		{
			noFallbackLogged = true;
		}
	}

	// Clean up rigid bodies marked for removal (happens regardless of GPU/CPU physics path)
	CleanupMarkedBodies();
}

void PhysicsSystem::EnqueueRigidBodyCreation(Entity        *entity,
                                             CollisionShape shape,
                                             float          mass,
                                             bool           kinematic,
                                             float          restitution,
                                             float          friction)
{
	if (!entity)
		return;
	std::lock_guard<std::mutex> lk(pendingMutex);
	pendingCreations.push_back(PendingCreation{entity, shape, mass, kinematic, restitution, friction});
}

RigidBody *PhysicsSystem::CreateRigidBody(Entity *entity, CollisionShape shape, float mass)
{
	// Create a new rigid body
	auto rigidBody = std::make_unique<ConcreteRigidBody>(entity, shape, mass);

	// Store the rigid body with thread-safe access
	std::lock_guard<std::mutex> lock(rigidBodiesMutex);
	rigidBodies.push_back(std::move(rigidBody));

	return rigidBodies.back().get();
}

bool PhysicsSystem::RemoveRigidBody(RigidBody *rigidBody)
{
	std::lock_guard<std::mutex> lock(rigidBodiesMutex);

	// Find the rigid body in the vector
	auto it = std::ranges::find_if(rigidBodies,
	                               [rigidBody](const std::unique_ptr<RigidBody> &rb) {
		                               return rb.get() == rigidBody;
	                               });

	if (it != rigidBodies.end())
	{
		// Remove the rigid body
		rigidBodies.erase(it);

		return true;
	}

	std::cerr << "PhysicsSystem::RemoveRigidBody: Rigid body not found" << std::endl;
	return false;
}

void PhysicsSystem::SetGravity(const glm::vec3 &_gravity)
{
	gravity = _gravity;
}

glm::vec3 PhysicsSystem::GetGravity() const
{
	return gravity;
}

bool PhysicsSystem::Raycast(const glm::vec3 &origin, const glm::vec3 &direction, float maxDistance,
                            glm::vec3 *hitPosition, glm::vec3 *hitNormal, Entity **hitEntity) const
{
	// Normalize the direction vector
	glm::vec3 normalizedDirection = glm::normalize(direction);

	// Variables to track the closest hit
	float     closestHitDistance = maxDistance;
	bool      hitFound           = false;
	glm::vec3 closestHitPosition;
	glm::vec3 closestHitNormal;
	Entity   *closestHitEntity = nullptr;

	// Protect access to rigidBodies vector during iteration
	std::lock_guard<std::mutex> lock(rigidBodiesMutex);

	// Check each rigid body for intersection
	for (const auto &rigidBody : rigidBodies)
	{
		auto    concreteRigidBody = dynamic_cast<ConcreteRigidBody *>(rigidBody.get());
		Entity *entity            = concreteRigidBody->GetEntity();

		// Skip if the entity is null
		if (!entity)
		{
			continue;
		}

		// Get the position and shape of the rigid body
		glm::vec3      position = concreteRigidBody->GetPosition();
		CollisionShape shape    = concreteRigidBody->GetShape();

		// Variables for hit detection
		float     hitDistance = 0.0f;
		glm::vec3 localHitPosition;
		glm::vec3 localHitNormal;
		bool      hit = false;

		// Check for intersection based on the shape
		switch (shape)
		{
			case CollisionShape::Sphere:
			{
				// Sphere intersection test
				float radius = 0.0335f;        // Tennis ball radius to match actual ball

				// Calculate coefficients for quadratic equation
				glm::vec3 oc           = origin - position;
				float     a            = glm::dot(normalizedDirection, normalizedDirection);
				float     b            = 2.0f * glm::dot(oc, normalizedDirection);
				float     c            = glm::dot(oc, oc) - radius * radius;
				float     discriminant = b * b - 4 * a * c;

				if (discriminant >= 0)
				{
					// Calculate intersection distance
					float t = (-b - std::sqrt(discriminant)) / (2.0f * a);

					// Check if the intersection is within range
					if (t > 0 && t < closestHitDistance)
					{
						hitDistance      = t;
						localHitPosition = origin + normalizedDirection * t;
						localHitNormal   = glm::normalize(localHitPosition - position);
						hit              = true;
					}
				}
				break;
			}
			case CollisionShape::Box:
			{
				// Box intersection test (AABB)
				glm::vec3 halfExtents(0.5f, 0.5f, 0.5f);        // Default box size

				// Calculate min and max bounds of the box
				glm::vec3 boxMin = position - halfExtents;
				glm::vec3 boxMax = position + halfExtents;

				// Calculate intersection with each slab
				float tmin = -INFINITY, tmax = INFINITY;

				for (int i = 0; i < 3; i++)
				{
					if (std::abs(normalizedDirection[i]) < 0.0001f)
					{
						// Ray is parallel to the slab, check if origin is within slab
						if (origin[i] < boxMin[i] || origin[i] > boxMax[i])
						{
							// No intersection
							hit = false;
							break;
						}
					}
					else
					{
						// Calculate intersection distances
						float ood = 1.0f / normalizedDirection[i];
						float t1  = (boxMin[i] - origin[i]) * ood;
						float t2  = (boxMax[i] - origin[i]) * ood;

						// Ensure t1 <= t2
						if (t1 > t2)
						{
							std::swap(t1, t2);
						}

						// Update tmin and tmax
						tmin = std::max(tmin, t1);
						tmax = std::min(tmax, t2);

						if (tmin > tmax)
						{
							// No intersection
							hit = false;
							break;
						}
					}
				}

				// Check if the intersection is within range
				if (tmin > 0 && tmin < closestHitDistance)
				{
					hitDistance      = tmin;
					localHitPosition = origin + normalizedDirection * tmin;

					// Calculate normal based on which face was hit
					glm::vec3 center = position;
					glm::vec3 d      = localHitPosition - center;
					float     bias   = 1.00001f;        // Small bias to ensure we get the correct face

					localHitNormal = glm::vec3(0.0f);
					if (d.x > halfExtents.x * bias)
						localHitNormal = glm::vec3(1, 0, 0);
					else if (d.x < -halfExtents.x * bias)
						localHitNormal = glm::vec3(-1, 0, 0);
					else if (d.y > halfExtents.y * bias)
						localHitNormal = glm::vec3(0, 1, 0);
					else if (d.y < -halfExtents.y * bias)
						localHitNormal = glm::vec3(0, -1, 0);
					else if (d.z > halfExtents.z * bias)
						localHitNormal = glm::vec3(0, 0, 1);
					else if (d.z < -halfExtents.z * bias)
						localHitNormal = glm::vec3(0, 0, -1);

					hit = true;
				}
				break;
			}
			case CollisionShape::Capsule:
			{
				// Capsule intersection test
				// Simplified as a line segment with spheres at each end
				float radius     = 0.5f;        // Default radius
				float halfHeight = 0.5f;        // Default half-height

				// Define capsule line segment
				glm::vec3 capsuleA = position + glm::vec3(0, -halfHeight, 0);
				glm::vec3 capsuleB = position + glm::vec3(0, halfHeight, 0);

				// Calculate the closest point on a line segment
				glm::vec3 ab = capsuleB - capsuleA;
				glm::vec3 ao = origin - capsuleA;

				float t = glm::dot(ao, ab) / glm::dot(ab, ab);
				t       = glm::clamp(t, 0.0f, 1.0f);

				glm::vec3 closestPoint = capsuleA + ab * t;

				// Sphere intersection test with the closest point
				glm::vec3 oc = origin - closestPoint;
				float     a  = glm::dot(normalizedDirection, normalizedDirection);
				float     b  = 2.0f * glm::dot(oc, normalizedDirection);
				float     c  = glm::dot(oc, oc) - radius * radius;

				if (float discriminant = b * b - 4 * a * c; discriminant >= 0)
				{
					// Calculate intersection distance

					// Check if the intersection is within range
					if (float id = (-b - std::sqrt(discriminant)) / (2.0f * a); id > 0 && id < closestHitDistance)
					{
						hitDistance      = id;
						localHitPosition = origin + normalizedDirection * id;
						localHitNormal   = glm::normalize(localHitPosition - closestPoint);
						hit              = true;
					}
				}
				break;
			}
			case CollisionShape::Mesh:
			{
				// Proper mesh intersection test using triangle data
				if (auto *meshComponent = entity->GetComponent<MeshComponent>())
				{
					const auto &vertices = meshComponent->GetVertices();
					const auto &indices  = meshComponent->GetIndices();

					// Test intersection with each triangle in the mesh
					for (size_t i = 0; i < indices.size(); i += 3)
					{
						if (i + 2 >= indices.size())
							break;

						// Get triangle vertices
						glm::vec3 v0 = vertices[indices[i]].position;
						glm::vec3 v1 = vertices[indices[i + 1]].position;
						glm::vec3 v2 = vertices[indices[i + 2]].position;

						// Transform vertices to world space
						if (auto *transform = entity->GetComponent<TransformComponent>())
						{
							glm::mat4 transformMatrix = transform->GetModelMatrix();
							v0                        = glm::vec3(transformMatrix * glm::vec4(v0, 1.0f));
							v1                        = glm::vec3(transformMatrix * glm::vec4(v1, 1.0f));
							v2                        = glm::vec3(transformMatrix * glm::vec4(v2, 1.0f));
						}

						// Ray-triangle intersection using Möller-Trumbore algorithm
						glm::vec3 edge1 = v1 - v0;
						glm::vec3 edge2 = v2 - v0;
						glm::vec3 h     = glm::cross(normalizedDirection, edge2);
						float     a     = glm::dot(edge1, h);

						if (a > -0.00001f && a < 0.00001f)
							continue;        // Ray parallel to triangle

						float     f = 1.0f / a;
						glm::vec3 s = origin - v0;
						float     u = f * glm::dot(s, h);

						if (u < 0.0f || u > 1.0f)
							continue;

						glm::vec3 q = glm::cross(s, edge1);
						float     v = f * glm::dot(normalizedDirection, q);

						if (v < 0.0f || u + v > 1.0f)
							continue;

						float t = f * glm::dot(edge2, q);

						if (t > 0.00001f && t < closestHitDistance)
						{
							hitDistance        = t;
							localHitPosition   = origin + normalizedDirection * t;
							localHitNormal     = glm::normalize(glm::cross(edge1, edge2));
							hit                = true;
							closestHitDistance = t;        // Update for closer triangles
						}
					}
				}
				break;
			}
			default:
				break;
		}

		// Update the closest hit if a hit was found
		if (hit && hitDistance < closestHitDistance)
		{
			closestHitDistance = hitDistance;
			closestHitPosition = localHitPosition;
			closestHitNormal   = localHitNormal;
			closestHitEntity   = entity;
			hitFound           = true;
		}
	}

	// Set output parameters if a hit was found
	if (hitFound)
	{
		if (hitPosition)
		{
			*hitPosition = closestHitPosition;
		}

		if (hitNormal)
		{
			*hitNormal = closestHitNormal;
		}

		if (hitEntity)
		{
			*hitEntity = closestHitEntity;
		}
	}

	return hitFound;
}

// Helper function to read a shader file
static std::vector<char> readFile(const std::string &filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}

	size_t            fileSize = file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
	file.close();

	return buffer;
}

// Helper function to create a shader module
static vk::raii::ShaderModule createShaderModule(const vk::raii::Device &device, const std::vector<char> &code)
{
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = code.size();
	createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());

	return {device, createInfo};
}

bool PhysicsSystem::InitializeVulkanResources()
{
	if (!renderer)
	{
		std::cerr << "Renderer is not set" << std::endl;
		return false;
	}

	vk::Device device = renderer->GetDevice();
	if (!device)
	{
		std::cerr << "Vulkan device is not valid" << std::endl;
		return false;
	}

	try
	{
		// Create shader modules
		const vk::raii::Device &raiiDevice = renderer->GetRaiiDevice();

		std::vector<char> integrateShaderCode = readFile("shaders/physics.spv");
		vulkanResources.integrateShaderModule = createShaderModule(raiiDevice, integrateShaderCode);

		std::vector<char> broadPhaseShaderCode = readFile("shaders/physics.spv");
		vulkanResources.broadPhaseShaderModule = createShaderModule(raiiDevice, broadPhaseShaderCode);

		std::vector<char> narrowPhaseShaderCode = readFile("shaders/physics.spv");
		vulkanResources.narrowPhaseShaderModule = createShaderModule(raiiDevice, narrowPhaseShaderCode);

		std::vector<char> resolveShaderCode = readFile("shaders/physics.spv");
		vulkanResources.resolveShaderModule = createShaderModule(raiiDevice, resolveShaderCode);

		// Create a descriptor set layout
		std::array<vk::DescriptorSetLayoutBinding, 5> bindings = {
		    // Physics data buffer
		    vk::DescriptorSetLayoutBinding(
		        0,                                         // binding
		        vk::DescriptorType::eStorageBuffer,        // descriptorType
		        1,                                         // descriptorCount
		        vk::ShaderStageFlagBits::eCompute,         // stageFlags
		        nullptr                                    // pImmutableSamplers
		        ),
		    // Collision data buffer
		    vk::DescriptorSetLayoutBinding(
		        1,                                         // binding
		        vk::DescriptorType::eStorageBuffer,        // descriptorType
		        1,                                         // descriptorCount
		        vk::ShaderStageFlagBits::eCompute,         // stageFlags
		        nullptr                                    // pImmutableSamplers
		        ),
		    // Pair buffer
		    vk::DescriptorSetLayoutBinding(
		        2,                                         // binding
		        vk::DescriptorType::eStorageBuffer,        // descriptorType
		        1,                                         // descriptorCount
		        vk::ShaderStageFlagBits::eCompute,         // stageFlags
		        nullptr                                    // pImmutableSamplers
		        ),
		    // Counter buffer
		    vk::DescriptorSetLayoutBinding(
		        3,                                         // binding
		        vk::DescriptorType::eStorageBuffer,        // descriptorType
		        1,                                         // descriptorCount
		        vk::ShaderStageFlagBits::eCompute,         // stageFlags
		        nullptr                                    // pImmutableSamplers
		        ),
		    // Parameters buffer
		    vk::DescriptorSetLayoutBinding(
		        4,                                         // binding
		        vk::DescriptorType::eUniformBuffer,        // descriptorType
		        1,                                         // descriptorCount
		        vk::ShaderStageFlagBits::eCompute,         // stageFlags
		        nullptr                                    // pImmutableSamplers
		        )};

		vk::DescriptorSetLayoutCreateInfo layoutInfo;
		layoutInfo.bindingCount             = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings                = bindings.data();
		vulkanResources.descriptorSetLayout = vk::raii::DescriptorSetLayout(raiiDevice, layoutInfo);

		// Create pipeline layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
		pipelineLayoutInfo.setLayoutCount           = 1;
		vk::DescriptorSetLayout descriptorSetLayout = *vulkanResources.descriptorSetLayout;
		pipelineLayoutInfo.pSetLayouts              = &descriptorSetLayout;
		vulkanResources.pipelineLayout              = vk::raii::PipelineLayout(raiiDevice, pipelineLayoutInfo);

		// Create compute pipelines
		vk::ComputePipelineCreateInfo pipelineInfo;
		pipelineInfo.layout             = *vulkanResources.pipelineLayout;
		pipelineInfo.basePipelineHandle = nullptr;

		// Integrate pipeline
		vk::PipelineShaderStageCreateInfo integrateStageInfo;
		integrateStageInfo.stage          = vk::ShaderStageFlagBits::eCompute;
		integrateStageInfo.module         = *vulkanResources.integrateShaderModule;
		integrateStageInfo.pName          = "IntegrateCS";
		pipelineInfo.stage                = integrateStageInfo;
		vulkanResources.integratePipeline = vk::raii::Pipeline(raiiDevice, nullptr, pipelineInfo);

		// Broad phase pipeline
		vk::PipelineShaderStageCreateInfo broadPhaseStageInfo;
		broadPhaseStageInfo.stage          = vk::ShaderStageFlagBits::eCompute;
		broadPhaseStageInfo.module         = *vulkanResources.broadPhaseShaderModule;
		broadPhaseStageInfo.pName          = "BroadPhaseCS";
		pipelineInfo.stage                 = broadPhaseStageInfo;
		vulkanResources.broadPhasePipeline = vk::raii::Pipeline(raiiDevice, nullptr, pipelineInfo);

		// Narrow phase pipeline
		vk::PipelineShaderStageCreateInfo narrowPhaseStageInfo;
		narrowPhaseStageInfo.stage          = vk::ShaderStageFlagBits::eCompute;
		narrowPhaseStageInfo.module         = *vulkanResources.narrowPhaseShaderModule;
		narrowPhaseStageInfo.pName          = "NarrowPhaseCS";
		pipelineInfo.stage                  = narrowPhaseStageInfo;
		vulkanResources.narrowPhasePipeline = vk::raii::Pipeline(raiiDevice, nullptr, pipelineInfo);

		// Resolve pipeline
		vk::PipelineShaderStageCreateInfo resolveStageInfo;
		resolveStageInfo.stage          = vk::ShaderStageFlagBits::eCompute;
		resolveStageInfo.module         = *vulkanResources.resolveShaderModule;
		resolveStageInfo.pName          = "ResolveCS";
		pipelineInfo.stage              = resolveStageInfo;
		vulkanResources.resolvePipeline = vk::raii::Pipeline(raiiDevice, nullptr, pipelineInfo);

		// Create buffers
		vk::DeviceSize physicsBufferSize   = sizeof(GPUPhysicsData) * maxGPUObjects;
		vk::DeviceSize collisionBufferSize = sizeof(GPUCollisionData) * maxGPUCollisions;
		vk::DeviceSize pairBufferSize      = sizeof(uint32_t) * 2 * maxGPUCollisions;
		vk::DeviceSize counterBufferSize   = sizeof(uint32_t) * 2;
		vk::DeviceSize paramsBufferSize    = ((sizeof(PhysicsParams) + 63) / 64) * 64;

		// Create a physics buffer
		vk::BufferCreateInfo bufferInfo;
		bufferInfo.size        = physicsBufferSize;
		bufferInfo.usage       = vk::BufferUsageFlagBits::eStorageBuffer;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		try
		{
			vulkanResources.physicsBuffer = vk::raii::Buffer(raiiDevice, bufferInfo);

			vk::MemoryRequirements memRequirements = vulkanResources.physicsBuffer.getMemoryRequirements();

			vk::MemoryAllocateInfo allocInfo;
			allocInfo.allocationSize  = memRequirements.size;
			allocInfo.memoryTypeIndex = renderer->FindMemoryType(
			    memRequirements.memoryTypeBits,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			vulkanResources.physicsBufferMemory = vk::raii::DeviceMemory(raiiDevice, allocInfo);
			vulkanResources.physicsBuffer.bindMemory(*vulkanResources.physicsBufferMemory, 0);
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Failed to create physics buffer: " + std::string(e.what()));
		}

		// Create a collision buffer
		bufferInfo.size = collisionBufferSize;
		try
		{
			vulkanResources.collisionBuffer = vk::raii::Buffer(raiiDevice, bufferInfo);

			vk::MemoryRequirements memRequirements = vulkanResources.collisionBuffer.getMemoryRequirements();

			vk::MemoryAllocateInfo allocInfo;
			allocInfo.allocationSize  = memRequirements.size;
			allocInfo.memoryTypeIndex = renderer->FindMemoryType(
			    memRequirements.memoryTypeBits,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			vulkanResources.collisionBufferMemory = vk::raii::DeviceMemory(raiiDevice, allocInfo);
			vulkanResources.collisionBuffer.bindMemory(*vulkanResources.collisionBufferMemory, 0);
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Failed to create collision buffer: " + std::string(e.what()));
		}

		// Create a pair buffer
		bufferInfo.size = pairBufferSize;
		try
		{
			vulkanResources.pairBuffer = vk::raii::Buffer(raiiDevice, bufferInfo);

			vk::MemoryRequirements memRequirements = vulkanResources.pairBuffer.getMemoryRequirements();

			vk::MemoryAllocateInfo allocInfo;
			allocInfo.allocationSize  = memRequirements.size;
			allocInfo.memoryTypeIndex = renderer->FindMemoryType(
			    memRequirements.memoryTypeBits,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			vulkanResources.pairBufferMemory = vk::raii::DeviceMemory(raiiDevice, allocInfo);
			vulkanResources.pairBuffer.bindMemory(*vulkanResources.pairBufferMemory, 0);
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Failed to create pair buffer: " + std::string(e.what()));
		}

		// Create the counter-buffer
		bufferInfo.size = counterBufferSize;
		try
		{
			vulkanResources.counterBuffer = vk::raii::Buffer(raiiDevice, bufferInfo);

			vk::MemoryRequirements memRequirements = vulkanResources.counterBuffer.getMemoryRequirements();

			vk::MemoryAllocateInfo allocInfo;
			allocInfo.allocationSize  = memRequirements.size;
			allocInfo.memoryTypeIndex = renderer->FindMemoryType(
			    memRequirements.memoryTypeBits,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			vulkanResources.counterBufferMemory = vk::raii::DeviceMemory(raiiDevice, allocInfo);
			vulkanResources.counterBuffer.bindMemory(*vulkanResources.counterBufferMemory, 0);
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Failed to create counter buffer: " + std::string(e.what()));
		}

		// Create a params buffer
		bufferInfo.size  = paramsBufferSize;
		bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		try
		{
			vulkanResources.paramsBuffer = vk::raii::Buffer(raiiDevice, bufferInfo);

			vk::MemoryRequirements memRequirements = vulkanResources.paramsBuffer.getMemoryRequirements();

			vk::MemoryAllocateInfo allocInfo;
			allocInfo.allocationSize  = memRequirements.size;
			allocInfo.memoryTypeIndex = renderer->FindMemoryType(
			    memRequirements.memoryTypeBits,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			vulkanResources.paramsBufferMemory = vk::raii::DeviceMemory(raiiDevice, allocInfo);
			vulkanResources.paramsBuffer.bindMemory(*vulkanResources.paramsBufferMemory, 0);
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Failed to create params buffer: " + std::string(e.what()));
		}

		// Create persistent mapped memory pointers for improved performance
		try
		{
			// Map entire memory objects persistently to satisfy VK_WHOLE_SIZE flush alignment requirements
			vulkanResources.persistentPhysicsMemory = vulkanResources.physicsBufferMemory.mapMemory(0, VK_WHOLE_SIZE);
			vulkanResources.persistentCounterMemory = vulkanResources.counterBufferMemory.mapMemory(0, VK_WHOLE_SIZE);
			vulkanResources.persistentParamsMemory  = vulkanResources.paramsBufferMemory.mapMemory(0, VK_WHOLE_SIZE);
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Failed to create persistent mapped memory: " + std::string(e.what()));
		}

		// Initialize counter-buffer using persistent memory
		uint32_t initialCounters[2] = {0, 0};        // [0] = pair count, [1] = collision count
		memcpy(vulkanResources.persistentCounterMemory, initialCounters, sizeof(initialCounters));

		// Create a descriptor pool with capacity for 4 physics stages
		std::array poolSizes = {
		    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 16),        // 4 storage buffers × 4 stages
		    vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 4)          // 1 uniform buffer × 4 stages
		};

		vk::DescriptorPoolCreateInfo poolInfo;
		poolInfo.flags                 = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
		poolInfo.poolSizeCount         = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes            = poolSizes.data();
		poolInfo.maxSets               = 4;        // Support 4 descriptor sets for 4 physics stages
		vulkanResources.descriptorPool = vk::raii::DescriptorPool(raiiDevice, poolInfo);

		// Allocate descriptor sets
		vk::DescriptorSetAllocateInfo descriptorSetAllocInfo;
		descriptorSetAllocInfo.descriptorPool          = *vulkanResources.descriptorPool;
		descriptorSetAllocInfo.descriptorSetCount      = 1;
		vk::DescriptorSetLayout descriptorSetLayoutRef = *vulkanResources.descriptorSetLayout;
		descriptorSetAllocInfo.pSetLayouts             = &descriptorSetLayoutRef;

		try
		{
			vulkanResources.descriptorSets = raiiDevice.allocateDescriptorSets(descriptorSetAllocInfo);
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Failed to allocate descriptor sets: " + std::string(e.what()));
		}

		// Update descriptor sets
		vk::DescriptorBufferInfo physicsBufferInfo;
		physicsBufferInfo.buffer = *vulkanResources.physicsBuffer;
		physicsBufferInfo.offset = 0;
		physicsBufferInfo.range  = physicsBufferSize;

		vk::DescriptorBufferInfo collisionBufferInfo;
		collisionBufferInfo.buffer = *vulkanResources.collisionBuffer;
		collisionBufferInfo.offset = 0;
		collisionBufferInfo.range  = collisionBufferSize;

		vk::DescriptorBufferInfo pairBufferInfo;
		pairBufferInfo.buffer = *vulkanResources.pairBuffer;
		pairBufferInfo.offset = 0;
		pairBufferInfo.range  = pairBufferSize;

		vk::DescriptorBufferInfo counterBufferInfo;
		counterBufferInfo.buffer = *vulkanResources.counterBuffer;
		counterBufferInfo.offset = 0;
		counterBufferInfo.range  = counterBufferSize;

		vk::DescriptorBufferInfo paramsBufferInfo;
		paramsBufferInfo.buffer = *vulkanResources.paramsBuffer;
		paramsBufferInfo.offset = 0;
		paramsBufferInfo.range  = VK_WHOLE_SIZE;        // Use VK_WHOLE_SIZE to ensure the entire buffer is accessible

		std::array<vk::WriteDescriptorSet, 5> descriptorWrites;

		// Physics buffer
		descriptorWrites[0].setDstSet(*vulkanResources.descriptorSets[0]).setDstBinding(0).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer).setPBufferInfo(&physicsBufferInfo);

		// Collision buffer
		descriptorWrites[1].setDstSet(*vulkanResources.descriptorSets[0]).setDstBinding(1).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer).setPBufferInfo(&collisionBufferInfo);

		// Pair buffer
		descriptorWrites[2].setDstSet(*vulkanResources.descriptorSets[0]).setDstBinding(2).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer).setPBufferInfo(&pairBufferInfo);

		// Counter buffer
		descriptorWrites[3].setDstSet(*vulkanResources.descriptorSets[0]).setDstBinding(3).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer).setPBufferInfo(&counterBufferInfo);

		// Params buffer
		descriptorWrites[4].setDstSet(*vulkanResources.descriptorSets[0]).setDstBinding(4).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eUniformBuffer).setPBufferInfo(&paramsBufferInfo);

		raiiDevice.updateDescriptorSets(descriptorWrites, nullptr);

		// Create a command pool bound to the compute queue family used by the renderer
		vk::CommandPoolCreateInfo commandPoolInfo;
		commandPoolInfo.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		commandPoolInfo.queueFamilyIndex = renderer->GetComputeQueueFamilyIndex();
		vulkanResources.commandPool      = vk::raii::CommandPool(raiiDevice, commandPoolInfo);

		// Allocate command buffer
		vk::CommandBufferAllocateInfo commandBufferInfo;
		commandBufferInfo.commandPool        = *vulkanResources.commandPool;
		commandBufferInfo.level              = vk::CommandBufferLevel::ePrimary;
		commandBufferInfo.commandBufferCount = 1;

		try
		{
			std::vector<vk::raii::CommandBuffer> commandBuffers = raiiDevice.allocateCommandBuffers(commandBufferInfo);
			vulkanResources.commandBuffer                       = std::move(commandBuffers.front());
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Failed to allocate command buffer: " + std::string(e.what()));
		}

		// Create a dedicated fence for compute synchronization
		vk::FenceCreateInfo fenceInfo{};
		vulkanResources.computeFence = vk::raii::Fence(raiiDevice, fenceInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error initializing Vulkan resources: " << e.what() << std::endl;
		CleanupVulkanResources();
		return false;
	}
}

void PhysicsSystem::CleanupVulkanResources()
{
	if (!renderer)
	{
		return;
	}

	// Wait for the device to be idle before cleaning up
	renderer->WaitIdle();

	// Cleanup in proper order to avoid validation errors
	// 1. Clear descriptor sets BEFORE destroying the descriptor pool
	vulkanResources.descriptorSets.clear();

	// 2. Destroy pipelines before pipeline layout
	vulkanResources.resolvePipeline     = nullptr;
	vulkanResources.narrowPhasePipeline = nullptr;
	vulkanResources.broadPhasePipeline  = nullptr;
	vulkanResources.integratePipeline   = nullptr;

	// 3. Destroy pipeline layout before descriptor set layout
	vulkanResources.pipelineLayout      = nullptr;
	vulkanResources.descriptorSetLayout = nullptr;

	// 4. Destroy shader modules
	vulkanResources.resolveShaderModule     = nullptr;
	vulkanResources.narrowPhaseShaderModule = nullptr;
	vulkanResources.broadPhaseShaderModule  = nullptr;
	vulkanResources.integrateShaderModule   = nullptr;

	// 5. Destroy the descriptor pool after descriptor sets are cleared
	vulkanResources.descriptorPool = nullptr;

	// 6. Destroy the command buffer before the command pool
	vulkanResources.commandBuffer = nullptr;
	vulkanResources.commandPool   = nullptr;

	// 7. Destroy compute fence
	vulkanResources.computeFence = nullptr;

	// 8. Unmap persistent memory pointers before destroying buffer memory
	if (vulkanResources.persistentPhysicsMemory && *vulkanResources.physicsBufferMemory)
	{
		vulkanResources.physicsBufferMemory.unmapMemory();
		vulkanResources.persistentPhysicsMemory = nullptr;
	}

	if (vulkanResources.persistentCounterMemory && *vulkanResources.counterBufferMemory)
	{
		vulkanResources.counterBufferMemory.unmapMemory();
		vulkanResources.persistentCounterMemory = nullptr;
	}

	if (vulkanResources.persistentParamsMemory && *vulkanResources.paramsBufferMemory)
	{
		vulkanResources.paramsBufferMemory.unmapMemory();
		vulkanResources.persistentParamsMemory = nullptr;
	}

	// 8. Destroy buffers and their memory
	vulkanResources.paramsBuffer          = nullptr;
	vulkanResources.paramsBufferMemory    = nullptr;
	vulkanResources.counterBuffer         = nullptr;
	vulkanResources.counterBufferMemory   = nullptr;
	vulkanResources.pairBuffer            = nullptr;
	vulkanResources.pairBufferMemory      = nullptr;
	vulkanResources.collisionBuffer       = nullptr;
	vulkanResources.collisionBufferMemory = nullptr;
	vulkanResources.physicsBuffer         = nullptr;
	vulkanResources.physicsBufferMemory   = nullptr;
}

void PhysicsSystem::UpdateGPUPhysicsData(std::chrono::milliseconds deltaTime) const
{
	if (!renderer)
	{
		return;
	}

	// Validate Vulkan resources and persistent memory pointers before using them
	if (*vulkanResources.physicsBuffer == VK_NULL_HANDLE || *vulkanResources.physicsBufferMemory == VK_NULL_HANDLE ||
	    *vulkanResources.counterBuffer == VK_NULL_HANDLE || *vulkanResources.counterBufferMemory == VK_NULL_HANDLE ||
	    *vulkanResources.paramsBuffer == VK_NULL_HANDLE || *vulkanResources.paramsBufferMemory == VK_NULL_HANDLE ||
	    !vulkanResources.persistentPhysicsMemory || !vulkanResources.persistentCounterMemory || !vulkanResources.persistentParamsMemory)
	{
		std::cerr << "PhysicsSystem::UpdateGPUPhysicsData: Invalid Vulkan resources or persistent memory pointers" << std::endl;
		return;
	}

	// Skip physics buffer operations if no rigid bodies exist
	if (!rigidBodies.empty())
	{
		// Use persistent mapped memory for physics buffer
		auto        *gpuData = static_cast<GPUPhysicsData *>(vulkanResources.persistentPhysicsMemory);
		const size_t count   = std::min(rigidBodies.size(), static_cast<size_t>(maxGPUObjects));
		for (size_t i = 0; i < count; i++)
		{
			const auto concreteRigidBody = dynamic_cast<ConcreteRigidBody *>(rigidBodies[i].get());
			if (!concreteRigidBody)
			{
				continue;
			}

			gpuData[i].position        = glm::vec4(concreteRigidBody->GetPosition(), concreteRigidBody->GetInverseMass());
			gpuData[i].rotation        = glm::vec4(concreteRigidBody->GetRotation().x, concreteRigidBody->GetRotation().y,
			                                       concreteRigidBody->GetRotation().z, concreteRigidBody->GetRotation().w);
			gpuData[i].linearVelocity  = glm::vec4(concreteRigidBody->GetLinearVelocity(), concreteRigidBody->GetRestitution());
			gpuData[i].angularVelocity = glm::vec4(concreteRigidBody->GetAngularVelocity(), concreteRigidBody->GetFriction());
			// CRITICAL FIX: Initialize forces properly instead of always resetting to zero
			// For balls, we want to start with zero force and let the shader apply gravity
			// For static geometry, forces should remain zero
			auto initialForce  = glm::vec3(0.0f);
			auto initialTorque = glm::vec3(0.0f);

			// For dynamic bodies (balls), allow forces to be applied by
			// The shader will add gravity and other forces each frame
			bool isKinematic = concreteRigidBody->IsKinematic();
			gpuData[i].force = glm::vec4(initialForce, isKinematic ? 1.0f : 0.0f);
			// Use gravity only for dynamic bodies
			gpuData[i].torque = glm::vec4(initialTorque, isKinematic ? 0.0f : 1.0f);

			// Set collider data based on a collider type
			switch (concreteRigidBody->GetShape())
			{
				case CollisionShape::Sphere:
					// Use tennis ball radius (0.0335f) instead of hardcoded 0.5f
					gpuData[i].colliderData  = glm::vec4(0.0335f, 0.0f, 0.0f, static_cast<float>(0));        // 0 = Sphere
					gpuData[i].colliderData2 = glm::vec4(0.0f);
					break;
				case CollisionShape::Box:
					gpuData[i].colliderData  = glm::vec4(0.5f, 0.5f, 0.5f, static_cast<float>(1));        // 1 = Box
					gpuData[i].colliderData2 = glm::vec4(0.0f);
					break;
				case CollisionShape::Mesh:
				{
					// Compute an axis-aligned bounding box from the entity's mesh in WORLD space
					// and pass half-extents and local offset to the GPU. This enables sphere-geometry
					// collisions against actual imported GLTF geometry rather than a constant box.
					glm::vec3 halfExtents(5.0f);
					glm::vec3 localOffset(0.0f);

					if (auto *entity = concreteRigidBody->GetEntity())
					{
						auto *meshComp = entity->GetComponent<MeshComponent>();
						auto *xform    = entity->GetComponent<TransformComponent>();
						if (meshComp && xform && meshComp->HasLocalAABB())
						{
							glm::vec3 localMin         = meshComp->GetLocalAABBMin();
							glm::vec3 localMax         = meshComp->GetLocalAABBMax();
							glm::vec3 localCenter      = 0.5f * (localMin + localMax);
							glm::vec3 localHalfExtents = 0.5f * (localMax - localMin);

							glm::mat4 model    = (meshComp->GetInstanceCount() > 0) ? meshComp->GetInstance(0).getModelMatrix() : xform->GetModelMatrix();
							glm::vec3 centerWS = glm::vec3(model * glm::vec4(localCenter, 1.0f));

							glm::mat3 RS = glm::mat3(model);
							glm::mat3 absRS;
							absRS[0] = glm::abs(RS[0]);
							absRS[1] = glm::abs(RS[1]);
							absRS[2] = glm::abs(RS[2]);

							glm::vec3 worldHalfExtents = absRS * localHalfExtents;
							halfExtents                = glm::max(worldHalfExtents, glm::vec3(0.01f));

							// Offset relative to rigid body position
							localOffset = centerWS - concreteRigidBody->GetPosition();
						}
					}

					// Encode Mesh collider as Mesh (type=2) for GPU narrowphase handling (sphere vs mesh)
					gpuData[i].colliderData  = glm::vec4(halfExtents, static_cast<float>(2));        // 2 = Mesh (represented as world AABB)
					gpuData[i].colliderData2 = glm::vec4(localOffset, 0.0f);
				}
				break;
				default:
					gpuData[i].colliderData  = glm::vec4(0.0f, 0.0f, 0.0f, -1.0f);        // Invalid
					gpuData[i].colliderData2 = glm::vec4(0.0f);
					break;
			}
		}
	}

	// Reset counters using persistent mapped memory
	uint32_t initialCounters[2] = {0, 0};        // [0] = pair count, [1] = collision count
	memcpy(vulkanResources.persistentCounterMemory, initialCounters, sizeof(initialCounters));

	// Update params buffer
	PhysicsParams params{};
	params.deltaTime     = deltaTime.count() * 0.001f;        // Use actual deltaTime instead of fixed timestep
	params.numBodies     = static_cast<uint32_t>(std::min(rigidBodies.size(), static_cast<size_t>(maxGPUObjects)));
	params.maxCollisions = maxGPUCollisions;
	params.padding       = 0.0f;                            // Initialize padding to zero for proper std140 alignment
	params.gravity       = glm::vec4(gravity, 0.0f);        // Pack gravity into vec4 with padding

	// Update params buffer using persistent mapped memory
	memcpy(vulkanResources.persistentParamsMemory, &params, sizeof(PhysicsParams));

	// CRITICAL FIX: Explicit memory flush to ensure HOST_COHERENT memory is fully visible to GPU
	// Even with HOST_COHERENT flag, some systems may have cache coherency issues with partial writes
	// Use VK_WHOLE_SIZE to avoid nonCoherentAtomSize alignment validation errors
	try
	{
		const vk::raii::Device &device = renderer->GetRaiiDevice();
		// Flush params buffer
		vk::MappedMemoryRange flushRangeParams;
		flushRangeParams.memory = *vulkanResources.paramsBufferMemory;
		flushRangeParams.offset = 0;
		flushRangeParams.size   = VK_WHOLE_SIZE;
		device.flushMappedMemoryRanges(flushRangeParams);
		// Flush physics buffer (object data)
		vk::MappedMemoryRange flushRangePhysics;
		flushRangePhysics.memory = *vulkanResources.physicsBufferMemory;
		flushRangePhysics.offset = 0;
		flushRangePhysics.size   = VK_WHOLE_SIZE;
		device.flushMappedMemoryRanges(flushRangePhysics);
		// Flush counter buffer (pair and collision counters)
		vk::MappedMemoryRange flushRangeCounter;
		flushRangeCounter.memory = *vulkanResources.counterBufferMemory;
		flushRangeCounter.offset = 0;
		flushRangeCounter.size   = VK_WHOLE_SIZE;
		device.flushMappedMemoryRanges(flushRangeCounter);
	}
	catch (const std::exception &e)
	{
		fprintf(stderr, "WARNING: Failed to flush mapped physics memory: %s", e.what());
	}
}

void PhysicsSystem::ReadbackGPUPhysicsData() const
{
	if (!renderer)
	{
		return;
	}

	// Validate Vulkan resources and persistent memory pointers before using them
	if (*vulkanResources.physicsBuffer == VK_NULL_HANDLE || *vulkanResources.physicsBufferMemory == VK_NULL_HANDLE ||
	    !vulkanResources.persistentPhysicsMemory)
	{
		return;
	}

	// Wait for a dedicated compute fence to ensure GPU compute operations are complete before reading back data
	const vk::raii::Device &device = renderer->GetRaiiDevice();
	vk::Result              result = device.waitForFences(*vulkanResources.computeFence, VK_TRUE, UINT64_MAX);
	if (result != vk::Result::eSuccess)
	{
		return;
	}

	// Ensure GPU writes to HOST_VISIBLE memory are visible to the host before reading
	try
	{
		vk::MappedMemoryRange invalidateRangePhysics;
		invalidateRangePhysics.memory = *vulkanResources.physicsBufferMemory;
		invalidateRangePhysics.offset = 0;
		invalidateRangePhysics.size   = VK_WHOLE_SIZE;

		vk::MappedMemoryRange invalidateRangeCounter;
		invalidateRangeCounter.memory = *vulkanResources.counterBufferMemory;
		invalidateRangeCounter.offset = 0;
		invalidateRangeCounter.size   = VK_WHOLE_SIZE;

		device.invalidateMappedMemoryRanges({invalidateRangePhysics, invalidateRangeCounter});
	}
	catch (const std::exception &)
	{
		// On HOST_COHERENT heaps this may not be required; ignore errors
	}

	// Optional debug: read and log pair/collision counters for a few frames
	if (vulkanResources.persistentCounterMemory)
	{
		static uint32_t lastPairCount      = UINT32_MAX;
		static uint32_t lastCollisionCount = UINT32_MAX;
		const uint32_t *counters           = static_cast<const uint32_t *>(vulkanResources.persistentCounterMemory);
		uint32_t        pairCount          = counters[0];
		uint32_t        collisionCount     = counters[1];
		if (pairCount != lastPairCount || collisionCount != lastCollisionCount)
		{
			// std::cout << "Physics GPU counters - pairs: " << pairCount << ", collisions: " << collisionCount << std::endl;
			lastPairCount      = pairCount;
			lastCollisionCount = collisionCount;
		}
	}

	// Skip physics buffer operations if no rigid bodies exist
	if (!rigidBodies.empty())
	{
		// Use persistent mapped memory for physics buffer readback
		const auto  *gpuData = static_cast<const GPUPhysicsData *>(vulkanResources.persistentPhysicsMemory);
		const size_t count   = std::min(rigidBodies.size(), static_cast<size_t>(maxGPUObjects));
		for (size_t i = 0; i < count; i++)
		{
			const auto concreteRigidBody = dynamic_cast<ConcreteRigidBody *>(rigidBodies[i].get());
			if (!concreteRigidBody)
			{
				continue;
			}

			// Skip kinematic bodies
			if (concreteRigidBody->IsKinematic())
			{
				continue;
			}

			auto newPosition = glm::vec3(gpuData[i].position);
			auto newVelocity = glm::vec3(gpuData[i].linearVelocity);

			concreteRigidBody->SetPosition(newPosition);
			concreteRigidBody->SetRotation(glm::quat(gpuData[i].rotation.w, gpuData[i].rotation.x,
			                                         gpuData[i].rotation.y, gpuData[i].rotation.z));
			concreteRigidBody->SetLinearVelocity(newVelocity);
			concreteRigidBody->SetAngularVelocity(glm::vec3(gpuData[i].angularVelocity));
		}
	}
}

void PhysicsSystem::SimulatePhysicsOnGPU(const std::chrono::milliseconds deltaTime) const
{
	if (!renderer)
	{
		fprintf(stderr, "SimulatePhysicsOnGPU: No renderer available");
		return;
	}

	// Validate Vulkan resources before using them
	if (*vulkanResources.broadPhasePipeline == VK_NULL_HANDLE || *vulkanResources.narrowPhasePipeline == VK_NULL_HANDLE ||
	    *vulkanResources.integratePipeline == VK_NULL_HANDLE || *vulkanResources.pipelineLayout == VK_NULL_HANDLE ||
	    vulkanResources.descriptorSets.empty() || *vulkanResources.physicsBuffer == VK_NULL_HANDLE ||
	    *vulkanResources.counterBuffer == VK_NULL_HANDLE || *vulkanResources.paramsBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	// Update physics data on the GPU
	UpdateGPUPhysicsData(deltaTime);

	// Reset the command buffer before beginning (required for reuse)
	vulkanResources.commandBuffer.reset();

	// Begin command buffer
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	vulkanResources.commandBuffer.begin(beginInfo);

	vulkanResources.commandBuffer.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute,
	    *vulkanResources.pipelineLayout,
	    0,
	    **vulkanResources.descriptorSets.data(),
	    nullptr);

	// Add a memory barrier to ensure all host-written buffer data (uniform + storage) is visible to compute shaders
	// We use ShaderRead | ShaderWrite since compute will read and write storage buffers
	vk::MemoryBarrier hostToDeviceBarrier;
	hostToDeviceBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
	hostToDeviceBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;

	vulkanResources.commandBuffer.pipelineBarrier(
	    vk::PipelineStageFlagBits::eHost,
	    vk::PipelineStageFlagBits::eComputeShader,
	    vk::DependencyFlags(),
	    hostToDeviceBarrier,
	    nullptr,
	    nullptr);

	// Step 1: Integrate forces and velocities
	vulkanResources.commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *vulkanResources.integratePipeline);
	vulkanResources.commandBuffer.dispatch((rigidBodies.size() + 63) / 64, 1, 1);

	// Memory barrier to ensure integration is complete before collision detection
	vk::MemoryBarrier memoryBarrier;
	memoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
	memoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	vulkanResources.commandBuffer.pipelineBarrier(
	    vk::PipelineStageFlagBits::eComputeShader,
	    vk::PipelineStageFlagBits::eComputeShader,
	    vk::DependencyFlags(),
	    memoryBarrier,
	    nullptr,
	    nullptr);

	// Step 2: Broad-phase collision detection
	vulkanResources.commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *vulkanResources.broadPhasePipeline);
	uint32_t numPairs = (rigidBodies.size() * (rigidBodies.size() - 1)) / 2;
	// Dispatch number of workgroups matching [numthreads(64,1,1)] in BroadPhaseCS
	// One workgroup has 64 threads, each processes one pair by index
	uint32_t broadPhaseThreads = (numPairs + 63) / 64;
	vulkanResources.commandBuffer.dispatch(std::max(1u, broadPhaseThreads), 1, 1);

	// Memory barrier to ensure the broad phase is complete before the narrow phase
	vulkanResources.commandBuffer.pipelineBarrier(
	    vk::PipelineStageFlagBits::eComputeShader,
	    vk::PipelineStageFlagBits::eComputeShader,
	    vk::DependencyFlags(),
	    memoryBarrier,
	    nullptr,
	    nullptr);

	// Step 3: Narrow-phase collision detection
	vulkanResources.commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *vulkanResources.narrowPhasePipeline);
	// Dispatch enough threads to process all potential collision pairs found by broad-phase
	// The shader will check counterBuffer[0] to determine the actual number of pairs to process
	uint32_t narrowPhaseThreads = (maxGPUCollisions + 63) / 64;
	vulkanResources.commandBuffer.dispatch(narrowPhaseThreads, 1, 1);

	// Memory barrier to ensure the narrow phase is complete before resolution
	vulkanResources.commandBuffer.pipelineBarrier(
	    vk::PipelineStageFlagBits::eComputeShader,
	    vk::PipelineStageFlagBits::eComputeShader,
	    vk::DependencyFlags(),
	    memoryBarrier,
	    nullptr,
	    nullptr);

	// Step 4: Collision resolution
	vulkanResources.commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *vulkanResources.resolvePipeline);
	uint32_t resolveThreads = (maxGPUCollisions + 63) / 64;
	vulkanResources.commandBuffer.dispatch(resolveThreads, 1, 1);

	// End command buffer
	vulkanResources.commandBuffer.end();

	// Reset fence before submitting new work
	const vk::raii::Device &device = renderer->GetRaiiDevice();
	device.resetFences(*vulkanResources.computeFence);

	// Submit the command buffer with the dedicated fence for synchronization
	vk::CommandBuffer cmdBuffer = *vulkanResources.commandBuffer;
	renderer->SubmitToComputeQueue(cmdBuffer, *vulkanResources.computeFence);

	// Read back physics data from the GPU (fence wait moved to ReadbackGPUPhysicsData)
	ReadbackGPUPhysicsData();
}

void PhysicsSystem::CleanupMarkedBodies()
{
	// Remove rigid bodies that are marked for removal
	auto it = rigidBodies.begin();
	while (it != rigidBodies.end())
	{
		auto concreteRigidBody = dynamic_cast<ConcreteRigidBody *>(it->get());
		if (concreteRigidBody && concreteRigidBody->markedForRemoval)
		{
			it = rigidBodies.erase(it);
		}
		else
		{
			++it;
		}
	}
}
