// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/NodeFactory.hpp"

#include "flow/core/Env.hpp"
#include "flow/core/Node.hpp"

FLOW_NAMESPACE_START

void NodeFactory::UnregisterCategory(const Category& category)
{
    for (const auto& [class_name, _] : category._classes)
    {
        UnregisterNodeClass(category._category_name, class_name);
    }
}

void NodeFactory::UnregisterNodeClass(const std::string& category, const std::string& class_name)
{
    _constructor_map.erase(class_name);
    _friendly_names.erase(class_name);
    std::erase_if(_category_map, [&](const auto& c) {
        const auto& [cat, name] = c;
        return cat == category && name == class_name;
    });

    OnNodeClassUnregistered.Broadcast(class_name);
}

SharedNode NodeFactory::CreateNode(const std::string& className, const UUID& uuid, const std::string& name,
                                   std::shared_ptr<Env> env)
{
    auto found = _constructor_map.find(className);
    if (found == _constructor_map.end())
    {
        return nullptr;
    }

    return std::shared_ptr<Node>(reinterpret_cast<Node*>(found->second(uuid, name, std::move(env))));
}

const CategoryMap& NodeFactory::GetCategories() const { return _category_map; }

std::string NodeFactory::GetFriendlyName(const std::string& class_name) const
{
    auto found = _friendly_names.find(class_name);
    if (found != _friendly_names.end()) return found->second;

    return class_name;
}

SharedNodeData NodeFactory::Convert(const SharedNodeData& data, std::string_view to_type)
{
    return _conversion_registry.Convert(data, to_type);
}

bool NodeFactory::IsConvertible(std::string_view from_type, std::string_view to_type) const
{
    return _conversion_registry.IsConvertible(from_type, to_type);
}

Category::Category(const std::string& name) : _category_name{name} {}

Category::Category(const Category& parent, const std::string& name)
    : _category_name(parent._category_name + "::" + name)
{
}

FLOW_NAMESPACE_END
