# flow-core

[![CMake](https://github.com/InFlowStructure/flow-core/actions/workflows/cmake.yml/badge.svg)](https://github.com/InFlowStructure/flow-core/actions/workflows/cmake.yml)
[![License: MIT](https://img.shields.io/github/license/InFlowStructure/flow-core)](https://github.com/InFlowStructure/flow-core/blob/main/LICENSE)
[![Language: C++20](https://img.shields.io/badge/Language-C%2B%2B20%20-blue)](https://en.cppreference.com/w/cpp/20)

A cross-platform C++20 graph based code engine for building easily modified code flows on the fly. Intended as a Low-Code / No-Code solution.

## Building

To build the project with CMake, simply run
```bash
cmake -B build
cmake --build build --parallel
```

To build tests, configure the build directory with the following
```bash
cmake -B build -Dflow-core_BUILD_TESTS=ON
```

## Installing

To install, configure the cmake build as follows:
```bash
cmake -B build -Dflow-core_INSTALL=ON
```
Then, simply build the project normally, then run one of
```bash
cmake --install build
```

> [!TIP]
> Try adding the `--config` flag with the appropriate build type.

## Dependencies

This project depends on the following open source projects:
- [nlohmann_json](https://github.com/nlohmann/json)
- [thread-pool](https://github.com/bshoshany/thread-pool)
