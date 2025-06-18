# Getting Started with Flow Core

## Basic Concepts

Flow Core is built around these key concepts:

- **Nodes**: Basic computation units
- **Connections**: Data flow between nodes
- **Graph**: Network of connected nodes
- **Flow**: Complete execution sequence

## Creating Your First Flow

### 1. Basic Setup

```cpp
#include <flow/core/Env.hpp>
#include <flow/core/Graph.hpp>
#include <flow/core/NodeFactory.hpp>

int main() {
    auto factory = std::make_shared<flow::NodeFactory>();
    auto env = flow::Env::Create(factory);
    auto graph = std::make_shared<flow::Graph>("MyGraph", env);

    // ...

    return 0;
}
```

### 2. Creating Nodes

```cpp
// Create nodes via factory
auto source_node = factory->CreateNode(flow::TypeName_v<NumberSourceNode>, UUID{}, "Source", env);
auto processing_node = factory->CreateNode(flow::TypeName_v<MultiplyNode>, UUID{}, "Multiply", env);
auto output_node = factory->CreateNode(flow::TypeName_v<PrintNode>, UUID{}, "Output", env);

// Add nodes to graph
graph->AddNode(source_node);
graph->AddNode(processing_node);
graph->AddNode(output_node);
```

### 3. Connecting Nodes

```cpp
// Connect nodes to create a flow
graph->ConnectNodes(source_node->ID(), "output", processing_node->ID(), "input");
graph->ConnectNodes(processing_node->ID(), "output", output_node->ID(), "input");

// Start the flow
graph->Run();
```

## Building Custom Nodes

### 1. Define Node Class

```cpp
// filepath: custom_node.hpp

class CustomNode : public flow::Node
{
  public:
    // Inherit the Node constructor to be compliant with the Factory
    using flow::Node::Node;

    // Virtual destructor for polymorphism
    virtual ~CustomNode() = default;

    void Compute() override
    {
        // Your computation logic
    }
};
```

### 2. Register Node

```cpp
factory->RegisterNodeClass<CustomNode>("CustomCategory", "Custom Node");
```

## Best Practices

1. **Type Safety**

   - Always use proper type checking
   - Validate inputs before processing

2. **Performance**

   - Minimize allocations in Compute() methods
   - Use thread-safe operations when needed

3. **Testing**
   - Write unit tests for custom nodes
   - Test edge cases and error conditions
