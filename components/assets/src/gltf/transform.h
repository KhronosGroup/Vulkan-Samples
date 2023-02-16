#pragma once

#include <components/common/error.hpp>
#include <components/scene_graph/components/transform.hpp>

#include "tinygltf.h"

VKBP_DISABLE_WARNINGS()
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
VKBP_ENABLE_WARNINGS()

namespace components
{
namespace gltf
{

/**
 * @brief Helper Function to change array type T to array type Y
 * Create a struct that can be used with std::transform so that we do not need to recreate lambda functions
 * @param T
 * @param Y
 */
template <class T, class Y>
struct TypeCast
{
	Y operator()(T value) const noexcept
	{
		return static_cast<Y>(value);
	}
};

inline sg::Transform parse_node_transform(tinygltf::Node &gltf_node)
{
	sg::Transform transform;

	if (!gltf_node.translation.empty())
	{
		glm::vec3 translation;

		std::transform(gltf_node.translation.begin(), gltf_node.translation.end(), glm::value_ptr(translation), TypeCast<double, float>{});

		transform.position = translation;
	}

	if (!gltf_node.rotation.empty())
	{
		glm::quat rotation;

		std::transform(gltf_node.rotation.begin(), gltf_node.rotation.end(), glm::value_ptr(rotation), TypeCast<double, float>{});

		transform.rotation = rotation;
	}

	if (!gltf_node.scale.empty())
	{
		glm::vec3 scale;

		std::transform(gltf_node.scale.begin(), gltf_node.scale.end(), glm::value_ptr(scale), TypeCast<double, float>{});

		transform.scale = scale;
	}

	if (!gltf_node.matrix.empty())
	{
		glm::mat4 matrix;

		std::transform(gltf_node.matrix.begin(), gltf_node.matrix.end(), glm::value_ptr(matrix), TypeCast<double, float>{});

		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, transform.scale, transform.rotation, transform.position, skew, perspective);
	}

	return transform;
}
}        // namespace gltf
}        // namespace components