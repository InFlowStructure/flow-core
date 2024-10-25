#include <gtest/gtest.h>

#include "TypeName.hpp"

using FLOW_NAMESPACE::TypeName;

TEST(TypeNameTest, LanguageTypes)
{
    EXPECT_EQ(TypeName<bool>::value, "bool");
    EXPECT_EQ(TypeName<int>::value, "int");
    EXPECT_EQ(TypeName<float>::value, "float");
    EXPECT_EQ(TypeName<double>::value, "double");

#ifdef FLOW_LINUX
    EXPECT_EQ(TypeName<short>::value, "short int");
    EXPECT_EQ(TypeName<long>::value, "long int");
    EXPECT_EQ(TypeName<long long>::value, "long long int");
    EXPECT_EQ(TypeName<short int>::value, "short int");
    EXPECT_EQ(TypeName<long int>::value, "long int");
    EXPECT_EQ(TypeName<long long int>::value, "long long int");
    EXPECT_EQ(TypeName<long double>::value, "long double");
#elif defined(FLOW_WINDOWS)
    // TODO: Add windows expected types
#elif defined(FLOW_APPLE)
    EXPECT_EQ(TypeName<short>::value, "short");
    EXPECT_EQ(TypeName<long>::value, "long");
    EXPECT_EQ(TypeName<long long>::value, "long long");
    EXPECT_EQ(TypeName<short int>::value, "short");
    EXPECT_EQ(TypeName<long int>::value, "long");
    EXPECT_EQ(TypeName<long long int>::value, "long long");
    EXPECT_EQ(TypeName<long double>::value, "long double");
#endif

    EXPECT_EQ(TypeName<signed>::value, "int");
    EXPECT_EQ(TypeName<unsigned>::value, "unsigned int");
    EXPECT_EQ(TypeName<signed int>::value, "int");
    EXPECT_EQ(TypeName<unsigned int>::value, "unsigned int");
}

TEST(TypeNameTest, AliasTypes)
{
    using alias_type = int;
    EXPECT_EQ(TypeName<alias_type>::value, "int");
}

struct TestType
{
};
namespace TestNS
{
struct TestType;
}
TEST(TypeNameTest, CustomTypes)
{
    EXPECT_EQ(TypeName<TestType>::value, "TestType");
    EXPECT_EQ(TypeName<TestNS::TestType>::value, "TestNS::TestType");
}

TEST(TypeNameTest, CheckEquality)
{
    TypeName<int> int_typename;
    EXPECT_EQ(int_typename, TypeName<int>{});
    EXPECT_NE(int_typename, TypeName<unsigned int>{});

    EXPECT_EQ(TypeName<int>::value, "int");
    EXPECT_EQ(TypeName<unsigned int>::value, "unsigned int");
    EXPECT_NE(TypeName<unsigned int>::value, "uint32_t");
}
