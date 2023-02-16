#pragma once

#include <memory>
#include <mutex>
#include <stdint.h>
#include <vector>

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <entt/entity/registry.hpp>
VKBP_ENABLE_WARNINGS()

#include "components/scene_graph/components/transform.hpp"

namespace components
{
namespace sg
{

using Registry = std::shared_ptr<entt::registry>;

inline Registry create_registry()
{
	return std::make_shared<entt::registry>();
}

class Node;

using NodePtr = std::shared_ptr<Node>;

struct SceneNode
{
	std::weak_ptr<Node> ptr;
};

class Node
{
  public:
	static NodePtr create(Registry &registry, const std::string &name, Transform transform = {});

	virtual ~Node();

	const Node *parent() const;
	Node       *parent();

	void remove_child(NodePtr &child);

	void add_child(NodePtr &child);

	const std::vector<NodePtr>  children() const;
	const std::vector<NodePtr> &children();

	template <typename Component>
	void set_component(const Component &component);

	template <typename Component, typename... Args>
	void set_component(Args &&...args);

	template <typename... Components>
	bool has_components() const;

	template <typename Component>
	Component &get_component() const;

	glm::mat4 world_matrix() const;

	std::string name;

	inline Registry registry() const
	{
		return m_registry;
	}

  private:
	Node(Registry &registry, const std::string &name);

	void remove_child(Node *child);

	mutable Registry m_registry;
	entt::entity     m_entity;

	Node                *m_parent{nullptr};
	std::vector<NodePtr> m_children;

	mutable std::mutex m_mut;
};

template <typename Component>
void Node::set_component(const Component &component)
{
	m_registry->emplace<Component>(m_entity) = component;
}

template <typename Component, typename... Args>
void Node::set_component(Args &&...args)
{
	m_registry->emplace_or_replace<Component>(m_entity, std::move(args)...);
}

template <typename... Components>
bool Node::has_components() const
{
	return m_registry->all_of<Components...>(m_entity);
}

template <typename Component>
Component &Node::get_component() const
{
	assert(has_components<Component>() && "attempting to get a component which was not set");
	return m_registry->get<Component>(m_entity);
}

}        // namespace sg
}        // namespace components