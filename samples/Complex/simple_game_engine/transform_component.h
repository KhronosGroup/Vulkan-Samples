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
#include <glm/gtc/quaternion.hpp>

#include "component.h"

/**
 * @brief Component that handles the position, rotation, and scale of an entity.
 *
 * This class implements the transform system as described in the Camera_Transformations chapter:
 * @see en/Building_a_Simple_Engine/Camera_Transformations/04_transformation_matrices.adoc#model-matrix
 */
class TransformComponent final : public Component
{
  private:
	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	glm::vec3 rotation = {0.0f, 0.0f, 0.0f};        // Euler angles in radians
	glm::vec3 scale    = {1.0f, 1.0f, 1.0f};

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	bool      matrixDirty = true;

  public:
	/**
	 * @brief Constructor with an optional name.
	 * @param componentName The name of the component.
	 */
	explicit TransformComponent(const std::string &componentName = "TransformComponent") :
	    Component(componentName)
	{}

	/**
	 * @brief Set the position of the entity.
	 * @param newPosition The new position.
	 */
	void SetPosition(const glm::vec3 &newPosition)
	{
		position    = newPosition;
		matrixDirty = true;
	}

	/**
	 * @brief Get the position of the entity.
	 * @return The position.
	 */
	const glm::vec3 &GetPosition() const
	{
		return position;
	}

	/**
	 * @brief Set the rotation of the entity using Euler angles.
	 * @param newRotation The new rotation in radians.
	 */
	void SetRotation(const glm::vec3 &newRotation)
	{
		rotation    = newRotation;
		matrixDirty = true;
	}

	/**
	 * @brief Get the rotation of the entity as Euler angles.
	 * @return The rotation in radians.
	 */
	const glm::vec3 &GetRotation() const
	{
		return rotation;
	}

	/**
	 * @brief Set the scale of the entity.
	 * @param newScale The new scale.
	 */
	void SetScale(const glm::vec3 &newScale)
	{
		scale       = newScale;
		matrixDirty = true;
	}

	/**
	 * @brief Get the scale of the entity.
	 * @return The scale.
	 */
	const glm::vec3 &GetScale() const
	{
		return scale;
	}

	/**
	 * @brief Set the uniform scale of the entity.
	 * @param uniformScale The new uniform scale.
	 */
	void SetUniformScale(float uniformScale)
	{
		scale       = glm::vec3(uniformScale);
		matrixDirty = true;
	}

	/**
	 * @brief Translate the entity relative to its current position.
	 * @param translation The translation to apply.
	 */
	void Translate(const glm::vec3 &translation)
	{
		position += translation;
		matrixDirty = true;
	}

	/**
	 * @brief Rotate the entity relative to its current rotation.
	 * @param eulerAngles The rotation to apply in radians.
	 */
	void Rotate(const glm::vec3 &eulerAngles)
	{
		rotation += eulerAngles;
		matrixDirty = true;
	}

	/**
	 * @brief Scale the entity relative to its current scale.
	 * @param scaleFactors The scale factors to apply.
	 */
	void Scale(const glm::vec3 &scaleFactors)
	{
		scale *= scaleFactors;
		matrixDirty = true;
	}

	/**
	 * @brief Get the model matrix for this transform.
	 * @return The model matrix.
	 */
	const glm::mat4 &GetModelMatrix();

  private:
	/**
	 * @brief Update the model matrix based on position, rotation, and scale.
	 */
	void UpdateModelMatrix();
};
