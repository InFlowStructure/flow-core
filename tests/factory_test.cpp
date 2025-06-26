#include "flow/core/Env.hpp"
#include "flow/core/Node.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>

using namespace flow;

namespace
{
struct TestNode : public Node
{
    TestNode(const UUID& id, std::string_view name, std::shared_ptr<Env> env)
        : Node(id, TypeName_v<TestNode>, name, std::move(env))
    {
        AddInput<int>("in", "");
        AddInput<int>("other_in", "");
        AddOutput<int>("out", "");
        AddOutput<int>("other_out", "");
    }

    void Compute() override
    {
        if (auto data = GetInputData<int>("in"))
        {
            SetOutputData("out", std::move(data));
        }
        if (auto data = GetInputData<int>("other_in"))
        {
            SetOutputData("other_out", std::move(data));
        }
    }
};
} // namespace

TEST(FactoryTest, Construction) { ASSERT_NO_THROW(auto factory = std::make_shared<NodeFactory>()); }

TEST(FactoryTest, RegisterNodeClass)
{
    auto factory = std::make_shared<NodeFactory>();
    ASSERT_NO_THROW(factory->RegisterNodeClass<TestNode>("Test"));
}

TEST(FactoryTest, CreateNode)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);
    ASSERT_NO_THROW(factory->RegisterNodeClass<TestNode>("Test"));
    ASSERT_NO_THROW(auto node = factory->CreateNode(std::string{TypeName_v<TestNode>}, UUID{}, "test", env));
    ASSERT_NE(factory->CreateNode(std::string{TypeName_v<TestNode>}, UUID{}, "test", env), nullptr);
}
