#pragma once

#include <memory>
#include <mutex>
#include <stdint.h>
#include <vector>

#include <entt/entity/registry.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

namespace components
{
namespace sg
{
using Registry = std::shared_ptr<entt::registry>;

inline Registry create_registry()
{
	return std::make_shared<entt::registry>();
}

// Bounding Box
struct AABB
{
	glm::vec3 min;
	glm::vec3 max;
};

// Spacial Hierarchy
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

class Node;

using NodePtr = std::shared_ptr<Node>;

class Node
{
  public:
	static NodePtr create(Registry &registry, const std::string &name)
	{
		return std::unique_ptr<Node>(new Node(registry, name));
	}

	virtual ~Node()
	{
		if (m_parent)
		{
			m_parent->remove_child(this);
		}

		m_registry->destroy(m_entity);
	}

	inline const Node *parent() const
	{
		std::lock_guard<std::mutex> lock{m_mut};
		return m_parent;
	}

	inline Node *parent()
	{
		std::lock_guard<std::mutex> lock{m_mut};
		return m_parent;
	}

	void remove_child(NodePtr &child)
	{
		std::lock_guard<std::mutex> lock{m_mut};

		remove_child(child.get());
	}

	void add_child(NodePtr &child)
	{
		std::lock_guard<std::mutex> lock{m_mut};

		if (child->parent() == this)
		{
			return;
		}

		if (child->parent())
		{
			child->parent()->remove_child(child);
		}

		child->m_parent = this;
		m_children.emplace_back(child);
	}

	const std::vector<NodePtr> children() const
	{
		std::lock_guard<std::mutex> lock{m_mut};

		// return {m_children.begin(), m_children.end()};
		return {};
	}

	const std::vector<NodePtr> &children()
	{
		std::lock_guard<std::mutex> lock{m_mut};

		return m_children;
	}

	template <typename Component, typename... Args>
	void emplace_component(Args &&... args)
	{
		m_registry->emplace_or_replace<Component>(m_entity, std::move(args)...);
	}

	template <typename... Components>
	bool contains_components() const
	{
		return m_registry->all_of<Components...>(m_entity);
	}

	template <typename Component>
	Component &get_component()
	{
		assert(contains_components<Component>() && "attempting to get a component which was not set");
		return m_registry->get<Component>(m_entity);
	}

	glm::mat4 world_matrix() const
	{
		glm::mat4 world_mat = transform.matrix();

		if (m_parent)
		{
			world_mat = m_parent->world_matrix() * world_mat;
		}

		return world_mat;
	}

	std::string name;
	Transform   transform;

  private:
	Node(Registry &registry, const std::string &name) :
	    name{name},
	    m_entity{registry->create()},
	    m_registry{registry}
	{
	}

	void remove_child(Node *child)
	{
		for (size_t i = 0; i < m_children.size(); i++)
		{
			if (m_children[i].get() == child)
			{
				if (child->m_parent)
				{
					child->m_parent = nullptr;
				}

				m_children.erase(m_children.begin() + i);

				return;
			}
		}
	}

	Registry     m_registry;
	entt::entity m_entity;

	Node *               m_parent{nullptr};
	std::vector<NodePtr> m_children;

	mutable std::mutex m_mut;
};

}        // namespace sg
}        // namespace components