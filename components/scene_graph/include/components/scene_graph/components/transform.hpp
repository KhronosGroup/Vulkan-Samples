#pragma once

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
VKBP_ENABLE_WARNINGS()

namespace components
{
namespace sg
{
struct Transform
{
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	inline glm::mat4 matrix() const
	{
		return glm::translate(glm::mat4(1.0), position) *
		       glm::mat4_cast(rotation) *
		       glm::scale(glm::mat4(1.0), scale);
	}
};
}        // namespace sg
}        // namespace components