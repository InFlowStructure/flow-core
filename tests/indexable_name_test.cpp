#include <gtest/gtest.h>

#include "IndexableName.hpp"

#include <string>
#include <string_view>

using FLOW_NAMESPACE::IndexableName;

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
