# flow-core

[![CMake](https://github.com/InFlowStructure/flow-core/actions/workflows/cmake.yml/badge.svg)](https://github.com/InFlowStructure/flow-core/actions/workflows/cmake.yml)
[![License: MIT](https://img.shields.io/github/license/InFlowStructure/flow-core)](https://github.com/InFlowStructure/flow-core/blob/main/LICENSE)
[![Language: C++20](https://img.shields.io/badge/Language-C%2B%2B20%20-blue)](https://en.cppreference.com/w/cpp/20)

![Linux](https://img.shields.io/badge/OS-Linux-blue)
![Windows](https://img.shields.io/badge/OS-Windows-blue)
![macOS](https://img.shields.io/badge/OS-macOS-blue)

## Overview

Flow Core is a cross-platform C++20 graph-based code engine designed for building dynamically modifiable code flows. It serves as a foundation for Low-Code/No-Code solutions by providing a flexible and extensible graph computation system.

## Features

- Dynamic node-based computation graph
- Runtime module loading system
- Thread-safe execution environment
- Type-safe data propagation
- JSON serialization support
- Cross-platform compatibility

## Requirements

- C++20 compliant compiler
- CMake 3.15 or higher
- Modern operating system (Windows, macOS, Linux)

## Dependencies

Flow Core relies on these open-source libraries:

- [nlohmann_json](https://github.com/nlohmann/json) - Modern JSON handling
- [thread-pool](https://github.com/bshoshany/thread-pool) - Efficient thread management

## Building

### Basic Build

```bash
cmake -B build
cmake --build build --parallel
```

### Build with Tests

```bash
cmake -B build -Dflow-core_BUILD_TESTS=ON
cmake --build build --parallel
```

## Installation

Configure and install:

```bash
cmake -B build -Dflow-core_INSTALL=ON
cmake --build build --parallel
cmake --install build
```

## Getting Started

Check out our [documentation](docs/getting-started.md) for:

- Basic concepts and architecture
- Creating your first flow
- Building custom nodes
- Best practices and examples

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details on how to submit pull requests, report issues, and contribute to the project.
