// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/Module.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <filesystem>

using namespace flow;

const std::filesystem::path module_path = std::filesystem::current_path() / "test_module.fmod";

auto factory = NodeFactory::Create();
auto env     = Env::Create(factory);

TEST(ModuleTest, Load)
{
    Module m(factory);
    ASSERT_NO_THROW(ASSERT_TRUE(m.Load(module_path)));
    ASSERT_TRUE(m.IsLoaded());
}

TEST(ModuleTest, Unload)
{
    Module module(module_path, factory);
    ASSERT_TRUE(module.IsLoaded());
    ASSERT_TRUE(module.Unload());
    ASSERT_FALSE(module.IsLoaded());
}

TEST(ModuleTest, LoadInvalidPath)
{
    Module m(factory);
    ASSERT_THROW(m.Load("invalid_path.fmod"), std::runtime_error);
}

TEST(ModuleTest, UnloadWithoutLoad)
{
    Module m(factory);
    ASSERT_FALSE(m.IsLoaded());
    ASSERT_FALSE(m.Unload());
    ASSERT_FALSE(m.IsLoaded());
}

TEST(ModuleTest, ValidateMetaData)
{
    Module module(module_path, factory);
    ASSERT_TRUE(module.IsLoaded());

    const auto& meta_data = module.GetMetaData();
    ASSERT_EQ(meta_data->Name, "test_module");
    ASSERT_EQ(meta_data->Version, "0.0.0");
    ASSERT_EQ(meta_data->Author, "Cisco Systems, Inc.");
    ASSERT_EQ(meta_data->Description, "A test module.");
}

TEST(ModuleTest, RegisterModuleNodes)
{
    Module module(module_path, factory);
    ASSERT_TRUE(module.IsLoaded());

    ASSERT_NO_THROW(module.RegisterModuleNodes());
    ASSERT_NE(factory->CreateNode("TestNode", UUID{}, "test", env), nullptr);
}

TEST(ModuleTest, UnregisterModuleNodes)
{
    Module module(module_path, factory);
    ASSERT_TRUE(module.IsLoaded());

    module.RegisterModuleNodes();
    ASSERT_NE(factory->CreateNode("TestNode", UUID{}, "test", env), nullptr);

    ASSERT_NO_THROW(module.UnregisterModuleNodes());
    ASSERT_EQ(factory->CreateNode("TestNode", UUID{}, "test", env), nullptr);
}

TEST(ModuleTest, RunModuleNodes)
{
    Module module(module_path, factory);
    ASSERT_TRUE(module.IsLoaded());

    module.RegisterModuleNodes();

    SharedNode node;
    ASSERT_NO_THROW(node = factory->CreateNode("TestNode", UUID{}, "test", env));
    ASSERT_NE(node, nullptr);
    node->OnCompute.Bind("Test", [&] { FAIL(); });
    node->OnError.Bind("Test", [&](auto&& e) { ASSERT_THROW(throw e, std::exception); });
    node->InvokeCompute();
}
