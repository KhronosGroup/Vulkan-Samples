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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "component.h"
#include "entity.h"
#include "transform_component.h"

/**
 * @brief Component that handles the camera view and projection.
 *
 * This class implements the camera system as described in the Camera_Transformations chapter:
 * @see en/Building_a_Simple_Engine/Camera_Transformations/03_camera_implementation.adoc
 */
class CameraComponent : public Component
{
  public:
	enum class ProjectionType
	{
		Perspective,
		Orthographic
	};

  private:
	ProjectionType projectionType = ProjectionType::Perspective;

	// Perspective projection parameters
	float fieldOfView = 45.0f;
	float aspectRatio = 16.0f / 9.0f;

	// Orthographic projection parameters
	float orthoWidth  = 10.0f;
	float orthoHeight = 10.0f;

	// Common parameters
	float nearPlane = 0.1f;
	float farPlane  = 100.0f;

	// Matrices
	glm::mat4 viewMatrix       = glm::mat4(1.0f);
	glm::mat4 projectionMatrix = glm::mat4(1.0f);

	// Camera properties
	glm::vec3 target = {0.0f, 0.0f, 0.0f};
	glm::vec3 up     = {0.0f, 1.0f, 0.0f};

	bool viewMatrixDirty       = true;
	bool projectionMatrixDirty = true;

  public:
	/**
	 * @brief Constructor with optional name.
	 * @param componentName The name of the component.
	 */
	explicit CameraComponent(const std::string &componentName = "CameraComponent") :
	    Component(componentName)
	{}

	/**
	 * @brief Initialize the camera component.
	 */
	void Initialize() override;

	/**
	 * @brief Set the projection type.
	 * @param type The projection type.
	 */
	void SetProjectionType(ProjectionType type)
	{
		projectionType        = type;
		projectionMatrixDirty = true;
	}

	/**
	 * @brief Get the projection type.
	 * @return The projection type.
	 */
	ProjectionType GetProjectionType() const
	{
		return projectionType;
	}

	/**
	 * @brief Set the field of view for perspective projection.
	 * @param fov The field of view in degrees.
	 */
	void SetFieldOfView(float fov)
	{
		fieldOfView           = fov;
		projectionMatrixDirty = true;
	}

	/**
	 * @brief Get the field of view.
	 * @return The field of view in degrees.
	 */
	float GetFieldOfView() const
	{
		return fieldOfView;
	}

	/**
	 * @brief Set the aspect ratio for perspective projection.
	 * @param ratio The aspect ratio (width / height).
	 */
	void SetAspectRatio(float ratio)
	{
		aspectRatio           = ratio;
		projectionMatrixDirty = true;
	}

	/**
	 * @brief Get the aspect ratio.
	 * @return The aspect ratio.
	 */
	float GetAspectRatio() const
	{
		return aspectRatio;
	}

	/**
	 * @brief Set the orthographic width and height.
	 * @param width The width of the orthographic view.
	 * @param height The height of the orthographic view.
	 */
	void SetOrthographicSize(float width, float height)
	{
		orthoWidth            = width;
		orthoHeight           = height;
		projectionMatrixDirty = true;
	}

	/**
	 * @brief Set the near and far planes.
	 * @param near The near plane distance.
	 * @param far The far plane distance.
	 */
	void SetClipPlanes(float near, float far)
	{
		nearPlane             = near;
		farPlane              = far;
		projectionMatrixDirty = true;
	}

	float GetNearPlane() const
	{
		return nearPlane;
	}
	float GetFarPlane() const
	{
		return farPlane;
	}

	/**
	 * @brief Set the camera target.
	 * @param newTarget The new target position.
	 */
	void SetTarget(const glm::vec3 &newTarget)
	{
		target          = newTarget;
		viewMatrixDirty = true;
	}

	/**
	 * @brief Set the camera up vector.
	 * @param newUp The new up vector.
	 */
	void SetUp(const glm::vec3 &newUp)
	{
		up              = newUp;
		viewMatrixDirty = true;
	}

	/**
	 * @brief Make the camera look at a specific target position.
	 * @param targetPosition The position to look at.
	 * @param upVector The up vector (optional, defaults to current up vector).
	 */
	void LookAt(const glm::vec3 &targetPosition, const glm::vec3 &upVector = glm::vec3(0.0f, 1.0f, 0.0f))
	{
		target          = targetPosition;
		up              = upVector;
		viewMatrixDirty = true;
	}

	/**
	 * @brief Get the view matrix.
	 * @return The view matrix.
	 */
	const glm::mat4 &GetViewMatrix();

	/**
	 * @brief Get the projection matrix.
	 * @return The projection matrix.
	 */
	const glm::mat4 &GetProjectionMatrix();

	/**
	 * @brief Get the camera position.
	 * @return The camera position.
	 */
	glm::vec3 GetPosition() const
	{
		auto transform = GetOwner()->GetComponent<TransformComponent>();
		return transform ? transform->GetPosition() : glm::vec3(0.0f, 0.0f, 0.0f);
	}

	/**
	 * @brief Get the camera target.
	 * @return The camera target.
	 */
	const glm::vec3 &GetTarget() const
	{
		return target;
	}

	/**
	 * @brief Get the camera up vector.
	 * @return The camera up vector.
	 */
	const glm::vec3 &GetUp() const
	{
		return up;
	}

	/**
	 * @brief Force view matrix recalculation without modifying camera orientation.
	 * This is used when the camera's transform position changes externally (e.g., from GLTF loading).
	 */
	void ForceViewMatrixUpdate()
	{
		viewMatrixDirty = true;
	}

  private:
	/**
	 * @brief Update the view matrix based on the camera position and target.
	 */
	void UpdateViewMatrix();

	/**
	 * @brief Update the projection matrix based on the projection type and parameters.
	 */
	void UpdateProjectionMatrix();
};
