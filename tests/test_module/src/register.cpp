#include <flow/core/Node.hpp>
#include <flow/core/NodeFactory.hpp>

#ifdef FLOW_WINDOWS
#ifdef TEST_MODULE_EXPORT
#define TEST_MODULE_API __declspec(dllexport) FLOW_CORE_CALL
#else
#define TEST_MODULE_API __declspec(dllimport) FLOW_CORE_CALL
#endif
#else
#define TEST_MODULE_API
#endif

struct TestNode : public flow::Node
{
    explicit TestNode(const std::string& uuid_str, const std::string& name, std::shared_ptr<flow::Env> env)
        : Node(uuid_str, flow::TypeName_v<TestNode>, name, std::move(env))

    {
    }

    virtual void Compute() override { throw std::runtime_error("Computed TestNode successfully!"); }
};

extern "C"
{
    namespace test_flow
    {
    void TEST_MODULE_API RegisterModule(std::shared_ptr<flow::NodeFactory> factory)
    {
        factory->RegisterNodeClass<TestNode>("test", "Test");
    }

    void TEST_MODULE_API UnregisterModule(std::shared_ptr<flow::NodeFactory> factory)
    {
        factory->UnregisterNodeClass<TestNode>("test");
    }
    } // namespace test_flow
}
