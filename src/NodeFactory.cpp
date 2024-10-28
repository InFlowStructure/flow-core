// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "NodeFactory.hpp"

#include "Env.hpp"
#include "Log.hpp"
#include "Node.hpp"

#include <spdlog/spdlog.h>

FLOW_NAMESPACE_START

SharedNode NodeFactory::CreateNode(const std::string& className, const UUID& uuid, const std::string& name,
                                   std::shared_ptr<Env> env)
{
    auto found = _constructor_map.find(className);
    if (found == _constructor_map.end())
    {
        FLOW_ERROR("NodeFactory::CreateNode() - unable to find object for class: {0}", className);
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

Category::Category(std::shared_ptr<NodeFactory> factory, const std::string& name)
    : _factory{std::move(factory)}, _category_name{name}
{
}

Category::Category(const Category& parent, const std::string& name)
    : _factory{parent._factory}, _category_name(parent._category_name + "::" + name)
{
}

FLOW_NAMESPACE_END
