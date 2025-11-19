# Node Serialization Issue: Default Values and Port Metadata Loss

## Executive Summary

The Flow-Core `Node` class has a critical design issue where **default values and port metadata are not persisted during JSON serialization**. This means graphs saved and restored from JSON lose important configuration data, potentially causing runtime failures or incorrect behavior when nodes are re-instantiated.

---

## Problem Description

### The Core Issue

When a `Node` is serialized to JSON via the `Save()` method, only metadata about the node itself is preserved:
- Node ID (UUID)
- Class name
- Friendly name
- Empty "inputs" field (null)

**Port information is completely lost:**
- Input port definitions (names, types, captions)
- Output port definitions
- Default values for inputs
- Port connectivity flags
- Whether ports are required (reference types)

### Current Behavior

**Example 1: Base Node Serialization**
```cpp
NodeTest::TestNode node;
node.SetName("MyTestNode");
node.AddInput<int>("input_no_default", "Integer Input");
node.AddInput<float>("input_with_default", "Float Input", MakeNodeData<float>(3.14f));
node.AddOutput<int>("output_result", "Result Output");

json node_json = node.Save();
// Result:
// {
//   "class": "NodeTest::TestNode",
//   "id": "3fffa214-f786-4165-ae8b-373fee0fc5d7",
//   "inputs": null,      // <-- Empty! No port data serialized
//   "name": "MyTestNode"
// }
```

The `inputs` field is `null` because `Node::SaveInputs()` in `src/Node.cpp:76` returns an empty JSON object:
```cpp
json Node::SaveInputs() const { return {}; }  // Returns empty object, serializes as null
```

### Why This Is a Problem

1. **Graph State Loss**: When a graph is saved to disk and restored, nodes lose their input port configuration.

2. **Runtime Failures**: On deserialization, if a node's `Compute()` method tries to access an input port that should have a default value, it gets `nullptr` instead.

3. **Manual Workaround Required**: Each custom node must manually override `SaveInputs()` and `RestoreInputs()` to preserve state:
   ```cpp
   struct CustomNode : public Node {
       json SaveInputs() const override {
           // Must manually serialize all port metadata and defaults
           json inputs = json::object();
           for (const auto& [key, port] : GetInputPorts()) {
               inputs[std::string(key)] = {
                   {"caption", port->GetCaption()},
                   {"type", port->GetDataType()},
                   {"has_default", port->GetData() != nullptr},
               };
           }
           return inputs;
       }
   };
   ```

4. **Inconsistency**: The burden of serialization is entirely on derived classes. There's no standard or documented approach, leading to:
   - Incomplete serialization implementations
   - Inconsistent formats across different node types
   - Difficulty maintaining and debugging graph files

5. **Port Metadata Not Tracked**: Default values are stored in the Port's `_data` member, but there's no way to:
   - Distinguish between "no default provided" vs "default provided but is nullptr"
   - Serialize the actual default values (only a boolean flag "has_default")
   - Restore ports with their original defaults

---

## Technical Root Cause

### Architecture Misunderstanding

The issue stems from how the serialization architecture is designed:

1. **Port Creation is Dynamic**: Ports are created during node construction via `AddInput()`/`AddOutput()`, not from serialized data.

2. **Serialization Only Saves Data**: The design assumes that input/output data (what flows through ports at runtime) can be serialized, but not the port definitions themselves.

3. **Responsibility Mismatch**: The `SaveInputs()`/`RestoreInputs()` methods are intended for runtime data (what's flowing through the ports), not for port definitions (the schema).

**Code Location**: `include/flow/core/Node.hpp:232-233`
```cpp
protected:
    virtual json SaveInputs() const;      // Supposed to save runtime data
    virtual void RestoreInputs(const json&);  // Supposed to restore runtime data
```

### Why Defaults Are Stored in Ports

Default values are stored when you call `AddInput()` with optional data:
```cpp
template<typename T>
void AddInput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
{
    return AddInput(key, caption, TypeName_v<T>, std::move(data));  // data = default value
}
```

The `data` parameter is passed to the `Port` constructor (`src/Port.cpp:8-11`):
```cpp
Port::Port(const IndexableName& key, const std::string& caption, std::string_view type,
           SharedNodeData data, bool required, std::size_t index)
    : _data{std::move(data)}, ...  // Stored here, but never serialized
{
}
```

---

## Impact Analysis

### Severity: **HIGH**

This affects any real-world use case where:
- Graphs need to be saved and restored from files
- Nodes have default input values that affect behavior
- Graphs are loaded in different sessions or applications

### Affected Code Paths

1. **Graph Serialization** (`include/flow/core/Graph.hpp`, `src/Graph.cpp`):
   - When saving a graph, nodes are serialized via `node->Save()`
   - Port data is lost
   - On restoration, nodes don't have proper port configuration

2. **Module Loading** (`include/flow/core/Module.hpp`, `src/Module.cpp`):
   - Modules that contain graphs with default values lose that data

3. **Custom Node Implementation**:
   - Any custom node that relies on `SaveInputs()`/`RestoreInputs()`
   - Current implementation is undocumented and error-prone

---

## Proposed Solutions

### Solution 1: Base Class Handles Port Serialization (RECOMMENDED)

**Approach**: Move port metadata serialization to the base `Node` class.

**Changes Required**:

1. **Modify `Node::SaveInputs()`** in `src/Node.cpp:76`:
   ```cpp
   json Node::SaveInputs() const
   {
       json inputs = json::object();
       for (const auto& [key, port] : GetInputPorts()) {
           inputs[std::string(key)] = {
               {"caption", port->GetCaption()},
               {"type", std::string(port->GetDataType())},
               {"required", port->IsRequired()},
               // Note: Actual default values require INodeData serialization
               // which is type-specific and handled by derived classes
           };
       }
       return inputs;
   }
   ```

2. **Add new protected method `SaveInputDefaults()`** for derived classes to override:
   ```cpp
   protected:
       virtual json SaveInputDefaults() const { return {}; }
   ```

3. **Update `SaveInputs()` to use composition**:
   ```cpp
   json Node::SaveInputs() const
   {
       json result = json::object();

       // Part 1: Port definitions (handled by base class)
       json definitions = json::object();
       for (const auto& [key, port] : GetInputPorts()) {
           definitions[std::string(key)] = {
               {"caption", port->GetCaption()},
               {"type", std::string(port->GetDataType())},
               {"required", port->IsRequired()},
           };
       }
       result["__definitions"] = definitions;

       // Part 2: Default values (handled by derived class)
       result["__defaults"] = SaveInputDefaults();

       return result;
   }
   ```

**Advantages**:
- ✅ Base class handles common serialization automatically
- ✅ Derived classes only override for custom data
- ✅ Backward compatible (empty defaults for nodes that don't override)
- ✅ Minimal changes to existing code
- ✅ Clear separation of concerns

**Disadvantages**:
- ❌ Requires documenting the two-part approach
- ❌ Still requires type-specific serialization for actual default values
- ❌ Breaking change if code already interprets `SaveInputs()` data

**Implementation Effort**: Medium (2-3 hours)

---

### Solution 2: Port-Centric Serialization

**Approach**: Create a separate serialization system for port definitions.

**Changes Required**:

1. **Add to `Node` class**:
   ```cpp
   public:
       virtual json SavePortDefinitions() const;
       virtual void RestorePortDefinitions(const json& j);

   protected:
       // Base implementation
       json SaveInputPortDefinitions() const;
       json SaveOutputPortDefinitions() const;
   ```

2. **Update `Save()` method**:
   ```cpp
   json Node::Save() const
   {
       return {
           {"id", std::string(_id)},
           {"class", _class_name},
           {"name", _name},
           {"port_definitions", SavePortDefinitions()},  // NEW
           {"inputs", SaveInputs()},
       };
   }
   ```

3. **Port definitions include**:
   ```json
   {
       "port_definitions": {
           "inputs": {
               "in1": {
                   "caption": "Input 1",
                   "type": "int",
                   "required": false,
                   "index": 0
               }
           },
           "outputs": {
               "out1": {
                   "caption": "Output 1",
                   "type": "int",
                   "required": false,
                   "index": 0
               }
           }
       }
   }
   ```

**Advantages**:
- ✅ Clear separation between port schema and port data
- ✅ Single source of truth for port metadata
- ✅ Non-breaking change (add new field, keep existing)
- ✅ Enables validation: restore can check port definitions match

**Disadvantages**:
- ❌ More extensive refactoring
- ❌ Larger JSON files
- ❌ Reconstruction still requires port recreation logic

**Implementation Effort**: High (5-7 hours)

---

### Solution 3: Hybrid Approach - Default Values in Port Metadata

**Approach**: Track which ports have defaults, serialize actual defaults in `SaveInputDefaults()`.

**Changes Required**:

1. **Extend `Port` class** with default tracking:
   ```cpp
   class Port {
   private:
       SharedNodeData _default_value;  // Separate from current _data
       bool _has_default = false;

   public:
       bool HasDefaultValue() const { return _has_default; }
       const SharedNodeData& GetDefaultValue() const { return _default_value; }
   };
   ```

2. **Modify `Port` constructor**:
   ```cpp
   Port(const IndexableName& key, const std::string& caption, std::string_view type,
        SharedNodeData data, bool required, std::size_t index)
       : _data{std::move(data)},
         _default_value{data},  // Keep reference to original default
         _has_default{data != nullptr},
         ...
   {
   }
   ```

3. **Update `Node::SaveInputs()`**:
   ```cpp
   json Node::SaveInputs() const
   {
       json inputs = json::object();
       for (const auto& [key, port] : GetInputPorts()) {
           json port_data = {
               {"caption", port->GetCaption()},
               {"type", std::string(port->GetDataType())},
               {"required", port->IsRequired()},
               {"has_default", port->HasDefaultValue()},
           };

           // Add serialized default if present
           if (port->HasDefaultValue()) {
               port_data["default"] = port->GetDefaultValue()->ToJSON();
           }

           inputs[std::string(key)] = port_data;
       }
       return inputs;
   }
   ```

**Advantages**:
- ✅ Complete default value preservation
- ✅ All port metadata in one place
- ✅ No additional methods to override
- ✅ Clear distinction between default and current value

**Disadvantages**:
- ❌ Requires `INodeData::ToJSON()` interface
- ❌ May not serialize complex types easily
- ❌ Larger changes to Port class
- ❌ Potential memory overhead (storing defaults twice)

**Implementation Effort**: Medium (3-4 hours)

---

## Recommended Solution Path

**Implement Solution 1 (Base Class Handles Port Serialization)** with the following progression:

### Phase 1: Minimal Fix (1-2 hours)
- Modify `Node::SaveInputs()` to serialize port metadata (caption, type, required flag)
- Add documentation about the limitation on default values
- Update tests

### Phase 2: Default Values (2-3 hours)
- Add `SaveInputDefaults()` virtual method
- Update `Node::SaveInputs()` to include defaults section
- Modify key node types to implement `SaveInputDefaults()`
- Add comprehensive tests

### Phase 3: Restore Logic (2-3 hours)
- Implement `RestoreInputDefaults()`
- Add validation during restore to ensure port definitions match saved configuration
- Error handling for mismatched schemas
- Documentation and examples

---

## Testing Strategy

### Unit Tests Required

1. **Test port metadata is preserved**:
   ```cpp
   TEST(NodeSerialization, PortMetadataPreserved) {
       Node node = CreateNodeWithPorts();
       json saved = node.Save();

       // Verify all port metadata is in JSON
       ASSERT_TRUE(saved["inputs"].contains("input_key"));
       ASSERT_EQ(saved["inputs"]["input_key"]["caption"], "Expected Caption");
       ASSERT_EQ(saved["inputs"]["input_key"]["type"], "int");
   }
   ```

2. **Test default values are preserved**:
   ```cpp
   TEST(NodeSerialization, DefaultValuesPreserved) {
       Node node = CreateNodeWithDefaults();
       json saved = node.Save();

       ASSERT_TRUE(saved["inputs"]["with_default"]["has_default"]);
       ASSERT_FALSE(saved["inputs"]["no_default"]["has_default"]);
   }
   ```

3. **Test round-trip restoration**:
   ```cpp
   TEST(NodeSerialization, RoundTripPreservesState) {
       Node original = CreateComplexNode();
       json saved = original.Save();

       Node restored = NodeFactory::Create(saved);

       ASSERT_EQ(restored.GetInputPorts().size(), original.GetInputPorts().size());
       ASSERT_EQ(restored.GetOutputPorts().size(), original.GetOutputPorts().size());
       // Verify defaults match...
   }
   ```

### Integration Tests

4. **Test graph save/restore with nodes having defaults**
5. **Test module loading preserves node configuration**
6. **Test FunctionNode port metadata is preserved**

---

## Documentation Requirements

### User Documentation

1. **Node Serialization Guide**: Explain default behavior and how to customize
2. **Custom Node Implementation**: Document `SaveInputDefaults()` and `RestoreInputDefaults()`
3. **Best Practices**: When to override serialization methods

### Code Documentation

1. Update comments in `Node::SaveInputs()` explaining new behavior
2. Add doxygen comments for new methods
3. Add example in header files

---

## Migration Path

For existing code that manually overrides `SaveInputs()`:

**Before**:
```cpp
json SaveInputs() const override {
    json inputs = json::object();
    for (const auto& [key, port] : GetInputPorts()) {
        inputs[std::string(key)] = {
            {"caption", port->GetCaption()},
            {"type", port->GetDataType()},
        };
    }
    return inputs;
}
```

**After** (using Solution 1):
```cpp
json SaveInputDefaults() const override {
    json defaults = json::object();
    // Only custom default serialization
    if (auto default_val = GetInputPort("special")->GetDefaultValue()) {
        defaults["special"] = serialize_my_custom_type(default_val);
    }
    return defaults;
}
// Port metadata is now handled by base class automatically
```

---

## Risk Assessment

### Low Risk
- Adding new virtual methods (backward compatible via default implementations)
- Expanding JSON format (backward compatible if new fields are optional)

### Medium Risk
- Changing what `SaveInputs()` returns (could break existing code parsing this field)
- JSON format changes could affect graph file compatibility

### Mitigation
- Keep format expansions backward compatible (new fields are optional)
- Version the JSON format for graph files
- Provide migration utilities if needed
- Extensive testing before release

---

## Conclusion

The current serialization design is insufficient for production use of Flow-Core because it loses critical port metadata and default values. **Solution 1 (Base Class Handles Port Serialization)** is the recommended approach because it:

1. Fixes the core issue with minimal code changes
2. Remains backward compatible
3. Shifts the burden from derived classes to the framework
4. Provides a clear standard for serialization
5. Can be implemented incrementally

This should be prioritized as a critical fix before any production deployment of graphs that rely on default values or port metadata.
