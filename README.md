# flow-core

[![CMake](https://github.com/InFlowStructure/flow-core/actions/workflows/cmake.yml/badge.svg)](https://github.com/InFlowStructure/flow-core/actions/workflows/cmake.yml)

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

## Dependencies

This project depends on the following open source projects:
- [spdlog](https://github.com/gabime/spdlog)
- [thread-pool](https://github.com/bshoshany/thread-pool)

The documentation for this project uses [doxygen-aesome-css](https://github.com/jothepro/doxygen-awesome-css).

