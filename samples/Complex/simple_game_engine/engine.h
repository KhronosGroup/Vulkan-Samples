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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "audio_system.h"
#include "camera_component.h"
#include "entity.h"
#include "imgui_system.h"
#include "model_loader.h"
#include "physics_system.h"
#include "platform.h"
#include "renderer.h"
#include "resource_manager.h"

/**
 * @brief Main engine class that manages the game loop and subsystems.
 *
 * This class implements the core engine architecture as described in the Engine_Architecture chapter:
 * @see en/Building_a_Simple_Engine/Engine_Architecture/02_architectural_patterns.adoc
 */
class Engine
{
  public:
	using TimeDelta = std::chrono::milliseconds;
	/**
	 * @brief Default constructor.
	 */
	Engine();

	/**
	 * @brief Destructor for proper cleanup.
	 */
	~Engine();

	/**
	 * @brief Initialize the engine.
	 * @param appName The name of the application.
	 * @param width The width of the window.
	 * @param height The height of the window.
	 * @param enableValidationLayers Whether to enable Vulkan validation layers.
	 * @return True if initialization was successful, false otherwise.
	 */
	bool Initialize(const std::string &appName, int width, int height, bool enableValidationLayers = true);

	/**
	 * @brief Run the main game loop.
	 */
	void Run();

	/**
	 * @brief Clean up engine resources.
	 */
	void Cleanup();

	/**
	 * @brief Create a new entity.
	 * @param name The name of the entity.
	 * @return A pointer to the newly created entity.
	 */
	Entity *CreateEntity(const std::string &name);

	/**
	 * @brief Get an entity by name.
	 * @param name The name of the entity.
	 * @return A pointer to the entity, or nullptr if not found.
	 */
	Entity *GetEntity(const std::string &name);

	/**
	 * @brief Get all entities.
	 * @return A const reference to the vector of entities.
	 */
	const std::vector<std::unique_ptr<Entity>> &GetEntities() const
	{
		return entities;
	}

	/**
	 * @brief Remove an entity.
	 * @param entity The entity to remove.
	 * @return True if the entity was removed, false otherwise.
	 */
	bool RemoveEntity(Entity *entity);

	/**
	 * @brief Remove an entity by name.
	 * @param name The name of the entity to remove.
	 * @return True if the entity was removed, false otherwise.
	 */
	bool RemoveEntity(const std::string &name);

	/**
	 * @brief Set the active camera.
	 * @param cameraComponent The camera component to set as active.
	 */
	void SetActiveCamera(CameraComponent *cameraComponent);

	/**
	 * @brief Get the active camera.
	 * @return A pointer to the active camera component, or nullptr if none is set.
	 */
	const CameraComponent *GetActiveCamera() const;

	/**
	 * @brief Get the resource manager.
	 * @return A pointer to the resource manager.
	 */
	const ResourceManager *GetResourceManager() const;

	/**
	 * @brief Get the platform.
	 * @return A pointer to the platform.
	 */
	const Platform *GetPlatform() const;

	/**
	 * @brief Get the renderer.
	 * @return A pointer to the renderer.
	 */
	Renderer *GetRenderer();

	/**
	 * @brief Get the model loader.
	 * @return A pointer to the model loader.
	 */
	ModelLoader *GetModelLoader();

	/**
	 * @brief Get the audio system.
	 * @return A pointer to the audio system.
	 */
	const AudioSystem *GetAudioSystem() const;

	/**
	 * @brief Get the physics system.
	 * @return A pointer to the physics system.
	 */
	PhysicsSystem *GetPhysicsSystem();

	/**
	 * @brief Get the ImGui system.
	 * @return A pointer to the ImGui system.
	 */
	const ImGuiSystem *GetImGuiSystem() const;

	/**
	 * @brief Handles mouse input for interaction and camera control.
	 *
	 * This method processes mouse input for various functionalities, including interacting with the scene,
	 * camera rotation, and delegating handling to ImGui or hover systems.
	 *
	 * @param x The x-coordinate of the mouse position.
	 * @param y The y-coordinate of the mouse position.
	 * @param buttons A bitmask representing the state of mouse buttons.
	 *                Bit 0 corresponds to the left button, and Bit 1 corresponds to the right button.
	 */
	void handleMouseInput(float x, float y, uint32_t buttons);

	/**
	 * @brief Handles keyboard input events for controlling the camera and other subsystems.
	 *
	 * This method processes key press and release events to update the camera's movement state.
	 * It also forwards the input to other subsystems like the ImGui interface if applicable.
	 *
	 * @param key The key code of the keyboard input.
	 * @param pressed Indicates whether the key is pressed (true) or released (false).
	 */
	void handleKeyInput(uint32_t key, bool pressed);

#if defined(PLATFORM_ANDROID)
/**
 * @brief Initialize the engine for Android.
 * @param app The Android app.
 * @param appName The name of the application.
 * @param enableValidationLayers Whether to enable Vulkan validation layers.
 * @return True if initialization was successful, false otherwise.
 */
#	if defined(NDEBUG)
	bool InitializeAndroid(android_app *app, const std::string &appName, bool enableValidationLayers = false);
#	else
	bool InitializeAndroid(android_app *app, const std::string &appName, bool enableValidationLayers = true);
#	endif

	/**
	 * @brief Run the engine on Android.
	 */
	void RunAndroid();
#endif

  private:
	// Subsystems
	std::unique_ptr<Platform>        platform;
	std::unique_ptr<Renderer>        renderer;
	std::unique_ptr<ResourceManager> resourceManager;
	std::unique_ptr<ModelLoader>     modelLoader;
	std::unique_ptr<AudioSystem>     audioSystem;
	std::unique_ptr<PhysicsSystem>   physicsSystem;
	std::unique_ptr<ImGuiSystem>     imguiSystem;

	// Entities
	std::vector<std::unique_ptr<Entity>>      entities;
	std::unordered_map<std::string, Entity *> entityMap;

	// Active camera
	CameraComponent *activeCamera = nullptr;

	// Engine state
	bool initialized = false;
	bool running     = false;

	// Delta time calculation
	// deltaTimeMs: time since last frame in milliseconds (for clarity)
	std::chrono::milliseconds deltaTimeMs{0};
	uint64_t                  lastFrameTimeMs = 0;

	// Frame counter and FPS calculation
	uint64_t frameCount         = 0;
	float    fpsUpdateTimer     = 0.0f;
	float    currentFPS         = 0.0f;
	uint64_t lastFPSUpdateFrame = 0;

	// Camera control state
	struct CameraControlState
	{
		bool      moveForward             = false;
		bool      moveBackward            = false;
		bool      moveLeft                = false;
		bool      moveRight               = false;
		bool      moveUp                  = false;
		bool      moveDown                = false;
		bool      mouseLeftPressed        = false;
		bool      mouseRightPressed       = false;
		float     lastMouseX              = 0.0f;
		float     lastMouseY              = 0.0f;
		float     yaw                     = 0.0f;
		float     pitch                   = 0.0f;
		bool      firstMouse              = true;
		float     cameraSpeed             = 5.0f;
		float     mouseSensitivity        = 0.1f;
		bool      baseOrientationCaptured = false;
		glm::quat baseOrientation{1.0f, 0.0f, 0.0f, 0.0f};
	} cameraControl;

	// Mouse position tracking
	float currentMouseX = 0.0f;
	float currentMouseY = 0.0f;

	// Ball material properties for PBR
	struct BallMaterial
	{
		glm::vec3 albedo;
		float     metallic;
		float     roughness;
		float     ao;
		glm::vec3 emissive;
		float     bounciness;
	};

	BallMaterial ballMaterial;

	// Physics scaling configuration
	// The bistro scene spans roughly 20 game units and represents a realistic cafe/bistro space
	// Based on issue feedback: game units should NOT equal 1m and need proper scaling
	// Analysis shows bistro geometry pieces are much smaller than assumed
	struct PhysicsScaling
	{
		float gameUnitsToMeters = 0.1f;        // 1 game unit = 0.1 meter (10cm) - more realistic scale
		float physicsTimeScale  = 1.0f;        // Normal time scale for stable physics
		float forceScale        = 2.0f;        // Much reduced force scaling for visual gameplay (was 10.0f)
		float gravityScale      = 0.1f;        // Scaled gravity for smaller world scale
	};

	PhysicsScaling physicsScaling;

	// Pending ball creation data
	struct PendingBall
	{
		glm::vec3   spawnPosition;
		glm::vec3   throwDirection;
		float       throwForce;
		glm::vec3   randomSpin;
		std::string ballName;
	};

	std::vector<PendingBall> pendingBalls;

	/**
	 * @brief Update the engine state.
	 * @param deltaTime The time elapsed since the last update.
	 */
	// Accepts a time delta in milliseconds for clarity
	void Update(TimeDelta deltaTime);

	/**
	 * @brief Render the scene.
	 */
	void Render();

	/**
	 * @brief Calculate the time delta between frames.
	 * @return The delta time in milliseconds (steady_clock based).
	 */
	std::chrono::milliseconds CalculateDeltaTimeMs();

	/**
	 * @brief Handle window resize events.
	 * @param width The new width of the window.
	 * @param height The new height of the window.
	 */
	void HandleResize(int width, int height) const;

	/**
	 * @brief Update camera controls based on input state.
	 * @param deltaTime The time elapsed since the last update.
	 */
	void UpdateCameraControls(TimeDelta deltaTime);

	/**
	 * @brief Generate random PBR material properties for the ball.
	 */
	void GenerateBallMaterial();

	/**
	 * @brief Initialize physics scaling based on scene analysis.
	 */
	void InitializePhysicsScaling();

	/**
	 * @brief Convert a force value from game units to physics units.
	 * @param gameForce Force in game units.
	 * @return Force scaled for physics simulation.
	 */
	float ScaleForceForPhysics(float gameForce) const;

	/**
	 * @brief Convert gravity from real-world units to game physics units.
	 * @param realWorldGravity Gravity in m/s².
	 * @return Gravity scaled for game physics.
	 */
	glm::vec3 ScaleGravityForPhysics(const glm::vec3 &realWorldGravity) const;

	/**
	 * @brief Convert time delta for physics simulation.
	 * @param deltaTime Real delta time.
	 * @return Scaled delta time for physics.
	 */
	float ScaleTimeForPhysics(float deltaTime) const;

	/**
	 * @brief Throw a ball into the scene with random properties.
	 * @param mouseX The x-coordinate of the mouse click.
	 * @param mouseY The y-coordinate of the mouse click.
	 */
	void ThrowBall(float mouseX, float mouseY);

	/**
	 * @brief Process pending ball creations outside the rendering loop.
	 */
	void ProcessPendingBalls();

	/**
	 * @brief Handle mouse hover to track current mouse position.
	 * @param mouseX The x-coordinate of the mouse position.
	 * @param mouseY The y-coordinate of the mouse position.
	 */
	void HandleMouseHover(float mouseX, float mouseY);
};
