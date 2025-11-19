# Node Metadata API Development Plan

## Executive Summary

This document outlines a comprehensive development plan for introducing a unified **Node Metadata API** to Flow-Core. This API will simultaneously address two critical issues:

1. **Serialization Problem** (`serial_to_json_issue.md`): Port metadata and defaults are lost during graph save/restore
2. **UI Introspection Problem** (`flow-core_port_metadata.md`): Tools cannot query port configuration from running nodes

By implementing a single unified metadata system, we solve both problems with a consistent, extensible API that serves as the "contract" between node implementations, serialization, UI tooling, and validation systems.

**Estimated Total Effort**: 6-7 weeks across 4 phases
**Risk Level**: Medium (introduces new API surface, changes serialization format)
**Priority**: High (critical for production use)

---

## Part 1: Current State Analysis

### Current Node API Surface

#### Public Methods (Read-Only)
```cpp
class Node {
  public:
    // Identity
    [[nodiscard]] const UUID& ID() const noexcept;
    [[nodiscard]] const std::string& GetName() const noexcept;
    [[nodiscard]] const std::string& GetClass() const noexcept;
    [[nodiscard]] const std::shared_ptr<Env>& GetEnv() const;

    // Port queries - Limited to names and types
    [[nodiscard]] const PortMap& GetInputPorts() const noexcept;
    [[nodiscard]] const PortMap& GetOutputPorts() const noexcept;
    const SharedPort& GetInputPort(const IndexableName& key) const;
    const SharedPort& GetOutputPort(const IndexableName& key) const;

    // Data access
    [[nodiscard]] const SharedNodeData& GetInputData(const IndexableName& key) const;
    [[nodiscard]] const SharedNodeData& GetOutputData(const IndexableName& key) const;

    template<typename T>
    [[nodiscard]] auto GetInputData(const IndexableName& key) const noexcept;

    template<typename T>
    [[nodiscard]] auto GetOutputData(const IndexableName& key) const noexcept;

    // Serialization
    json Save() const;
    void Restore(const json& j);

    // Thread safety
    void lock();
    void unlock();
};
```

#### Protected Methods
```cpp
  protected:
    // Port definition
    void AddInput(std::string_view key, const std::string& caption,
                  std::string_view type, SharedNodeData data);
    void AddOutput(std::string_view key, const std::string& caption,
                   std::string_view type, SharedNodeData data);

    template<typename T>
    void AddInput(std::string_view key, const std::string& caption,
                  SharedNodeData data = nullptr);

    template<typename T>
    void AddOutput(std::string_view key, const std::string& caption,
                   SharedNodeData data = nullptr);

    template<typename T>
    void AddRequiredInput(std::string_view key, const std::string& caption,
                          std::remove_reference_t<T>& data);

    // Lifecycle
    virtual void Start();
    virtual void Stop();
    virtual void Compute() = 0;

    // Serialization hooks
    virtual json SaveInputs() const;
    virtual void RestoreInputs(const json&);

    // Data propagation
    void EmitUpdate(const IndexableName& key, const SharedNodeData& data);
};
```

#### Data Setting Methods
```cpp
  public:
    void SetInputData(const IndexableName& key, SharedNodeData data, bool compute = true);
    void SetOutputData(const IndexableName& key, SharedNodeData data = nullptr, bool emit = true);
    void SetName(std::string new_name);
```

#### Events
```cpp
    EventDispatcher<> OnCompute;
    EventDispatcher<const IndexableName&, const SharedNodeData&> OnSetInput;
    EventDispatcher<const IndexableName&, const SharedNodeData&> OnSetOutput;
    EventDispatcher<const std::exception&> OnError;
    EventDispatcher<const UUID&, const IndexableName&, const SharedNodeData&> OnEmitOutput;
```

### Current Port API Surface

```cpp
class Port {
  public:
    // Construction
    Port(const IndexableName& key, const std::string& caption, std::string_view type,
         SharedNodeData data, bool required, std::size_t index);

    // Status queries
    bool IsConnected() const noexcept;
    bool Connect() noexcept;
    bool Disconnect() noexcept;

    // Data queries
    const SharedNodeData& GetData() const noexcept;
    const IndexableName& GetKey() const noexcept;
    std::string_view GetVarName() const noexcept;
    std::string_view GetCaption() const noexcept;
    std::string_view GetDataType() const noexcept;
    bool IsRequired() const noexcept;
    std::size_t Index() const noexcept;

    // Data modification
    void SetData(SharedNodeData data, bool output = false);
    void SetCaption(std::string new_caption);

    // Events
    Event<const IndexableName&, const SharedNodeData&, bool> OnSetData;
};
```

### Current Serialization Behavior

**Current `Node::Save()` output**:
```json
{
  "id": "3fffa214-f786-4165-ae8b-373fee0fc5d7",
  "class": "NodeTest::TestNode",
  "name": "MyTestNode",
  "inputs": null
}
```

**Problems**:
- ❌ No port definitions persisted
- ❌ No default values recorded
- ❌ No type information
- ❌ No port captions/descriptions
- ❌ No constraints or metadata
- ❌ Graph restoration cannot validate port compatibility

---

## Part 2: Proposed Metadata API

### Proposed New JSON Format

**Enhanced `Node::Save()` output with metadata**:
```json
{
  "id": "3fffa214-f786-4165-ae8b-373fee0fc5d7",
  "class": "NodeTest::TestNode",
  "name": "MyTestNode",
  "port_definitions": {
    "inputs": [
      {
        "key": "input_no_default",
        "caption": "Integer Input",
        "type": "int",
        "index": 0,
        "required": false,
        "has_default": false,
        "direction": "input",
        "constraints": null,
        "metadata": null
      },
      {
        "key": "input_with_default",
        "caption": "Float Input",
        "type": "float",
        "index": 1,
        "required": false,
        "has_default": true,
        "default_value": "3.14",
        "direction": "input",
        "constraints": {
          "min": 0.0,
          "max": 100.0,
          "step": 0.1
        },
        "metadata": null
      }
    ],
    "outputs": [
      {
        "key": "output_result",
        "caption": "Result Output",
        "type": "int",
        "index": 0,
        "required": false,
        "has_default": false,
        "direction": "output",
        "constraints": null,
        "metadata": null
      }
    ]
  },
  "inputs": null
}
```

### New Public APIs

#### Port Metadata Query (Single Port)
```cpp
class Node {
  public:
    // Get metadata for a single input port
    json GetInputPortMetadata(const IndexableName& key) const;

    // Get metadata for a single output port
    json GetOutputPortMetadata(const IndexableName& key) const;

    // Get metadata for all input ports as array
    json GetAllInputPortsMetadata() const;

    // Get metadata for all output ports as array
    json GetAllOutputPortsMetadata() const;

    // Get combined input and output port definitions
    json GetPortDefinitions() const;

    // Get complete node metadata (includes port definitions and identity)
    json GetNodeMetadata() const;
};
```


#### Updated Serialization APIs
```cpp
class Node {
  protected:
    // NEW: Allows derived classes to contribute custom default value serialization
    virtual json SaveInputDefaults() const;
    virtual void RestoreInputDefaults(const json&);

    // UPDATED: Now includes port_definitions automatically
    json Save() const override;
    void Restore(const json& j) override;
};
```

### Port Metadata Structure Definition

```cpp
// New type: PortMetadata
struct PortMetadata {
    std::string key;                          // Port identifier
    std::string caption;                      // Display name
    std::string type;                         // Type string (e.g., "int", "float")
    std::string direction;                    // "input" or "output"
    std::size_t index;                        // Sort order
    bool required;                            // true if must be connected/provided
    bool has_default;                         // true if default value exists
    std::optional<std::string> default_value; // Serialized default (if has_default)
    std::optional<json> constraints;          // min, max, step, allowed_values, pattern
    std::optional<json> metadata;             // Custom metadata for extensions

    // Conversion to JSON
    json ToJSON() const;

    // Construction from JSON
    static PortMetadata FromJSON(const json& j);
};

// New type: PortDefinitions
struct PortDefinitions {
    std::vector<PortMetadata> inputs;
    std::vector<PortMetadata> outputs;

    json ToJSON() const;
    static PortDefinitions FromJSON(const json& j);
};
```

### Constraint Handling & Validation

#### Constraint Definitions

Constraints are optional metadata that describe validation rules for port data. They are stored as JSON objects in `PortMetadata::constraints`. The following constraint types are supported:

**Numeric Constraints** (for `int`, `float`, `double`):
```json
{
  "min": 0,
  "max": 100,
  "step": 5
}
```

**String Constraints** (for `std::string`):
```json
{
  "min_length": 1,
  "max_length": 256,
  "pattern": "^[a-zA-Z0-9_]+$"
}
```

**Enumeration Constraints** (for any type):
```json
{
  "allowed_values": ["option1", "option2", "option3"]
}
```

**Size Constraints** (for collections):
```json
{
  "min_items": 1,
  "max_items": 100
}
```

**Custom Constraints** (extensible for node-specific rules):
```json
{
  "custom_rule_name": "custom_value",
  "description": "Explanation of constraint"
}
```

#### Setting Constraints

Constraints are defined at port creation time and are immutable after creation. There are two ways to set constraints:

**1. At Port Creation** (Recommended):
```cpp
// In Node subclass constructor
void AddInputWithConstraints(std::string_view key,
                            const std::string& caption,
                            std::string_view type,
                            SharedNodeData data,
                            const json& constraints);
```

**Example**:
```cpp
AddInputWithConstraints("vbr", "Variable Bitrate", "int",
                       MakeNodeData<int>(128),
                       json{
                           {"min", 0},
                           {"max", 320},
                           {"step", 1},
                           {"description", "Audio bitrate in kbps"}
                       });
```

**2. Post-Creation via Port API**:
```cpp
// In Node subclass
void ConfigureConstraints() {
    if (auto port = GetInputPort("quality_level")) {
        port->SetConstraints(json{
            {"allowed_values", {"low", "medium", "high"}},
            {"description", "Output quality level"}
        });
    }
}
```

#### Constraint Application & Validation

Constraints serve two purposes:

**1. UI Validation** (Client-side):
- UI tools read constraints from `GetInputPortMetadata()`
- Generate appropriate input controls (sliders for range, dropdowns for enums, etc.)
- Enforce constraints before allowing value submission
- Display error messages for constraint violations

**2. Runtime Validation** (Server-side, optional):
Nodes can optionally validate input data against constraints before computation:

```cpp
class AudioCodecNode : public Node {
  protected:
    void Compute() override {
        // Get input with optional constraint validation
        auto vbr = GetInputData<int>("vbr");

        // Manual validation against constraints
        if (!ValidateConstraints("vbr", vbr)) {
            throw std::invalid_argument("VBR constraint violation");
        }

        // ... continue with computation
    }

  private:
    bool ValidateConstraints(const std::string& port_key,
                            const SharedNodeData& data) const {
        auto metadata = GetInputPortMetadata(port_key);
        auto constraints = metadata["constraints"];

        if (constraints.is_null()) {
            return true;  // No constraints
        }

        // Type-specific validation logic
        if (constraints.contains("min")) {
            auto value = data->Get<int>();
            if (value < constraints["min"].get<int>()) {
                return false;
            }
        }

        if (constraints.contains("max")) {
            auto value = data->Get<int>();
            if (value > constraints["max"].get<int>()) {
                return false;
            }
        }

        return true;
    }
};
```

#### Constraint Serialization & Restoration

Constraints are automatically persisted in the `port_definitions` section of saved graphs:

```json
{
  "port_definitions": {
    "inputs": [
      {
        "key": "vbr",
        "caption": "Variable Bitrate",
        "type": "int",
        "constraints": {
          "min": 0,
          "max": 320,
          "step": 1,
          "description": "Audio bitrate in kbps"
        }
      }
    ]
  }
}
```

When a graph is restored:
1. Port definitions are read from JSON
2. Constraints are loaded into the Port object
3. Constraints become available via `GetInputPortMetadata()`
4. UI tools can immediately access constraint information without re-instantiation

#### Design Decisions

**Why Constraints Are Optional**:
- Not all ports need validation rules
- Keeps port definition schema simple
- Allows gradual adoption in existing nodes

**Why Constraints Are Immutable**:
- Prevents inconsistencies between serialized and runtime state
- Simplifies validation logic (no need to handle dynamic changes)
- Port definition is a contract that shouldn't change during execution

**Why Validation Is Optional in Compute**:
- Nodes may have complex validation logic specific to their domain
- UI already enforces constraints on input
- Server-side validation is defensive programming best practice but adds overhead
- Nodes can choose the validation strategy that fits their use case

**Constraint Scope**:
- Constraints apply to port data values, not to connections
- A port can be `required` (must be connected) AND have constraints on its data
- Constraint validation happens on data received via SetInputData, not at connection time

---

## Part 3: Impacted Data Structures

### 1. Port Class Changes

**File**: `include/flow/core/Port.hpp`

#### New Members
```cpp
class Port {
  private:
    // NEW: Separate tracking of default value from current value
    SharedNodeData _default_value;           // Original default (immutable)
    std::optional<json> _constraints;        // Validation constraints
    std::optional<json> _metadata;           // Custom metadata

    // EXISTING: Keep for backward compatibility
    std::shared_ptr<INodeData> _data;        // Current value
    IndexableName _key;
    std::string _caption;
    std::string _type;
    bool _required;
    bool _connected;
    std::size_t _index;
};
```

#### New Public Methods
```cpp
  public:
    // NEW: Metadata queries
    bool HasDefaultValue() const noexcept;
    const SharedNodeData& GetDefaultValue() const noexcept;

    std::optional<const json&> GetConstraints() const noexcept;
    void SetConstraints(json constraints);

    std::optional<const json&> GetMetadata() const noexcept;
    void SetMetadata(json metadata);

    // NEW: Serialization
    json GetMetadata() const;
    static PortMetadata MetadataFromPort(const Port& port);
```

#### Updated Constructor
```cpp
  public:
    // UPDATED: Now captures default value separately
    Port(const IndexableName& key,
         const std::string& caption,
         std::string_view type,
         SharedNodeData data,           // Now becomes _default_value
         bool required,
         std::size_t index);
```

### 2. Node Class Changes

**File**: `include/flow/core/Node.hpp`

#### New Private Members
```cpp
class Node {
  private:
    // NEW: Cached port definitions (populated during AddInput/AddOutput)
    PortDefinitions _port_definitions;
};
```

#### New Public Methods (see "Proposed New Public APIs" section above)

#### Updated Protected Methods
```cpp
  protected:
    // UPDATED: Port definition methods now also update _port_definitions
    void AddInput(std::string_view key, const std::string& caption,
                  std::string_view type, SharedNodeData data);
    void AddOutput(std::string_view key, const std::string& caption,
                   std::string_view type, SharedNodeData data);

    // NEW: Allow derived classes to specify constraints at port creation time
    void AddInputWithConstraints(std::string_view key,
                                const std::string& caption,
                                std::string_view type,
                                SharedNodeData data,
                                const json& constraints);
    void AddOutputWithConstraints(std::string_view key,
                                 const std::string& caption,
                                 std::string_view type,
                                 SharedNodeData data,
                                 const json& constraints);

    // NEW: Serialization hooks for custom defaults
    virtual json SaveInputDefaults() const;
    virtual void RestoreInputDefaults(const json&);

    // UPDATED: Now delegates to SaveInputDefaults
    virtual json SaveInputs() const override;
    virtual void RestoreInputs(const json& j) override;
};
```

#### Updated Save/Restore Methods
```cpp
  public:
    // UPDATED: Now includes port_definitions
    json Save() const;
    void Restore(const json& j);
```

### 3. New Type Definitions

**File**: `include/flow/core/PortMetadata.hpp` (NEW FILE)

```cpp
namespace flow {

/**
 * @brief Metadata for a single port, queryable at runtime.
 *
 * Serves as the "contract" between:
 * - Node implementations (what they expose)
 * - Serialization system (what it saves)
 * - UI/tooling (what it queries)
 * - Validation system (what it enforces)
 */
struct PortMetadata {
    std::string key;                          // Unique port identifier
    std::string caption;                      // Display name for UI
    std::string type;                         // Type string (e.g., "int")
    std::string direction;                    // "input" or "output"
    std::size_t index;                        // Sort order in port list

    // Configuration flags
    bool required;                            // true if must be connected
    bool has_default;                         // true if default value exists

    // Optional data fields
    std::optional<std::string> default_value; // Serialized default value
    std::optional<json> constraints;          // {min, max, step, allowed_values, pattern}
    std::optional<json> metadata;             // Custom metadata for extensions

    /**
     * @brief Convert metadata to JSON representation.
     * @returns JSON object conforming to port metadata schema
     */
    json ToJSON() const;

    /**
     * @brief Construct metadata from JSON.
     * @param j JSON object to parse
     * @returns Parsed PortMetadata
     */
    static PortMetadata FromJSON(const json& j);

    /**
     * @brief Validate that JSON conforms to metadata schema.
     * @param j JSON to validate
     * @returns true if valid, false otherwise
     */
    static bool ValidateSchema(const json& j);
};

/**
 * @brief Complete port definition set for a node.
 *
 * Represents all input and output port definitions.
 */
struct PortDefinitions {
    std::vector<PortMetadata> inputs;
    std::vector<PortMetadata> outputs;

    /**
     * @brief Convert definitions to JSON.
     * @returns JSON object with "inputs" and "outputs" arrays
     */
    json ToJSON() const;

    /**
     * @brief Construct definitions from JSON.
     * @param j JSON to parse
     * @returns Parsed PortDefinitions
     */
    static PortDefinitions FromJSON(const json& j);

    /**
     * @brief Get metadata for a single port by key.
     * @param key Port identifier
     * @param direction "input" or "output"
     * @returns PortMetadata if found, empty optional otherwise
     */
    std::optional<PortMetadata> GetPort(const std::string& key,
                                       const std::string& direction) const;
};

} // namespace flow
```

### 4. Graph Class Impact

**File**: `include/flow/core/Graph.hpp`, `src/Graph.cpp`

#### Changes Required
```cpp
class Graph {
  public:
    // UPDATED: Save now preserves port definitions
    json Save() const;
    void Restore(const json& j);

    // VALIDATION: New method to validate restored graph
    // Checks that saved port definitions match current node implementations
    bool ValidatePortDefinitions(const json& saved_graph) const;
};
```

### 5. Module Class Impact

**File**: `include/flow/core/Module.hpp`, `src/Module.cpp`

#### Changes Required
```cpp
class Module {
  public:
    // UPDATED: Embedded graphs now preserve port definitions
    json Save() const;
    void Restore(const json& j);
};
```

### 6. FunctionNode Class Impact

**File**: `include/flow/core/FunctionNode.hpp`

#### Changes Required
```cpp
template<concepts::Function F, std::add_pointer_t<std::remove_pointer_t<F>> Func>
class FunctionNode : public Node {
  protected:
    // UPDATED: ParseArguments now populates _port_definitions
    template<int... Idx>
    void ParseArguments(std::integer_sequence<int, Idx...>,
                       std::vector<std::string> arg_names);

    // NEW: Optional method for derived classes to add constraints
    virtual void ConfigurePortConstraints() {}
};
```

---

## Part 4: Current vs. Proposed API Comparison

### Port Query Capabilities

| Capability | Current | Proposed | Benefit |
|------------|---------|----------|---------|
| Get port names | ✓ GetInputPorts() | ✓ GetAllInputPortsMetadata() | More structured |
| Get port type | ✓ GetInputPort()->GetDataType() | ✓ metadata.type | Direct access |
| Get port caption | ✓ GetInputPort()->GetCaption() | ✓ metadata.caption | Direct access |
| Get default value | ❌ | ✓ metadata.default_value | **NEW** |
| Check has default | ❌ | ✓ metadata.has_default | **NEW** |
| Get constraints | ❌ | ✓ metadata.constraints | **NEW** |
| Validate port compatibility | ❌ | ✓ via PortDefinitions | **NEW** |

### Serialization Capabilities

| Capability | Current | Proposed | Benefit |
|------------|---------|----------|---------|
| Serialize port names | ❌ | ✓ Save() includes port_definitions | **NEW** |
| Serialize port types | ❌ | ✓ Save() includes port_definitions | **NEW** |
| Serialize defaults | ❌ | ✓ Save() includes port_definitions | **NEW** |
| Serialize constraints | ❌ | ✓ Save() includes port_definitions | **NEW** |
| Serialize captions | ❌ | ✓ Save() includes port_definitions | **NEW** |
| Restore port schema | ❌ | ✓ Restore() validates port_definitions | **NEW** |
| Backward compatible | N/A | ✓ Old graphs still load | **Design** |
| Custom defaults | ✓ SaveInputs() | ✓ SaveInputDefaults() | Clearer purpose |

### API Complexity

| Aspect | Current | Proposed | Impact |
|--------|---------|----------|--------|
| Public Node methods | ~20 | ~25 | +5 methods (+25%) |
| Public Port methods | ~10 | ~14 | +4 methods (+40%) |
| New types | 0 | 2 (PortMetadata, PortDefinitions) | Improved structure |
| Breaking changes | N/A | Minimal - backward compatible | Low risk |

---

## Part 5: Development Phases

### Phase 1: Foundation & Port Metadata (Weeks 1-2)

**Goal**: Establish metadata infrastructure without breaking changes

#### Deliverables
- [ ] New `PortMetadata` struct with JSON serialization
- [ ] New `PortDefinitions` struct
- [ ] Port class enhancements for metadata tracking
- [ ] Comprehensive unit tests
- [ ] Documentation

#### Implementation Details

**1.1 Create PortMetadata types** (`include/flow/core/PortMetadata.hpp`)
- Define `PortMetadata` struct
- Define `PortDefinitions` struct
- Implement JSON serialization/deserialization
- Add schema validation

**1.2 Enhance Port class** (`include/flow/core/Port.hpp`, `src/Port.cpp`)
- Add `_default_value` member (keep separate from `_data`)
- Add `_constraints` optional member
- Add `_metadata` optional member
- Add accessor methods: `HasDefaultValue()`, `GetDefaultValue()`, `GetConstraints()`, `GetMetadata()`
- Update constructor to capture default separately
- Add `PortMetadata MetadataFromPort()` method

**1.3 Update Port::SetData logic**
- Ensure `_default_value` is immutable (set once in constructor)
- Current `_data` can still be modified via `SetData()`
- Add `GetDefaultValue()` to retrieve original default

**1.4 Tests** (`tests/port_metadata_test.cpp`)
```cpp
TEST(PortMetadata, SerializationRoundTrip) { }
TEST(PortMetadata, DefaultValueTracking) { }
TEST(PortMetadata, ConstraintsStorage) { }
TEST(PortMetadata, CustomMetadata) { }
```

#### Files Modified
- `include/flow/core/PortMetadata.hpp` (NEW)
- `src/PortMetadata.cpp` (NEW)
- `include/flow/core/Port.hpp`
- `src/Port.cpp`
- `tests/port_metadata_test.cpp` (NEW)

#### Testing Strategy
- Unit tests for PortMetadata struct
- Unit tests for Port metadata methods
- Backward compatibility tests (ensure existing Port functionality unchanged)

#### Estimated Effort: 1 week
- Type definitions: 2 days
- Port class updates: 3 days
- Tests: 2 days

---

### Phase 2: Node Metadata API & Serialization (Weeks 3-4)

**Goal**: Add metadata query methods to Node; update serialization to include port definitions

#### Deliverables
- [ ] New public metadata query methods on Node
- [ ] Updated `Node::Save()` to include `port_definitions`
- [ ] Updated `Node::Restore()` with validation
- [ ] New `SaveInputDefaults()` / `RestoreInputDefaults()` hooks
- [ ] Comprehensive tests
- [ ] Migration guide for custom nodes

#### Implementation Details

**2.1 Add metadata query methods to Node** (`include/flow/core/Node.hpp`, `src/Node.cpp`)
```cpp
// Single port queries
json GetInputPortMetadata(const IndexableName& key) const;
json GetOutputPortMetadata(const IndexableName& key) const;

// Batch queries
json GetAllInputPortsMetadata() const;
json GetAllOutputPortsMetadata() const;

// Complete definitions
json GetPortDefinitions() const;
json GetNodeMetadata() const;
```

**2.2 Update Node::Save()** (`src/Node.cpp`)
- Include `port_definitions` in saved JSON
- Maintain backward compatibility (still include `inputs` field)
- Call new `SaveInputDefaults()` method

**2.3 Update Node::Restore()** (`src/Node.cpp`)
- Extract and validate `port_definitions` if present
- Call new `RestoreInputDefaults()` method
- Handle old format (missing `port_definitions`)

**2.4 Add serialization hooks** (`include/flow/core/Node.hpp`, `src/Node.cpp`)
```cpp
protected:
    virtual json SaveInputDefaults() const;
    virtual void RestoreInputDefaults(const json&);
```

**2.5 Implement Constraint Handling**
- Implement `AddInputWithConstraints()` and `AddOutputWithConstraints()` methods
- Implement `Port::SetConstraints()` and `Port::GetConstraints()` methods
- Constraints are stored in `Port::_constraints` as optional JSON
- Constraints are serialized/deserialized as part of `port_definitions`
- Constraints are immutable after port creation (can be set at creation or via Port API before graph runs)
- Support constraint types: numeric (min/max/step), string (pattern/length), enumeration, custom
- Document constraint best practices and validation patterns

**2.6 Update FunctionNode** (`include/flow/core/FunctionNode.hpp`)
- Ensure `ParseArguments()` populates port definitions
- Add support for auto-generating constraints where applicable (e.g., numeric types could have default min/max)
- Provide virtual `ConfigurePortConstraints()` method for derived classes to override
- Example: function parameters with numeric types could suggest constraint ranges

**2.7 Tests** (`tests/node_metadata_test.cpp`, `tests/constraint_test.cpp`)
```cpp
// Node Metadata Tests
TEST(NodeMetadata, SaveIncludesPortDefinitions) { }
TEST(NodeMetadata, RestoreValidatesPortDefinitions) { }
TEST(NodeMetadata, GetInputPortMetadata) { }
TEST(NodeMetadata, GetAllInputPortsMetadata) { }
TEST(NodeMetadata, GetAllOutputPortsMetadata) { }
TEST(NodeMetadata, BackwardCompatibilityWithOldFormat) { }
TEST(NodeMetadata, CustomNodeDefaults) { }

// Constraint Tests
TEST(Constraint, SetConstraintsAtCreation) { }
TEST(Constraint, SetConstraintsViaPort) { }
TEST(Constraint, ConstraintsSerializedInPortDefinitions) { }
TEST(Constraint, ConstraintsRestoredFromJSON) { }
TEST(Constraint, NumericConstraints) { }
TEST(Constraint, StringConstraints) { }
TEST(Constraint, EnumerationConstraints) { }
TEST(Constraint, CustomConstraints) { }
TEST(Constraint, ConstraintImmutability) { }
```

#### Files Modified
- `include/flow/core/Node.hpp`
- `src/Node.cpp`
- `include/flow/core/Port.hpp`
- `src/Port.cpp`
- `include/flow/core/FunctionNode.hpp`
- `src/FunctionNode.cpp`
- `tests/node_metadata_test.cpp` (NEW)
- `tests/constraint_test.cpp` (NEW)

#### Testing Strategy
- Test metadata generation for various node types
- Test round-trip save/restore
- Test backward compatibility with old JSON format
- Test custom node implementations
- Test constraint setting at port creation time
- Test constraint setting via Port API
- Test constraint serialization and restoration
- Test constraint immutability (prevent modification after creation)
- Test all constraint types (numeric, string, enum, custom)
- Test constraint interactions with default values

#### Estimated Effort: 2 weeks
- Constraint infrastructure: 2 days
- Method implementations: 2 days
- Constraint handling: 2 days
- Serialization updates: 2 days
- Tests: 3 days
- Documentation: 1 day

---

### Phase 3: Graph & Module Updates (Weeks 5-6)

**Goal**: Update Graph and Module serialization to leverage metadata; implement constraint validation

#### Deliverables
- [ ] Updated `Graph::Save()` / `Graph::Restore()`
- [ ] Updated `Module::Save()` / `Module::Restore()`
- [ ] Port definition validation
- [ ] Comprehensive tests
- [ ] Migration documentation

#### Implementation Details

**3.1 Update Graph class** (`include/flow/core/Graph.hpp`, `src/Graph.cpp`)
- Update `Save()` to call node's new metadata methods
- Update `Restore()` to validate port definitions
- Add `ValidatePortDefinitions()` method

**3.2 Update Module class** (`include/flow/core/Module.hpp`, `src/Module.cpp`)
- Update `Save()` to preserve node metadata
- Update `Restore()` with validation
- Ensure embedded graphs preserve port definitions

**3.3 Tests** (`tests/graph_metadata_test.cpp`, `tests/module_metadata_test.cpp`)
```cpp
TEST(GraphMetadata, SaveIncludesNodePortDefinitions) { }
TEST(GraphMetadata, RestoreValidatesPortDefinitions) { }
TEST(GraphMetadata, ValidationReportsErrors) { }
TEST(ModuleMetadata, EmbeddedGraphsPreserveMetadata) { }
```

#### Files Modified
- `include/flow/core/Graph.hpp`
- `src/Graph.cpp`
- `include/flow/core/Module.hpp`
- `src/Module.cpp`
- `tests/graph_metadata_test.cpp` (NEW)
- `tests/module_metadata_test.cpp` (NEW)

#### Testing Strategy
- Graph save/restore with metadata
- Module save/restore with metadata
- Port definition validation
- Migration from old format

#### Estimated Effort: 2 weeks
- Graph updates: 2 days
- Module updates: 2 days
- Constraint validation: 1.5 days
- Tests: 3 days
- Documentation/polish: 1.5 days

---

### Phase 4: Documentation & Polish (Week 7)

**Goal**: Complete documentation, examples, and final testing

#### Deliverables
- [ ] API documentation (Doxygen)
- [ ] User guide for metadata API
- [ ] Migration guide for custom nodes
- [ ] Architectural overview
- [ ] Example applications
- [ ] Final testing and bug fixes

#### Implementation Details

**5.1 API Documentation**
- Update Doxygen comments for all new methods
- Document JSON schema for metadata
- Document constraints format
- Document error codes and error handling

**5.2 User Guides**
- "Using Node Metadata in Custom Nodes" guide
- "Visual Editor Integration Guide" for port metadata
- "Serialization and Metadata" guide
- Troubleshooting guide

**5.3 Migration Guide**
- How to update custom `SaveInputs()` to use new hooks
- Backward compatibility notes
- What changed in JSON format

**5.4 Examples**
- Simple metadata query example
- Visual node editor integration
- Graph save/restore with metadata
- Custom node with constraints

**5.5 Final Testing**
- Full regression test suite
- Performance testing
- Memory profiling
- Edge case testing

#### Deliverables
- Technical documentation
- API reference
- User guides
- Migration guide
- Example code
- Test suite enhancements

#### Files Modified
- `README.md`
- `docs/` directory (new documentation files)
- `examples/` directory (new example files)

#### Estimated Effort: 1 week
- Documentation: 3 days
- Examples: 2 days
- Testing and polish: 2 days

---

## Part 6: Detailed Implementation Schedule

### Week 1-2: Phase 1 - Foundation
```
Mon-Tue:  Design PortMetadata types, review with team
Wed-Thu:  Implement PortMetadata struct and JSON serialization
Fri:      Enhance Port class, add metadata members
Mon-Tue:  Implement Port metadata accessors
Wed-Thu:  Comprehensive unit tests
Fri:      Documentation, code review
```

### Week 3-4: Phase 2 - Node Metadata API & Constraints
```
Mon-Tue:  Implement Node metadata query methods
Wed-Thu:  Update Node::Save() and Node::Restore()
Fri:      Add SaveInputDefaults() / RestoreInputDefaults() hooks

Mon-Tue:  Implement constraint infrastructure
Wed-Thu:  Implement AddInputWithConstraints/AddOutputWithConstraints
Fri:      Implement Port constraint methods and serialization

Mon:      Update FunctionNode for constraint support
Tue-Wed:  Comprehensive node metadata and constraint tests
Thu-Fri:  Code review, fixes, migration guide
```

### Week 5-6: Phase 3 - Graph & Module
```
Mon-Tue:  Update Graph save/restore, constraint validation
Wed:      Update Module save/restore
Thu:      Comprehensive graph and module tests
Fri:      Code review, fixes
```

### Week 7: Phase 4 - Documentation & Polish
```
Mon-Wed:  Complete documentation, constraint guides, examples
Thu-Fri:  Final testing, bug fixes, release prep
```

---

## Part 7: Risk Assessment & Mitigation

### Risk: Breaking Changes

**Impact**: High - Could break existing code that relies on current serialization format
**Probability**: Medium
**Mitigation**:
- Keep `inputs` field in JSON for backward compatibility
- Old graphs without `port_definitions` still load
- Add version number to graph format
- Provide migration utilities

### Risk: Performance Impact

**Impact**: Medium - Additional metadata queries could slow down hot paths
**Probability**: Low
**Mitigation**:
- Cache metadata results in Node class
- Lazy evaluation of JSON serialization
- Profile before and after implementation
- Optimize hot paths

### Risk: JSON Schema Evolution

**Impact**: Medium - Changes to metadata schema could break tools
**Probability**: Medium
**Mitigation**:
- Version the metadata schema
- Add extensibility via `metadata` field
- Document schema clearly
- Provide schema validator

### Risk: Feature Creep

**Impact**: High - Scope could expand beyond plan
**Probability**: Medium
**Mitigation**:
- Strict phase gates
- Clear scope definition
- Feature flagging for experimental features
- Regular status reviews

---

## Part 8: Success Criteria

### Phase 1 Success
- [ ] PortMetadata and PortDefinitions types fully implemented
- [ ] Port class metadata tracking working
- [ ] All unit tests passing
- [ ] No performance degradation

### Phase 2 Success
- [ ] All Node metadata query methods working
- [ ] Save/restore includes port definitions
- [ ] Backward compatibility verified
- [ ] All node-level tests passing
- [ ] FunctionNode port metadata correct

### Phase 3 Success
- [ ] Graph serialization includes node metadata
- [ ] Module embedded graphs preserve metadata
- [ ] Port definition validation working
- [ ] All graph/module tests passing

### Phase 4 Success
- [ ] Complete API documentation
- [ ] Migration guide for custom nodes
- [ ] Example applications
- [ ] Full test coverage (>90%)
- [ ] No known bugs in final testing

### Overall Success Criteria
- ✅ Graphs save and restore with complete port configuration
- ✅ UI tools can query port metadata from running nodes
- ✅ Backward compatible with existing graphs
- ✅ Thread-safe for concurrent queries
- ✅ Well documented with examples
- ✅ No breaking changes to existing public API (except Save/Restore format)

---

## Part 9: Resource Requirements

### Team Composition
- **Lead Developer** (1): Architecture, coordination, critical paths
- **Core Developers** (1-2): Implementation of core functionality
- **QA Engineer** (1): Testing, validation
- **Documentation Writer** (0.5): Guides, migration docs
- **Architect** (0.5, part-time): Design reviews, decisions

### Tools & Infrastructure
- C++20 compiler (existing)
- nlohmann/json library (existing)
- GoogleTest framework (existing)
- Documentation tools (Doxygen, Markdown)

### Dependencies
- No new external dependencies required
- Leverages existing infrastructure

---

## Part 10: Conclusion

The unified **Node Metadata API** provides a comprehensive solution to two critical problems:

1. **Serialization**: Port definitions and defaults are now persisted during save/restore
2. **Introspection**: UI tools can query port metadata without temporary node instantiation

### Key Benefits
✅ **Single Source of Truth**: Port metadata defined once, used everywhere
✅ **Backward Compatible**: Old graphs still load, new features optional
✅ **Extensible**: Custom metadata supported for future enhancements
✅ **Well-Defined**: Clear JSON schema for port metadata
✅ **Tooling-Friendly**: Supports visual editors and introspection

### Timeline
**Total Estimated Effort**: 6-7 weeks
**Phase Breakdown**:
- Phase 1: Foundation (2 weeks)
- Phase 2: Node API & Constraints (2 weeks)
- Phase 3: Graph/Module (2 weeks)
- Phase 4: Documentation & Polish (1 week)

This phased approach allows for:
- Early validation of core concepts (Phase 1)
- Incremental feature delivery
- Continuous testing and quality assurance
- Clear stakeholder communication at each phase gate
- Flexibility to adjust scope based on learnings

The metadata API will be a foundational improvement enabling modern visual programming tooling and production-ready graph serialization for Flow-Core.
