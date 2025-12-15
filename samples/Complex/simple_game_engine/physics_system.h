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
#pragma once

#include <chrono>
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

class Entity;
class Renderer;

/**
 * @brief Enum for different collision shapes.
 */
enum class CollisionShape
{
	Box,
	Sphere,
	Capsule,
	Mesh
};

/**
 * @brief Class representing a rigid body for physics simulation.
 */
class RigidBody
{
  public:
	/**
	 * @brief Default constructor.
	 */
	RigidBody() = default;

	/**
	 * @brief Destructor for proper cleanup.
	 */
	virtual ~RigidBody() = default;

	/**
	 * @brief Set the position of the rigid body.
	 * @param position The position.
	 */
	virtual void SetPosition(const glm::vec3 &position) = 0;

	/**
	 * @brief Set the rotation of the rigid body.
	 * @param rotation The rotation quaternion.
	 */
	virtual void SetRotation(const glm::quat &rotation) = 0;

	/**
	 * @brief Set the scale of the rigid body.
	 * @param scale The scale.
	 */
	virtual void SetScale(const glm::vec3 &scale) = 0;

	/**
	 * @brief Set the mass of the rigid body.
	 * @param mass The mass.
	 */
	virtual void SetMass(float mass) = 0;

	/**
	 * @brief Set the restitution (bounciness) of the rigid body.
	 * @param restitution The restitution (0.0f to 1.0f).
	 */
	virtual void SetRestitution(float restitution) = 0;

	/**
	 * @brief Set the friction of the rigid body.
	 * @param friction The friction (0.0f to 1.0f).
	 */
	virtual void SetFriction(float friction) = 0;

	/**
	 * @brief Apply a force to the rigid body.
	 * @param force The force vector.
	 * @param localPosition The local position to apply the force at.
	 */
	virtual void ApplyForce(const glm::vec3 &force, const glm::vec3 &localPosition = glm::vec3(0.0f)) = 0;

	/**
	 * @brief Apply an impulse to the rigid body.
	 * @param impulse The impulse vector.
	 * @param localPosition The local position to apply the impulse at.
	 */
	virtual void ApplyImpulse(const glm::vec3 &impulse, const glm::vec3 &localPosition = glm::vec3(0.0f)) = 0;

	/**
	 * @brief Set the linear velocity of the rigid body.
	 * @param velocity The linear velocity.
	 */
	virtual void SetLinearVelocity(const glm::vec3 &velocity) = 0;

	/**
	 * @brief Set the angular velocity of the rigid body.
	 * @param velocity The angular velocity.
	 */
	virtual void SetAngularVelocity(const glm::vec3 &velocity) = 0;

	/**
	 * @brief Get the position of the rigid body.
	 * @return The position.
	 */
	[[nodiscard]] virtual glm::vec3 GetPosition() const = 0;

	/**
	 * @brief Get the rotation of the rigid body.
	 * @return The rotation quaternion.
	 */
	[[nodiscard]] virtual glm::quat GetRotation() const = 0;

	/**
	 * @brief Get the linear velocity of the rigid body.
	 * @return The linear velocity.
	 */
	[[nodiscard]] virtual glm::vec3 GetLinearVelocity() const = 0;

	/**
	 * @brief Get the angular velocity of the rigid body.
	 * @return The angular velocity.
	 */
	[[nodiscard]] virtual glm::vec3 GetAngularVelocity() const = 0;

	/**
	 * @brief Set whether the rigid body is kinematic.
	 * @param kinematic Whether the rigid body is kinematic.
	 */
	virtual void SetKinematic(bool kinematic) = 0;

	/**
	 * @brief Check if the rigid body is kinematic.
	 * @return True if kinematic, false otherwise.
	 */
	[[nodiscard]] virtual bool IsKinematic() const = 0;
};

/**
 * @brief Structure for GPU physics data.
 */
struct GPUPhysicsData
{
	glm::vec4 position;               // xyz = position, w = inverse mass
	glm::vec4 rotation;               // quaternion
	glm::vec4 linearVelocity;         // xyz = velocity, w = restitution
	glm::vec4 angularVelocity;        // xyz = angular velocity, w = friction
	glm::vec4 force;                  // xyz = force, w = is kinematic (0 or 1)
	glm::vec4 torque;                 // xyz = torque, w = use gravity (0 or 1)
	glm::vec4 colliderData;           // type-specific data (e.g., radius for spheres)
	glm::vec4 colliderData2;          // additional collider data (e.g., box half extents)
};

/**
 * @brief Structure for GPU collision data.
 */
struct GPUCollisionData
{
	uint32_t  bodyA;
	uint32_t  bodyB;
	glm::vec4 contactNormal;        // xyz = normal, w = penetration depth
	glm::vec4 contactPoint;         // xyz = contact point, w = unused
};

/**
 * @brief Structure for physics simulation parameters.
 */
struct PhysicsParams
{
	float     deltaTime;            // Time step - 4 bytes
	uint32_t  numBodies;            // Number of rigid bodies - 4 bytes
	uint32_t  maxCollisions;        // Maximum number of collisions - 4 bytes
	float     padding;              // Explicit padding to align gravity to 16-byte boundary - 4 bytes
	glm::vec4 gravity;              // Gravity vector (xyz) + padding (w) - 16 bytes
	// Total: 32 bytes (aligned to 16-byte boundaries for std140 layout)
};

/**
 * @brief Structure to store collision prediction data for a ray-based collision system.
 */
struct CollisionPrediction
{
	float     collisionTime = -1.0f;        // Time within deltaTime when the collision occurs (-1 = no collision)
	glm::vec3 collisionPoint;               // World position where collision occurs
	glm::vec3 collisionNormal;              // Surface normal at collision point
	glm::vec3 newVelocity;                  // Predicted velocity after bounce
	Entity   *hitEntity = nullptr;          // Entity that was hit
	bool      isValid   = false;            // Whether this prediction is valid
};

/**
 * @brief Class for managing physics simulation.
 *
 * This class implements the physics system as described in the Subsystems chapter:
 * @see en/Building_a_Simple_Engine/Subsystems/04_physics_basics.adoc
 * @see en/Building_a_Simple_Engine/Subsystems/05_vulkan_physics.adoc
 */
class PhysicsSystem
{
  public:
	/**
	 * @brief Default constructor.
	 */
	PhysicsSystem() = default;

	// Constructor-based initialization replacing separate Initialize/Set* calls
	explicit PhysicsSystem(Renderer *_renderer, bool enableGPU = true)
	{
		SetRenderer(_renderer);
		SetGPUAccelerationEnabled(enableGPU);
		if (!Initialize())
		{
			throw std::runtime_error("PhysicsSystem: initialization failed");
		}
	}

	/**
	 * @brief Destructor for proper cleanup.
	 */
	~PhysicsSystem();

	/**
	 * @brief Initialize the physics system.
	 * @return True if initialization was successful, false otherwise.
	 */
	bool Initialize();

	/**
	 * @brief Update the physics system.
	 * @param deltaTime The time elapsed since the last update.
	 */
	void Update(std::chrono::milliseconds deltaTime);

	/**
	 * @brief Create a rigid body.
	 * @param entity The entity to attach the rigid body to.
	 * @param shape The collision shape.
	 * @param mass The mass.
	 * @return Pointer to the created rigid body, or nullptr if creation failed.
	 */
	RigidBody *CreateRigidBody(Entity *entity, CollisionShape shape, float mass);

	/**
	 * @brief Remove a rigid body.
	 * @param rigidBody The rigid body to remove.
	 * @return True if removal was successful, false otherwise.
	 */
	bool RemoveRigidBody(RigidBody *rigidBody);

	/**
	 * @brief Set the gravity of the physics world.
	 * @param _gravity The gravity vector.
	 */
	void SetGravity(const glm::vec3 &_gravity);

	/**
	 * @brief Get the gravity of the physics world.
	 * @return The gravity vector.
	 */
	[[nodiscard]] glm::vec3 GetGravity() const;

	/**
	 * @brief Perform a raycast.
	 * @param origin The origin of the ray.
	 * @param direction The direction of the ray.
	 * @param maxDistance The maximum distance of the ray.
	 * @param hitPosition Output parameter for the hit position.
	 * @param hitNormal Output parameter for the hit normal.
	 * @param hitEntity Output parameter for the hit entity.
	 * @return True if the ray hit something, false otherwise.
	 */
	bool Raycast(const glm::vec3 &origin, const glm::vec3 &direction, float maxDistance,
	             glm::vec3 *hitPosition, glm::vec3 *hitNormal, Entity **hitEntity) const;

	/**
	 * @brief Enable or disable GPU acceleration.
	 * @param enabled Whether GPU acceleration is enabled.
	 */
	void SetGPUAccelerationEnabled(bool enabled)
	{
		// Enforce GPU-only policy: disabling GPU acceleration is not allowed in this project.
		// Ignore attempts to disable and keep GPU acceleration enabled.
		gpuAccelerationEnabled = true;
	}

	/**
	 * @brief Check if GPU acceleration is enabled.
	 * @return True, if GPU acceleration is enabled, false otherwise.
	 */
	[[nodiscard]] bool IsGPUAccelerationEnabled() const
	{
		return gpuAccelerationEnabled;
	}

	/**
	 * @brief Set the maximum number of objects that can be simulated on the GPU.
	 * @param maxObjects The maximum number of objects.
	 */
	void SetMaxGPUObjects(uint32_t maxObjects)
	{
		maxGPUObjects = maxObjects;
	}

	/**
	 * @brief Set the renderer to use during GPU acceleration.
	 * @param _renderer The renderer.
	 */
	void SetRenderer(Renderer *_renderer)
	{
		renderer = _renderer;
	}

	/**
	 * @brief Set the current camera position for geometry-relative ball checking.
	 * @param _cameraPosition The current camera position.
	 */
	void SetCameraPosition(const glm::vec3 &_cameraPosition)
	{
		cameraPosition = _cameraPosition;
	}

	// Thread-safe enqueue for rigid body creation from any thread
	void EnqueueRigidBodyCreation(Entity        *entity,
	                              CollisionShape shape,
	                              float          mass,
	                              bool           kinematic,
	                              float          restitution,
	                              float          friction);

  private:
	/**
	 * @brief Clean up rigid bodies that are marked for removal.
	 */
	void CleanupMarkedBodies();

	// Pending rigid body creations queued from background threads
	struct PendingCreation
	{
		Entity        *entity;
		CollisionShape shape;
		float          mass;
		bool           kinematic;
		float          restitution;
		float          friction;
	};
	std::mutex                   pendingMutex;
	std::vector<PendingCreation> pendingCreations;

	// Rigid bodies
	mutable std::mutex                      rigidBodiesMutex;        // Protect concurrent access to rigidBodies
	std::vector<std::unique_ptr<RigidBody>> rigidBodies;

	// Gravity
	glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);

	// Whether the physics system is initialized
	bool initialized = false;

	// GPU acceleration
	bool      gpuAccelerationEnabled = false;
	uint32_t  maxGPUObjects          = 1024;
	uint32_t  maxGPUCollisions       = 4096;
	Renderer *renderer               = nullptr;

	// Camera position for geometry-relative ball checking
	glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);

	// Vulkan resources for physics simulation
	struct VulkanResources
	{
		// Shader modules
		vk::raii::ShaderModule integrateShaderModule   = nullptr;
		vk::raii::ShaderModule broadPhaseShaderModule  = nullptr;
		vk::raii::ShaderModule narrowPhaseShaderModule = nullptr;
		vk::raii::ShaderModule resolveShaderModule     = nullptr;

		// Pipeline layouts and compute pipelines
		vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
		vk::raii::PipelineLayout      pipelineLayout      = nullptr;
		vk::raii::Pipeline            integratePipeline   = nullptr;
		vk::raii::Pipeline            broadPhasePipeline  = nullptr;
		vk::raii::Pipeline            narrowPhasePipeline = nullptr;
		vk::raii::Pipeline            resolvePipeline     = nullptr;

		// Descriptor pool and sets
		vk::raii::DescriptorPool             descriptorPool = nullptr;
		std::vector<vk::raii::DescriptorSet> descriptorSets;

		// Buffers for physics data
		vk::raii::Buffer       physicsBuffer         = nullptr;
		vk::raii::DeviceMemory physicsBufferMemory   = nullptr;
		vk::raii::Buffer       collisionBuffer       = nullptr;
		vk::raii::DeviceMemory collisionBufferMemory = nullptr;
		vk::raii::Buffer       pairBuffer            = nullptr;
		vk::raii::DeviceMemory pairBufferMemory      = nullptr;
		vk::raii::Buffer       counterBuffer         = nullptr;
		vk::raii::DeviceMemory counterBufferMemory   = nullptr;
		vk::raii::Buffer       paramsBuffer          = nullptr;
		vk::raii::DeviceMemory paramsBufferMemory    = nullptr;

		// Persistent mapped memory pointers for improved performance
		void *persistentPhysicsMemory = nullptr;
		void *persistentCounterMemory = nullptr;
		void *persistentParamsMemory  = nullptr;

		// Command buffer for compute operations
		vk::raii::CommandPool   commandPool   = nullptr;
		vk::raii::CommandBuffer commandBuffer = nullptr;

		// Dedicated fence for compute synchronization
		vk::raii::Fence computeFence = nullptr;
	};

	VulkanResources vulkanResources;

	// Initialize Vulkan resources for physics simulation
	bool InitializeVulkanResources();
	void CleanupVulkanResources();

	// Update physics data on the GPU
	void UpdateGPUPhysicsData(std::chrono::milliseconds deltaTime) const;

	// Read back physics data from the GPU
	void ReadbackGPUPhysicsData() const;

	// Perform GPU-accelerated physics simulation
	void SimulatePhysicsOnGPU(std::chrono::milliseconds deltaTime) const;
};
