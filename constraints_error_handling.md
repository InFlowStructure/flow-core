# Constraint Validation and Error Handling Strategy

## Overview

This document analyzes the design decisions required for handling values that violate port constraints. It explores various use cases, proposes validation strategies, and provides recommendations for implementation.

---

## Problem Statement

When a port has constraints (e.g., min=0, max=320 for bitrate), what happens if:
1. A UI sets a value that violates the constraint?
2. A graph is restored with data that now violates constraints?
3. A node sets its own output data to an invalid value?
4. Data flows through a connection to a port with constraints?
5. A test needs to set an invalid value to verify error handling?

The current design is ambiguous about:
- Whether `SetInputData()` should validate
- Whether violations throw exceptions or just log warnings
- How graph restoration handles constraint violations
- Whether validation can be disabled

---

## Use Cases for Constraint Violations

### Use Case 1: UI Setting Value

UI sends data that bypasses client-side validation:
```cpp
node->SetInputData("bitrate", MakeNodeData<int>(500));  // max is 320
```

**Questions**:
- Should this throw immediately?
- Should it log a warning?
- Should it be allowed?

---

### Use Case 2: Graph Restoration from Saved File

Loading an old graph where constraints have changed:
```json
{
  "node_state": {
    "bitrate": 500
  },
  "port_definitions": {
    "inputs": [{
      "key": "bitrate",
      "constraints": {"min": 0, "max": 320}
    }]
  }
}
```

**Scenario**: Graph was saved with old constraints (max=500), now restored with new constraints (max=320).

**Questions**:
- Should restoration fail with exception?
- Should it log warning and continue?
- Should it clamp/adjust the value?

---

### Use Case 3: Direct Code Setting (Compute)

Node's own Compute() method sets invalid output:
```cpp
void Compute() override {
    // Bug: violates own output constraint
    SetOutputData("result", MakeNodeData<int>(999));
}
```

**Questions**:
- Catch programmer errors?
- Allow internal flexibility?

---

### Use Case 4: Data Flowing Through Connections

Source outputs value that violates destination's constraints:
```cpp
// Source: outputs range 0-1000
sourceNode->SetOutputData("out", MakeNodeData<int>(750));

// Graph propagates to destination with constraint max=320
// What happens?
```

**Questions**:
- Validate at propagation time?
- Type conversion respects constraints?
- Destination rejects data?

---

### Use Case 5: Constraint Evolution

Schema changes between versions:
```
Version 1: bitrate constraint max=500
Version 2: bitrate constraint max=320 (stricter)

Load v1 graph with v2 code → value 400 is now invalid
```

**Questions**:
- Automatic migration/clamping?
- Manual migration required?
- Fail and report?

---

### Use Case 6: Test Code Bypassing UI

Unit tests need to set invalid values:
```cpp
// Testing error handling for out-of-range input
node->SetInputData("bitrate", MakeNodeData<int>(-100), true, false);
// Last parameter: validate_constraints = false
```

**Questions**:
- Need way to disable validation for testing?
- Use flag, try-catch, or separate method?

---

### Use Case 7: Temporary Invalid States

Building up complex state through multiple operations:
```cpp
node->SetInputData("start", MakeNodeData<int>(100), false);   // compute=false
node->SetInputData("end", MakeNodeData<int>(50), false);      // compute=false
// State: start > end (violates constraint), but about to fix
node->SetInputData("start", MakeNodeData<int>(40), true);     // compute=true, now valid
```

**Questions**:
- Allow intermediate invalid states?
- Validate only at compute time?

---

## Proposed Validation Strategies

### Strategy A: Permissive (Constraints are Metadata Only)

**Philosophy**: Constraints are hints, not enforced by framework.

**Implementation**:
```cpp
void Node::SetInputData(const IndexableName& key, SharedNodeData data, bool compute) {
    _input_ports.at(key)->SetData(data);
    OnSetInput.Broadcast(key, data);
    if (compute) InvokeCompute();
    // NO constraint validation
}
```

**Validation only in Compute()**:
```cpp
void Compute() override {
    auto bitrate = GetInputData<int>("bitrate");

    if (!ValidateConstraints("bitrate", bitrate)) {
        throw std::invalid_argument("Bitrate out of range");
    }

    // ... continue
}
```

**Pros**:
- ✅ Fully backward compatible
- ✅ Flexible - nodes control validation
- ✅ No exceptions in SetInputData()
- ✅ Allows temporary invalid states
- ✅ Minimal framework overhead

**Cons**:
- ❌ Silent failures if validation not implemented
- ❌ Data integrity not guaranteed by framework
- ❌ Each node must duplicate validation logic
- ❌ Graph restoration doesn't validate
- ❌ Constraints don't prevent invalid states

**Best For**: Permissive systems, internal tools, experimental code

**Worst For**: Production systems, critical data paths, safety-critical applications

---

### Strategy B: Strict (Constraints are Enforced)

**Philosophy**: Framework enforces constraints everywhere.

**Implementation**:
```cpp
void Node::SetInputData(const IndexableName& key, SharedNodeData data, bool compute) {
    auto port = _input_ports.at(key);

    // Validate constraints
    if (auto violation = CheckConstraints(port, data)) {
        throw std::invalid_argument(
            fmt::format("Value {} violates constraint on port '{}': {}",
                       data->AsString(), key, violation.message));
    }

    port->SetData(data);
    OnSetInput.Broadcast(key, data);
    if (compute) InvokeCompute();
}
```

**Graph Restoration**:
```cpp
void Graph::Restore(const json& j) {
    // ... create nodes and connections ...

    // Restore node data - throws if constraints violated
    for (auto& node : _nodes) {
        for (auto& [port_key, port_data] : saved_node_data) {
            try {
                node->SetInputData(port_key, port_data, false);
            } catch (const std::invalid_argument& e) {
                throw std::runtime_error(
                    fmt::format("Node {} port {} restoration failed: {}",
                               node->GetName(), port_key, e.what()));
            }
        }
    }
}
```

**Pros**:
- ✅ Prevents invalid states
- ✅ Fails fast with clear error
- ✅ Automatic enforcement everywhere
- ✅ Graph restoration validates
- ✅ Data integrity guaranteed
- ✅ No per-node validation code needed

**Cons**:
- ❌ Breaking change - existing code may fail
- ❌ Can't set invalid values even temporarily
- ❌ Testing invalid states requires try-catch
- ❌ Graph restoration may fail due to schema changes
- ❌ Exception overhead on every SetInputData
- ❌ UI must pre-validate or handle exceptions

**Best For**: Production systems, safety-critical, data integrity critical

**Worst For**: Backwards compatibility, testing, exploratory code

---

### Strategy C: Hybrid (Configurable Validation)

**Philosophy**: Validate by default, but allow opt-out for special cases.

**Implementation**:
```cpp
void Node::SetInputData(const IndexableName& key, SharedNodeData data,
                       bool compute = true, bool validate_constraints = true) {
    auto port = _input_ports.at(key);

    if (validate_constraints) {
        if (auto violation = CheckConstraints(port, data)) {
            throw std::invalid_argument(
                fmt::format("Value violates constraint on port '{}': {}",
                           key, violation.message));
        }
    }

    port->SetData(data);
    OnSetInput.Broadcast(key, data);
    if (compute) InvokeCompute();
}
```

**Usage Patterns**:
```cpp
// Default - validates automatically
node->SetInputData("bitrate", MakeNodeData<int>(128));

// Explicit validation (same as default)
node->SetInputData("bitrate", MakeNodeData<int>(128), true, true);

// Test case - skip validation to test error handling
node->SetInputData("bitrate", MakeNodeData<int>(-100), true, false);

// Batch operations - validate only at end
node->SetInputData("start", MakeNodeData<int>(100), false, false);
node->SetInputData("end", MakeNodeData<int>(50), false, false);
node->SetInputData("step", MakeNodeData<int>(10), true, true);  // validate now
```

**Graph Restoration** (always validates):
```cpp
void Graph::Restore(const json& j) {
    // ... create nodes and connections ...

    // Always validate during restoration
    for (auto& node : _nodes) {
        for (auto& [port_key, port_data] : saved_node_data) {
            // validate_constraints = true (hardcoded, no option to skip)
            node->SetInputData(port_key, port_data, false, true);
        }
    }
}
```

**Pros**:
- ✅ Backward compatible (default: validate)
- ✅ Can opt-out for testing/special cases
- ✅ Automatic validation by default
- ✅ Graph restoration always validates
- ✅ Explicit intent (clear when skipping)
- ✅ No breaking changes

**Cons**:
- ❌ Easy to forget validation flag
- ❌ More complex API (4 parameters)
- ❌ Could mask bugs if flag always false
- ❌ Inconsistent if flag used inconsistently

**Best For**: Most production code, testing, gradual adoption

**Worst For**: Minimal API surface, strict frameworks

---

### Strategy D: Three-Tier (Warnings + Validation Helpers)

**Philosophy**: Allow invalid values, log warnings, provide validation helpers.

**Implementation**:
```cpp
struct ConstraintViolation {
    std::string message;
    std::string type;  // "min", "max", "enum", "pattern", etc.
    bool is_critical;  // affects computation correctness
};

void Node::SetInputData(const IndexableName& key, SharedNodeData data, bool compute) {
    auto port = _input_ports.at(key);

    if (auto violation = CheckConstraints(port, data)) {
        if (violation.is_critical) {
            LOG_ERROR("CRITICAL: Constraint violation on port '{}': {}",
                     key, violation.message);
        } else {
            LOG_WARN("Constraint violation on port '{}': {}",
                    key, violation.message);
        }
    }

    port->SetData(data);
    OnSetInput.Broadcast(key, data);
    if (compute) InvokeCompute();
}

// Helper methods for strict validation
bool Node::ValidateInputConstraint(const IndexableName& key,
                                  const SharedNodeData& data) const {
    if (auto violation = CheckConstraints(GetInputPort(key), data)) {
        throw std::invalid_argument(violation.message);
    }
    return true;
}

// Checks without throwing
std::optional<ConstraintViolation> Node::CheckInputConstraint(
    const IndexableName& key, const SharedNodeData& data) const {
    return CheckConstraints(GetInputPort(key), data);
}
```

**Usage Patterns**:
```cpp
// Automatic warning if invalid, but doesn't fail
node->SetInputData("bitrate", MakeNodeData<int>(500));  // LOG_WARN if violates

// Strict validation when needed
if (auto violation = node->CheckInputConstraint("bitrate", value)) {
    // Handle violation programmatically
    LOG_ERROR("Invalid bitrate: {}", violation.message);
} else {
    node->SetInputData("bitrate", value);
}

// In Compute - manually throw if desired
void Compute() override {
    node->ValidateInputConstraint("bitrate", GetInputData("bitrate"));
    // ... continue
}
```

**Graph Restoration** (warns but continues):
```cpp
void Graph::Restore(const json& j) {
    // ... create nodes ...

    for (auto& node : _nodes) {
        for (auto& [port_key, port_data] : saved_node_data) {
            // Will log warning if constraint violated, but continues
            node->SetInputData(port_key, port_data, false);
        }
    }

    // Nodes must validate in Compute() if they care
}
```

**Pros**:
- ✅ Fully backward compatible
- ✅ Logs issues for debugging
- ✅ Nodes can use helpers for strict validation
- ✅ Graph restoration always continues
- ✅ Flexible - different validation for different ports
- ✅ Non-blocking - doesn't fail on constraint violations

**Cons**:
- ❌ Warnings might be ignored
- ❌ Two code paths (warning vs exception)
- ❌ Data integrity not guaranteed
- ❌ More complex validation logic
- ❌ Logging dependency required

**Best For**: Diagnostic systems, gradual migrations, user-facing tools

**Worst For**: Safety-critical, strict data integrity requirements

---

## Comparison Table

| Aspect | Strategy A | Strategy B | Strategy C | Strategy D |
|--------|-----------|-----------|-----------|-----------|
| **SetInputData() validates** | ❌ No | ✅ Yes | ✅ Yes (default) | ⚠️ Warns |
| **Throws exception** | ❌ Never | ✅ Always | ✅ By default | ❌ Never |
| **Graph restoration** | ❌ No check | ✅ Validates | ✅ Validates | ⚠️ Warns |
| **Test invalid value** | ✅ Easy | ❌ Hard | ✅ With flag | ✅ Easy |
| **Temporary states** | ✅ Allowed | ❌ Blocked | ❌ Blocked | ✅ Allowed |
| **Backward compat** | ✅ Full | ❌ Breaking | ✅ Full | ✅ Full |
| **API complexity** | Low | Low | Medium | Medium-High |
| **Data integrity** | ❌ Not guaranteed | ✅ Guaranteed | ✅ Guaranteed | ⚠️ Best effort |
| **Performance** | ✅ No overhead | ⚠️ Check overhead | ⚠️ Check overhead | ⚠️ Check + log |
| **Per-node overhead** | ✅ Low | ✅ Low | ✅ Low | ❌ High |

---

## Scenario Analysis

### Scenario 1: UI Setting Invalid Value

**Strategy A**: Allowed, no error. Compute() may fail.
```cpp
node->SetInputData("bitrate", 500);  // Allowed
node->InvokeCompute();  // May throw in Compute() if validation not done
```

**Strategy B**: Throws immediately.
```cpp
try {
    node->SetInputData("bitrate", 500);  // Throws std::invalid_argument
} catch (const std::invalid_argument& e) {
    ui->ShowError(e.what());
}
```

**Strategy C**: Throws by default.
```cpp
try {
    node->SetInputData("bitrate", 500);  // Throws
} catch (const std::invalid_argument& e) {
    ui->ShowError(e.what());
}
```

**Strategy D**: Logs warning, allowed.
```cpp
node->SetInputData("bitrate", 500);  // LOG_WARN, but allowed
// UI should check before calling
if (auto v = node->CheckInputConstraint("bitrate", 500)) {
    ui->ShowError(v->message);
}
```

---

### Scenario 2: Graph Restoration with Schema Change

**Strategy A**: Silently loads (may fail during compute).
```cpp
graph->Restore(json_data);  // Loads with old constraint values
// May fail later if values now invalid
```

**Strategy B**: Throws on restoration.
```cpp
try {
    graph->Restore(json_data);  // Throws if constraints violated
} catch (const std::runtime_error& e) {
    // Migration required - load with old constraints, update values
}
```

**Strategy C**: Throws on restoration.
```cpp
try {
    graph->Restore(json_data);  // Throws if constraints violated
} catch (...) {
    // Handle migration
}
```

**Strategy D**: Logs warnings on restoration.
```cpp
graph->Restore(json_data);  // LOG_WARN if violated, but loads
// Data is loaded, user must verify validity
```

---

### Scenario 3: Test Setting Invalid Value

**Strategy A**: Easy (no validation).
```cpp
node->SetInputData("bitrate", -100);  // Allowed
// Test Compute() to verify error handling
```

**Strategy B**: Difficult (must skip constraint in test).
```cpp
// Hard to test - either modify node or catch exception
try {
    node->SetInputData("bitrate", -100);  // Throws
    FAIL("Should have thrown");
} catch (const std::invalid_argument&) {
    PASS();
}
```

**Strategy C**: With flag (easy).
```cpp
node->SetInputData("bitrate", -100, true, false);  // validate_constraints=false
// Test Compute() error handling
```

**Strategy D**: Easy (warnings logged).
```cpp
node->SetInputData("bitrate", -100);  // LOG_WARN but allowed
// Test Compute() error handling
```

---

## Recommendation: Strategy C (Hybrid with Validation Flag)

**Chosen**: Strategy C - Hybrid with Configurable Validation

**Rationale**:

1. **Default Safe**: `validate_constraints=true` by default prevents most issues
2. **Backward Compatible**: Existing code continues to work
3. **Explicit Intent**: Flag makes validation requirement clear
4. **Flexible**: Can disable for tests and special cases
5. **Predictable**: Graph restoration always validates (hardcoded true)
6. **Testable**: Easy to test both valid and invalid scenarios
7. **Best Balance**: Catches bugs without breaking existing systems

**Implementation API**:

```cpp
class Node {
  public:
    /**
     * @brief Set data on an input port.
     *
     * @param key The port identifier
     * @param data The data to set
     * @param compute Whether to trigger computation after setting
     * @param validate_constraints Whether to validate against port constraints
     *
     * @throws std::invalid_argument if validate_constraints=true and
     *         data violates port constraints
     */
    void SetInputData(const IndexableName& key, SharedNodeData data,
                     bool compute = true, bool validate_constraints = true);
};
```

**Usage Guidelines**:

1. **Default (production code)**:
   ```cpp
   node->SetInputData("bitrate", value);  // Validates automatically
   ```

2. **Testing**:
   ```cpp
   // Test error handling with invalid value
   node->SetInputData("bitrate", invalid_value, true, false);
   ```

3. **Batch operations**:
   ```cpp
   node->SetInputData("start", val1, false, false);
   node->SetInputData("end", val2, false, false);
   node->SetInputData("step", val3, true, true);  // Validate on final set
   ```

4. **Graph restoration** (always validates):
   ```cpp
   // Framework ensures: validate_constraints = true
   graph->Restore(json_data);
   ```

---

## Implementation Checklist

For Strategy C implementation, needed components:

- [ ] `CheckConstraints()` helper function
  - Takes Port and data
  - Returns `std::optional<ConstraintViolation>`
  - Type-aware validation (numeric, string, enum, etc.)

- [ ] Updated `Node::SetInputData()` signature
  - Add `bool validate_constraints = true` parameter
  - Call CheckConstraints() if flag true
  - Throw `std::invalid_argument` on violation

- [ ] Validation exception format
  - Consistent error messages
  - Include port name, constraint type, violation details
  - Example: `"Value 500 violates max constraint of 320 for port 'bitrate'"`

- [ ] Graph restoration
  - Ensure `validate_constraints = true` (no option)
  - Handle exceptions gracefully
  - Report which nodes/ports failed

- [ ] Connection propagation
  - Validate data when flowing through connections
  - Type conversion respects constraints
  - Reject data that violates destination constraints

- [ ] Tests
  - Test validation with valid values
  - Test validation with invalid values
  - Test flag to skip validation
  - Test exception messages
  - Test graph restoration with violations

- [ ] Documentation
  - Usage examples (valid, invalid, skipped validation)
  - Error messages and handling
  - Migration guide for constraint schemas
  - Best practices

---

## Open Questions

1. **Constraint violation recovery**: Should invalid values be clamped to valid range, or rejected outright?
   - Current recommendation: Reject outright (fail-fast)
   - Alternative: Auto-clamp (may mask bugs)

2. **Backward compatibility window**: How long to support old schema before dropping compatibility?
   - Affects migration strategy
   - May need version negotiation

3. **Custom validation**: Should nodes be able to define custom validation logic beyond constraint checks?
   - Could be extension point for future

4. **Connection-time validation**: Validate constraints when connections are created?
   - Prevents mis-connected ports
   - But ports don't have data at connection time

5. **Async validation**: Should validation be async for expensive checks?
   - Unlikely needed for simple constraints
   - Could be extension for future

---

## References

- `metadata_api.md` - Main metadata API design
- Constraint definitions - See "Constraint Definitions" section
- SetInputData() - Current implementation at `src/Node.cpp:100`

