# Flow-Core Port Metadata Query Feature Plan

## Executive Summary

This document outlines the requirements for adding port metadata query capabilities to the flow-core library. Currently, the library exposes port structure (names, types) and data operations (set/get values), but lacks methods to query port default values and complete metadata from instantiated nodes. This limitation prevents tooling, editors, and runtime introspection from displaying port configuration information to users.

---

## Problem Statement

### User Perspective: Visual Node Editor Integration

As a developer building a visual node editor (or any tool) that uses flow-core for computational execution, I encounter a critical limitation:

**I can see the ports that exist on a node, but I cannot see their default values or configuration.**

#### Concrete Example

When I create a node of type `audio_mp3_codec_Node` in my editor:

```dart
final node = graph.addNode("audio_mp3_codec_Node", "node_1");
final inputPorts = node.getInputPortKeys();  // ✓ Returns: ["vbr", "quality", "bitrate", "encode_mode", "encoded_in", "audio_in"]
final inputTypes = node.getInputPortType("vbr");  // ✓ Returns: port type info

// But I cannot do this:
final defaultValue = node.getInputPortDefaultValue("vbr");  // ❌ Method doesn't exist
final portMetadata = node.getInputPortMetadata("vbr");  // ❌ Method doesn't exist
```

**Why this matters:**

1. **UI Display**: My editor needs to show users "Port 'vbr' has a default value of '128'" so they can see what values are pre-configured
2. **Data Binding**: I need to know if a port has a default so I can decide whether to show an editor field or default display
3. **Validation**: I need to know port constraints (min/max, allowed values) before user input
4. **State Recovery**: When loading a saved graph, I need to restore ports to their configured defaults
5. **Configuration**: I need to display which ports are optional (have defaults) vs required (no default)

#### Current Workaround (Undesirable)

The only way to get this information currently is to create a **temporary node at module load time**, query its metadata, destroy it, and freeze that metadata in a prototype. This approach:
- ❌ Creates unnecessary node instances
- ❌ Has FFI handle lifecycle side effects
- ❌ Freezes metadata at one point in time
- ❌ Cannot reflect per-instance configuration variations
- ❌ Makes dynamic/runtime node discovery difficult

---

## Root Cause Analysis

### What's Currently Exposed

The flow-core library currently exposes:

| Operation | Available | Returns |
|-----------|-----------|---------|
| **Structure** | | |
| `flow_node_get_input_port_keys()` | ✓ Yes | Array of port names |
| `flow_node_get_output_port_keys()` | ✓ Yes | Array of port names |
| `flow_node_get_input_port_type()` | ✓ Yes | Type string for one port |
| `flow_node_get_output_port_type()` | ✓ Yes | Type string for one port |
| `flow_node_get_port_description()` | ✓ Yes | Description string for one port |
| **Data Operations** | | |
| `flow_node_set_input_data()` | ✓ Yes | Sets runtime value |
| `flow_node_get_input_data()` | ✓ Yes | Gets runtime value |
| `flow_node_get_output_data()` | ✓ Yes | Gets computed output |
| **Metadata Queries** | | |
| Default values | ❌ No | Missing |
| Full port metadata | ❌ No | Missing |
| Port constraints/validation rules | ❌ No | Missing |
| Interworking type information | ❌ No | Missing |

### What's Missing

1. **Default Value Query**: No way to retrieve the configured default value for a port
2. **Metadata Batch Query**: No way to get all metadata (types, defaults, constraints) for all ports at once
3. **Interworking Type Info**: No exposure of semantic type information (integer, float, string, boolean, etc.)
4. **Port Constraints**: No way to query validation rules, min/max values, allowed options, etc.

---

## Requirements

### Functional Requirements

#### FR1: Query Default Value for Single Port

**Capability**: Retrieve the default value (if any) for a specific input port on a node instance.

**API Signature (C)**:
```c
FlowErrorCode flow_node_get_input_port_default_value(
    FlowNodeHandle node,
    const char* port_key,
    FlowValuePtr* out_value
);
```

**Behavior**:
- Input: Valid node handle and port key name
- Output: Pointer to default value in `out_value` parameter
- Return: Error code indicating success or failure
- Notes:
  - Return `FLOW_ERROR_NOT_FOUND` if port doesn't exist
  - Return `FLOW_ERROR_NO_DEFAULT` if port has no default value (is required)
  - Caller responsible for memory management of returned value

**Usage Example (Dart)**:
```dart
final defaultValue = node.getInputPortDefaultValue("vbr");
if (defaultValue != null) {
  print("Port 'vbr' has default: $defaultValue");
} else {
  print("Port 'vbr' is required (no default)");
}
```

#### FR2: Query Metadata for Single Port

**Capability**: Retrieve complete metadata for a specific port (type, default, constraints, interworking info).

**API Signature (C)**:
```c
FlowErrorCode flow_node_get_input_port_metadata(
    FlowNodeHandle node,
    const char* port_key,
    const char** out_metadata_json
);
```

**Behavior**:
- Input: Valid node handle and port key name
- Output: JSON string containing port metadata
- Return: Error code indicating success or failure
- Notes:
  - JSON must be freed using `flow_free_string()`
  - Return `FLOW_ERROR_NOT_FOUND` if port doesn't exist
  - Metadata should follow schema defined in "Port Metadata JSON Schema" section below

**Usage Example (Dart)**:
```dart
final metadataJson = node.getInputPortMetadata("vbr");
final metadata = jsonDecode(metadataJson);
// metadata = {
//   "key": "vbr",
//   "type": "integer",
//   "default_value": "128",
//   "has_default": true,
//   "constraints": {
//     "min": 0,
//     "max": 320,
//     "step": 1
//   },
//   "interworking_type": "integer",
//   "description": "Variable Bitrate mode"
// }
```

#### FR3: Query All Port Metadata (Batch)

**Capability**: Retrieve metadata for all input ports of a node in a single call.

**API Signature (C)**:
```c
FlowErrorCode flow_node_get_all_input_ports_metadata(
    FlowNodeHandle node,
    const char** out_metadata_json
);
```

**Behavior**:
- Input: Valid node handle
- Output: JSON string containing metadata for all input ports
- Return: Error code indicating success or failure
- Notes:
  - JSON must be freed using `flow_free_string()`
  - Returns empty array `[]` if node has no input ports
  - Metadata should follow schema defined in "Port Metadata JSON Schema" section below
  - Single FFI call is more efficient than N individual port queries

**Usage Example (Dart)**:
```dart
final allMetadataJson = node.getAllInputPortsMetadata();
final metadataList = jsonDecode(allMetadataJson) as List;
// metadataList = [
//   { "key": "vbr", "type": "integer", "default_value": "128", ... },
//   { "key": "quality", "type": "float", "default_value": "0.8", ... },
//   { "key": "bitrate", "type": "integer", "default_value": null, ... },
//   ...
// ]

for (final metadata in metadataList) {
  print("Port ${metadata['key']}: default=${metadata['default_value']}");
}
```

#### FR4: Query Output Port Metadata

**Capability**: Retrieve metadata for output ports using same API as input ports.

**API Signature (C)**:
```c
FlowErrorCode flow_node_get_output_port_metadata(
    FlowNodeHandle node,
    const char* port_key,
    const char** out_metadata_json
);

FlowErrorCode flow_node_get_all_output_ports_metadata(
    FlowNodeHandle node,
    const char** out_metadata_json
);
```

**Behavior**: Same as input port metadata queries, but for output ports.

---

## Port Metadata JSON Schema

### Port Metadata Object (Single Port)

Each port metadata object SHALL conform to the following JSON schema:

```json
{
  "key": "port_name",
  "type": "string | integer | float | boolean | object | array",
  "direction": "input | output",
  "default_value": "value_as_string | null",
  "has_default": true | false,
  "is_required": true | false,
  "description": "Human-readable description of the port",
  "interworking_type": "integer | float | string | boolean | none",
  "constraints": {
    "min": "numeric or null",
    "max": "numeric or null",
    "step": "numeric or null",
    "allowed_values": ["value1", "value2"] | null,
    "pattern": "regex pattern | null"
  },
  "metadata": {
    "custom_field_1": "value",
    "custom_field_2": "value"
  }
}
```

### Port Metadata Collection (All Ports)

When querying all ports, return a JSON array:

```json
[
  { /* Port metadata object 1 */ },
  { /* Port metadata object 2 */ },
  { /* Port metadata object 3 */ }
]
```

### JSON Schema Field Definitions

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `key` | string | Yes | Unique port identifier within the node |
| `type` | string enum | Yes | Core data type (string, integer, float, boolean, object, array) |
| `direction` | string enum | Yes | "input" or "output" |
| `default_value` | string \| null | No | Default value as JSON string; null if no default |
| `has_default` | boolean | Yes | true if port has a default value |
| `is_required` | boolean | Yes | true if port must be provided; opposite of has_default |
| `description` | string | No | Human-readable description for UI display |
| `interworking_type` | string enum | No | Semantic type: "integer", "float", "string", "boolean", "none" |
| `constraints` | object | No | Validation constraints (min, max, step, allowed_values, pattern) |
| `metadata` | object | No | Custom metadata for extended use cases |

### Example Port Metadata Objects

**Input Port with Default Value:**
```json
{
  "key": "vbr",
  "type": "integer",
  "direction": "input",
  "default_value": "128",
  "has_default": true,
  "is_required": false,
  "description": "Variable Bitrate mode (0-320 kbps)",
  "interworking_type": "integer",
  "constraints": {
    "min": 0,
    "max": 320,
    "step": 1,
    "allowed_values": null,
    "pattern": null
  }
}
```

**Input Port Required (No Default):**
```json
{
  "key": "audio_in",
  "type": "object",
  "direction": "input",
  "default_value": null,
  "has_default": false,
  "is_required": true,
  "description": "Audio input stream",
  "interworking_type": "none",
  "constraints": null
}
```

**Output Port:**
```json
{
  "key": "audio_out",
  "type": "object",
  "direction": "output",
  "default_value": null,
  "has_default": false,
  "is_required": false,
  "description": "Encoded audio output",
  "interworking_type": "none",
  "constraints": null
}
```

---

## Implementation Guidelines

### Error Handling

All metadata query functions SHALL return `FlowErrorCode`:

```c
typedef enum {
  FLOW_ERROR_SUCCESS = 0,
  FLOW_ERROR_INVALID_HANDLE = 1,
  FLOW_ERROR_NOT_FOUND = 2,
  FLOW_ERROR_NO_DEFAULT = 3,
  FLOW_ERROR_INVALID_ARGUMENT = 4,
  FLOW_ERROR_MEMORY = 5,
  FLOW_ERROR_UNKNOWN = 6
} FlowErrorCode;
```

### Memory Management

- Returned JSON strings allocated via flow-core's memory allocator
- Caller responsible for freeing using `flow_free_string(char* str)`
- Caller should not modify returned strings
- Strings remain valid until `flow_free_string()` is called

### Performance Considerations

- **Single Port Query**: O(n) where n is number of ports on node (linear search by key)
- **Batch Query**: O(n) - enumerate all ports once, efficient for rendering full node UI
- **Caching**: Clients SHOULD cache metadata results if querying repeatedly
- **No Memory Leaks**: Ensure all temporary allocations freed during query execution

### Thread Safety

- Query functions MUST be thread-safe
- Multiple threads MAY query metadata from different node instances simultaneously
- Per-node queries MAY access node fields under read lock
- No long-held locks during JSON serialization

---

## Addendum: Alternative Approach - Unified Port Metadata Query

### Option A: Current Approach (Recommended for Phase 1)

Implement individual methods as specified in "Requirements" section above:
- `flow_node_get_input_port_default_value()`
- `flow_node_get_input_port_metadata()`
- `flow_node_get_all_input_ports_metadata()`
- Similar for output ports

**Advantages**:
- Granular control (query only what you need)
- Can query individual ports in lazy fashion
- Backward compatible with future extensions
- Simpler initial implementation

**Disadvantages**:
- Multiple FFI calls for full node introspection
- More binding code to maintain

### Option B: Unified Metadata Query (Recommended for Phase 2+)

Create a single comprehensive method that returns all node metadata:

```c
FlowErrorCode flow_node_get_all_metadata(
    FlowNodeHandle node,
    const char** out_metadata_json
);
```

Returns complete node introspection data:

```json
{
  "node_id": "node_1",
  "node_class": "audio_mp3_codec_Node",
  "input_ports": [
    { /* Port metadata */ },
    { /* Port metadata */ }
  ],
  "output_ports": [
    { /* Port metadata */ }
  ],
  "fields": [
    { /* Field metadata */ }
  ],
  "node_metadata": {
    "description": "MP3 codec node",
    "category": "Audio Codec",
    "version": "1.0"
  }
}
```

**Advantages**:
- Single FFI call for complete node introspection
- More efficient for UI rendering (all information in one query)
- Matches common introspection patterns (Python's `inspect`, Java's reflection)
- Better for caching entire node structure

**Disadvantages**:
- Larger JSON responses (more data transferred)
- Less granular if only partial info needed
- Requires more complex C++ serialization code

### Recommendation

**Phase 1**: Implement individual methods (Option A)
- Allows incremental rollout
- Reduces implementation complexity
- Sufficient for current editor needs

**Phase 2+**: Add unified method (Option B)
- For tools that need full introspection
- Can be built on top of Phase 1 methods
- No breaking changes to Phase 1 API

---

## Impact on Downstream Tools

### Visual Node Editors

**Current Problem**:
```dart
// Editor can see ports but not defaults
final ports = node.getInputPortKeys();  // ✓ Works
final defaults = <String, String>{};
for (final port in ports) {
  // ❌ No way to get default value
  // Editor must show empty field instead of default
}
```

**After Implementation**:
```dart
// Editor can show defaults to user
final metadata = node.getAllInputPortsMetadata();
final metadataList = jsonDecode(metadata) as List;
for (final portMeta in metadataList) {
  defaults[portMeta['key']] = portMeta['default_value'] ?? '';
  // ✓ Can display: "Port 'vbr' = 128 (default)"
}
```

### Runtime Configuration

**Use Case**: Save/restore node graphs with proper defaults

```dart
// When saving: capture default-aware configuration
final metadata = node.getAllInputPortsMetadata();
final config = <String, dynamic>{};
for (final port in metadataList) {
  config[port['key']] = {
    'value': node.getInputData(port['key']),
    'default': port['default_value'],
    'has_default': port['has_default']
  };
}

// When loading: restore to proper defaults
for (final entry in config.entries) {
  if (entry.value['value'] == null && entry.value['has_default']) {
    // Use default (either via node creation or explicit set)
  }
}
```

### Client Code Generation

**Use Case**: Auto-generate node wrapper classes with metadata

```dart
// Generator can introspect ports and generate typed wrappers
class AudioMP3CodecNode {
  final Node _node;

  // Generated from port metadata
  int get vbr => _node.getInputData('vbr') ?? 128;  // Default from metadata
  set vbr(int value) => _node.setInputData('vbr', value);

  String get audioIn => _node.getInputData('audio_in');  // Required, no default
  set audioIn(String value) => _node.setInputData('audio_in', value);
}
```

---

## API Surface Changes Summary

### New Functions (C API)

```c
// Single port queries
FlowErrorCode flow_node_get_input_port_default_value(
    FlowNodeHandle node,
    const char* port_key,
    FlowValuePtr* out_value
);

FlowErrorCode flow_node_get_input_port_metadata(
    FlowNodeHandle node,
    const char* port_key,
    const char** out_metadata_json
);

FlowErrorCode flow_node_get_output_port_metadata(
    FlowNodeHandle node,
    const char* port_key,
    const char** out_metadata_json
);

// Batch queries
FlowErrorCode flow_node_get_all_input_ports_metadata(
    FlowNodeHandle node,
    const char** out_metadata_json
);

FlowErrorCode flow_node_get_all_output_ports_metadata(
    FlowNodeHandle node,
    const char** out_metadata_json
);
```

### New Dart Bindings (flow_ffi package)

```dart
// In Node class
Future<dynamic> getInputPortDefaultValue(String portKey)
Future<PortMetadata> getInputPortMetadata(String portKey)
Future<List<PortMetadata>> getAllInputPortsMetadata()
Future<PortMetadata> getOutputPortMetadata(String portKey)
Future<List<PortMetadata>> getAllOutputPortsMetadata()

// Updated PortMetadata class
class PortMetadata {
  final String key;
  final String type;
  final String direction;
  final String? defaultValue;
  final bool hasDefault;
  final bool isRequired;
  final String? description;
  final String? interworkingType;
  final PortConstraints? constraints;
}

class PortConstraints {
  final num? min;
  final num? max;
  final num? step;
  final List<String>? allowedValues;
  final String? pattern;
}
```

---

## Testing Strategy

### Unit Tests (C++)

1. **Default Value Queries**
   - Query default value for port with default
   - Query default value for required port (no default)
   - Query non-existent port
   - Query from invalid handle

2. **Metadata Queries**
   - Single port metadata includes all fields
   - All ports metadata returns correct count
   - JSON structure conforms to schema
   - Special characters in descriptions properly escaped

3. **Error Handling**
   - Invalid handle returns FLOW_ERROR_INVALID_HANDLE
   - Non-existent port returns FLOW_ERROR_NOT_FOUND
   - Memory properly allocated and freed

### Integration Tests (Dart)

1. **Query Operations**
   - Query metadata from instantiated nodes
   - Cache and reuse results
   - Handle errors gracefully
   - Verify JSON parsing

2. **Editor Integration**
   - Display default values in UI
   - Validate against constraints
   - Populate forms from metadata
   - Show/hide optional vs required ports

---

## Success Criteria

1. ✅ All port metadata queryable on-demand from instantiated nodes
2. ✅ No temporary node creation required for introspection
3. ✅ Single FFI call can retrieve all port metadata
4. ✅ Metadata format supports editor, validation, and code generation use cases
5. ✅ Backward compatible - existing code continues to work
6. ✅ Thread-safe for concurrent queries
7. ✅ Comprehensive documentation and examples
8. ✅ Full test coverage (unit and integration)

---

## Timeline and Phases

### Phase 1: Core Implementation (Estimated 2-3 weeks)

- [ ] Design C API signatures
- [ ] Implement port metadata query in C++
- [ ] Generate JSON output
- [ ] Implement FFI bindings in Dart
- [ ] Unit tests (C++)
- [ ] Integration tests (Dart)

### Phase 2: Polish and Optimization (Estimated 1-2 weeks)

- [ ] Performance profiling
- [ ] Cache optimization
- [ ] Error handling robustness
- [ ] Documentation
- [ ] Example code

### Phase 3: Future Enhancements (Future)

- [ ] Unified node metadata query method
- [ ] Field/property metadata queries
- [ ] Validation rules enforcement
- [ ] Custom metadata extensions

---

## Conclusion

Adding port metadata query capabilities to flow-core will enable:

1. **Better Tooling**: Visual editors can display complete port configuration to users
2. **No Workarounds**: Eliminates need for temporary node discovery
3. **Runtime Introspection**: Supports dynamic node configuration and validation
4. **Better Developer Experience**: Metadata-aware code generation and bindings

This feature is essential for flow-core to support modern visual programming paradigms where users expect to see node configuration details before connecting data.
