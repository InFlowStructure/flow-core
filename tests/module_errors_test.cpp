// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/Module.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

using namespace flow;

namespace
{
auto factory = std::make_shared<NodeFactory>();
}

TEST(ModuleErrorsTest, RegisterWithoutLoadThrows)
{
    Module m(factory);
    EXPECT_THROW(m.RegisterModuleNodes(), std::runtime_error);
}

TEST(ModuleErrorsTest, UnregisterWithoutLoadThrows)
{
    Module m(factory);
    EXPECT_THROW(m.UnregisterModuleNodes(), std::runtime_error);
}

TEST(ModuleErrorsTest, RegisterWithNullFactoryThrows)
{
    const std::filesystem::path module_path = std::filesystem::current_path() / "test_module.fmod";
    if (!std::filesystem::exists(module_path)) GTEST_SKIP() << "no test_module.fmod";

    Module m(module_path, factory);
    ASSERT_TRUE(m.IsLoaded());
    EXPECT_THROW(m.RegisterModuleNodes(std::shared_ptr<NodeFactory>{}), std::invalid_argument);
}

TEST(ModuleErrorsTest, UnregisterWithNullFactoryThrows)
{
    const std::filesystem::path module_path = std::filesystem::current_path() / "test_module.fmod";
    if (!std::filesystem::exists(module_path)) GTEST_SKIP() << "no test_module.fmod";

    Module m(module_path, factory);
    ASSERT_TRUE(m.IsLoaded());
    EXPECT_THROW(m.UnregisterModuleNodes(std::shared_ptr<NodeFactory>{}), std::invalid_argument);
}

TEST(ModuleErrorsTest, ValidateMetaDataMissingName)
{
    json bad = {{"Version", "1.0.0"}, {"Author", "x"}, {"Description", "x"}};
    EXPECT_THROW(ModuleMetaData::Validate(bad), std::invalid_argument);
}

TEST(ModuleErrorsTest, ValidateMetaDataNameWrongType)
{
    json bad = {{"Name", 42}, {"Version", "1.0.0"}, {"Author", "x"}, {"Description", "x"}};
    EXPECT_THROW(ModuleMetaData::Validate(bad), std::invalid_argument);
}

TEST(ModuleErrorsTest, ValidateMetaDataMissingVersion)
{
    json bad = {{"Name", "n"}, {"Author", "x"}, {"Description", "x"}};
    EXPECT_THROW(ModuleMetaData::Validate(bad), std::invalid_argument);
}

TEST(ModuleErrorsTest, ValidateMetaDataInvalidSemver)
{
    // The semver branch throws flow::invalid_argument, which derives from
    // flow::formatted_error (a std::runtime_error), NOT std::invalid_argument.
    json bad = {{"Name", "n"}, {"Version", "not-a-version"}, {"Author", "x"}, {"Description", "x"}};
    EXPECT_THROW(ModuleMetaData::Validate(bad), std::runtime_error);
}

TEST(ModuleErrorsTest, ValidateMetaDataMissingAuthor)
{
    json bad = {{"Name", "n"}, {"Version", "1.0.0"}, {"Description", "x"}};
    EXPECT_THROW(ModuleMetaData::Validate(bad), std::invalid_argument);
}

TEST(ModuleErrorsTest, ValidateMetaDataMissingDescription)
{
    json bad = {{"Name", "n"}, {"Version", "1.0.0"}, {"Author", "x"}};
    EXPECT_THROW(ModuleMetaData::Validate(bad), std::invalid_argument);
}

TEST(ModuleErrorsTest, ValidateMetaDataValidPasses)
{
    json good = {{"Name", "n"}, {"Version", "1.0.0"}, {"Author", "x"}, {"Description", "y"}};
    ASSERT_NO_THROW(ModuleMetaData::Validate(good));
}

TEST(ModuleErrorsTest, LoadDirectoryNotFile)
{
    auto tmp = std::filesystem::temp_directory_path() / "flow_module_dir_test";
    std::filesystem::create_directories(tmp);
    Module m(factory);
    EXPECT_THROW(m.Load(tmp), std::runtime_error);
    std::filesystem::remove_all(tmp);
}

TEST(ModuleErrorsTest, LoadNonZipFile)
{
    auto tmp = std::filesystem::temp_directory_path() / "notamodule.fmod";
    {
        std::ofstream o(tmp);
        o << "this is not a zip file";
    }
    Module m(factory);
    EXPECT_THROW(m.Load(tmp), std::runtime_error);
    std::filesystem::remove(tmp);
}

TEST(ModuleErrorsTest, DoubleLoadReturnsFalse)
{
    const std::filesystem::path module_path = std::filesystem::current_path() / "test_module.fmod";
    if (!std::filesystem::exists(module_path)) GTEST_SKIP() << "no test_module.fmod";

    Module m(module_path, factory);
    EXPECT_FALSE(m.Load(module_path));
}
