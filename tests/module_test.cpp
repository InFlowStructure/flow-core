// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/Module.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <filesystem>

using namespace flow;

const std::filesystem::path module_path = std::filesystem::current_path() / "test_module.flowmod";

auto factory = std::make_shared<NodeFactory>();
auto env     = Env::Create(factory);

TEST(ModuleTest, Load) { ASSERT_NO_THROW(Module(module_path, factory)); }

TEST(ModuleTest, ValidateMetaData)
{
    Module module(module_path, factory);
    ASSERT_TRUE(module.IsLoaded());

    const auto& meta_data = module.GetMetaData();
    ASSERT_EQ(meta_data.Name, "test_module");
    ASSERT_EQ(meta_data.Version, "0.0.0");
    ASSERT_EQ(meta_data.Author, "Cisco Systems, Inc.");
    ASSERT_EQ(meta_data.Description, "A test module.");
}

TEST(ModuleTest, RunModuleNodes)
{
    Module module(module_path, factory);
    ASSERT_TRUE(module.IsLoaded());

    SharedNode node;
    ASSERT_NO_THROW(node = factory->CreateNode("TestNode", UUID{}, "test", env));
    ASSERT_NE(node, nullptr);
    node->OnCompute.Bind("Test", [&] { FAIL(); });
    node->OnError.Bind("Test", [&](auto&& e) { ASSERT_THROW(throw e, std::exception); });
    node->InvokeCompute();
}

TEST(ModuleTest, Unload)
{
    Module module(module_path, factory);
    ASSERT_TRUE(module.IsLoaded());
    ASSERT_NO_THROW(module.Unload());
    ASSERT_FALSE(module.IsLoaded());
}

TEST(ModuleTest, LoadUnloadLoad)
{
    Module module(module_path, factory);
    ASSERT_TRUE(module.IsLoaded());
    ASSERT_NO_THROW(ASSERT_FALSE(module.Load(module_path)));
    ASSERT_TRUE(module.IsLoaded());
    ASSERT_NO_THROW(ASSERT_TRUE(module.Unload()));
    ASSERT_FALSE(module.IsLoaded());
    ASSERT_NO_THROW(ASSERT_TRUE(module.Load(module_path)));
    ASSERT_TRUE(module.IsLoaded());
}
