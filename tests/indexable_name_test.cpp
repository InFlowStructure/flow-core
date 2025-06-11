#include <gtest/gtest.h>

#include "IndexableName.hpp"

#include <random>
#include <set>
#include <string>
#include <string_view>

using flow::IndexableName;

TEST(IndexableNameTest, Construction)
{
    ASSERT_NO_THROW(IndexableName{"tests"});
    ASSERT_NO_THROW(IndexableName{std::string("tests")});
    ASSERT_NO_THROW(IndexableName{std::string_view("tests")});
}

TEST(IndexableNameTest, Equality)
{
    ASSERT_EQ(IndexableName{"tests"}, IndexableName{std::string("tests")});
    ASSERT_EQ(IndexableName{"tests"}, IndexableName{std::string_view("tests")});

    ASSERT_NE(IndexableName{"tests"}, IndexableName{"stset"});
}

namespace
{
std::string generate_random_string(std::size_t length)
{
    static constexpr const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

    std::string result;
    for (std::size_t i = 0; i < length; ++i)
    {
        result += charset[dist(generator)];
    }

    return result;
}
} // namespace

TEST(IndexableNameTest, Uniqueness)
{
    std::set<std::string> unique_strings;
    for (int i = 0; i < 10000; ++i)
    {
        unique_strings.insert(::generate_random_string(4));
    }

    std::vector<IndexableName> names;
    for (const auto& unique_string : unique_strings)
    {
        names.emplace_back(unique_string);
    }

    std::sort(names.begin(), names.end());

    const std::size_t orig_num_names = names.size();

    auto last = std::unique(names.begin(), names.end());
    names.erase(last, names.end());

    ASSERT_EQ(orig_num_names, names.size());
}
